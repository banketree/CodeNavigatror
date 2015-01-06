#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>

#include <assert.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <linux/videodev.h>

#include <common.h>
#include "CFimcMjpeg.h"

#include <IDBT.h>
#include <IDBTCfg.h>
#include <YKSystem.h>
#include <LANIntfLayer.h>
#include <LOGIntfLayer.h>
 
int DebugMode = 0;           //����ģʽ

pthread_t idbt_notify_delegate_pthread;
pthread_t cc_pthread;

pthread_mutex_t audio_open_lock;
pthread_mutex_t audio_close_lock;
pthread_mutex_t audio_lock;

extern int InitArpSocket(void);
extern void AddMultiGroup(int m_Socket, char *McastAddr);
extern void InitRecVideo(void);
//******************************end****************************

#ifdef  _MESSAGE_SUPPORT  //��Ϣ����
#include "type.h"
#endif

#define idcard_name "/mnt/mtd/config/idcardcfg"

char sZkPath[80] = "/usr/zk/";
void ReadCfgFile(void);
//д���������ļ�
void WriteCfgFile(void);

#if 0//def _BRUSHIDCARD_SUPPORT   //ˢ������֧��
//��ID���ļ�

void ReadIDCardCfgFile(void);

//дID���ļ�

void WriteIDCardFile(void);

//��ID��Ӧ��

void ReadIDCardReply(char *cFromIP);

#endif

extern struct timeval ref_time; //��׼ʱ��,��Ƶ����Ƶ��һ֡

//---------------------------------------------------------------------------
#ifdef _CAPTUREPIC_TO_CENTER  //����ͼƬ��������
//�������ͼƬ
void Deal_CapturePic_Center(void);
#endif
void P_Debug(char *DebugInfo); //������Ϣ
#ifdef _ECHO_STATE_SUPPORT  //��������֧��
extern int echo_num;
#endif 
//---------------------------------------------------------------------------
void P_Debug(char *DebugInfo) //������Ϣ
{
	unsigned char send_b[1500];
	int sendlength;
	int infolen;
	printf(DebugInfo);
	LOG_RUNLOG_DEBUG("LAN_Debug %s", DebugInfo);

	if (Local.RemoteDebugInfo == 1) //����Զ�̵�����Ϣ
	{
		memcpy(send_b, UdpPackageHead, 6);
		send_b[6] = REMOTEDEBUGINFO;
		send_b[7] = ASK; //����
		send_b[8] = 2; //������
		memcpy(send_b + 9, FLAGTEXT, 20); //��־
		infolen = strlen(DebugInfo);
		memcpy(send_b + 29, &infolen, 4);
		memcpy(send_b + 33, DebugInfo, infolen);
		sendlength = 33 + infolen;
		UdpSendBuff(m_DataSocket, Local.DebugIP, RemoteDataPort, send_b, sendlength);
		//printf("P_Debug1112, Local.DebugIP = %s, send_b = %s\n", Local.DebugIP, send_b);
	}
}
//---------------------------------------------------------------------------
//int main(int argc, char *argv[])
int LANTask(int argc, char *argv[])
{

	pthread_attr_t attr;
	int ch;
	int programrun; //�������б�־
 	int i, j;
	uint32_t Ip_Int;

	DebugMode = 1;

	ref_time.tv_sec = 0; //��ʼʱ���
	ref_time.tv_usec = 0;
	TimeStamp.OldCurrVideo = 0; //��һ�ε�ǰ��Ƶʱ��
	TimeStamp.CurrVideo = 0;
	TimeStamp.OldCurrAudio = 0; //��һ�ε�ǰ��Ƶʱ��
	TimeStamp.CurrAudio = 0;

	//��ʼ������ͷ���
	if (TempAudioNode_h == NULL)
		TempAudioNode_h = (TempAudioNode1 *) init_audionode();

	RemoteDataPort = 8300;
	RemoteVideoPort = 8302;
	RemoteVideoServerPort = 8306; //��Ƶ����������ƵUDP�˿�
	strcpy(RemoteHost, "192.168.0.88");
	LocalDataPort = 8300; //�������UDP�˿�
	LocalVideoPort = 8302; //����ƵUDP�˿�
	Local.IP_VideoServer[0] = 192; //��Ƶ��������ַ
	Local.IP_VideoServer[1] = 168;
	Local.IP_VideoServer[2] = 68;
	Local.IP_VideoServer[3] = 192;

	Local.NetStatus = 0; //����״̬ 0 �Ͽ�  1 ��ͨ
	Local.OldNetSpeed = 100; //�����ٶ�
 
  	Local.LoginServer = 0; //�Ƿ��¼������

	//�������ļ�
  	GetCfg();
	//ReadCfgFile();
	if (LocalCfg.Addr[0] == 'M')
	{
		Local.AddrLen = 8; //��ַ����  S 12  B 6 M 8 H 6
		memcpy(LocalCfg.Addr + 8, "0000", 4);

	}
	if (LocalCfg.Addr[0] == 'W')
	{
		Local.AddrLen = 5; //��ַ����  S 12  B 6 M 8 H 6
		memcpy(LocalCfg.Addr + 5, "0000000", 7);
	}
	LocalCfg.Addr[12] = '\0';


#if 0//def _BRUSHIDCARD_SUPPORT   //ˢ������֧��
	//��ID���ļ�
	ReadIDCardCfgFile();
	sprintf(Local.DebugInfo, "IDCardNo.Num = %d\n", IDCardNo.Num);
	P_Debug(Local.DebugInfo);

	//дID����Ϣ
	RecvIDCardNo.isNewWriteFlag = 0;
	RecvIDCardNo.Num = 0;
	RecvIDCardNo.serialnum = IDCardNo.serialnum;
#endif

 	Local._TESTNSSERVER = 0; //���Է���������ģʽ
	Local._TESTTRANS = 0; //������Ƶ��תģʽ
 
	Local.DownProgramOK = 0; //����Ӧ�ó������
	Local.download_image_flag = 0; //����ϵͳӳ��

 	//ˢID����¼
#if 0//def _BRUSHIDCARD_SUPPORT   //ˢ������֧��
	BrushIDCard.Num = 0;
#endif

	DeltaLen = 9 + sizeof(struct talkdata1); //���ݰ���Ч����ƫ����

	strcpy(UdpPackageHead, "XXXCID");
	for (i = 0; i < 20; i++)
		NullAddr[i] = '0';
	NullAddr[20] = '\0';

	Local.Status = 0; //״̬Ϊ����
	Local.NSReplyFlag = 0; //NSӦ�����־
	Local.RecPicSize = 1; //Ĭ����Ƶ��СΪ352*288
	Local.PlayPicSize = 1; //Ĭ����Ƶ��СΪ352*288

	Ip_Int = inet_addr("192.168.68.88");
	memcpy(Remote.IP, &Ip_Int, 4);
	memcpy(Remote.Addr[0], NullAddr, 20);
	memcpy(Remote.Addr[0], "S00010101010", 12); //

	//InitVideoParam();   //��ʼ����Ƶ����

	//InitAudioParam(); //��ʼ����Ƶ����
	//InitRecVideo();

	Local.RemoteDebugInfo = 0; //����Զ�̵�����Ϣ

#ifdef _CAPTUREPIC_TO_CENTER  //����ͼƬ��������
	Capture_Pic_Num = 0;
	for (i = 0; i < MAX_CAPTUREPIC_NUM; i++)
	{
		Capture_Pic_Center[i].isVilid = 0;
		Capture_Pic_Center[i].jpeg_pic = NULL;
	}
	Local.RecordPic = 0;
	Local.IFrameNo = 25; //75;    //���ڼ���I֡
	Local.IFrameCount = 0; //֡����
	Local.Pic_Width = CIF_W;
	Local.Pic_Height = CIF_H;
	Local.HavePicRecorded = 0; //����Ƭ��¼��
	Local.SendToCetnering = 0; //���ڴ�������
	Local.ConnectCentered = 0; //����������״̬
#endif

	//�����������
	m_EthSocket = 0;
	//UDP
	if (InitUdpSocket(LocalDataPort) == 0)
	{
		P_Debug("can't create data socket.\n\r");
		return FALSE;
	}

	if (InitUdpSocket(LocalVideoPort) == 0)
	{
		P_Debug("can't create video socket.\n\r");
		return FALSE;
	}

#if 0//def _BRUSHIDCARD_SUPPORT   //ˢ������֧��
	//����ְ�������Ϊ��Ч
	for (i = 0; i < DISPARTMAX; i++)
		Dispart_Send_Buff[i].isValid = 0;
	//��ְ������߳�
	sem_init(&dispart_send_sem, 0, 0);
	dispart_send_flag = 1;
	//����������
	pthread_mutex_init(&Local.dispart_send_lock, NULL);
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&dispart_send_thread, &attr, (void *) dispart_send_thread_func, NULL);
	pthread_attr_destroy(&attr);
	if (dispart_send_thread == 0)
	{
		P_Debug("�޷�������ְ������߳�\n");
		return FALSE;
	}
#endif

#ifdef _MESSAGE_SUPPORT  //��Ϣ����
	InitMsgQueue();
#endif

//��UDP���ͻ�����Ϊ��Ч
	for (i = 0; i < UDPSENDMAX; i++)
	{
		Multi_Udp_Buff[i].isValid = 0;
		Multi_Udp_Buff[i].SendNum = 0;
		Multi_Udp_Buff[i].DelayTime = 100;
		Multi_Udp_Buff[i].SendDelayTime = 0; //���͵ȴ�ʱ�����  20101112
	}

//�����������ݷ����̣߳��ն����������������ʱһ��û�յ���Ӧ�����η���
//����UDP��Commͨ��
	sem_init(&multi_send_sem, 0, 0);
	multi_send_flag = 1;
	//����������
	pthread_mutex_init(&Local.udp_lock, NULL);
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&multi_send_thread, &attr, (void *) multi_send_thread_func, NULL);
	pthread_attr_destroy(&attr);
	if (multi_send_thread == 0)
	{
		P_Debug("don't create UDP send thread. \n");
		return FALSE;
	}

	//�����������ݷ����̣߳��ն����������������ʱһ��û�յ���Ӧ�����η���

#if 0//def _BRUSHIDCARD_SUPPORT   //ˢ������֧��
	pthread_mutex_init(&Local.idcard_lock, NULL);

#endif

	//��ʼ��ARP Socket
	//InitArpSocket();
	//����NS�鲥��
	AddMultiGroup(m_VideoSocket, NSMULTIADDR);
	AddMultiGroup(m_DataSocket, NSMULTIADDR);

	sprintf(Local.DebugInfo, "LocalCfg.IP=%d.%d.%d.%d, LocalCfg.Addr = %s\n",
			LocalCfg.IP[0], LocalCfg.IP[1], LocalCfg.IP[2], LocalCfg.IP[3], LocalCfg.Addr);
	P_Debug(Local.DebugInfo);

	//�������ARP
//  SendFreeArp();

	Init_Timer(); //��ʼ����ʱ��

	programrun = 1;
	ch = ' ';
	P_Debug("Please select: 'q' is exit!\n");

	g_iRunLANTaskFlag = TRUE;
	//Call_Func(2, "0105", "");
	while(g_iRunLANTaskFlag)
	{
//		sleep(20);//rectify by xuqd ǰ��ĳ�����20����ֵ�ע���ˣ����ʱ������Ϊ30�룬���ģ���˳���ʱ����ȴ�ʱ��Ҳ��30��
//		printf("LAN thread is alive!\n");
		usleep(10000);
	}

	Uninit_Timer(); //�ͷŶ�ʱ��

	//�˳�NS�鲥��
	DropNsMultiGroup(m_VideoSocket, NSMULTIADDR);
	DropNsMultiGroup(m_DataSocket, NSMULTIADDR);

#if 0//def _BRUSHIDCARD_SUPPORT   //ˢ������֧��
	//��ְ������߳�
	dispart_send_flag = 0;
	usleep(40 * 1000);
	pthread_cancel(dispart_send_thread);
	sem_destroy(&dispart_send_sem);
	pthread_mutex_destroy(&Local.dispart_send_lock); //ɾ��������
#endif

	//UDP�����������ݷ����߳�
	multi_send_flag = 0;
	usleep(40 * 1000);
	//pthread_cancel(multi_send_thread);
	sem_destroy(&multi_send_sem);
	pthread_mutex_destroy(&Local.udp_lock); //ɾ��������

	//COMM�����������ݷ����߳�
	multi_comm_send_flag = 0;
	usleep(40 * 1000);
	//pthread_cancel(multi_comm_send_thread);
	sem_destroy(&multi_comm_send_sem);
	pthread_mutex_destroy(&Local.comm_lock); //ɾ��������

#if 0//def _BRUSHIDCARD_SUPPORT   //ˢ������֧��
	pthread_mutex_destroy(&Local.idcard_lock); //ɾ��������
#endif

#ifdef _MESSAGE_SUPPORT  //��Ϣ����
	DestroyMsgQueue();
#endif

	pthread_mutex_destroy(&audio_open_lock);
	pthread_mutex_destroy(&audio_close_lock);
	pthread_mutex_destroy(&audio_lock);

 	//�ر�ARP
	CloseArpSocket();
	//�ر�UDP�������߳�
	CloseUdpSocket();
 
	return (0);

}
//---------------------------------------------------------------------------
void save_file_thread_func(void)
{
#if 1
	int i;
	if (DebugMode == 1)
		P_Debug("create FLASH thread \n");
	//ϵͳ��ʼ����־
	//InitSuccFlag = 1;

	while (save_file_flag == 1)
	{
		sem_wait(&save_file_sem);
		printf("save_file_thread_func \n");

		if (Local.Save_File_Run_Flag == 0)
			Local.Save_File_Run_Flag = 1;
		else
		{
			//����������
			pthread_mutex_lock(&Local.save_lock);
			P_Debug("save thread is run\n");
			for (i = 0; i < SAVEMAX; i++)
				if (Save_File_Buff[i].isValid == 1)
				{
					switch (Save_File_Buff[i].Type)
					{
					case 1: //һ����Ϣ
						break;
					case 2: //������Ϣ
						break;
					case 3:
#ifdef _CAPTUREPIC_TO_CENTER  //����ͼƬ��������
						Deal_CapturePic_Center();
#endif
						break;
					case 4: //��������
						WriteCfgFile();
						break;
					case 5: //ID����Ϣ
#if 0//def _BRUSHIDCARD_SUPPORT   //ˢ������֧��
						WriteIDCardFile();
#endif
						break;
					}

					Save_File_Buff[i].isValid = 0;
					system("sync");
					break;
				}
			//�򿪻�����
			pthread_mutex_unlock(&Local.save_lock);
		}
	}
#endif   
}
//---------------------------------------------------------------------------
#ifdef _CAPTUREPIC_TO_CENTER  //����ͼƬ��������
//�������ͼƬ
void Deal_CapturePic_Center(void)
{
	int i;
	int ReadOk;

	if (Capture_Pic_Num >= MAX_CAPTUREPIC_NUM)
	{
		sprintf(Local.DebugInfo, "capture picture send centre, Capture_Pic_Num = %d\n", Capture_Pic_Num);
		P_Debug(Local.DebugInfo);
		return;
	}
	for (i = 0; i < MAX_CAPTUREPIC_NUM; i++)
		if (Capture_Pic_Center[i].isVilid == 0)
		{
			Capture_Pic_Center[i].isVilid = 1;
			//printf("Deal_CapturePic_Center 1\n");
			if (Capture_Pic_Center[i].jpeg_pic != NULL)
			{
				free(Capture_Pic_Center[i].jpeg_pic);
				Capture_Pic_Center[i].jpeg_pic = NULL;
			}
			strcpy(Capture_Pic_Center[i].RemoteAddr, Local.RemoteAddr);
			Capture_Pic_Center[i].recpic_tm_t = Local.recpic_tm_t;

			Capture_Pic_Center[i].jpegsize = Local.Pic_Size;
			//printf("Deal_CapturePic_Center 2 Capture_Pic_Center[i].jpegsize = %d\n", Capture_Pic_Center[i].jpegsize);
			Capture_Pic_Center[i].jpeg_pic = (unsigned char *) malloc(
					Capture_Pic_Center[i].jpegsize);
			memcpy(Capture_Pic_Center[i].jpeg_pic, Local.yuv,
					Capture_Pic_Center[i].jpegsize);
			break;
		}
}
#endif
//---------------------------------------------------------------------------
#if 0//def _BRUSHIDCARD_SUPPORT   //ˢ������֧��
void dispart_send_thread_func(void)
{
	int i;
#ifdef _DEBUG
	P_Debug("create dispart send thread \n");
#endif
	while (dispart_send_flag == 1)
	{
		sem_wait(&dispart_send_sem);
		if (Local.Dispart_Send_Run_Flag == 0)
			Local.Dispart_Send_Run_Flag = 1;
		else
		{
			//����������
			pthread_mutex_lock(&Local.dispart_send_lock);
			P_Debug("dispart thread is run\n");
			for (i = 0; i < DISPARTMAX; i++)
				if (Dispart_Send_Buff[i].isValid == 1)
				{
					switch (Dispart_Send_Buff[i].Type)
					{
					case 10: //ID����Ϣ  ReadIDCardReply
						ReadIDCardReply(Dispart_Send_Buff[i].cFromIP);
						break;
					}

					Dispart_Send_Buff[i].isValid = 0;

				}
			//�򿪻�����
			pthread_mutex_unlock(&Local.dispart_send_lock);
		}
	}
}
#endif
//---------------------------------------------------------------------------
//struct timeval tv;
void multi_send_thread_func(void)
{
	int i, j, k;
	int isAdded;
	int HaveDataSend;
	char wavFile[80];
	int CurrNo;
	short CurrPack;
	struct timeval tv1;
	int SendCapturePicFailFlag = 0;
	int FindError;
	if (DebugMode == 1)
	{
		P_Debug("create multi send thread \n");
		printf("multi_send_flag=%d\n", multi_send_flag);
	}
	while (multi_send_flag == 1)
	{
		//�ȴ��а������µ��ź�
		sem_wait(&multi_send_sem);
		HaveDataSend = 1;
		while (HaveDataSend)
		{
			//����������
			//  pthread_mutex_lock (&Local.udp_lock);
			for (i = 0; i < UDPSENDMAX; i++)
			{
				if (Multi_Udp_Buff[i].isValid == 1)
				{
					//if(DebugMode == 1)
					//     printf("send Multi_Udp_Buff[i].RemoteHost = %s,Multi_Udp_Buff[i].buf[6] = %d [7] = %d [8] = %d\n",
					//       Multi_Udp_Buff[i].RemoteHost,Multi_Udp_Buff[i].buf[6], Multi_Udp_Buff[i].buf[7], Multi_Udp_Buff[i].buf[8]);
					if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM)
					{
						if (Multi_Udp_Buff[i].m_Socket == ARP_Socket)
							ArpSendBuff(); //���ARP����
						else
						{
							//UDP����
							if (Multi_Udp_Buff[i].isValid == 1)
							{
								//20101112
								if (Multi_Udp_Buff[i].SendDelayTime == 0)
								{
									UdpSendBuff(Multi_Udp_Buff[i].m_Socket, Multi_Udp_Buff[i].RemoteHost,
											Multi_Udp_Buff[i].RemotePort, Multi_Udp_Buff[i].buf, Multi_Udp_Buff[i].nlength);
#if  0
									if((Multi_Udp_Buff[i].buf[6] == VIDEOTALK)&&(Multi_Udp_Buff[i].buf[8] == CALL))
									{
										gettimeofday(&tv1, NULL);
										sprintf(Local.DebugInfo, "tv1.tv_sec=%d, tv1.tv_usec=%d, Multi_Udp_Buff[i].RemoteHost = %s\n", tv1.tv_sec,tv1.tv_usec, Multi_Udp_Buff[i].RemoteHost);
										P_Debug(Local.DebugInfo);
									}
#endif
								}
							}
						}
						//20101112 ���͵ȴ�ʱ�����, ���ⷢ��ʱ��̫�����������
						Multi_Udp_Buff[i].SendDelayTime += 100;
						if (Multi_Udp_Buff[i].SendDelayTime >= Multi_Udp_Buff[i].DelayTime)
						{
							Multi_Udp_Buff[i].SendDelayTime = 0;
							Multi_Udp_Buff[i].SendNum++;
						}
					}
					if (Multi_Udp_Buff[i].SendNum >= MAXSENDNUM)
					{
						//����������
						//    pthread_mutex_lock (&Local.udp_lock);
						if (Multi_Udp_Buff[i].m_Socket == ARP_Socket)
						{
							Multi_Udp_Buff[i].isValid = 0;
							if (DebugMode == 1)
								P_Debug("free ARP send finish\n");
						}
						else
						{
							switch (Multi_Udp_Buff[i].buf[6])
							{
#ifdef _CAPTUREPIC_TO_CENTER  //����ͼƬ��������
							case REPORTSTATUS:
								Multi_Udp_Buff[i].isValid = 0;
								Local.ConnectCentered = 0; //����������״̬
								Local.LoginServer = 0; //�Ƿ��¼������
								//if(DebugMode == 1)
								// P_Debug("�豸��ʱ����״̬����ʧ��\n");
								break;
							case CAPTUREPIC_SEND_START: //��Ԫ�ſڻ���Χǽ���������ſڻ����ͺ�����Ƭ->���Ϳ�ʼ
								Multi_Udp_Buff[i].isValid = 0;
								break;
							case CAPTUREPIC_SEND_DATA: //��Ԫ�ſڻ���Χǽ���������ſڻ����ͺ�����Ƭ->��������
								Multi_Udp_Buff[i].isValid = 0;
								memcpy(&CurrNo, Multi_Udp_Buff[i].buf + 48,
										sizeof(CurrNo));
								memcpy(&CurrPack,
										Multi_Udp_Buff[i].buf + 56,
										sizeof(CurrPack));
								SendCapturePicFailFlag = 1;
								Local.SendToCetnering = 0; //���ڴ�������
								break;
							case CAPTUREPIC_SEND_SUCC: //��Ԫ�ſڻ���Χǽ���������ſڻ����ͺ�����Ƭ->���ͽ������ɹ���
								Multi_Udp_Buff[i].isValid = 0;
								Local.SendToCetnering = 0; //���ڴ�������
								break;
							case CAPTUREPIC_SEND_FAIL: //��Ԫ�ſڻ���Χǽ���������ſڻ����ͺ�����Ƭ->���ͽ�����ʧ�ܣ�
								Multi_Udp_Buff[i].isValid = 0;
								Local.SendToCetnering = 0; //���ڴ�������
								break;
#endif
							case NSORDER:
								if (Multi_Udp_Buff[i].CurrOrder == 255) //�����򸱻�����
								{
									Multi_Udp_Buff[i].isValid = 0;
									if (DebugMode == 1)
										P_Debug("find deputy fail \n");
								}
								else
								{
									MsgLAN2CCCallFail();

									Multi_Udp_Buff[i].isValid = 0;
								}
								break;
							case NSSERVERORDER: //����������
								Multi_Udp_Buff[i].isValid = 0;
#ifdef _DEBUG
								printf("service resolved fail\n");
#endif

								//���д���
								/*if (Local.CurrentWindow == 0)
								{
									FindError = 1;
									if (Remote.Addr[0][0] == 'Z')
									{
										if (Remote.Addr[0][4] != '1')
										{
											WaitAudioUnuse(200); //�ȴ���������
											sprintf(wavFile,"%s/inuse.wav\0",wavPath);
											StartPlayWav(wavFile, 0);
											FindError = 0;
										}
									}
									if (FindError == 1)
									{
										{
											WaitAudioUnuse(200); //�ȴ���������
											sprintf(wavFile,"%s/callfail.wav\0",wavPath);
											StartPlayWav(wavFile, 0);
										}
									}
									ClearTalkEdit(); //�� �ı���
								}*/

								break;
							case VIDEOTALK: //���������ӶԽ�
							case VIDEOTALKTRANS: //���������ӶԽ���ת����
								switch (Multi_Udp_Buff[i].buf[8])
								{
								case JOINGROUP: //�����鲥�飨���з�->���з������з�Ӧ��
									Multi_Udp_Buff[i].isValid = 0;
#ifdef _DEBUG
									printf("add multicast fail, %d\n", Multi_Udp_Buff[i].buf[6]);
#endif
									break;
								case CALL:
									if (Multi_Udp_Buff[i].buf[6] == VIDEOTALK)
									{
										//�����������������ĺ���
										for (k = 0; k < UDPSENDMAX; k++)
										{
											if (Multi_Udp_Buff[k].isValid == 1)
											{
												if (Multi_Udp_Buff[k].buf[8] == CALL)
												{
													if (k != i)
														Multi_Udp_Buff[k].isValid = 0;
												}
											}
										}
										Multi_Udp_Buff[i].isValid = 0;
#ifdef _DEBUG
										printf("call fail, %d\n", Multi_Udp_Buff[i].buf[6]);
#endif
									}
									break;
								case CALLEND: //ͨ������
									Multi_Udp_Buff[i].isValid = 0;
									Local.OnlineFlag = 0;
									Local.CallConfirmFlag = 0; //�������߱�־
									printf("multi_send_thread_func :: TalkEnd_ClearStatus()\n");
									//�Խ���������״̬�͹ر�����Ƶ
									TalkEnd_ClearStatus();
									break;
								default: //Ϊ�����������ͨ�Ž���
									Multi_Udp_Buff[i].isValid = 0;
#ifdef _DEBUG
									printf("communicate fail 1, %d\n", Multi_Udp_Buff[i].buf[6]);
#endif
									break;
								}
								break;
							case VIDEOWATCH: //���������
							case VIDEOWATCHTRANS: //�����������ת����
								switch (Multi_Udp_Buff[i].buf[8])
								{
								case CALL:
									if (Multi_Udp_Buff[i].buf[6] == VIDEOWATCH)
									{
										Multi_Udp_Buff[i].isValid = 0;
										//  Local.Status = 0;
#ifdef _DEBUG
										printf("watch fail, %d\n", Multi_Udp_Buff[i].buf[6]);
#endif
									}
									break;
								case CALLEND: //ͨ������
									Multi_Udp_Buff[i].isValid = 0;
									Local.OnlineFlag = 0;
									Local.CallConfirmFlag = 0; //�������߱�־

									switch (Local.Status)
									{
									case 3: //��������
										break;
									case 4: //����������
										Local.Status = 0; //״̬Ϊ����
										MsgLAN2CCCallHangOff();
										//	StopRecVideo();
										break;
									}
									break;
								default: //Ϊ�����������ͨ�Ž���
									Multi_Udp_Buff[i].isValid = 0;
#ifdef _DEBUG
									printf("communicate fail 2, %d\n", Multi_Udp_Buff[i].buf[6]);
#endif
									break;
								}
								break;
							default: //Ϊ�����������ͨ�Ž���
								Multi_Udp_Buff[i].isValid = 0;
								//     Local.Status = 0;
#ifdef _DEBUG
								printf("communicate fail 3, %d\n", Multi_Udp_Buff[i].buf[6]);
#endif
								break;
							}
						}
						//�򿪻�����
						// pthread_mutex_unlock (&Local.udp_lock);
					}
					if ((Multi_Udp_Buff[i].buf[6] == VIDEOTALK)
							&& (Multi_Udp_Buff[i].buf[8] == CALL))
					{
#if 0
						SendMessage(LMSG_PLAYAUDIO, (WPARAM)(START_REC_AUDIO),(LPARAM)NULL);
						SendMessage(LMSG_PLAYAUDIO, (WPARAM)(START_PLAY_AUDIO),(LPARAM)NULL);
#endif
					}
				}
			}

#ifdef _CAPTUREPIC_TO_CENTER  //����ͼƬ��������
			if (SendCapturePicFailFlag == 1)
			{
				//printf("3333333333, CurrNo = %d, CurrPack = %d\n", CurrNo, CurrPack);
				SendCapturePicFinish(CurrNo, 0);
			}
			SendCapturePicFailFlag = 0;
#endif

			//�ж������Ƿ�ȫ�������꣬���ǣ��߳���ֹ
			HaveDataSend = 0;
			for (i = 0; i < UDPSENDMAX; i++)
				if (Multi_Udp_Buff[i].isValid == 1)
				{
					HaveDataSend = 1;
					break;
				}
			//�򿪻�����
			//   pthread_mutex_unlock (&Local.udp_lock);
			usleep(100 * 1000);
		}
	}
}
//---------------------------------------------------------------------------
void GetCfg(void)
{
	FILE *cfg_fd;
	int bytes_write;
	int bytes_read;
	uint32_t Ip_Int;
	int ReadOk;
	DIR *dirp;
	char SystemOrder[100];
	int i, j;

	//��ַ����
	memcpy(LocalCfg.Addr, NullAddr, 20);
	memcpy(LocalCfg.Addr, g_stIdbtCfg.acPositionCode, 12);
	//������ַ
	LocalCfg.Mac_Addr[0] = 0x00;
	LocalCfg.Mac_Addr[1] = 0x19;
	LocalCfg.Mac_Addr[2] = 0xA6;
	LocalCfg.Mac_Addr[3] = 0x44;
	LocalCfg.Mac_Addr[4] = 0x55;
	LocalCfg.Mac_Addr[5] = 0x88;
	//IP��ַ
	NMGetIpAddress2(LocalCfg.IP);
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
	for (i = 0; i < 4; i++)
	{
		if (LocalCfg.IP_Mask[i] != 0)
			LocalCfg.IP_Broadcast[i] = LocalCfg.IP_Mask[i] & LocalCfg.IP[i];
		else
			LocalCfg.IP_Broadcast[i] = 0xFF;
	}

	//�豸��ʱ����״̬ʱ��
	LocalCfg.ReportTime = 10;

	//���뿪��
	LocalCfg.PassOpenLock = 1;
	//ˢ������
	LocalCfg.CardOpenLock = 1;
	//��������
	LocalCfg.RoomType = 0;
	//���л���
	LocalCfg.CallWaitRing = 0;

	//������ʾ  0 �ر�  1  ��
	LocalCfg.VoiceHint = 1;
	//������    0 �ر�  1  ��
	LocalCfg.KeyVoice = 1;
}
//�������ļ�
void ReadCfgFile(void)
{
	FILE *cfg_fd;
	int bytes_write;
	int bytes_read;
	uint32_t Ip_Int;
	int ReadOk;
	DIR *dirp;
	char SystemOrder[100];
	int i, j;
	//���������ļ�Ŀ¼�Ƿ����
	/*if ((dirp = opendir("/data/app")) == NULL)
	{
		if (mkdir("/data/app", 1) < 0)
			P_Debug("error to mkdir /mnt/mtd/config\n");
	}
	else
		closedir(dirp);*/

//  unlink(cfg_name);
	ReadOk = 0;
	while (ReadOk == 0)
	{
		//�������ļ�
		//  if(access(cfg_name, R_OK|W_OK) == NULL)
		if ((cfg_fd = fopen(cfg_name, "rb")) == NULL)
		{
			P_Debug("config file is not exist! create new config file \n");
			// if((cfg_fd = open(cfg_name, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) == -1)
			if ((cfg_fd = fopen(cfg_name, "wb")) == NULL)
				P_Debug("don't create config file!\n");
			else
			{
				//��ַ����
				memcpy(LocalCfg.Addr, NullAddr, 20);
				memcpy(LocalCfg.Addr, "M00010100000", 12);

				//������ַ
				LocalCfg.Mac_Addr[0] = 0x00;
				LocalCfg.Mac_Addr[1] = 0x19;
				LocalCfg.Mac_Addr[2] = 0xA6;
				LocalCfg.Mac_Addr[3] = 0x44;
				LocalCfg.Mac_Addr[4] = 0x55;
				LocalCfg.Mac_Addr[5] = 0x88;
				//IP��ַ
				Ip_Int = inet_addr("192.168.68.180");
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

				//�豸��ʱ����״̬ʱ��
				LocalCfg.ReportTime = 10;

				//��������  0 ���� �����  1 ���� �����
				LocalCfg.LockType = 0;
				//����ʱ��  ����� 0 �D�D100ms  1�D�D200ms
				//          ����� 0 �D�D5s  1�D�D10s
				LocalCfg.OpenLockTime = 0;
				//��ʱ����  0 0s  1 3s  2 5s  3 10s
				LocalCfg.DelayLockTime = 0;
				//���뿪��
				LocalCfg.PassOpenLock = 1;
				//ˢ������
				LocalCfg.CardOpenLock = 1;
				//ˢ��ͨ��
				LocalCfg.CardComm = 1;
				//�Ŵż��
				LocalCfg.DoorDetect = 1;
				//��������
				LocalCfg.RoomType = 0;
				//���л���
				LocalCfg.CallWaitRing = 0;

				//��������
				strcpy(LocalCfg.EngineerPass, "888888");
				strcpy(LocalCfg.OpenLockPass, "123456");
				//������ʾ  0 �ر�  1  ��
				LocalCfg.VoiceHint = 1;
				//������    0 �ر�  1  ��
				LocalCfg.KeyVoice = 1;
				//������
				LocalCfg.Ts_X0 = 491; //1901;
				LocalCfg.Ts_Y0 = 499; //2001;
				LocalCfg.Ts_deltaX = -830; //3744;
				LocalCfg.Ts_deltaY = 683; //3555;

				//   bytes_write=write(cfg_fd, &LocalCfg, sizeof(LocalCfg));
				bytes_write = fwrite(&LocalCfg, sizeof(LocalCfg), 1, cfg_fd);
				//  bytes_write=fwrite(&LocalCfg, 20, 1, cfg_fd);
				fclose(cfg_fd);
			}
		}
		else
		{
			//    if((cfg_fd = open(cfg_name, O_RDONLY)) == -1)
//        printf("�޷��������ļ�\n");
			//    else
			{
				ReadOk = 1;
				//  bytes_read=read(cfg_fd, Local.Addr, 12);
				bytes_read = fread(&LocalCfg, sizeof(LocalCfg), 1, cfg_fd);
				if (bytes_read != 1)
				{
					P_Debug("read config file fail, recreate default config file \n");
					ReadOk = 0;
					fclose(cfg_fd);
					unlink(cfg_name);
				}
				else
				{
					RefreshNetSetup(0); //ˢ����������
					//�㲥��ַ
					for (i = 0; i < 4; i++)
					{
						if (LocalCfg.IP_Mask[i] != 0)
							LocalCfg.IP_Broadcast[i] = LocalCfg.IP_Mask[i] & LocalCfg.IP[i];
						else
							LocalCfg.IP_Broadcast[i] = 0xFF;
					}
					fclose(cfg_fd);
				}
			}
		}
	}
//  exit(0);
}
//---------------------------------------------------------------------------
//д���������ļ�
void WriteCfgFile(void)
{
	FILE *cfg_fd;
	int j;
	uint8_t isValid;
	if ((cfg_fd = fopen(cfg_name, "wb")) == NULL)
		P_Debug("don't create config file \n");
	else
	{
		//��д���������ļ�
		fwrite(&LocalCfg, sizeof(LocalCfg), 1, cfg_fd);
		fclose(cfg_fd);
		P_Debug("save local set succeed \n");
	}
}
//---------------------------------------------------------------------------
#if 0//def _BRUSHIDCARD_SUPPORT   //ˢ������֧��
//��ID���ļ�
void ReadIDCardCfgFile(void)
{
	FILE *idcardcfg_fd;
	int bytes_write;
	int bytes_read;
	int ReadOk;
	DIR *dirp;
	int i, j;
	//���������ļ�Ŀ¼�Ƿ����
	if ((dirp = opendir("/mnt/mtd/config")) == NULL)
	{
		if (mkdir("/mnt/mtd/config", 1) < 0)
			P_Debug("error to mkdir /mnt/mtd/config\n");
	}
	else
		closedir(dirp);

//  unlink(cfg_name);
	ReadOk = 0;
	while (ReadOk == 0)
	{
		//�������ļ�
		//  if(access(cfg_name, R_OK|W_OK) == NULL)
		if ((idcardcfg_fd = fopen(idcard_name, "rb")) == NULL)
		{
			P_Debug("ID�������ļ������ڣ��������ļ�\n");
			// if((cfg_fd = open(cfg_name, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) == -1)
			if ((idcardcfg_fd = fopen(idcard_name, "wb")) == NULL)
				P_Debug("�޷�����ID�������ļ�\n");
			else
			{
				//����
				IDCardNo.Num = 0;
				//���
				IDCardNo.serialnum = 0;

				bytes_write = fwrite(&IDCardNo.Num, sizeof(IDCardNo.Num), 1, idcardcfg_fd);
				bytes_write = fwrite(&IDCardNo.serialnum, sizeof(IDCardNo.serialnum), 1, idcardcfg_fd);
				fclose(idcardcfg_fd);
			}
		}
		else
		{
			ReadOk = 1;
			bytes_read = fread(&IDCardNo.Num, sizeof(IDCardNo.Num), 1,
					idcardcfg_fd);
			if (bytes_read == 1)
				bytes_read = fread(&IDCardNo.serialnum,
						sizeof(IDCardNo.serialnum), 1, idcardcfg_fd);
			if (bytes_read != 1)
			{
				P_Debug("��ȡID�������ļ�ʧ��,��Ĭ�Ϸ�ʽ�ؽ�ID�������ļ�\n");
				ReadOk = 0;
				fclose(idcardcfg_fd);
				unlink(idcard_name);
			}
			else
			{
				/*     for(i=0; i<IDCardNo.Num; i++)
				 {
				 bytes_read=fread(IDCardNo.SN[i], sizeof(IDCardNo.SN[i]), 1, idcardcfg_fd);
				 if(bytes_read != 1)
				 {
				 IDCardNo.Num = i;
				 break;
				 }
				 }    */
				bytes_read = fread(IDCardNo.SN, BYTEPERSN * IDCardNo.Num, 1,
						idcardcfg_fd);
				fclose(idcardcfg_fd);
			}
		}
	}
//  exit(0);
}
//---------------------------------------------------------------------------
//дID���ļ�
void WriteIDCardFile(void)
{
	FILE *idcardcfg_fd;
	int j;
	uint8_t isValid;
	if ((idcardcfg_fd = fopen(idcard_name, "wb")) == NULL
		)
		P_Debug("�޷�����IDCard�ļ�\n");
	else
	{
		//��дIDCard�ļ�
		fwrite(&IDCardNo.Num, sizeof(IDCardNo.Num), 1, idcardcfg_fd);
		fwrite(&IDCardNo.serialnum, sizeof(IDCardNo.serialnum), 1,
				idcardcfg_fd);
		fwrite(IDCardNo.SN, BYTEPERSN * IDCardNo.Num, 1, idcardcfg_fd);
		fclose(idcardcfg_fd);
		P_Debug("�洢IDCard�ļ��ɹ�\n");
	}
}
//---------------------------------------------------------------------------
//��ID��Ӧ��
void ReadIDCardReply(char *cFromIP)
{
	unsigned char send_b[1520];
	int sendlength;
	int i;
	//��дID�����ݽṹ
	struct iddata1 iddata;

	if ((IDCardNo.Num % CARDPERPACK) == 0)
		iddata.TotalPackage = IDCardNo.Num / CARDPERPACK;
	else
		iddata.TotalPackage = IDCardNo.Num / CARDPERPACK + 1;
	printf("iddata.TotalPackage = %d, cFromIP = %s\n", iddata.TotalPackage, cFromIP);
	if (iddata.TotalPackage == 0)
		iddata.TotalPackage = 1;
	//for(i=0; i < IDCardNo.Num; i++)
	//  printf("%p  %p  %p  %p\n", IDCardNo.SN[0 + BYTEPERSN*i], IDCardNo.SN[1 + BYTEPERSN*i], IDCardNo.SN[2 + BYTEPERSN*i], IDCardNo.SN[3 + BYTEPERSN*i]);
	for (i = 0; i < iddata.TotalPackage; i++)
	{
		//ͷ��
		memcpy(send_b, UdpPackageHead, 6);
		//����
		send_b[6] = READIDCARD;
		send_b[7] = REPLY; //Ӧ��

		memcpy(iddata.Addr, LocalCfg.Addr, 20);
		//���
		iddata.serialnum = IDCardNo.serialnum;

		//������
		iddata.Num = IDCardNo.Num;

		iddata.CurrPackage = i + 1;

		if (iddata.CurrPackage == iddata.TotalPackage)
			iddata.CurrNum = IDCardNo.Num - CARDPERPACK * (iddata.CurrPackage - 1);
		else
			iddata.CurrNum = CARDPERPACK;

		memcpy(send_b + 8, &iddata, sizeof(iddata));
		memcpy(send_b + 48, IDCardNo.SN + CARDPERPACK * (iddata.CurrPackage - 1) * BYTEPERSN , BYTEPERSN * iddata.CurrNum);

		sendlength = 48 + BYTEPERSN * iddata.CurrNum;

		UdpSendBuff(m_DataSocket, cFromIP, RemoteDataPort, send_b, sendlength);

		if (((i % 3) == 0) && (i != 0))
			usleep(10);
	}
	//�򿪻�����
}
#endif

