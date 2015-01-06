/************************************************************
 *  File:
 *  Author:
 *  Version :
 *  Date:
 *  Description:
 *  History:
 *            <chensq>  <2012-03-31>  <����δ������ʱ�����Լ��������߹���>
*            <chensq>  <2012-04-01>  <����SIPע��״̬����������ʾ����>
 ***********************************************************/

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>

#include <linux/fb.h>

#include <LANIntfLayer.h>
//#include <linux/ethtool.h>
#include <net/if.h>
#include <linux/sockios.h>

typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned char u8;

#define ETHTOOL_GLINK		0x0000000a /* Get link status (ethtool_value) */
#define ETHTOOL_GSET		0x00000001 /* Get settings. */
#define ETHTOOL_SSET		0x00000002 /* Set ����settings, privileged. */
/* for passing single values */
struct ethtool_value {
	u32 cmd;
	u32 data;
};
struct ethtool_cmd {
	u32 cmd;
	u32 supported; /* Features this interface supports */
	u32 advertising; /* Features this interface advertises */
	u16 speed; /* The forced speed, 10Mb, 100Mb, gigabit */
	u8 duplex; /* Duplex, half or full */
	u8 port; /* Which connector port */
	u8 phy_address;
	u8 transceiver; /* Which tranceiver to use */
	u8 autoneg; /* Enable or disable autonegotiation */
	u32 maxtxpkt; /* Tx pkts before generating tx int */
	u32 maxrxpkt; /* Rx pkts before generating rx int */
	u32 reserved[4];
};
#define CommonH
#include "common.h"

//��������ź�
void CheckEth(void);
int detect_ethtool(int skfd, char *ifname);
int get_ethtool_speed(int skfd, char *ifname); //��ȡ��������
int set_ethtool_speed(int skfd, char *ifname, int speed, int duplex, int autoneg); //������������

//���IP��ַ��ͻ
void CheckIPConflict(void);

void OnlineCheckFunc(void); //����ȷ�ϼ�⺯��
void TimeReportStatusFunc(void); //�豸��ʱ����״̬����

void ResetParam(void); //�ָ�����

int CursorFlag;
int ShowCursor; //�Ƿ���ʾ���

void TalkCtrlFunc(void); //�Խ����ƣ���ʾͨ��ʱ����жϳ�ʱ

extern struct audiobuf1 recbuf; //��Ƶ�ɼ����λ���
extern struct videobuf1 videorecbuf; //��Ƶ�ɼ����λ���  ����?

#ifdef _CAPTUREPIC_TO_CENTER  //����ͼƬ��������
//��鲶��ͼƬ��������
void CheckCapturePicCenter(void);
void SendCapturePicData(int CurrNo);
void SendCapturePicFinish(int CurrNo, int Result);
#endif

//timer ��ʱ��
int timer_fd;
int timer_thread_flag;
pthread_t timer_thread;
void timer_thread_func(void);
//char cRegOldFlg = 0;

int Init_Timer(void); //��ʼ����ʱ��
int Uninit_Timer(void); //�ͷŶ�ʱ��

void CheckMainFbPage(void); //����Ƿ���framebuffer �ĵ�0ҳ
//---------------------------------------------------------------------------
int Init_Timer(void) {
	pthread_attr_t attr;
	timer_thread_flag = 1;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&timer_thread, &attr, (void *) timer_thread_func, NULL);
	pthread_attr_destroy(&attr);
	if (timer_thread == 0) {
		P_Debug("don't create timer thread \n");
		return -1;
	}
	return 0;
}
//---------------------------------------------------------------------------
int Uninit_Timer(void) {
	//timer
	timer_thread_flag = 0;
	usleep(40 * 1000);
	//pthread_cancel(timer_thread);
	return 0;
}
//---------------------------------------------------------------------------
void timer_thread_func(void)
{
#if 1
	int timenum; //����

	P_Debug("create timer thread :0.001\n");

	timenum = 0;

	while (timer_thread_flag)
	{
		//���IP��ַ��ͻ
		CheckIPConflict();

		//����6��û���յ�����ȷ�ϣ�����Ϊ����
		if (Local.OnlineFlag == 1)
		{
			OnlineCheckFunc();
		}

		//�豸��ʱ����״̬����
		if (LocalCfg.ReportTime != 0)
		{
			if (Local.ReportSend == 1)
			{
				if (Local.ReportTimeNum >= (LocalCfg.ReportTime * INTRPERSEC))
				{
					Local.RandReportTime = (int) (1.0 * LocalCfg.ReportTime * rand() / (RAND_MAX + 1.0)) + 1;
					Local.ReportSend = 0;
					Local.ReportTimeNum = 0;
				}
			}
			if (Local.ReportSend == 0)
			{
				if (Local.ReportTimeNum >= (Local.RandReportTime * INTRPERSEC))
				{
					//CheckRecVideoStatus(); //��� ��Ƶ¼��״̬����gpio.c ������//del by zlin
					Local.ReportSend = 1;
					TimeReportStatusFunc();
				}
			}
			Local.ReportTimeNum++;
		}

		if ((timenum % (INTRPERSEC * 6)) == 0)

		{

#ifdef _CAPTUREPIC_TO_CENTER  //����ͼƬ��������
			//��鲶��ͼƬ��������
			CheckCapturePicCenter();
#endif
		}

		timenum++;

		if (timenum > 0xFFFFFF)

			timenum = 0;

		usleep((INTRTIME - 10) * 1000); //40ms
	}
#endif
}
//---------------------------------------------------------------------------
void ResetParam(void) //�ָ�����
{
	uint32_t Ip_Int;

	//IP��ַ
	Ip_Int = inet_addr("192.168.68.170");
	memcpy(LocalCfg.IP, &Ip_Int, 4);
	//��������
	Ip_Int = inet_addr("255.255.255.0");
	memcpy(LocalCfg.IP_Mask, &Ip_Int, 4);
	//���ص�ַ
	Ip_Int = inet_addr("192.168.68.1");
	memcpy(LocalCfg.IP_Gate, &Ip_Int, 4);
	//NS�����ƽ�������������ַ
	Ip_Int = inet_addr("192.168.68.1");
	memcpy(LocalCfg.IP_NS, &Ip_Int, 4);
	//����������ַ
	Ip_Int = inet_addr("192.168.68.1");
	memcpy(LocalCfg.IP_Server, &Ip_Int, 4);
	//�㲥��ַ
	Ip_Int = inet_addr("192.168.68.255");
	memcpy(LocalCfg.IP_Broadcast, &Ip_Int, 4);

	//Save_Setup(); //�洢����
	RefreshNetSetup(1); //ˢ����������
}
//---------------------------------------------------------------------------
#ifdef _CAPTUREPIC_TO_CENTER  //����ͼƬ��������
//��鲶��ͼƬ��������
void CheckCapturePicCenter(void)
{
	int i, j, k;
	if (Local.ConnectCentered == 0) //����������״̬
		return;
	if (Local.SendToCetnering == 1) //���ڴ�������
		return;

	for (j = 0; j < MAX_CAPTUREPIC_NUM; j++)
	{
		if (Capture_Pic_Center[j].isVilid == 1)
		{
			//����������
			pthread_mutex_lock(&Local.udp_lock);
			//���ҿ��÷��ͻ��岢���
			for (i = 0; i < UDPSENDMAX; i++)
			{
				if (Multi_Udp_Buff[i].isValid == 0)
				{
					sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0",
							LocalCfg.IP_Server[0], LocalCfg.IP_Server[1], LocalCfg.IP_Server[2], LocalCfg.IP_Server[3]);

					Multi_Udp_Buff[i].SendNum = 0; //��෢6��
					Multi_Udp_Buff[i].m_Socket = m_DataSocket;
					Multi_Udp_Buff[i].RemotePort = RemoteDataPort;
					Multi_Udp_Buff[i].CurrOrder = 0;
					//ͷ��
					memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
					//����
					Multi_Udp_Buff[i].buf[6] = CAPTUREPIC_SEND_START;
					Multi_Udp_Buff[i].buf[7] = ASK; //����

					memcpy(Multi_Udp_Buff[i].buf + 8, LocalCfg.Addr, 20);
					memcpy(Multi_Udp_Buff[i].buf + 28, Capture_Pic_Center[j].RemoteAddr, 20); //Ŀ���ַ
					memcpy(Multi_Udp_Buff[i].buf + 48, &j, sizeof(j)); //���к�

					Multi_Udp_Buff[i].buf[52] = (Capture_Pic_Center[j].recpic_tm_t->tm_year + 1900) / 256;
					Multi_Udp_Buff[i].buf[53] = (Capture_Pic_Center[j].recpic_tm_t->tm_year + 1900) % 256;
					Multi_Udp_Buff[i].buf[54] = Capture_Pic_Center[j].recpic_tm_t->tm_mon + 1;
					Multi_Udp_Buff[i].buf[55] = Capture_Pic_Center[j].recpic_tm_t->tm_mday;
					Multi_Udp_Buff[i].buf[56] = Capture_Pic_Center[j].recpic_tm_t->tm_hour;
					Multi_Udp_Buff[i].buf[57] = Capture_Pic_Center[j].recpic_tm_t->tm_min;
					Multi_Udp_Buff[i].buf[58] = Capture_Pic_Center[j].recpic_tm_t->tm_sec;

					sprintf(Local.DebugInfo, "Capture_Pic_Center[%d].jpegsize = %d\n", j, Capture_Pic_Center[j].jpegsize);
					P_Debug(Local.DebugInfo);

					memcpy(Multi_Udp_Buff[i].buf + 59, &Capture_Pic_Center[j].jpegsize, sizeof(Capture_Pic_Center[j].jpegsize)); //�ļ�����
					if ((Capture_Pic_Center[j].jpegsize % PACKDATALEN) == 0)
						Capture_Total_Package = Capture_Pic_Center[j].jpegsize / PACKDATALEN;
					else
						Capture_Total_Package = Capture_Pic_Center[j].jpegsize / PACKDATALEN + 1;
					memcpy(Multi_Udp_Buff[i].buf + 63, &Capture_Total_Package, sizeof(Capture_Total_Package)); //�ܰ���
					for (k = 0; k < Capture_Total_Package; k++)
					{
						Capture_Send_Flag[k] = 0;
					}
					Multi_Udp_Buff[i].nlength = 67;
					Multi_Udp_Buff[i].DelayTime = 100;
					Multi_Udp_Buff[i].SendDelayTime = 0; //���͵ȴ�ʱ�����  20101112
					Multi_Udp_Buff[i].isValid = 1;
					sem_post(&multi_send_sem);
					break;
				}
			}
			//�򿪻�����
			pthread_mutex_unlock(&Local.udp_lock);
			break;
		}
	}
}
//---------------------------------------------------------------------------
void SendCapturePicData(int CurrNo)
{
	int i, j;
	int CurrSize;
	short CurrPack;
	if (Local.ConnectCentered == 0)
		return;
	//P_Debug("SendCapturePicData 1\n");
	Local.SendToCetnering = 1; //���ڴ�������
	for (j = 0; j < Capture_Total_Package; j++)
	{
		if (Capture_Send_Flag[j] == 0)
		{
			//����������
			pthread_mutex_lock(&Local.udp_lock);
			//���ҿ��÷��ͻ��岢���
			for (i = 0; i < UDPSENDMAX; i++)
			{
				if (Multi_Udp_Buff[i].isValid == 0)
				{
					sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0", LocalCfg.IP_Server[0],
							LocalCfg.IP_Server[1], LocalCfg.IP_Server[2], LocalCfg.IP_Server[3]);
					Multi_Udp_Buff[i].SendNum = 0; //��෢6��
					Multi_Udp_Buff[i].m_Socket = m_DataSocket;
					Multi_Udp_Buff[i].RemotePort = RemoteDataPort;
					Multi_Udp_Buff[i].CurrOrder = 0;
					//ͷ��
					memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
					//����
					Multi_Udp_Buff[i].buf[6] = CAPTUREPIC_SEND_DATA;
					Multi_Udp_Buff[i].buf[7] = ASK; //����

					memcpy(Multi_Udp_Buff[i].buf + 8, LocalCfg.Addr, 20);
					memcpy(Multi_Udp_Buff[i].buf + 28, Capture_Pic_Center[CurrNo].RemoteAddr, 20); //Ŀ���ַ
					memcpy(Multi_Udp_Buff[i].buf + 48, &CurrNo, sizeof(CurrNo)); //���к�

					//sprintf(Local.DebugInfo, "SendCapturePicData j = %d, Capture_Total_Package = %d\n", j, Capture_Total_Package);
					//P_Debug(Local.DebugInfo);
					if (j != (Capture_Total_Package - 1))
						CurrSize = PACKDATALEN;
					else
						CurrSize = Capture_Pic_Center[CurrNo].jpegsize % PACKDATALEN;
					//sprintf(Local.DebugInfo, "SendCapturePicData CurrSize = %d\n", CurrSize);
					//P_Debug(Local.DebugInfo);
					memcpy(Multi_Udp_Buff[i].buf + 52, &CurrSize, sizeof(CurrSize)); //���ݳ���

					CurrPack = j;
					memcpy(Multi_Udp_Buff[i].buf + 56, &CurrPack, sizeof(CurrPack)); //���ݰ����

					//sprintf(Local.DebugInfo, "SendCapturePicData CurrPack = %d\n", CurrPack);
					//P_Debug(Local.DebugInfo);

					memcpy(Multi_Udp_Buff[i].buf + 58, Capture_Pic_Center[CurrNo].jpeg_pic + CurrPack * PACKDATALEN, CurrSize);

					//P_Debug("SendCapturePicData 3\n");
					Multi_Udp_Buff[i].nlength = 58 + CurrSize;
					Multi_Udp_Buff[i].DelayTime = 100;
					Multi_Udp_Buff[i].SendDelayTime = 0; //���͵ȴ�ʱ�����  20101112
					Multi_Udp_Buff[i].isValid = 1;
					sem_post(&multi_send_sem);
					break;
				}
			}
			//�򿪻�����
			pthread_mutex_unlock(&Local.udp_lock);
		}
	}
}
//---------------------------------------------------------------------------
void SendCapturePicFinish(int CurrNo, int Result)
{
	int i;
	if (Result == 1)
		P_Debug("send capture->finish(succeed)command \n");
	else
		P_Debug("send capture->finish(fail) command \n");
	//����������
	pthread_mutex_lock(&Local.udp_lock);
	//���ҿ��÷��ͻ��岢���
	for (i = 0; i < UDPSENDMAX; i++)
	{
		if (Multi_Udp_Buff[i].isValid == 0)
		{
			sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0", LocalCfg.IP_Server[0],
					LocalCfg.IP_Server[1], LocalCfg.IP_Server[2], LocalCfg.IP_Server[3]);
			Multi_Udp_Buff[i].SendNum = 0; //��෢6��
			Multi_Udp_Buff[i].m_Socket = m_DataSocket;
			Multi_Udp_Buff[i].RemotePort = RemoteDataPort;
			Multi_Udp_Buff[i].CurrOrder = 0;
			//ͷ��
			memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
			//����
			if (Result == 1)
				Multi_Udp_Buff[i].buf[6] = CAPTUREPIC_SEND_SUCC;
			else
				Multi_Udp_Buff[i].buf[6] = CAPTUREPIC_SEND_FAIL;
			Multi_Udp_Buff[i].buf[7] = ASK; //����

			memcpy(Multi_Udp_Buff[i].buf + 8, LocalCfg.Addr, 20);
			memcpy(Multi_Udp_Buff[i].buf + 28, Capture_Pic_Center[CurrNo].RemoteAddr, 20); //Ŀ���ַ
			memcpy(Multi_Udp_Buff[i].buf + 48, &CurrNo, sizeof(CurrNo)); //���к�

			Multi_Udp_Buff[i].nlength = 52;
			Multi_Udp_Buff[i].DelayTime = 100;
			Multi_Udp_Buff[i].SendDelayTime = 0; //���͵ȴ�ʱ�����  20101112
			Multi_Udp_Buff[i].isValid = 1;
			sem_post(&multi_send_sem);
			break;
		}
	}
	//�򿪻�����
	pthread_mutex_unlock(&Local.udp_lock);
}
#endif
//---------------------------------------------------------------------------
void OnlineCheckFunc(void) //����ȷ�ϼ�⺯��
{
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;
	if (Local.Timer1Num > INTRPERSEC * 6)
	{
		if (Local.CallConfirmFlag == 0)
		{
			if ((Local.Status == 1) || (Local.Status == 2) || (Local.Status == 5)
					|| (Local.Status == 6) || (Local.Status == 7) || (Local.Status == 8)
					|| (Local.Status == 9) || (Local.Status == 10)) //�Խ�
				TalkEnd_Func();
			if ((Local.Status == 3) || (Local.Status == 4)) //����
				WatchEnd_Func();

			Local.OnlineFlag = 0;
			//ͨѶ�ж�
			if (DebugMode == 1)
				P_Debug("don't receive online confirm, force end\n");

		} else
			Local.CallConfirmFlag = 0;
		Local.Timer1Num = 0;
	}
	else if ((Local.Timer1Num % INTRPERSEC) == 0)
	{
		//�Խ�ʱ���з���������ȷ�ϰ���ÿ��һ��
		//���ʱ���ط���������ȷ�ϰ���ÿ��һ��
		if ((Local.Status == 2) || (Local.Status == 6) || (Local.Status == 8)
				|| (Local.Status == 10) || (Local.Status == 3))
		{
			//ͷ��
			memcpy(send_b, UdpPackageHead, 6);
			//����
			if ((Local.Status == 2) || (Local.Status == 6) || (Local.Status == 8)
					|| (Local.Status == 10)) //�Խ�
			{
				if (Remote.isDirect == 0) //ֱͨ����
					send_b[6] = VIDEOTALK;
				else
					send_b[6] = VIDEOTALKTRANS;//��ת����
			}
			if (Local.Status == 3) //����
			{
				if (Remote.isDirect == 0) //ֱͨ����
					send_b[6] = VIDEOWATCH;
				else
					send_b[6] = VIDEOWATCHTRANS;//��ת����
			}

			send_b[7] = ASK; //����
			send_b[8] = CALLCONFIRM; //ͨ������ȷ��
			//������
			if ((Local.Status == 1) || (Local.Status == 3) || (Local.Status == 5)
					|| (Local.Status == 7) || (Local.Status == 9)) //����Ϊ���з�
			{
				memcpy(send_b + 9, LocalCfg.Addr, 20);
				memcpy(send_b + 29, LocalCfg.IP, 4);
				memcpy(send_b + 33, Remote.Addr[0], 20);
				memcpy(send_b + 53, Remote.IP[0], 4);
			}
			if ((Local.Status == 2) || (Local.Status == 4) || (Local.Status == 6)
					|| (Local.Status == 8) || (Local.Status == 10)) //����Ϊ���з�
			{
				memcpy(send_b + 9, Remote.Addr[0], 20);
				memcpy(send_b + 29, Remote.IP[0], 4);
				memcpy(send_b + 33, LocalCfg.Addr, 20);
				memcpy(send_b + 53, LocalCfg.IP, 4);
			}
			//ȷ�����
			send_b[57] = (Local.OnlineNum & 0xFF000000) >> 24;
			send_b[58] = (Local.OnlineNum & 0x00FF0000) >> 16;
			send_b[59] = (Local.OnlineNum & 0x0000FF00) >> 8;
			send_b[60] = Local.OnlineNum & 0x000000FF;
			Local.OnlineNum++;
			sendlength = 62;//edit by zlin 61
			sprintf(RemoteHost, "%d.%d.%d.%d\0", Remote.DenIP[0], Remote.DenIP[1], Remote.DenIP[2], Remote.DenIP[3]);
			if (Remote.isDirect == 0) //ֱͨ����
				RemotePort = RemoteVideoPort;
			else
				RemotePort = RemoteVideoServerPort;//��ת����
			UdpSendBuff(m_VideoSocket, RemoteHost, RemotePort, send_b, sendlength);
		}
	}
	Local.Timer1Num++;

	//�Խ����ƣ���ʾͨ��ʱ����жϳ�ʱ
	TalkCtrlFunc();
}
//---------------------------------------------------------------------------
void TimeReportStatusFunc(void) //�豸��ʱ����״̬����
{
	int i;
	//����������
	pthread_mutex_lock(&Local.udp_lock);
	//���ҿ��÷��ͻ��岢���
	for (i = 0; i < UDPSENDMAX; i++)
	{
		if (Multi_Udp_Buff[i].isValid == 0)
		{
			sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0",
					LocalCfg.IP_Server[0], LocalCfg.IP_Server[1], LocalCfg.IP_Server[2], LocalCfg.IP_Server[3]);
			Multi_Udp_Buff[i].SendNum = 0; //��෢6��
			Multi_Udp_Buff[i].m_Socket = m_DataSocket;
			Multi_Udp_Buff[i].RemotePort = RemoteDataPort;
			Multi_Udp_Buff[i].CurrOrder = 0;
			//ͷ��
			memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
			//����
			Multi_Udp_Buff[i].buf[6] = REPORTSTATUS;
			Multi_Udp_Buff[i].buf[7] = ASK; //����

			memcpy(Multi_Udp_Buff[i].buf + 8, LocalCfg.Addr, 20);
			memcpy(Multi_Udp_Buff[i].buf + 28, LocalCfg.Mac_Addr, 6);

			Multi_Udp_Buff[i].nlength = 34;//edit by zlin 34
			Multi_Udp_Buff[i].DelayTime = 100;
			Multi_Udp_Buff[i].SendDelayTime = 0; //���͵ȴ�ʱ�����  20101112
			Multi_Udp_Buff[i].isValid = 1;
			sem_post(&multi_send_sem);
			break;
		}
	}
	//�򿪻�����
	pthread_mutex_unlock(&Local.udp_lock);
}

//---------------------------------------------------------------------------
void TalkCtrlFunc(void) //�Խ����ƣ���ʾͨ��ʱ����жϳ�ʱ
{
	char strtime[30];
	int PrevStep = 0;
	int CurrStep = 0;
	//1S
	if ((Local.TimeOut % INTRPERSEC) == 0)
	{
		switch (Local.Status)
		{
		case 1: //���жԽ�
		case 2: //���жԽ�
			if (Local.TimeOut > CALLTIMEOUT)
			{
				//�鿴�Ƿ��������鲥����
				DropMultiGroup(m_VideoSocket, NULL);
				//���г�ʱ
				CallTimeOut_Func();
				MsgLAN2CCCallTimeOut();
			}
			break;
		case 5: //����ͨ��
		case 6: //����ͨ��
				//��ʱ
			sprintf(strtime, "%02d:%02d\0", Local.TimeOut / INTRPERSEC / 60, (Local.TimeOut / INTRPERSEC) % 60);
			if (Local.TimeOut > TMCCGetIMSCallTimeOut()*24)//TALKTIMEOUT)
			{
				TalkEnd_Func();
				MsgLAN2CCTalkTimeOut();
				Local.OnlineFlag = 0;
				if (DebugMode == 1)
					P_Debug("talk timeout \n");
			}
			break;
		case 7: //����������Ӱ����Ԥ��
		case 8: //����������Ӱ����Ԥ��
			if (Local.TimeOut > PREPARETIMEOUT)
			{
				TalkEnd_Func();
				Local.OnlineFlag = 0;
				if (DebugMode == 1)
					P_Debug("record prepare timeout \n");
			}
			break;
		case 9: //����������Ӱ����
		case 10: //����������Ӱ����
			if (Local.TimeOut > RECORDTIMEOUT)
			{
				TalkEnd_Func();
				Local.OnlineFlag = 0;
				if (DebugMode == 1)
					P_Debug("record timeout \n");
			}
			break;
		case 3: //����    ��ʱ
		case 4: //������
			if (Local.TimeOut > WATCHTIMEOUT)
			{
				WatchEnd_Func();
				Local.OnlineFlag = 0;
				if (DebugMode == 1)
					P_Debug("watch timeout \n");
			}
			break;
		}
	}
	Local.TimeOut++; //���ӳ�ʱ,  ͨ����ʱ,  ���г�ʱ�����˽���
}
//---------------------------------------------------------------------------
//���IP��ַ��ͻ
void CheckIPConflict(void)
{
	int result;
	int i;
	unsigned char to_user[7];
	result = 0;
	result = ioctl(m_EthSocket, CONFLICTARP, to_user);//m_DataSocket
	if (result == CONFLICTARP)
	{
		if (to_user[0] == 1) //����
		{
			sprintf(Local.DebugInfo, "IP address conflict, ohter IP address %02X:%02X:%02X:%02X:%02X:%02X\n",
					to_user[1], to_user[2], to_user[3], to_user[4], to_user[5], to_user[6]);
			P_Debug(Local.DebugInfo);
		}
		if (to_user[0] == 2) //Ӧ��
		{
			//����������
			pthread_mutex_lock(&Local.udp_lock);
			for (i = 0; i < UDPSENDMAX; i++)
			{
				if (Multi_Udp_Buff[i].isValid == 1)
				{
					if (Multi_Udp_Buff[i].m_Socket == ARP_Socket)
					{
						Multi_Udp_Buff[i].isValid = 0;
						sprintf(Local.DebugInfo, "IP address conflict, other IP address %02X:%02X:%02X:%02X:%02X:%02X\n",
								to_user[1], to_user[2], to_user[3], to_user[4], to_user[5], to_user[6]);
						P_Debug(Local.DebugInfo);
						break;
					}
				}
			}
			//�򿪻�����
			pthread_mutex_unlock(&Local.udp_lock);
		}
	}
}
//---------------------------------------------------------------------------
static int oldNet = 0;
//��������ź�
void CheckEth(void) {
	if(oldNet != Local.NetStatus)
	{
		printf("**********CheckEth net status:%d\n", Local.NetStatus);
		oldNet = Local.NetStatus;
	}
	int retval;
	int NewNetSpeed;
	retval = detect_ethtool(m_EthSocket, "eth0");
//  retval = detect_mii(m_DataSocket, "eth0");

	if (retval != Local.NetStatus) //����״̬ 0 �Ͽ�  1 ��ͨ
	{
		if (retval == 0)
		{
			Local.NetStatus = retval;
			P_Debug("Link down\n");
		}

		if (retval == 1) {
			Local.NetStatus = retval;
			P_Debug("Link up\n");
			//�������ARP
			SendFreeArp();

			NewNetSpeed = get_ethtool_speed(m_EthSocket, "eth0");
			if (NewNetSpeed != Local.OldNetSpeed) {
				Local.OldNetSpeed = NewNetSpeed;
				system("ethtool -s eth0 autoneg off");
				usleep(1000 * 1000);
				system("ethtool -s eth0 autoneg on");
				//set_ethtool_speed(m_EthSocket, "eth0", 100, 0, 0); //�ر�����Ӧ
				//set_ethtool_speed(m_EthSocket, "eth0", 100, 0, 1); //������Ӧ
				P_Debug("reset net speed \n");
			}
		}
	}
}
//---------------------------------------------------------------------------
int detect_ethtool(int skfd, char *ifname)
{
	struct ifreq ifr;
	struct ethtool_value edata;

	memset(&ifr, 0, sizeof(ifr));
	edata.cmd = ETHTOOL_GLINK;

	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
	ifr.ifr_data = (char *) &edata;

	if (ioctl(skfd, SIOCETHTOOL, &ifr) == -1) {
		P_Debug("ETHTOOL_GLINK failed\n");
		return 2;
	}

	//printf("*******************detect_ethtool:%d\n", edata.data);
	return (edata.data); // ? 0 : 1);
}
//---------------------------------------------------------------------------
struct ethtool_cmd eth_data;
int get_ethtool_speed(int skfd, char *ifname) //��ȡ��������
{
	struct ifreq ifr;
	//  struct ethtool_cmd eth_data;

	memset(&ifr, 0, sizeof(ifr));
	eth_data.cmd = ETHTOOL_GSET;

	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
	ifr.ifr_data = (char*) &eth_data;

	if (ioctl(skfd, SIOCETHTOOL, &ifr) == -1) {
		P_Debug("ETHTOOL_GSET failed\n");
		return 2;
	}
	//printf("eth_data.speed = %d, eth_data.duplex = %d, eth_data.autoneg = %d\n", eth_data.speed, eth_data.duplex, eth_data.autoneg);
	return (eth_data.speed); // ? 0 : 1);
}
//---------------------------------------------------------------------------
int set_ethtool_speed(int skfd, char *ifname, int speed, int duplex, int autoneg) //������������
{
	struct ifreq ifr;

	memset(&ifr, 0, sizeof(ifr));
	eth_data.cmd = ETHTOOL_SSET;

	strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
	//  eth_data.speed = speed;
	//  eth_data.duplex = duplex;
	eth_data.autoneg = autoneg;
	ifr.ifr_data = (char*) &eth_data;

	if (ioctl(skfd, SIOCETHTOOL, &ifr) == -1) {
		P_Debug("ETHTOOL_SSET failed\n");
		return 2;
	}
	return (eth_data.speed); // ? 0 : 1);
}
//---------------------------------------------------------------------------
