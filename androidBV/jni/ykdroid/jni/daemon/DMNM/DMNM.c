/*
 * DMNM.c
 *
 *  Created on: 2013-3-29
 *      Author: chensq
 */
#include <stdio.h>
#include <pthread.h>
#include "DMNMUtils.h"
#include "DMNM.h"
#include "IDBTCfg.h"
#include <android/log.h>



#define MONITOR_NETWORK_TIME	30

static pthread_t g_DMNMThread_id;
static char g_cNetworkRunning = 0;
char g_cPppoeRouteAddFlg = 0;
int g_iUpdateMac = 0;

void *DMNMEventHndl(void *ctx) {
	int iMonitorNet = 30;
	char acMac[18] = { 0x0 };
	int iNetStatusNew = -9;
	int iNetStatusOld = -9;
	int iReConnect = 0;

	notifyServiceNetOn();
	__android_log_print(ANDROID_LOG_INFO, "DMNM", "DMNM DMNMEventHndl begin:%d", g_stIdbtCfg.stNetWorkInfo.acNetFlag[0]);

	system("busybox ifconfig eth0 up");
	//system("/data/fsti_tools/telnetd &");

	if (strcmp(g_stIdbtCfg.acSn, SN_NULL) != 0) {
		if (serialNumToMac(g_stIdbtCfg.acSn, acMac, 18) == 0) {
			strcpy(g_stIdbtCfg.stNetWorkInfo.acMac, acMac);
			SaveIdbtConfig();
			DMNMChangeMac();
			__android_log_print(ANDROID_LOG_INFO, "DMNM", "DMNM set acMac:%s", acMac);
		}
	}

	if (g_stIdbtCfg.stNetWorkInfo.acNetFlag[0] == NET_STATIC) {
		DMNMConnectStatic(g_stIdbtCfg.stNetWorkInfo.acStaticIP,
				g_stIdbtCfg.stNetWorkInfo.acStaticMask,
				g_stIdbtCfg.stNetWorkInfo.acStaticGateWay,
				g_stIdbtCfg.stNetWorkInfo.acStaticDNS);
		__android_log_print(ANDROID_LOG_INFO, "DMNM", "DMNM set static");
	}


	while (g_cNetworkRunning) {
		sleep(1);

		if(g_iUpdateMac == 1)
		{
			g_iUpdateMac = 0;
			//重新载入配置文件
			LoadIdbtConfig();
			DMNMChangeMac();
		}

		if (g_stIdbtCfg.stNetWorkInfo.acNetFlag[0] == NET_PPPOE && g_cPppoeRouteAddFlg == 0)
		{
			__android_log_print(ANDROID_LOG_INFO, "DMNM", "DMNM check pppoe route add result");
			system("echo `busybox ifconfig ppp0 | busybox grep 'P-t-P' |busybox awk -F '[ :]+' '{print $6}'|busybox wc -l` > route_value");
			FILE *stream;
			char msg[5] = {0x00};
			stream = fopen("/data/fsti_tools/route_value", "r");
			if(stream != NULL)
			{
				fgets(msg, 5, stream);
				fclose(stream);
				if(strncmp(msg, "1", 1) == 0)
				{
					__android_log_print(ANDROID_LOG_INFO, "DMNM", "DMNM check pppoe ok will add route");
					system("busybox route del -net default dev ppp0");
					system("busybox route add default gw  `busybox ifconfig ppp0 | busybox grep 'P-t-P' |busybox awk -F '[ :]+' '{print $6}'`");
					g_cPppoeRouteAddFlg = 1;
				}
				else
				{
					__android_log_print(ANDROID_LOG_INFO, "DMNM", "DMNM check pppoe not ok");
				}
			}
		}

		if(iReConnect >= 3)
		{
			iReConnect= 0;
			system("busybox ifconfig eth0 down");
			system("busybox ifconfig eth0 up");
		}

		if (++iMonitorNet >= MONITOR_NETWORK_TIME) {
			iMonitorNet = 0;

			iNetStatusNew = DMNMNetTest();
			__android_log_print(ANDROID_LOG_INFO, "DMNM", "DMNM iNetStatusNew=%d", iNetStatusNew);

			if (iNetStatusNew != iNetStatusOld) {
				if (iNetStatusNew == 0) {
					//
					iReConnect= 0;
					notifyServiceNetOn();
					__android_log_print(ANDROID_LOG_INFO, "DMNM", "DMNM iNetStatusNew==0");
				} else if (iNetStatusNew == -1) {
					//
					iReConnect++;
					notifyServiceNetOff();
					__android_log_print(ANDROID_LOG_INFO, "DMNM", "DMNM iNetStatusNew==-1");
				}
				iNetStatusOld = iNetStatusNew;
			}

			if (iNetStatusNew == 0) {
				continue;
			}

			if (g_stIdbtCfg.stNetWorkInfo.acNetFlag[0] == NET_DHCP) {
				__android_log_print(ANDROID_LOG_INFO, "DMNM", "DMNM will exec dhcpc");
				system("busybox ps | busybox grep 'netcfg'| busybox grep -v 'grep'| busybox cut -c 0-6|busybox xargs kill -9");
				system("netcfg eth0 dhcp &");
			} else if (g_stIdbtCfg.stNetWorkInfo.acNetFlag[0] == NET_PPPOE) {
				__android_log_print(ANDROID_LOG_INFO, "DMNM", "DMNM will exec pppoe");
				DMNMDisconnectPppoe();
				DMNMConnectPppoe();
			} else {
				__android_log_print(ANDROID_LOG_INFO, "DMNM", "DMNM will set static");
				DMNMConnectStatic(g_stIdbtCfg.stNetWorkInfo.acStaticIP,
						g_stIdbtCfg.stNetWorkInfo.acStaticMask,
						g_stIdbtCfg.stNetWorkInfo.acStaticGateWay,
						g_stIdbtCfg.stNetWorkInfo.acStaticDNS);
			}

		}
	}
}

int DMNMInit(void) {
	__android_log_print(ANDROID_LOG_INFO, "DMNM", "DMNM init begin");

	g_cNetworkRunning = 1;

	LoadIdbtConfig();

	if (pthread_create(&g_DMNMThread_id, NULL, DMNMEventHndl, NULL) < 0) {
		return -1;
	}
	__android_log_print(ANDROID_LOG_INFO, "DMNM", "DMNM init success");
	return 0;
}

void DMNMFini(void) {
	__android_log_print(ANDROID_LOG_INFO, "DMNM", "DMNM finish begin");

	if (g_stIdbtCfg.stNetWorkInfo.acNetFlag[0] == NET_PPPOE) {
		DMNMDisconnectPppoe();
	}
	g_cNetworkRunning = 0;
	pthread_join(g_DMNMThread_id, NULL);
	close(g_DMNMThread_id);
	__android_log_print(ANDROID_LOG_INFO, "DMNM", "DMNM finish success");

}

int __main(int argc, char **argv) {
	if(DMNMInit() != 0)
	{
		DMNMFini();
	}
	while(1)
	{
		sleep(100);
	}
}

