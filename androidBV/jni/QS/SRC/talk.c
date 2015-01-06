#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>       //sem_t
#include "LPIntfLayer.h"
#include "TMInterface.h"
#include <LOGIntfLayer.h>

#define CommonH
#include "common.h"
extern char NullAddr[21]; //���ַ���

#include <LANIntfLayer.h>

void Call_Func(int uFlag, char *call_addr, char *CenterAddr); //����   1  ����  2 ס��

void Talk_Func(void); //ͨ�� ����
void TalkEnd_Func(void);
void WatchEnd_Func(void);
void CallTimeOut_Func(void); //���г�ʱ

void Call_Func(int uFlag, char *call_addr, char *CenterAddr) //����   1  ����  2 ס��
{
	int i;
	sprintf(Local.DebugInfo, "****Call_Func()***Local.Status = %d addr = %s ****\n", Local.Status, call_addr);
	P_Debug(Local.DebugInfo);
	if ((Local.Status == 0) && ((uFlag == 1) || (uFlag == 2)))
	{
		Local.NSReplyFlag = 0; //NSӦ�����־
		//����������
		pthread_mutex_lock(&Local.udp_lock);
		//���ҿ��÷��ͻ��岢���
		for (i = 0; i < UDPSENDMAX; i++)
		{
			if (Multi_Udp_Buff[i].isValid == 0)
			{
				GetCfg();
				Multi_Udp_Buff[i].SendNum = 0;
				Multi_Udp_Buff[i].m_Socket = m_VideoSocket;
				Multi_Udp_Buff[i].RemotePort = RemoteVideoPort;
				Multi_Udp_Buff[i].CurrOrder = VIDEOTALK;
				//   sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0",LocalCfg.IP_Broadcast[0],
				//           LocalCfg.IP_Broadcast[1],LocalCfg.IP_Broadcast[2],LocalCfg.IP_Broadcast[3]);

				strcpy(Multi_Udp_Buff[i].RemoteHost, NSMULTIADDR);
				//ͷ��
				memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
				//����  ,�����㲥����
				Multi_Udp_Buff[i].buf[6] = NSORDER;
				Multi_Udp_Buff[i].buf[7] = ASK; //����

				memcpy(Multi_Udp_Buff[i].buf + 8, LocalCfg.Addr, 20);
				memcpy(Multi_Udp_Buff[i].buf + 28, LocalCfg.IP, 4);
				memcpy(Remote.Addr[0], NullAddr, 20);
				switch (uFlag)
				{
				case 1: //��������
					memcpy(Remote.Addr[0], CenterAddr, 12);
					break;
				case 2: //����ס��
					switch (LocalCfg.Addr[0])
					{
					case 'M':
						Remote.Addr[0][0] = 'S';
						memcpy(Remote.Addr[0] + 1, LocalCfg.Addr + 1, 6);
						memcpy(Remote.Addr[0] + 7, call_addr, 4);
						Remote.Addr[0][11] = '0';
						printf("%s\n", Remote.Addr[0]);
						break;
					case 'W':
						if (strlen(call_addr) == 4) //����
						{
							Remote.Addr[0][0] = 'B';
							memcpy(Remote.Addr[0] + 1, call_addr, 4);
							Remote.Addr[0][11] = '0';
						}
						else
						{
							Remote.Addr[0][0] = 'S';
							memcpy(Remote.Addr[0] + 1, call_addr, 10);
							Remote.Addr[0][11] = '0';
						}
						break;
					}
					break;
				}
				printf("Remote.Addr[0] = %s\n", Remote.Addr[0]);
				memcpy(Multi_Udp_Buff[i].buf + 32, Remote.Addr[0], 20);
				Remote.IP[0][0] = 0;
				Remote.IP[0][1] = 0;
				Remote.IP[0][2] = 0;
				Remote.IP[0][3] = 0;
				memcpy(Multi_Udp_Buff[i].buf + 52, Remote.IP[0], 4);

				Multi_Udp_Buff[i].nlength = 56;
				Multi_Udp_Buff[i].DelayTime = 100;
				Multi_Udp_Buff[i].SendDelayTime = 0; //���͵ȴ�ʱ�����  20101112
				Multi_Udp_Buff[i].isValid = 1;
#ifdef _DEBUG
				printf("resolving address\n");
#endif
				sem_post(&multi_send_sem);
				break;
			}
		}
		//�򿪻�����
		pthread_mutex_unlock(&Local.udp_lock);
	}
	else
	{
#ifdef _DEBUG
		printf("������æ,�޷�����\n");
#endif
	}
}

//---------------------------------------------------------------------------
void Talk_Func(void) //ͨ�� ����
{
	int i;
	if (Local.Status == 2) //״̬Ϊ���Խ�
	{
		//WaitAudioUnuse(1000);
		for (i = 0; i < UDPSENDMAX; i++)
		{
			if (Multi_Udp_Buff[i].isValid == 0)
			{
				//����������
				pthread_mutex_lock(&Local.udp_lock);
				Multi_Udp_Buff[i].SendNum = 0;
				Multi_Udp_Buff[i].m_Socket = m_VideoSocket;
				Multi_Udp_Buff[i].RemotePort = RemoteVideoPort;
				sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0",
						Remote.DenIP[0], Remote.DenIP[1], Remote.DenIP[2],
						Remote.DenIP[3]);
				//ͷ��
				memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
				//����  ,�����㲥����
				if (Remote.isDirect == 1)
					Multi_Udp_Buff[i].buf[6] = VIDEOTALKTRANS;
				else
					Multi_Udp_Buff[i].buf[6] = VIDEOTALK;
				Multi_Udp_Buff[i].CurrOrder = Multi_Udp_Buff[i].buf[6];
				Multi_Udp_Buff[i].buf[7] = ASK; //����
				Multi_Udp_Buff[i].buf[8] = CALLSTART;

				//����Ϊ���з�
				if ((Local.Status == 1) || (Local.Status == 3)
						|| (Local.Status == 5) || (Local.Status == 7)
						|| (Local.Status == 9))
				{
					memcpy(Multi_Udp_Buff[i].buf + 9, LocalCfg.Addr, 20);
					memcpy(Multi_Udp_Buff[i].buf + 29, LocalCfg.IP, 4);
					memcpy(Multi_Udp_Buff[i].buf + 33, Remote.Addr[0], 20);
					memcpy(Multi_Udp_Buff[i].buf + 53, Remote.IP[0], 4);
				}
				//����Ϊ���з�
				if ((Local.Status == 2) || (Local.Status == 4)
						|| (Local.Status == 6) || (Local.Status == 8)
						|| (Local.Status == 10))
				{
					memcpy(Multi_Udp_Buff[i].buf + 9, Remote.Addr[0], 20);
					memcpy(Multi_Udp_Buff[i].buf + 29, Remote.IP[0], 4);
					memcpy(Multi_Udp_Buff[i].buf + 33, LocalCfg.Addr, 20);
					memcpy(Multi_Udp_Buff[i].buf + 53, LocalCfg.IP, 4);
				}

				Multi_Udp_Buff[i].nlength = 57;
				Multi_Udp_Buff[i].DelayTime = 100;
				Multi_Udp_Buff[i].SendDelayTime = 0; //���͵ȴ�ʱ�����  20101112
				Multi_Udp_Buff[i].isValid = 1;

//                Local.Status = 0;  //״̬Ϊ����
				//�򿪻�����
				pthread_mutex_unlock(&Local.udp_lock);
				sem_post(&multi_send_sem);
				break;
			}
		}
	}
}
//---------------------------------------------------------------------------
/*void TalkEnd_Func(void)
{
	int i, j;
	sprintf(Local.DebugInfo, "****TalkEnd_Func()***Local.Status = %d****\n", Local.Status);
	P_Debug(Local.DebugInfo);
	if ((Local.Status == 1) || (Local.Status == 2) || (Local.Status == 5)
			|| (Local.Status == 6) || (Local.Status == 7) || (Local.Status == 8)
			|| (Local.Status == 9) || (Local.Status == 10)) //״̬Ϊ�Խ�
	{
		//sprintf(Local.DebugInfo, "Remote.DenNum=%d\n", Remote.DenNum);
		//P_Debug(Local.DebugInfo);
		//����������
		pthread_mutex_lock(&Local.udp_lock);
		for (j = 0; j < Remote.DenNum; j++)
		{
			for (i = 0; i < UDPSENDMAX; i++)
			{
				if (Multi_Udp_Buff[i].isValid == 0)
				{
					//ͷ��
					memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
					//����  ,�����㲥����
					Multi_Udp_Buff[i].buf[6] = VIDEOTALK;
					Multi_Udp_Buff[i].buf[7] = ASK; //����
					Multi_Udp_Buff[i].buf[8] = CALLEND; //CALLEND
					Multi_Udp_Buff[i].SendNum = 0;
					Multi_Udp_Buff[i].m_Socket = m_VideoSocket;
					Multi_Udp_Buff[i].RemotePort = RemoteVideoPort;
					Multi_Udp_Buff[i].CurrOrder = VIDEOTALK;
					sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0",
							Remote.IP[j][0], Remote.IP[j][1], Remote.IP[j][2], Remote.IP[j][3]);

					sprintf(Local.DebugInfo, "%d.%d.%d.%d\n", Remote.IP[j][0],
							Remote.IP[j][1], Remote.IP[j][2], Remote.IP[j][3]);
					P_Debug(Local.DebugInfo);

					//����Ϊ���з�
					if ((Local.Status == 1) || (Local.Status == 3) || (Local.Status == 5)
							|| (Local.Status == 7) || (Local.Status == 9))
					{
						memcpy(Multi_Udp_Buff[i].buf + 9, LocalCfg.Addr, 20);
						memcpy(Multi_Udp_Buff[i].buf + 29, LocalCfg.IP, 4);
						memcpy(Multi_Udp_Buff[i].buf + 33, Remote.Addr[j], 20);
						memcpy(Multi_Udp_Buff[i].buf + 53, Remote.IP[j], 4);
					}
					//����Ϊ���з�
					if ((Local.Status == 2) || (Local.Status == 4) || (Local.Status == 6)
							|| (Local.Status == 8) || (Local.Status == 10))
					{
						memcpy(Multi_Udp_Buff[i].buf + 9, Remote.Addr[j], 20);
						memcpy(Multi_Udp_Buff[i].buf + 29, Remote.IP[j], 4);
						memcpy(Multi_Udp_Buff[i].buf + 33, LocalCfg.Addr, 20);
						memcpy(Multi_Udp_Buff[i].buf + 53, LocalCfg.IP, 4);
					}

					Multi_Udp_Buff[i].nlength = 57;
					Multi_Udp_Buff[i].DelayTime = DIRECTCALLTIME;
					Multi_Udp_Buff[i].SendDelayTime = 0; //���͵ȴ�ʱ�����  20101112
					Multi_Udp_Buff[i].isValid = 1;
					sem_post(&multi_send_sem);
					Local.Status = 0;  //״̬Ϊ����
					//MsgLAN2CCCallHangOff();

					break;
				}
			}
		}
		//�򿪻�����
		pthread_mutex_unlock(&Local.udp_lock);
		StopRecVideo();
		StopRecAudio();//add by zlin
	}
}*/
//---------------------------------------------------------------------------
void TalkEnd_Func(void)
{
	int i, j;
	sprintf(Local.DebugInfo, "****TalkEnd_Func()***Local.Status = %d****\n", Local.Status);
	P_Debug(Local.DebugInfo);
	//if ((Local.Status == 1) || (Local.Status == 2) || (Local.Status == 5)
	//		|| (Local.Status == 6) || (Local.Status == 7) || (Local.Status == 8)
	//		|| (Local.Status == 9) || (Local.Status == 10)) //״̬Ϊ�Խ�
	{
		//sprintf(Local.DebugInfo, "Remote.DenNum=%d\n", Remote.DenNum);
		//P_Debug(Local.DebugInfo);
		//����������
		pthread_mutex_lock(&Local.udp_lock);
		for (j = 0; j < Remote.DenNum; j++)
		{
			for (i = 0; i < UDPSENDMAX; i++)
			{
				if (Multi_Udp_Buff[i].isValid == 0)
				{
					//ͷ��
					memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
					//����  ,�����㲥����
					Multi_Udp_Buff[i].buf[6] = VIDEOTALK;
					Multi_Udp_Buff[i].buf[7] = ASK; //����
					Multi_Udp_Buff[i].buf[8] = CALLEND; //CALLEND
					Multi_Udp_Buff[i].SendNum = 0;
					Multi_Udp_Buff[i].m_Socket = m_VideoSocket;
					Multi_Udp_Buff[i].RemotePort = RemoteVideoPort;
					Multi_Udp_Buff[i].CurrOrder = VIDEOTALK;
					sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0",
							Remote.IP[j][0], Remote.IP[j][1], Remote.IP[j][2], Remote.IP[j][3]);

					sprintf(Local.DebugInfo, "%d.%d.%d.%d\n", Remote.IP[j][0],
							Remote.IP[j][1], Remote.IP[j][2], Remote.IP[j][3]);
					P_Debug(Local.DebugInfo);

					//����Ϊ���з�
					if ((Local.Status == 1) || (Local.Status == 3) || (Local.Status == 5)
							|| (Local.Status == 7) || (Local.Status == 9))
					{
						memcpy(Multi_Udp_Buff[i].buf + 9, LocalCfg.Addr, 20);
						memcpy(Multi_Udp_Buff[i].buf + 29, LocalCfg.IP, 4);
						memcpy(Multi_Udp_Buff[i].buf + 33, Remote.Addr[j], 20);
						memcpy(Multi_Udp_Buff[i].buf + 53, Remote.IP[j], 4);
					}
					//����Ϊ���з�
					if ((Local.Status == 2) || (Local.Status == 4) || (Local.Status == 6)
							|| (Local.Status == 8) || (Local.Status == 10))
					{
						memcpy(Multi_Udp_Buff[i].buf + 9, Remote.Addr[j], 20);
						memcpy(Multi_Udp_Buff[i].buf + 29, Remote.IP[j], 4);
						memcpy(Multi_Udp_Buff[i].buf + 33, LocalCfg.Addr, 20);
						memcpy(Multi_Udp_Buff[i].buf + 53, LocalCfg.IP, 4);
					}
					if (Local.Status == 0)
					{
						memcpy(Multi_Udp_Buff[i].buf + 9, LocalCfg.Addr, 20);
						memcpy(Multi_Udp_Buff[i].buf + 29, LocalCfg.IP, 4);
						memcpy(Multi_Udp_Buff[i].buf + 33, Remote.Addr[j], 20);
						memcpy(Multi_Udp_Buff[i].buf + 53, Remote.IP[j], 4);
					}

					Multi_Udp_Buff[i].nlength = 57;
					Multi_Udp_Buff[i].DelayTime = DIRECTCALLTIME;
					Multi_Udp_Buff[i].SendDelayTime = 0; //���͵ȴ�ʱ�����  20101112
					Multi_Udp_Buff[i].isValid = 1;
					sem_post(&multi_send_sem);
					Local.Status = 0;  //״̬Ϊ����
					//MsgLAN2CCCallHangOff();

					break;
				}
			}
		}
		//�򿪻�����

		LOG_RUNLOG_DEBUG("AndroidAudio will  StopRecAudio");
		pthread_mutex_unlock(&Local.udp_lock);
		StopRecVideo();
#ifdef	_SND_RECORD_
		StopRecAudio();//add by zlin
#endif
#ifdef  _SND_PLAY_
		StopPlayAudio();
#endif
	}
}
//---------------------------------------------------------------------------
void WatchEnd_Func(void)
{
	int i;
	if ((Local.Status == 3) || (Local.Status == 4)) //״̬Ϊ����
	{
		//����������
		pthread_mutex_lock(&Local.udp_lock);
		for (i = 0; i < UDPSENDMAX; i++)
		{
			if (Multi_Udp_Buff[i].isValid == 0)
			{
				Multi_Udp_Buff[i].SendNum = 0;
				Multi_Udp_Buff[i].m_Socket = m_VideoSocket;
				Multi_Udp_Buff[i].RemotePort = RemoteVideoPort;
				Multi_Udp_Buff[i].CurrOrder = VIDEOWATCH;
				sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0",
						Remote.DenIP[0], Remote.DenIP[1], Remote.DenIP[2], Remote.DenIP[3]);
				//ͷ��
				memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
				//����  ,�����㲥����
				if (Remote.isDirect == 1)
					Multi_Udp_Buff[i].buf[6] = VIDEOWATCHTRANS;
				else
					Multi_Udp_Buff[i].buf[6] = VIDEOWATCH;
				Multi_Udp_Buff[i].buf[7] = ASK; //����
				Multi_Udp_Buff[i].buf[8] = CALLEND; //CALLEND

				//����Ϊ���з�
				if ((Local.Status == 1) || (Local.Status == 3) || (Local.Status == 5)
						|| (Local.Status == 7) || (Local.Status == 9))
				{
					memcpy(Multi_Udp_Buff[i].buf + 9, LocalCfg.Addr, 20);
					memcpy(Multi_Udp_Buff[i].buf + 29, LocalCfg.IP, 4);
					memcpy(Multi_Udp_Buff[i].buf + 33, Remote.Addr[0], 20);
					memcpy(Multi_Udp_Buff[i].buf + 53, Remote.IP[0], 4);
				}
				//����Ϊ���з�
				if ((Local.Status == 2) || (Local.Status == 4) || (Local.Status == 6)
						|| (Local.Status == 8) || (Local.Status == 10))
				{
					memcpy(Multi_Udp_Buff[i].buf + 9, Remote.Addr[0], 20);
					memcpy(Multi_Udp_Buff[i].buf + 29, Remote.IP[0], 4);
					memcpy(Multi_Udp_Buff[i].buf + 33, LocalCfg.Addr, 20);
					memcpy(Multi_Udp_Buff[i].buf + 53, LocalCfg.IP, 4);
				}

				Multi_Udp_Buff[i].nlength = 57;
				Multi_Udp_Buff[i].DelayTime = 100;
				Multi_Udp_Buff[i].SendDelayTime = 0; //���͵ȴ�ʱ�����  20101112
				Multi_Udp_Buff[i].isValid = 1;

                Local.Status = 0;  //״̬Ϊ����
				MsgLAN2CCCallHangOff();
				sem_post(&multi_send_sem);
				break;
			}
		}
		//�򿪻�����
		pthread_mutex_unlock(&Local.udp_lock);
		StopRecVideo();
		StopRecAudio();//add by zlin
	}
}
//---------------------------------------------------------------------------
//���г�ʱ
void CallTimeOut_Func(void)
{
	int i;
	int isHost;
	//�籾��Ϊ���з�
	//�鿴SD�Ƿ���ں���Ӱ��־ �� ���з���ַΪΧǽ�����ſڻ��ͱ����ſڻ�
	if ((Local.SD_Status == 1) && (Local.Status == 2)
			&& ((Remote.Addr[0][0] == 'W') || (Remote.Addr[0][0] == 'M') || (Remote.Addr[0][0] == 'H')))
	{
		isHost = 0;
		if (LocalCfg.Addr[0] == 'S')
		{
			if (LocalCfg.Addr[11] == '0')
				isHost = 1;
		}
		if (LocalCfg.Addr[0] == 'B')
		{
			if (LocalCfg.Addr[5] == '0')
				isHost = 1;
		}
		if (isHost == 1)
		{
			//���Ϳ�ʼ��ӰԤ������
			for (i = 0; i < UDPSENDMAX; i++)
			{
				if (Multi_Udp_Buff[i].isValid == 0)
				{
					//����������
					pthread_mutex_lock(&Local.udp_lock);
					Multi_Udp_Buff[i].SendNum = 0;
					Multi_Udp_Buff[i].m_Socket = m_VideoSocket;
					Multi_Udp_Buff[i].RemotePort = RemoteVideoPort;

					sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0",
							Remote.DenIP[0], Remote.DenIP[1], Remote.DenIP[2], Remote.DenIP[3]);
					//ͷ��
					memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
					//����
					if (Remote.isDirect == 1)
						Multi_Udp_Buff[i].buf[6] = VIDEOTALKTRANS;
					else
						Multi_Udp_Buff[i].buf[6] = VIDEOTALK;
					Multi_Udp_Buff[i].buf[7] = ASK; //����
					Multi_Udp_Buff[i].buf[8] = PREPARERECORD;

					memcpy(Multi_Udp_Buff[i].buf + 9, LocalCfg.Addr, 20);
					memcpy(Multi_Udp_Buff[i].buf + 29, LocalCfg.IP, 4);
					memcpy(Multi_Udp_Buff[i].buf + 33, Remote.Addr[0], 20);
					memcpy(Multi_Udp_Buff[i].buf + 53, Remote.IP[0], 4);

					Multi_Udp_Buff[i].nlength = 57;
					Multi_Udp_Buff[i].DelayTime = 100;
					Multi_Udp_Buff[i].SendDelayTime = 0; //���͵ȴ�ʱ�����  20101112
					Multi_Udp_Buff[i].isValid = 1;

					//�򿪻�����
					pthread_mutex_unlock(&Local.udp_lock);
					sem_post(&multi_send_sem);
					break;
				}
			}
		}
		else
		{
			if ((Local.Status == 1) || (Local.Status == 2) || (Local.Status == 5) || (Local.Status == 6)
					|| (Local.Status == 7) || (Local.Status == 8) || (Local.Status == 9) || (Local.Status == 10))

				TalkEnd_Func();

			Local.OnlineFlag = 0;
#ifdef _DEBUG
			printf("call timeout\n");
#endif
		}
	}
	else
	{
		if ((Local.Status == 1) || (Local.Status == 2) || (Local.Status == 5) || (Local.Status == 6)
				|| (Local.Status == 7) || (Local.Status == 8) || (Local.Status == 9) || (Local.Status == 10))

			TalkEnd_Func();

		Local.OnlineFlag = 0;

#ifdef _DEBUG
		printf("call timeout\n");
#endif
	}
}
//---------------------------------------------------------------------------
