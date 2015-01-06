/*
 * DMNMUtils.c
 *
 *  Created on: 2013-3-29
 *      Author: chensq
 */

#ifndef DMNMUTILS_C_
#define DMNMUTILS_C_

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <assert.h>
#include "DMNMUtils.h"
#include "DMNM.h"
#include "IDBTCfg.h"
#include <android/log.h>


//IDBT_CONFIG_ST g_stIdbtCfg = { 0 };
//
//void SaveIdbtConfig(void)
//{
//	FILE *pCfgFd = NULL;
//
//	char *pcHelp = "\
//--------------------help info--------------------\n\
//log_upload_mode(1-clock 2-cycle)\n\
//log_upload_cycle_time(unit:hour)\n\
//network_type(1-DHCP 2-PPPOE 3-STATIC)\n\
//--------------------help info--------------------\
//";
//
//	pCfgFd=fopen(IDBT_CONFIG_PATH,"w+");
//	if(pCfgFd != NULL)
//	{
//		fprintf(pCfgFd,"sn=%s\n",g_stIdbtCfg.acSn);
//		fprintf(pCfgFd,"log_upload_mode=%d\n",g_stIdbtCfg.iLogUploadMode);
//		fprintf(pCfgFd,"log_upload_cycle_time=%d\n",g_stIdbtCfg.iLogUploadCycleTime);
//		fprintf(pCfgFd,"management_password=%s\n",g_stIdbtCfg.acManagementPassword);
//		fprintf(pCfgFd,"opendoor_password=%s\n",g_stIdbtCfg.acOpendoorPassword);
//
//		//网络相关参数
//		fprintf(pCfgFd,"network_type=%s\n",g_stIdbtCfg.stNetWorkInfo.acNetFlag);
//		fprintf(pCfgFd,"ip_addr=%s\n",g_stIdbtCfg.stNetWorkInfo.acStaticIP);
//		fprintf(pCfgFd,"net_mask=%s\n",g_stIdbtCfg.stNetWorkInfo.acStaticMask);
//		fprintf(pCfgFd,"gateway=%s\n", g_stIdbtCfg.stNetWorkInfo.acStaticGateWay);
//		fprintf(pCfgFd,"dns_server=%s\n", g_stIdbtCfg.stNetWorkInfo.acStaticDNS);
//		fprintf(pCfgFd,"mac_addr=%s\n", g_stIdbtCfg.stNetWorkInfo.acMac);
//
//		fprintf(pCfgFd,"pppoe_user=%s\n", g_stIdbtCfg.stNetWorkInfo.acWanUserName);
//		fprintf(pCfgFd,"pppoe_passwd=%s\n", g_stIdbtCfg.stNetWorkInfo.acWanPassword);
//
//		fprintf(pCfgFd,"dest_ip_1=%s\n", g_stIdbtCfg.stDestIpInfo.acDestIP1);
//		fprintf(pCfgFd,"port1=%d\n", g_stIdbtCfg.stDestIpInfo.port1);
//		fprintf(pCfgFd,"dest_ip_2=%s\n", g_stIdbtCfg.stDestIpInfo.acDestIP2);
//		fprintf(pCfgFd,"port2=%d\n", g_stIdbtCfg.stDestIpInfo.port2);
//
//
//
//
//		fprintf(pCfgFd,"%s\n", pcHelp);
//
//		//关闭配置文件
//		fclose(pCfgFd);
//	}
//}
//
//void LoadIdbtConfig(void)
//{
//	FILE *pCfgFd;
//
//	char *pcHelp = "\
//--------------------help info--------------------\n\
//log_upload_mode(1-clock 2-cycle)\n\
//log_upload_cycle_time(unit:hour)\n\
//network_type(1-DHCP 2-PPPOE 3-STATIC)\n\
//--------------------help info--------------------\n\
//";
//	strcpy(g_stIdbtCfg.acSn, SN_NULL);
////	g_stIdbtCfg.iInputVol = 5;
////	g_stIdbtCfg.iOutputCallVol = -2;
////	g_stIdbtCfg.iOutputPlayWavVol = 2;
////	g_stIdbtCfg.iVedioFps = 10;
////	g_stIdbtCfg.iVedioBr = 64;
//	g_stIdbtCfg.iLogUploadMode = 2;
//	g_stIdbtCfg.iLogUploadCycleTime = 6;
//	g_stIdbtCfg.stNetWorkInfo.acNetFlag[0] = NET_DHCP;
//	strcpy(g_stIdbtCfg.stNetWorkInfo.acStaticIP,"192.168.001.088");
//	strcpy(g_stIdbtCfg.stNetWorkInfo.acStaticMask,"255.255.255.000");
//	strcpy(g_stIdbtCfg.stNetWorkInfo.acStaticGateWay,"192.168.191.001");
//	strcpy(g_stIdbtCfg.stNetWorkInfo.acStaticDNS,"218.85.157.099");
//	strcpy(g_stIdbtCfg.stNetWorkInfo.acMac, "08:90:90:90:90:90");
//
//	strcpy(g_stIdbtCfg.acManagementPassword, "999999");
//	strcpy(g_stIdbtCfg.acOpendoorPassword, "123456");
//
//	strcpy(g_stIdbtCfg.stNetWorkInfo.acWanUserName, "15306919497");
//	strcpy(g_stIdbtCfg.stNetWorkInfo.acWanPassword, "2012180");
//
//	strcpy(g_stIdbtCfg.stDestIpInfo.acDestIP1, "115.239.210.026");
//	g_stIdbtCfg.stDestIpInfo.port1 = 80;
//	strcpy(g_stIdbtCfg.stDestIpInfo.acDestIP2, "220.162.097.165");
//	g_stIdbtCfg.stDestIpInfo.port2 = 80;
//
//	//打开IDBT配置文件
//	pCfgFd=fopen(IDBT_CONFIG_PATH,"r");
//	do
//	{
//		if(pCfgFd != NULL)
//		{
//			char buf[300];
//			fread(buf,300,1,pCfgFd);
//			fseek(pCfgFd,0,SEEK_SET);
//			fscanf(pCfgFd,"sn=%s\n",g_stIdbtCfg.acSn);
//			fscanf(pCfgFd,"log_upload_mode=%d\n",&g_stIdbtCfg.iLogUploadMode);
//			fscanf(pCfgFd,"log_upload_cycle_time=%d\n",&g_stIdbtCfg.iLogUploadCycleTime);
//			fscanf(pCfgFd,"management_password=%s\n",g_stIdbtCfg.acManagementPassword);
//			fscanf(pCfgFd,"opendoor_password=%s\n",g_stIdbtCfg.acOpendoorPassword);
//
//			//网络相关参数
//			fscanf(pCfgFd,"network_type=%s\n",g_stIdbtCfg.stNetWorkInfo.acNetFlag);
//			fscanf(pCfgFd,"ip_addr=%s\n",g_stIdbtCfg.stNetWorkInfo.acStaticIP);
//			fscanf(pCfgFd,"net_mask=%s\n",g_stIdbtCfg.stNetWorkInfo.acStaticMask);
//			fscanf(pCfgFd,"gateway=%s\n", g_stIdbtCfg.stNetWorkInfo.acStaticGateWay);
//			fscanf(pCfgFd,"dns_server=%s\n", g_stIdbtCfg.stNetWorkInfo.acStaticDNS);
//			fscanf(pCfgFd,"mac_addr=%s\n", g_stIdbtCfg.stNetWorkInfo.acMac);
//
//			fscanf(pCfgFd,"pppoe_user=%s\n", g_stIdbtCfg.stNetWorkInfo.acWanUserName);
//			fscanf(pCfgFd,"pppoe_passwd=%s\n", g_stIdbtCfg.stNetWorkInfo.acWanPassword);
//
//			fscanf(pCfgFd,"dest_ip_1=%s\n", g_stIdbtCfg.stDestIpInfo.acDestIP1);
//			fscanf(pCfgFd,"port1=%d\n", &g_stIdbtCfg.stDestIpInfo.port1);
//			fscanf(pCfgFd,"dest_ip_2=%s\n", g_stIdbtCfg.stDestIpInfo.acDestIP2);
//			fscanf(pCfgFd,"port2=%d\n", &g_stIdbtCfg.stDestIpInfo.port2);
//
//
//			//关闭配置文件
//			fclose(pCfgFd);
//			return;
//		}
//	}while(0);
//
//	pCfgFd=fopen(IDBT_CONFIG_PATH,"w+");
//	if(pCfgFd != NULL)
//	{
//		fprintf(pCfgFd,"sn=%s\n", g_stIdbtCfg.acSn);
//		fprintf(pCfgFd,"log_upload_mode=%d\n",g_stIdbtCfg.iLogUploadMode);
//		fprintf(pCfgFd,"log_upload_cycle_time=%d\n",g_stIdbtCfg.iLogUploadCycleTime);
//		fprintf(pCfgFd,"management_password=%s\n",g_stIdbtCfg.acManagementPassword);
//		fprintf(pCfgFd,"opendoor_password=%s\n",g_stIdbtCfg.acOpendoorPassword);
//
//
//		//网络相关参数
//		fprintf(pCfgFd,"network_type=%s\n",g_stIdbtCfg.stNetWorkInfo.acNetFlag);
//		fprintf(pCfgFd,"ip_addr=%s\n",g_stIdbtCfg.stNetWorkInfo.acStaticIP);
//		fprintf(pCfgFd,"net_mask=%s\n",g_stIdbtCfg.stNetWorkInfo.acStaticMask);
//		fprintf(pCfgFd,"gateway=%s\n", g_stIdbtCfg.stNetWorkInfo.acStaticGateWay);
//		fprintf(pCfgFd,"dns_server=%s\n", g_stIdbtCfg.stNetWorkInfo.acStaticDNS);
//		fprintf(pCfgFd,"mac_addr=%s\n", g_stIdbtCfg.stNetWorkInfo.acMac);
//
//		fprintf(pCfgFd,"pppoe_user=%s\n", g_stIdbtCfg.stNetWorkInfo.acWanUserName);
//		fprintf(pCfgFd,"pppoe_passwd=%s\n", g_stIdbtCfg.stNetWorkInfo.acWanPassword);
//
//		fprintf(pCfgFd,"dest_ip_1=%s\n", g_stIdbtCfg.stDestIpInfo.acDestIP1);
//		fprintf(pCfgFd,"port1=%d\n", g_stIdbtCfg.stDestIpInfo.port1);
//		fprintf(pCfgFd,"dest_ip_2=%s\n", g_stIdbtCfg.stDestIpInfo.acDestIP2);
//		fprintf(pCfgFd,"port2=%d\n", g_stIdbtCfg.stDestIpInfo.port2);
//
//		fprintf(pCfgFd,"%s\n", pcHelp);
//	}
//	//关闭配置文件
//	fclose(pCfgFd);
//}



void DMNMDisconnectPppoe(void)
{

	system("busybox ps | busybox grep 'connect'| busybox grep -v 'grep'| busybox cut -c 0-6|busybox xargs kill -9");
	system("busybox ps | busybox grep '/data/fsti_tools/pppoe'| busybox grep -v 'grep'| busybox cut -c 0-6|busybox xargs kill -9");
	system("busybox ps | busybox grep '/system/bin/pppd'| busybox grep -v 'grep'| busybox cut -c 0-6|busybox xargs kill -9");
	g_cPppoeRouteAddFlg = 0;
}


void DMNMConnectPppoe(void)
{
   char acCMD[128] = {0x0};
   //清空IP
   memset(acCMD, 0x0, sizeof(acCMD));
   sprintf(acCMD, "busybox ifconfig %s  0.0.0.0", ETH_NAME);
   system(acCMD);
   //PPP连接
   //system("/data/fsti_tools/connect eth0 15306919497 2012180 &");
   memset(acCMD, 0x0, sizeof(acCMD));
   sprintf(acCMD, "/data/fsti_tools/connect eth0 %s %s &", g_stIdbtCfg.stNetWorkInfo.acWanUserName, g_stIdbtCfg.stNetWorkInfo.acWanPassword);
   __android_log_print(ANDROID_LOG_INFO, "DMNM", "DMNM will exec %s", acCMD);
   system(acCMD);
}


int LPUtilsNetConnect(char *ip, int port)
{

	struct hostent* host = NULL;
	struct sockaddr_in saddr;
	unsigned int s = 0;
	int ret = 0;
	int error;
	host = gethostbyname(ip);
	if (host == NULL)
		return -1;

	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	saddr.sin_addr = *((struct in_addr*)host->h_addr);

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		return -1;
	}
//	LOG_RUNLOG_DEBUG("LP NET create socket %s : %d", ip, s);
	fcntl(s, F_SETFL, O_NONBLOCK);

	if (connect(s, (struct sockaddr*) &saddr, sizeof(saddr)) == -1)
	{
		if (errno == EINPROGRESS)
		{ // it is in the connect process
			struct timeval tv;
			fd_set writefds;
			tv.tv_sec = 7;
			tv.tv_usec = 0;
			FD_ZERO(&writefds);
			FD_SET(s, &writefds);
			if (select(s + 1, NULL, &writefds, NULL, &tv) > 0)
			{
				int len = sizeof(int);
				getsockopt(s, SOL_SOCKET, SO_ERROR, &error, &len);
				if (error == 0)
				{
					ret = 0;
				}
				else
					ret = -1;
			}
			else
				ret = -1; //timeout or error happen
		}
		else
			ret = -1;
	}
	else
		ret = 0;


	if(ret == 0)
	{
//		LOG_RUNLOG_DEBUG("LP NET connect ok %s", ip);
	}
	else
	{
//		LOG_RUNLOG_DEBUG("LP NET connect failed %s", ip);
	}

	if(s >= 0)
	{
		close(s);
//		LOG_RUNLOG_DEBUG("LP NET close socket %s : %d", ip, s);
	}
	return ret;
}


//000.000.000.000 length=15
static char *delPreZero(char* pcStr) {
	__android_log_print(ANDROID_LOG_INFO, "DMNM", "DMNM src:%s", pcStr);
	char cFlg = 0;
	char acBuff[32] = { 0x00 };
	char *acBak = pcStr;

	if (pcStr == NULL)
		return NULL;

	while (*pcStr != '\0') {
		if (*pcStr == '.') {
			cFlg = 0;
			pcStr++;
		} else if (cFlg == 0 && *pcStr != '0') {
			cFlg = 1;
			pcStr++;
		} else if (cFlg == 0 && *pcStr == '0') {
			if(*(pcStr+1) == '.' || *(pcStr+1) == '\0')
			{
				pcStr++;
				break;
			}
			memset(acBuff, 0x00, sizeof(acBuff));
			memcpy(acBuff, pcStr + 1, strlen(pcStr + 1));
			memcpy(pcStr, acBuff, strlen(pcStr + 1) + 1);

			if (*(pcStr) == '0' && *(pcStr + 1) == '0') {
				memset(acBuff, 0x00, sizeof(acBuff));
				memcpy(acBuff, pcStr + 1, strlen(pcStr + 1));
				memcpy(pcStr, acBuff, strlen(pcStr + 1) + 1);
				pcStr++;
			}

		} else if (cFlg == 1) {
			pcStr++;
		} else if (*pcStr == '\0') {
			break;
		}
	}
	__android_log_print(ANDROID_LOG_INFO, "DMNM", "DMNM dst:%s", acBak);
	return acBak;
}

int DMNMNetTest()
{
	int iRst = -1;
	char buff[NM_ADD_IP_LEN] = {0x00};
	//判断网络情况
	memset(buff, 0x00, NM_ADD_IP_LEN);
	strcpy(buff, g_stIdbtCfg.stDestIpInfo.acDestIP1);
	__android_log_print(ANDROID_LOG_INFO, "DMNM", "DMNM connect:%s", delPreZero(buff));
	iRst = LPUtilsNetConnect(delPreZero(g_stIdbtCfg.stDestIpInfo.acDestIP1), g_stIdbtCfg.stDestIpInfo.port1);
	if(0 != iRst)
	{
		memset(buff, 0x00, NM_ADD_IP_LEN);
		strcpy(buff, g_stIdbtCfg.stDestIpInfo.acDestIP2);
		__android_log_print(ANDROID_LOG_INFO, "DMNM", "DMNM connect:%s", delPreZero(buff));
		iRst = LPUtilsNetConnect(delPreZero(g_stIdbtCfg.stDestIpInfo.acDestIP2), g_stIdbtCfg.stDestIpInfo.port2);
	}
	return iRst;
}


int serialNumToMac(const char *pcSerialNum, char *pcMac, int iMacLen)
{
    char acMac[13] = {0};
    char acYear[3] = {0};
    char acMonth[3] = {0};
    char acDay[3] = {0};
    char acSn[4] = {0};
    char acDate[4] = {0};
    int iYear = 0;
    int iMonth = 0;
    int iDay = 0;
    int iDate = 0;


    if(NULL == pcSerialNum && (strlen(pcSerialNum) != 16))
    {
    	__android_log_print(ANDROID_LOG_INFO, "DMNM", "DMNM serialNumToMac input param error");
        return -1;
    }

    strncpy(acYear, pcSerialNum+6, 2);
    strncpy(acMonth, pcSerialNum+8, 2);
    strncpy(acDay, pcSerialNum+10, 2);
    strncpy(acSn, pcSerialNum+13, 3);

    sscanf(acYear, "%d", &iYear);
    sscanf(acMonth, "%d", &iMonth);
    sscanf(acDay, "%d", &iDay);

    iDate = (iYear-12)*512 + (iMonth-1)*32 + iDay;

    sprintf(acDate, "%03X", iDate);                   // modify by cs 20120927

    if(iMacLen >= 18)
    {
    	sprintf(pcMac,"00:50:2A:%c%c:%c%c:%c%c",acDate[0],acDate[1],acDate[2],acSn[0],acSn[1],acSn[2]);
    }
    __android_log_print(ANDROID_LOG_INFO, "DMNM", "DMNM serialNumToMac %s", pcMac);
    return 0;
}




void DMNMConnectStatic(char* pcIP, char* pcMask, char* pcGateway, char* pcNDS)
{

	char acCMD[128] = {0x0};
	char acIp[NM_ADD_IP_LEN] = {0x00};
	char acMask[NM_ADD_IP_LEN] = {0x00};
	char acGateway[NM_ADD_IP_LEN] = {0x00};
	char acDns[NM_ADD_IP_LEN] = {0x00};


	//修改IP地址
	if(pcIP && pcMask)
	{
		memset(acIp, 0x00, NM_ADD_IP_LEN);
		strcpy(acIp, pcIP);
		memset(acMask, 0x00, NM_ADD_IP_LEN);
		strcpy(acMask, pcMask);
		sprintf(acCMD,"busybox ifconfig eth0 %s netmask %s ",delPreZero(acIp),delPreZero(acMask));
		__android_log_print(ANDROID_LOG_INFO, "DMNM", "DMNM exec:%s", acCMD);
		system(acCMD);
	}

	//修改网关
	if(pcGateway)
	{
		memset(acGateway, 0x00, NM_ADD_IP_LEN);
		strcpy(acGateway, pcGateway);
		sprintf(acCMD,"busybox route add default gw %s",delPreZero(acGateway));
		__android_log_print(ANDROID_LOG_INFO, "DMNM", "DMNM exec:%s", acCMD);
		system(acCMD);
	}

	//设置DNS
	if(pcNDS)
	{
		memset(acDns, 0x00, NM_ADD_IP_LEN);
		strcpy(acDns, pcNDS);
		sprintf(acCMD, "setprop net.dns1 %s", delPreZero(acDns));
		__android_log_print(ANDROID_LOG_INFO, "DMNM", "DMNM exec:%s", acCMD);
		system(acCMD);
	}
}

void DMNMChangeMac()
{
	char acCMD[128] = {0x0};

	//修改mac地址
	if(g_stIdbtCfg.stNetWorkInfo.acMac)
	{
		sprintf(acCMD,"busybox ifconfig eth0 hw ether %s",g_stIdbtCfg.stNetWorkInfo.acMac);
		system("busybox ifconfig eth0 down");
		if(system(acCMD) == 0)
		{
			//LOG_RUNLOG_DEBUG("DMNM set mac: %s", g_stIdbtCfg.stNetWorkInfo.acMac);
		}
		system("busybox ifconfig eth0 up");
	}
}

#endif /* DMNMUTILS_C_ */
