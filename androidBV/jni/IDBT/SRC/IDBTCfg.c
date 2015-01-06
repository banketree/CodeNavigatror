/*
 * IDBTCfg.c
 *
 *  Created on: 2013-4-7
 *      Author: root
 */

#include <stdio.h>
#include "IDBTCfg.h"
#ifdef LOG_MODULE
#include "LOGIntfLayer.h"
#endif
#include <android/log.h>

IDBT_CONFIG_ST g_stIdbtCfg = { 0 };

/********************************************************************
*Function	:	SaveIdbtConfig
*Input		:	NULL
*Output		:	NULL
*Description:	获取音量大小
*Others		:
*********************************************************************/
int SaveIdbtConfig(void)
{
	FILE *pCfgFd = NULL;
	char *pcHelp = "\
	--------------------help info--------------------\n\
	log_upload_mode(1-clock 2-cycle)\n\
	log_upload_cycle_time(unit:hour)\n\
	network_type(1-DHCP 2-PPPOE 3-STATIC)\n\
	video_quality(0~50)\n\				
	--------------------help info--------------------\n\
	";

	pCfgFd=fopen(IDBT_CONFIG_PATH,"w+");
	if(pCfgFd != NULL)
	{
		fprintf(pCfgFd,"sn=%s\n",g_stIdbtCfg.acSn);
		fprintf(pCfgFd,"log_upload_mode=%d\n",g_stIdbtCfg.iLogUploadMode);
		fprintf(pCfgFd,"log_upload_cycle_time=%d\n",g_stIdbtCfg.iLogUploadCycleTime);
		fprintf(pCfgFd,"management_password=%s\n",g_stIdbtCfg.acManagementPassword);
		fprintf(pCfgFd,"opendoor_password=%s\n",g_stIdbtCfg.acOpendoorPassword);
		//网络相关参数
		fprintf(pCfgFd,"network_type=%s\n",g_stIdbtCfg.stNetWorkInfo.acNetFlag);
		fprintf(pCfgFd,"ip_addr=%s\n",g_stIdbtCfg.stNetWorkInfo.acStaticIP);
		fprintf(pCfgFd,"net_mask=%s\n",g_stIdbtCfg.stNetWorkInfo.acStaticMask);
		fprintf(pCfgFd,"gateway=%s\n", g_stIdbtCfg.stNetWorkInfo.acStaticGateWay);
		fprintf(pCfgFd,"dns_server=%s\n", g_stIdbtCfg.stNetWorkInfo.acStaticDNS);
		fprintf(pCfgFd,"mac_addr=%s\n", g_stIdbtCfg.stNetWorkInfo.acMac);

		fprintf(pCfgFd,"pppoe_user=%s\n", g_stIdbtCfg.stNetWorkInfo.acWanUserName);
		fprintf(pCfgFd,"pppoe_passwd=%s\n", g_stIdbtCfg.stNetWorkInfo.acWanPassword);

		fprintf(pCfgFd,"dest_ip_1=%s\n", g_stIdbtCfg.stDestIpInfo.acDestIP1);
		fprintf(pCfgFd,"port1=%d\n", g_stIdbtCfg.stDestIpInfo.port1);
		fprintf(pCfgFd,"dest_ip_2=%s\n", g_stIdbtCfg.stDestIpInfo.acDestIP2);
		fprintf(pCfgFd,"port2=%d\n", g_stIdbtCfg.stDestIpInfo.port2);
		fprintf(pCfgFd,"video_quality=%d\n", g_stIdbtCfg.iVedioQuality);
		fprintf(pCfgFd,"device_position=%s\n",g_stIdbtCfg.acPositionCode);
		fprintf(pCfgFd,"video_rotation=%d\n",g_stIdbtCfg.iRotation);
		fprintf(pCfgFd,"network environment=%d\n",g_stIdbtCfg.iNetMode);
		fprintf(pCfgFd,"video_format=%d\n", g_stIdbtCfg.iVedioFmt);
		fprintf(pCfgFd,"negotiateMode=%d\n", g_stIdbtCfg.iNegotiateMode);
		fprintf(pCfgFd,"reserve1=\n");
		fprintf(pCfgFd,"reserve2=\n");
		fprintf(pCfgFd,"reserve3=\n");
		fprintf(pCfgFd,"reserve4=\n");
		fprintf(pCfgFd,"reserve5=\n");

		fprintf(pCfgFd,"%s\n", pcHelp);

		//关闭配置文件
		fclose(pCfgFd);

		return 0;
	}
	else
	{
		return -1;
	}
}

/********************************************************************
*Function	:	LoadVolumeConfig
*Input		:	NULL
*Output		:	NULL
*Description:	获取音量大小
*Others		:
*********************************************************************/
void LoadIdbtConfig(void)
{
	FILE *pCfgFd;

	char *pcHelp = "\
--------------------help info--------------------\n\
log_upload_mode(1-clock 2-cycle)\n\
log_upload_cycle_time(unit:hour)\n\
network_type(1-DHCP 2-PPPOE 3-STATIC)\n\
--------------------help info--------------------\n\
";
	strcpy(g_stIdbtCfg.acSn, SN_NULL);
//	g_stIdbtCfg.iInputVol = 5;
//	g_stIdbtCfg.iOutputCallVol = -12;
//	g_stIdbtCfg.iOutputPlayWavVol = 2;
//	g_stIdbtCfg.iVedioFps = 10;
//	g_stIdbtCfg.iVedioBr = 64;
	g_stIdbtCfg.iLogUploadMode = 2;
	g_stIdbtCfg.iLogUploadCycleTime = 6;
	g_stIdbtCfg.stNetWorkInfo.acNetFlag[0] = NET_DHCP;
	strcpy(g_stIdbtCfg.stNetWorkInfo.acStaticIP,"196.196.197.213");
	strcpy(g_stIdbtCfg.stNetWorkInfo.acStaticMask,"255.255.0.0");
	strcpy(g_stIdbtCfg.stNetWorkInfo.acStaticGateWay,"196.196.196.1");
	strcpy(g_stIdbtCfg.stNetWorkInfo.acStaticDNS,"218.85.157.99");
	strcpy(g_stIdbtCfg.stNetWorkInfo.acMac, "08:90:90:90:90:90");
	strcpy(g_stIdbtCfg.acManagementPassword, "999999");
	strcpy(g_stIdbtCfg.acOpendoorPassword, "123456");


	strcpy(g_stIdbtCfg.stNetWorkInfo.acWanUserName, "15306919497");
	strcpy(g_stIdbtCfg.stNetWorkInfo.acWanPassword, "2012180");

	strcpy(g_stIdbtCfg.stDestIpInfo.acDestIP1, "115.239.210.26");
	g_stIdbtCfg.stDestIpInfo.port1 = 80;
	strcpy(g_stIdbtCfg.stDestIpInfo.acDestIP2, "220.162.97.165");
	g_stIdbtCfg.stDestIpInfo.port2 = 80;
	g_stIdbtCfg.iVedioQuality = 20;
	strcpy(g_stIdbtCfg.acPositionCode,"M00010100000");
	g_stIdbtCfg.iRotation = 0;
	g_stIdbtCfg.iNetMode = 0;
	g_stIdbtCfg.iVedioFmt = 0;//cif
	g_stIdbtCfg.iNegotiateMode = 0;//手动模式

	//打开IDBT配置文件
	pCfgFd=fopen(IDBT_CONFIG_PATH,"r");

	if(pCfgFd != NULL)
	{
		char buf[300];
		fread(buf,300,1,pCfgFd);
		fseek(pCfgFd,0,SEEK_SET);
		fscanf(pCfgFd,"sn=%s\n",g_stIdbtCfg.acSn);
		__android_log_print(ANDROID_LOG_INFO, "IDBTCfg", "sn=%s\n",g_stIdbtCfg.acSn);
		fscanf(pCfgFd,"log_upload_mode=%d\n",&g_stIdbtCfg.iLogUploadMode);
		fscanf(pCfgFd,"log_upload_cycle_time=%d\n",&g_stIdbtCfg.iLogUploadCycleTime);
		fscanf(pCfgFd,"management_password=%s\n",g_stIdbtCfg.acManagementPassword);
		fscanf(pCfgFd,"opendoor_password=%s\n",g_stIdbtCfg.acOpendoorPassword);
		//网络相关参数
		fscanf(pCfgFd,"network_type=%s\n",g_stIdbtCfg.stNetWorkInfo.acNetFlag);
		fscanf(pCfgFd,"ip_addr=%s\n",g_stIdbtCfg.stNetWorkInfo.acStaticIP);
		fscanf(pCfgFd,"net_mask=%s\n",g_stIdbtCfg.stNetWorkInfo.acStaticMask);
		fscanf(pCfgFd,"gateway=%s\n", g_stIdbtCfg.stNetWorkInfo.acStaticGateWay);
		fscanf(pCfgFd,"dns_server=%s\n", g_stIdbtCfg.stNetWorkInfo.acStaticDNS);
		fscanf(pCfgFd,"mac_addr=%s\n", g_stIdbtCfg.stNetWorkInfo.acMac);


		fscanf(pCfgFd,"pppoe_user=%s\n", g_stIdbtCfg.stNetWorkInfo.acWanUserName);
		fscanf(pCfgFd,"pppoe_passwd=%s\n", g_stIdbtCfg.stNetWorkInfo.acWanPassword);

		fscanf(pCfgFd,"dest_ip_1=%s\n", g_stIdbtCfg.stDestIpInfo.acDestIP1);
		fscanf(pCfgFd,"port1=%d\n", &g_stIdbtCfg.stDestIpInfo.port1);
		fscanf(pCfgFd,"dest_ip_2=%s\n", g_stIdbtCfg.stDestIpInfo.acDestIP2);
		fscanf(pCfgFd,"port2=%d\n", &g_stIdbtCfg.stDestIpInfo.port2);
		fscanf(pCfgFd,"video_quality=%d\n", &g_stIdbtCfg.iVedioQuality);
		fscanf(pCfgFd,"device_position=%s\n",g_stIdbtCfg.acPositionCode);
		fscanf(pCfgFd,"video_rotation=%d\n",&g_stIdbtCfg.iRotation);
		fscanf(pCfgFd,"network environment=%d\n",&g_stIdbtCfg.iNetMode);
		fscanf(pCfgFd,"video_format=%d\n", &g_stIdbtCfg.iVedioFmt);

		//关闭配置文件
		fclose(pCfgFd);
#ifdef LOG_MODULE
		LOG_RUNLOG_DEBUG("read config file succeed!");
#else
		__android_log_print(ANDROID_LOG_INFO, "IDBTCfg", "read config file succeed!");
#endif
		return;
	}
	else
	{
#ifdef LOG_MODULE
		LOG_RUNLOG_DEBUG("open config file failed!");
#else
		__android_log_print(ANDROID_LOG_INFO, "IDBTCfg", "open config file failed!");
#endif
	}


	pCfgFd=fopen(IDBT_CONFIG_PATH,"w+");
	if(pCfgFd != NULL)
	{
		fprintf(pCfgFd,"sn=%s\n", g_stIdbtCfg.acSn);
		fprintf(pCfgFd,"log_upload_mode=%d\n",g_stIdbtCfg.iLogUploadMode);
		fprintf(pCfgFd,"log_upload_cycle_time=%d\n",g_stIdbtCfg.iLogUploadCycleTime);
		fprintf(pCfgFd,"management_password=%s\n",g_stIdbtCfg.acManagementPassword);
		fprintf(pCfgFd,"opendoor_password=%s\n",g_stIdbtCfg.acOpendoorPassword);
		//网络相关参数
		fprintf(pCfgFd,"network_type=%s\n",g_stIdbtCfg.stNetWorkInfo.acNetFlag);
		fprintf(pCfgFd,"ip_addr=%s\n",g_stIdbtCfg.stNetWorkInfo.acStaticIP);
		fprintf(pCfgFd,"net_mask=%s\n",g_stIdbtCfg.stNetWorkInfo.acStaticMask);
		fprintf(pCfgFd,"gateway=%s\n", g_stIdbtCfg.stNetWorkInfo.acStaticGateWay);
		fprintf(pCfgFd,"dns_server=%s\n", g_stIdbtCfg.stNetWorkInfo.acStaticDNS);
		fprintf(pCfgFd,"mac_addr=%s\n", g_stIdbtCfg.stNetWorkInfo.acMac);

		fprintf(pCfgFd,"pppoe_user=%s\n", g_stIdbtCfg.stNetWorkInfo.acWanUserName);
		fprintf(pCfgFd,"pppoe_passwd=%s\n", g_stIdbtCfg.stNetWorkInfo.acWanPassword);

		fprintf(pCfgFd,"dest_ip_1=%s\n", g_stIdbtCfg.stDestIpInfo.acDestIP1);
		fprintf(pCfgFd,"port1=%d\n", g_stIdbtCfg.stDestIpInfo.port1);
		fprintf(pCfgFd,"dest_ip_2=%s\n", g_stIdbtCfg.stDestIpInfo.acDestIP2);
		fprintf(pCfgFd,"port2=%d\n", g_stIdbtCfg.stDestIpInfo.port2);
		fprintf(pCfgFd,"video_quality=%d\n", g_stIdbtCfg.iVedioQuality);
		fprintf(pCfgFd,"device_position=%s\n",g_stIdbtCfg.acPositionCode);
		fprintf(pCfgFd,"video_rotation=%d\n",g_stIdbtCfg.iRotation);
		fprintf(pCfgFd,"network environment=%d\n",g_stIdbtCfg.iNetMode);
		fprintf(pCfgFd,"video_format=%d\n", g_stIdbtCfg.iVedioFmt);
		fprintf(pCfgFd,"reserve1=\n");
		fprintf(pCfgFd,"reserve2=\n");
		fprintf(pCfgFd,"reserve3=\n");
		fprintf(pCfgFd,"reserve4=\n");
		fprintf(pCfgFd,"reserve5=\n");

		fprintf(pCfgFd,"%s\n", pcHelp);
	}
	//关闭配置文件
	fclose(pCfgFd);
}



