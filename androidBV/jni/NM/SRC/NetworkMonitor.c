/***************************************************************
  Copyright (C), 1988-2012, YouKe Tech. Co., Ltd.
  @file:
  @author:			hb
  @date:			2012.4.27
  @description:		//����
  @version:			//�汾
  @history:         // ��ʷ�޸ļ�¼
  		<author>	<time>   	<version >   <desc>
		hb	   		2012/5/7     1.0      	����ģ��
		csq         2012/06/28   1.1        �Ͱ汾busybox��֧���޸� (imx27)
***************************************************************/

#include "../INC/NetworkMonitor.h"
#include <IDBT.h>
#include <IDBTCfg.h>
#include <LOGIntfLayer.h>
#include <net/if.h>
#include <sys/ioctl.h>
#ifdef _YK_IMX27_AV_
#include "IMX27Gpio.h"
#endif
#include "../../TS/INC/TestSoft.h"			// add by cs 2012-10-22


#define ETH			"eth0"
#define PRODUCE_SOFT_PC_IP				"192.168.69.10"
#define PRODUCE_SOFT_TERMINAL_IP		"192.168.69.20"
#define PRODUCE_SOFT_TERMINAL_NETMASK 	"255.255.255.0"
#define PRODUCE_SOFT_TERMINAL_GATEWAY 	"192.168.69.1"
//#define PRODUCE_SOFT_PC_DNS "255.255.255.0"

#define REBOOT_DEVICE_TIME		120
#define MONITOR_NETWORK_TIME	30
#define RECONNECT_NETWORK_TIMES	10
#define REBOOT_DEVICE_TIMES		2

//PPPOE�����ļ�·��
#define PPPOE_CONFIG "/etc/ppp/pppoe.conf"
#define ETH0_CONFIG "/etc/eth0-setting"
#if _YK_OS_==_YK_OS_WIN32_
#define PPPD_CONFIG "E:/idbt/NetworkMonitor/options"
#else
#define PPPD_CONFIG "/etc/ppp/options"
#endif

#ifndef _YK_XT8126_BV_
static int iProduceSoft = 80;
#else
static int iProduceSoft = 2;
#endif
static BOOL bIsConnected = FALSE;
static BOOL g_bNetworkRunning = TRUE;	//��������ƿ���   TRUE--����   FALSE--ֹͣ
static BOOL g_bNetInfoChanged = FALSE;	//���������Ƿ����
static char* g_acPingIMS = "ping 42.120.52.115 -c 1 > /tmp/NM";	//����ping IMS

#if _YK_OS_==_YK_OS_WIN32_
static char* g_acPingDNS = "ping 218.85.157.99 -n 1 > nul";	//��������:����ping DNS
#else
static char g_acPingDNS[128];// = "ping 218.85.157.99 -c 1 > /tmp/NM";	//��������:����ping DNS
#endif

#define MAXINTERFACES   16

char server_host[64] = {0x0};
int server_port = 0;
static YKHandle g_hNetworkHndl = NULL;	//�������߳̾��

int NMNetIsOK()
{
	if(iProduceSoft == 2 && bIsConnected == TRUE)
	{
		return 1;
	}
	return 0;
}

int NMNetTest()
{
	int iRst = RST_OK;
	if(server_host[0] == 0x0)
	{
		TMGetHost(server_host,&server_port);
	}
	//�ж��������
	iRst = LPUtilsNetConnect(server_host, server_port);
	if(RST_OK != iRst)
	{
		iRst = LPUtilsNetConnect(g_stIdbtCfg.stDestIpInfo.acDestIP1, g_stIdbtCfg.stDestIpInfo.port1);
	}
	if(RST_OK != iRst)
	{
		iRst = LPUtilsNetConnect(g_stIdbtCfg.stDestIpInfo.acDestIP2, g_stIdbtCfg.stDestIpInfo.port2);
	}
	return iRst;
}

int NMGetNetCardIP(char *cardName,char *pcIP)
{
	int s;
	struct ifconf conf;
	struct ifreq *ifr;
	char buff[BUFSIZ];
	int num;
	int i;

	s = socket(PF_INET, SOCK_DGRAM, 0);
	conf.ifc_len = BUFSIZ;
	conf.ifc_buf = buff;

	ioctl(s, SIOCGIFCONF, &conf);//��ñ��������нӿ�
	num = conf.ifc_len / sizeof(struct ifreq);//�ܵ�buffer���� ���� ÿ���ӿڵĳ���
	ifr = conf.ifc_req;

	for(i=0;i < num;i++)
	{
		struct sockaddr_in *sin = (struct sockaddr_in *)(&ifr->ifr_addr);
		ioctl(s, SIOCGIFFLAGS, ifr);//��ȡ�ӿڱ�ʶ

		if(((ifr->ifr_flags & IFF_LOOPBACK) == 0) && (ifr->ifr_flags & IFF_UP))
		{
			close(s);
			if(strncmp(ifr->ifr_name, cardName,3) == 0)
			{
				strcpy(pcIP, inet_ntoa(sin->sin_addr));
				return TRUE;
			}
		}
		ifr++;
	}
	close(s);
	return FALSE;
}

int NMGetPPPoeIpAddress(char *pcIP)
{
	return NMGetNetCardIP("ppp",pcIP);
}

int NMGetIpAddress(char *pcIP)
{
	return NMGetNetCardIP("eth",pcIP);
}

void NMGetIpAddress2(char *pcIP)
{
	char buf[64];
	char *p;
	uint32_t Ip_Int = 0;
	NMGetIpAddress(buf);
	Ip_Int = inet_addr(buf);
	memcpy(pcIP, &Ip_Int, 4);
}

int NMGetLocalMacByName(char *pcMac,char *pcName)
{
	int s;
	struct ifconf conf;
	struct ifreq *ifr;
	char buff[BUFSIZ];
	int num;
	int i;

	s = socket(PF_INET, SOCK_DGRAM, 0);
	conf.ifc_len = BUFSIZ;
	conf.ifc_buf = buff;

	ioctl(s, SIOCGIFCONF, &conf);//��ñ��������нӿ�
	num = conf.ifc_len / sizeof(struct ifreq);//�ܵ�buffer���� ���� ÿ���ӿڵĳ���
	ifr = conf.ifc_req;

	for(i=0;i < num;i++)
	{
		struct sockaddr_in *sin = (struct sockaddr_in *)(&ifr->ifr_addr);
		ioctl(s, SIOCGIFHWADDR, ifr);//��ȡ�ӿڱ�ʶ

		if(((ifr->ifr_flags & IFF_LOOPBACK) == 0) && (ifr->ifr_flags & IFF_UP))
		{
			close(s);
			if(strncmp(ifr->ifr_name, pcName,3) == 0)
			{
				sprintf(pcMac,"%02X:%02X:%02X:%02X:%02X:%02X",
				(unsigned char)ifr->ifr_hwaddr.sa_data[0],
				(unsigned char)ifr->ifr_hwaddr.sa_data[1],
				(unsigned char)ifr->ifr_hwaddr.sa_data[2],
				(unsigned char)ifr->ifr_hwaddr.sa_data[3],
				(unsigned char)ifr->ifr_hwaddr.sa_data[4],
				(unsigned char)ifr->ifr_hwaddr.sa_data[5]);
				printf("NM NMGetLocalMacByName name : %s ,mac : %s\n",ifr->ifr_name,pcMac);
				return TRUE;
			}
		}
		ifr++;
	}
	close(s);
	return FALSE;
}

int NMGetRunningStatus()
{
	int s;
	struct ifconf conf;
	struct ifreq *ifr;
	char buff[BUFSIZ];
	int num;
	int i;

	s = socket(PF_INET, SOCK_DGRAM, 0);
	conf.ifc_len = BUFSIZ;
	conf.ifc_buf = buff;

	ioctl(s, SIOCGIFCONF, &conf);//��ñ��������нӿ�
	num = conf.ifc_len / sizeof(struct ifreq);//�ܵ�buffer���� ���� ÿ���ӿڵĳ���
	ifr = conf.ifc_req;

	for(i=0;i < num;i++)
	{
		struct sockaddr_in *sin = (struct sockaddr_in *)(&ifr->ifr_addr);
		ioctl(s, SIOCGIFFLAGS, ifr);//��ȡ�ӿڱ�ʶ

		if(((ifr->ifr_flags & IFF_LOOPBACK) == 0) && (ifr->ifr_flags & IFF_UP))
		{
			close(s);
			if(strncmp(ifr->ifr_name, "eth",3) == 0)
			{
				if((ifr->ifr_flags & IFF_LOOPBACK) == 0)
				{
					return TRUE;
				}
				else
				{
					return FALSE;
				}
			}
		}
		ifr++;
	}
	close(s);
	return FALSE;
}

int checkIsHexNum(char c)
{
	if( (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f') )
	{
		return 1;
	}
	return 0;
}

int serialNumToMac(const char *pcSerialNum, char *pcMac, int iMacLen)
{
    char acMac[13] = {0};
    char acYear[3] = {0};
    char acMonth[3] = {0};
    char acDay[3] = {0};
    char acSn[4] = {0};
    char acDate[5] = {0};
    int iYear = 0;
    int iMonth = 0;
    int iDay = 0;
    int iDate = 0;


    if(NULL == pcSerialNum && (strlen(pcSerialNum) != 16))
    {
    	LOG_RUNLOG_ERROR("NM serialNumToMac is error!\n");
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

    if(iMacLen >= 18 && checkIsHexNum(acSn[0]) && checkIsHexNum(acSn[1]) && checkIsHexNum(acSn[2]))
    {
    	sprintf(pcMac,"00:50:2A:%c%c:%c%c:%c%c",acDate[0],acDate[1],acDate[2],acSn[0],acSn[1],acSn[2]);
    }
    else
    {
    	LOG_RUNLOG_DEBUG("NM serialNumToMac parameter is error");
    	return -1;
    }
    LOG_RUNLOG_DEBUG("NM mac:%s",pcMac);

    return 0;
}


/**************************************************************
  @brief:       		NMDisconnectPppoe
  @Description:			ʹ��pppd�Ͽ���������
**************************************************************/
void NMDisconnectPppoe(void)
{
	//ppp�Ͽ�����
	//printf("===killing pppd====");
	//cut ��ȡ���ݵĵڼ�λ���ڼ�λ �� xargs ɱ�����
	system("ps |grep 'pppd'|grep -v 'grep'|cut -c 0-6|xargs kill -9");
	//awk   /[p]ppd/ ����  pppd  {print \"kill\",$1} printf���� kill pid  sh ִ��shell
	//system("ps |awk '/[p]ppd/{print \"kill\",$1}'|sh");
	//system("ps |grep 'pppd' | grep -v 'grep'|awk   '{print   $1} '`");
	//printf("===killed pppd====");
}

/**************************************************************
  @brief:       		NMConnectPppoe
  @Description:			ʹ��pppd������������
**************************************************************/
void NMConnectPppoe(void)
{
   char acCMD[64] = {0x0};
   //���IP
   memset(acCMD, 0x0, sizeof(acCMD));
   sprintf(acCMD, "busybox ifconfig %s  0.0.0.0", ETH_NAME);
   system(acCMD);
   //PPP����
   system("busybox pppd");
}

/**************************************************************
  @brief:       		NMUpdatePppdConfigFile
  @Description:			����pppd�����ļ�
**************************************************************/
void NMUpdatePppdConfigFile(void)
{
	FILE *hOptFd;
	char buf[128];

	//��PPPD�����ļ�
	hOptFd = open(PPPD_CONFIG, O_RDWR|O_CREAT|O_TRUNC);

	bzero(buf, sizeof(buf));
	sprintf(buf, "lock\n");
	write(hOptFd, buf, strlen(buf));

	bzero(buf, sizeof(buf));
	sprintf(buf, "plugin rp-pppoe.so eth0\n");
	write(hOptFd, buf, strlen(buf));

	bzero(buf, sizeof(buf));
	sprintf(buf, "user \"%s\"\n", g_stIdbtCfg.stNetWorkInfo.acWanUserName);
	write(hOptFd, buf, strlen(buf));

	bzero(buf, sizeof(buf));
	sprintf(buf, "password \"%s\"\n", g_stIdbtCfg.stNetWorkInfo.acWanPassword);
	write(hOptFd, buf, strlen(buf));

	bzero(buf, sizeof(buf));
	sprintf(buf, "defaultroute\n");
	write(hOptFd, buf, strlen(buf));

	close(hOptFd);

	//	hOptFd=fopen(PPPD_CONFIG,"r+");
	//	if(hOptFd == NULL)
	//	{
	//		hOptFd=fopen(PPPD_CONFIG,"w+");
	//		if(hOptFd == NULL)
	//		{
	//			return;
	//		}
	//	}
	//	fprintf(buffer,"lock\n");
	//
	//	fprintf(hOptFd,"plugin rp-pppoe.so eth0\n");
	//	//����Ϊ���µĿ���˺�
	//	fprintf(hOptFd,"user \"%s\"\n", g_stIdbtCfg.stNetWorkInfo.acWanUserName);
	//	//����Ϊ���µĿ������
	//	fprintf(hOptFd,"password \"%s\"\n", g_stIdbtCfg.stNetWorkInfo.acWanPassword);
	//	fprintf(hOptFd,"defaultroute\n");
	//	//�ر�PPPD�����ļ�
	//	fclose(hOptFd);
}

/**************************************************************
  @brief:       		NMNotifyInfo
  @Description:			֪ͨ��������
**************************************************************/
void NMNotifyInfo(void)
{
	//�޸�״̬
	g_bNetInfoChanged = TRUE;
}

/**************************************************************
  @brief:       		NMChangeMac
  @Description:			�޸�MAC
**************************************************************/
void NMChangeMac()
{
	char acCMD[128] = {0x0};

	//�޸�mac��ַ
	if(g_stIdbtCfg.stNetWorkInfo.acMac)
	{
		system("ifconfig eth0 down");
		sprintf(acCMD,"busybox ifconfig eth0 hw ether %s",g_stIdbtCfg.stNetWorkInfo.acMac);
		if(system(acCMD) == 0)
		{
			LOG_RUNLOG_DEBUG("NM set mac: %s", g_stIdbtCfg.stNetWorkInfo.acMac);
		}
		system("busybox ifconfig eth0 up");
	}
}

/**************************************************************
  @brief:       		NMConnectStatic
  @Description:			��̬����
**************************************************************/
void NMConnectStatic(char* pcIP, char* pcMask, char* pcGateway, char* pcNDS)
{
	char acCMD[128] = {0x0};

	//�޸�IP��ַ
	if(pcIP && pcMask)
	{
		sprintf(acCMD,"busybox ifconfig eth0 %s netmask %s ",pcIP,pcMask);
		system(acCMD);
	}

	//�޸�����
	if(pcGateway)
	{
		sprintf(acCMD,"busybox route add default gw %s",pcGateway);
		system(acCMD);
	}

	//����DNS
	if(pcNDS)
	{
		sprintf(acCMD,"busybox echo \"nameserver %s\" > /etc/resolv.conf",pcNDS);
		system(acCMD);
	}
}

/**************************************************************
  @brief:       		NMUpdateInfo
  @Description:			��������
**************************************************************/
void NMUpdateInfo(void)
{
	TMNMGetWANInfo(g_stIdbtCfg.stNetWorkInfo.acWanUserName,g_stIdbtCfg.stNetWorkInfo.acWanPassword);
	//TMGetIMSInfo(NULL,NULL,g_stIdbtCfg.stNetWorkInfo.acIMSProxyIP,0, NULL);

	if(g_stIdbtCfg.stNetWorkInfo.acNetFlag[0] == NET_PPPOE)
	{
		//����pppd�����ļ�
		NMUpdatePppdConfigFile();
	}

}

/**************************************************************
  @brief:       		NMEventHndl
  @Description:			������ģ��
  @param[in]:			ctx---�̲߳���
**************************************************************/
void *NMEventHndl(void *ctx)
{
	int iUpdateFirstFlg = 0;
	int iRst = RST_ERR;
	int iConnectNet = 0;
	int iRebootDevice = 0;
	int iMonitorNet  = 30;
	int iUpdateInfo  = 0;
	BOOL bRebootDevice = FALSE;
	g_bNetworkRunning = TRUE;
	char acSN[64]= {0x0};
	char acMac[18]= {0x0};
	int iNtpState = -1;
	char buf[128] = {0x0};
	int j = 0,k= 0;

//	//��ȡ������Ϣ
//	if(NMGetConfig() == RST_ERR)
//	{
//		return RST_ERR;
//	}

	TMGetHost(server_host);

	sprintf(g_acPingDNS,"ping %s -c 1 > /tmp/NM",g_stIdbtCfg.stNetWorkInfo.acStaticDNS);

	if(g_stIdbtCfg.stNetWorkInfo.acNetFlag[0] == NET_PPPOE)
	{
		system("busybox mkdir /etc/ppp");
	}

#ifdef _YK_IMX27_AV_
	if(g_stIdbtCfg.stNetWorkInfo.acNetFlag[0] == NET_STATIC)
	{
		system("busybox ifconfig eth0 up");
	}
#endif

#ifndef _YK_XT8126_
	if(TMNMGetSN(acSN) == TRUE)
	{
		if(serialNumToMac(acSN,acMac,18) == 0)
		{
			strcpy(g_stIdbtCfg.stNetWorkInfo.acMac,acMac);
#ifndef _YK_PC_
			SaveIdbtConfig();
			NMChangeMac();
#endif
		}
	}
#endif

	//����DNS
#if defined(_YK_XT8126_BV_) || defined(_YK_IMX27_AV_)
	sprintf(buf,"busybox echo \"nameserver %s\" > /etc/resolv.conf",g_stIdbtCfg.stNetWorkInfo.acStaticDNS);
	system(buf);
#endif

#ifndef _YK_XT8126_BV_
	if(g_stIdbtCfg.stNetWorkInfo.acNetFlag[0] == NET_STATIC)
	{
		NMConnectStatic(g_stIdbtCfg.stNetWorkInfo.acStaticIP, g_stIdbtCfg.stNetWorkInfo.acStaticMask, g_stIdbtCfg.stNetWorkInfo.acStaticGateWay, g_stIdbtCfg.stNetWorkInfo.acStaticDNS);
	}
#endif

	//�����������
	NMUpdateInfo();

	//��������
	while(g_bNetworkRunning)
	{
		YKSleep(1*1000);

//		if(++k > 3)
//		{
//			if(NMGetRunningStatus() == FALSE)
//			{
//				printf("link down\n");
//				bIsConnected = FALSE;
//			}
//			k = 0;
//		}

#ifndef _YK_XT8126_BV_
		if(iProduceSoft == 0)
		{
			goto CONNECT;
		}
#endif
//        if (g_iTSRunFlag)			// add by cs 2012-10-22
//        {
//            YKSleep(1*1000);
//            continue;
//        }
#ifndef _YK_XT8126_BV_
		if(iProduceSoft == 1)
		{
			if(++j < 5)
			{
				continue;
			}
			iProduceSoft = 2;
			iMonitorNet = MONITOR_NETWORK_TIME;
			goto CONNECT;
		}
#endif

		//���������·�����ȴ�
		if(TRUE == bRebootDevice)
		{
			if (++iMonitorNet >= REBOOT_DEVICE_TIME)
			{
				bRebootDevice = FALSE;
			}
			continue;
		}
		//�������
		if(++iMonitorNet >= MONITOR_NETWORK_TIME)
		{
			iMonitorNet = 0;
			//�������
			iRst = NMNetTest();
			if(RST_OK == iRst)
			{
				//�Ƿ����ӳɹ�
				if(TRUE != bIsConnected)
				{
					//�������
#ifdef _YK_IMX27_AV_
					SMCtlNetLed(0);
#endif
					bIsConnected = TRUE;

					//д����־ �������ӳɹ�
					LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_NET_CONNECT, 0, "");

				}

				//��������
				iConnectNet = 0;
				iRebootDevice = 0;
			}
			//�ж��Ƿ������ݸ���
			if(TRUE == g_bNetInfoChanged)
			{
				if(iUpdateInfo++ >= 1)
				{
					//�����������
					NMUpdateInfo();
					//�޸�״̬Ϊδ�޸�
					g_bNetInfoChanged = FALSE;
					//��������
					iRst = RST_ERR;

					iUpdateInfo = 0;
				}
			}

			if(RST_OK != iRst)
			{
CONNECT:
				//�Ƿ����ӳɹ�
				if(FALSE != bIsConnected)
				{
#ifdef _YK_IMX27_AV_
					//�������
					SMCtlNetLed(1);
					//SIP����
					SMCtlSipLed(1);
#endif
					//yk����
					//SMCtlYkLed(1);

					//д����־
					LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_NET_CONNECT, 1, "");

					bIsConnected = FALSE;
				}
#ifndef _YK_XT8126_BV_
				//���õ���ģʽ
				if(iProduceSoft == 0)
				{
					//���õ���ģʽIP
					iProduceSoft = 1;
#ifndef		_YK_PC_
					NMConnectStatic(PRODUCE_SOFT_TERMINAL_IP,PRODUCE_SOFT_TERMINAL_NETMASK, PRODUCE_SOFT_TERMINAL_GATEWAY, "218.85.157.99");
#endif
					continue;
				}

				//���������ж�
				if(++iConnectNet > RECONNECT_NETWORK_TIMES)
				{
					//�����ⲿ�����豸�����Ƿ����2��
					if(iRebootDevice < REBOOT_DEVICE_TIMES)
					{
						//������1
						iRebootDevice ++ ;
#ifndef _YK_PC_
						//�����ⲿ�����豸
						SMNMDevReboot();
#endif
						//д����־ �����豸����
						LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_NET_DEVICE_REBOOT, 0, "");

						//��ʱ2���� �ȴ������豸�������
						bRebootDevice = TRUE;
						iConnectNet = 0;
						continue;
					}
					//������������
					iConnectNet = 0;
				}		
				//�������������������
				if (g_stIdbtCfg.stNetWorkInfo.acNetFlag[0] == NET_DHCP)
				{
				//modified by csq
#if  _YK_OS_==_YK_OS_WIN32_
					system("ipconfig /release");
					system("ipconfig /renew");
#else

#ifdef _YK_IMX27_AV_
					//system("ifconfig eth0 down");
					//system("udhcpc -n -q");
					//ps -ef|grep ping|grep -v grep|cut -c 0-6|xargs kill -9
					//ps -ef|grep dhclient|grep -v grep|wc -l
					system("ps -ef|grep dhclient|grep -v grep|cut -c 0-6|xargs kill -9");
					system("echo `ps -ef|grep dhclient|grep -v grep|wc -l` > /tmp/dhclient_temp");
					FILE *stream;
					char msg[5] = {0x00};
					stream = fopen("/tmp/dhclient_temp", "r");
					fgets(msg, 5, stream);
					LOG_RUNLOG_DEBUG("NM zombile count=%s", msg);
					fclose(stream);
					if(strncmp(msg, "9", 1) >= 0)
					{
						LOG_RUNLOG_ERROR("NM too many zombile(%s)", msg);
						LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_APP_REBOOT, 0, "");
						LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_SYSTEM_ERROR, 0, "107");
						LOGUploadResPrepross();
						system("reboot");
					}
					system("busybox dhclient");
#else
					system("busybox udhcpc -n -q -i eth0 > /tmp/NM");
#endif

#endif
				}
				else if(g_stIdbtCfg.stNetWorkInfo.acNetFlag[0] == NET_PPPOE)
				{
					//system("pppoe-stop");
					//ɱ��pppd����
					NMDisconnectPppoe();
					NMConnectPppoe();
				}
				else
				{
					NMConnectStatic(g_stIdbtCfg.stNetWorkInfo.acStaticIP, g_stIdbtCfg.stNetWorkInfo.acStaticMask, g_stIdbtCfg.stNetWorkInfo.acStaticGateWay, g_stIdbtCfg.stNetWorkInfo.acStaticDNS);
				}

				//�л�Ϊ�޵���
				//iProduceSoft = 0;
#endif
			}
		}
	}
	return RST_OK;
}

typedef enum
{
	NM_COMMAND_HELP,
	NM_COMMAND_DHCP,
	NM_COMMAND_STATIC,
	NM_COMMAND_PPPOE,
	NM_COMMAND_PING,
	NM_COMMAND_IFCONFIG,
	NM_COMMAND_TEST
}NM_COMMAND_EN;

/**************************************************************
 *@brief		NMProcessCommand ������֧��.
 *@param[in]:	const char *cmd ����
 *@return		RST_OK �ɹ�  RST_ERR ʧ��
**************************************************************/
int NMProcessCommand(const char *cmd)
{
	char* argv[6];
	int i=0;
	int iChoice = -1;

	for(i=0;i<5;i++)
	{
		argv[i] = NULL;
	}

	argv[0] = strtok(cmd," \0");
	for(i=0;i<5;i++)
	{
		if(argv[i] == NULL)
		{
			if(i == 0)
			{
				return RST_ERR;
			}
			break;
		}
		argv[i+1] = strtok(NULL," \0");
	}
	if(strcmp(argv[0],"help") == 0)
	{
		iChoice = NM_COMMAND_HELP;
	}
	else if(strcmp(argv[0],"dhcp") == 0)
	{
		iChoice = NM_COMMAND_DHCP;
	}
	else if(strcmp(argv[0],"static") == 0)
	{
		iChoice = NM_COMMAND_STATIC;
	}
	else if(strcmp(argv[0],"pppoe") == 0)
	{
		iChoice = NM_COMMAND_PPPOE;
	}
	else if(strcmp(argv[0],"ping") == 0)
	{
		iChoice = NM_COMMAND_PING;
	}
	else if(strcmp(argv[0], "ifconfig") == 0)
	{
		iChoice = NM_COMMAND_IFCONFIG;
	}
	else if(strcmp(argv[0], "test") == 0)
	{
		iChoice = NM_COMMAND_TEST;
	}
	else
	{
		LOG_RUNLOG_DEBUG("NM ===================================================================");
		LOG_RUNLOG_DEBUG("NM input error command ,you can input \"NMC help\" to look for command");
		LOG_RUNLOG_DEBUG("NM ===================================================================");
	}

	switch(iChoice)
	{
	case NM_COMMAND_HELP:
		LOG_RUNLOG_DEBUG("NM ===================================================");
		LOG_RUNLOG_DEBUG("NM ---------------help----------------look for command");
		LOG_RUNLOG_DEBUG("NM ---------------dhcp---------------connect with dhcp");
		LOG_RUNLOG_DEBUG("NM ---------------pppoe--------------connect with dhcp");
		LOG_RUNLOG_DEBUG("NM ---------------ping-------------------check network");
		LOG_RUNLOG_DEBUG("NM static [IP] [MASK] [GW] [DNS]---connect with static");
		LOG_RUNLOG_DEBUG("NM ===================================================");
		break;

	case NM_COMMAND_PING:
		system("busybox ping 196.196.196.1 -c 1");
		system("busybox ping 218.85.157.99 -c 1");
		system("busybox ping 61.131.4.71 -c 1");
		break;
	case NM_COMMAND_IFCONFIG:
		system("busybox ifconfig");
		break;
	case NM_COMMAND_DHCP:
		system("busybox udhcpc -n -q -i eth0 > /tmp/NM");
		g_stIdbtCfg.stNetWorkInfo.acNetFlag[0] = '1';
		SaveIdbtConfig();
		break;

	case NM_COMMAND_TEST:
		system("busybox ifconfig eth0 196.196.196.234 netmask 255.255.0.0");
		system("busybox route add default gw 196.196.196.1");
		break;
	case NM_COMMAND_PPPOE:
		//system("pppoe-stop");
		NMDisconnectPppoe();
		NMConnectPppoe();
		g_stIdbtCfg.stNetWorkInfo.acNetFlag[0] = '2';
		SaveIdbtConfig();
		break;
	case NM_COMMAND_STATIC:
		if(argv[1] && argv[2] && argv[3] && argv[4] )
		{
			NMConnectStatic(argv[1], argv[2], argv[3], argv[4]);
			g_stIdbtCfg.stNetWorkInfo.acNetFlag[0] = '3';
			strcpy(g_stIdbtCfg.stNetWorkInfo.acStaticIP,argv[1]);
			strcpy(g_stIdbtCfg.stNetWorkInfo.acStaticMask,argv[2]);
			strcpy(g_stIdbtCfg.stNetWorkInfo.acStaticGateWay,argv[3]);
			strcpy(g_stIdbtCfg.stNetWorkInfo.acStaticDNS,argv[4]);
			sprintf(g_acPingDNS,"ping %s -c 1 > /tmp/NM",g_stIdbtCfg.stNetWorkInfo.acStaticDNS);
			SaveIdbtConfig();
		}
//		else if(argv[1]==NULL && argv[2]==NULL && argv[3]==NULL && argv[4]==NULL )
//		{
//			NMConnectStatic(g_stIdbtCfg.stNetWorkInfo.acStaticIP, g_stIdbtCfg.stNetWorkInfo.acStaticMask, g_stIdbtCfg.stNetWorkInfo.acStaticGateWay, g_stIdbtCfg.stNetWorkInfo.acStaticDNS);
//		}
		else
		{
			LOG_RUNLOG_DEBUG("NM ===================================================");
			LOG_RUNLOG_DEBUG("NM ==============parameter is error===================");
			LOG_RUNLOG_DEBUG("NM ==============you can use like below===============");
			LOG_RUNLOG_DEBUG("NM static [IP] [MASK] [GW] [DNS]");
			LOG_RUNLOG_DEBUG("NM NOW: IP--%s  Mask--%s Gateway--%s  DNS--%s ",g_stIdbtCfg.stNetWorkInfo.acStaticIP, g_stIdbtCfg.stNetWorkInfo.acStaticMask, g_stIdbtCfg.stNetWorkInfo.acStaticGateWay, g_stIdbtCfg.stNetWorkInfo.acStaticDNS);
			LOG_RUNLOG_DEBUG("NM ===================================================");
		}
		break;

	default:
		break;
	}

	return RST_OK;
}


/*
 *   ����ļ����ڡ�
 *   *@return		1 ����  ��0 ������
 */
BOOL  exist_file(const char * filename)
{
	return (access(filename, 0) == 0);
}
/**************************************************************
  @brief:       		NMInit
  @Description:			ģ�����
  @return:				RST_OK �ɹ�  RST_ERR ʧ��
**************************************************************/
int NMInit(void)
{
#ifdef _YK_IMX27_AV_
	//ɾ�� /bin/ping
	if (exist_file("/bin/ping"))
	{
		if (exist_file("/usr/sbin/ping"))
		{
			system("rm /bin/ping");
			LOG_RUNLOG_DEBUG("NM rm /bin/ping");
		}
	}

#endif

	//�����������߳�
	g_hNetworkHndl = YKCreateThread(NMEventHndl,NULL);
	if(g_hNetworkHndl == NULL)
	{
		//IDBT_DEBUG("Create network thread failed\n");
		return FALSE;
	}
	return TRUE;
}

/**************************************************************
  @brief:       		NMFini
  @Description:			ģ���˳�
**************************************************************/
void NMFini(void)
{
#if  _YK_OS_==_YK_OS_LINUX_
	//�˳�pppoe
	if(g_stIdbtCfg.stNetWorkInfo.acNetFlag[0] == NET_PPPOE)
	{
		//system("pppoe-stop");
		NMDisconnectPppoe();
		LOG_RUNLOG_DEBUG("NM PPPOE-STOP-END\n");
	}
#endif
	g_bNetworkRunning = FALSE;
	//�˳�������  �ر��߳�
	YKJoinThread(g_hNetworkHndl);
	YKDestroyThread(g_hNetworkHndl);
}

#if 0
void main(void)
{
	int iRst = RST_ERR;
	/* ����ģ���ʼ�� */
	iRst = NMInit();
	if (RST_ERR == iRst) {
		//IDBT_CRIT("app main: failed to init network module.");
		goto EXIT;
	}
	while(1)
	{
		Sleep(1);
	}
EXIT:
	NMFini();
	LOG_RUNLOG_DEBUG("NM %s : Exit NETWORK Module Successfully !\n", __FILE__);
	system("pause");
}
#endif
