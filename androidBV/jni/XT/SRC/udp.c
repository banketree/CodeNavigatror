//UDP
#include <stdio.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <semaphore.h>       //sem_t
#include <dirent.h>

#define CommonH
#include "common.h"

#ifdef  _MESSAGE_SUPPORT  //��Ϣ����
#include "../type.h"
#endif

//UDP
int SndBufLen = 1024 * 128;
int RcvBufLen = 1024 * 128;

//*****************************add by xuqd**************************
#include <XTIntfLayer.h>
//**********************************end*****************************

short UdpRecvFlag;
pthread_t udpdatarcvid;
pthread_t udpvideorcvid;
int InitUdpSocket(short lPort);
void CloseUdpSocket(void);
int UdpSendBuff(int m_Socket, char *RemoteHost, int RemotePort,
		unsigned char *buf, int nlength);
void CreateUdpDataRcvThread(void);
void CreateUdpVideoRcvThread(void);
void UdpDataRcvThread(void); //UDP���ݽ����̺߳���
void UdpVideoRcvThread(void); //UDP����Ƶ�����̺߳���
void AddMultiGroup(int m_Socket, char *McastAddr); //�����鲥��
void DropMultiGroup(int m_Socket, char *McastAddr); //�˳��鲥��
void DropNsMultiGroup(int m_Socket, char *McastAddr); //�˳�NS�鲥��
void RefreshNetSetup(int cType); //ˢ����������  0 -- δ����  1 -- ������

extern sem_t audiorec2playsem;
//extern struct tempbuf1 tempbuf;     //��Ƶ��ʱ���λ���   ������

extern sem_t videorec2playsem;
extern struct tempvideobuf1 tempvideobuf[MP4VNUM]; //��Ƶ��ʱ���λ���  ������

//int temp_video_head;   //��Ƶ���ջ���ͷ��
//int temp_video_end;    //��Ƶ���ջ���β��
//int temp_video_n;      //��Ƶ���ջ������
int AudioMuteFlag; //������־

extern struct _SYNC sync_s;

//�����߳��ڲ�������
//����
void RecvAlarm_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket);
//��Ϣ
void RecvMessage_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket); //��Ϣ
//�豸��ʱ����״̬
void RecvReportStatus_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket);
//�������Ĳ�ѯ�豸״̬
void RecvQueryStatus_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket);
//д��ַ����
void RecvWriteAddress_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket);
//����ַ����
void RecvReadAddress_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket);
//���������豸Ӧ��
void RecvSearchAllEquip_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket);
//д�豸��ַӦ��
void RecvWriteEquipAddr_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket);
//д������Ϣ
void RecvWriteHelp_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket);
//��������Ϣ
void RecvReadHelp_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket);
//д������Ϣ
void RecvWriteSetup_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket);
//��������Ϣ
void RecvReadSetup_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket);

#ifdef _BRUSHIDCARD_SUPPORT   //ˢ������֧��
//дID����Ϣ
void RecvWriteIDCard_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket);
//�洢ID����Ϣ
void SaveWriteIDCard_Func(void);
//��ID����Ϣ
void RecvReadIDCard_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket);
//ˢ����¼
void RecvBrushIDCard_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket);
#endif

//����Ӧ�ó���
void RecvDownLoadFile_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket);
//����ϵͳӳ��
void RecvDownLoadImage_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket);

#ifdef _CAPTUREPIC_TO_CENTER  //����ͼƬ��������
//��Ԫ�ſڻ���Χǽ���������ſڻ����ͺ�����Ƭ->���Ϳ�ʼ
void RecvCapture_Send_Start_Func(unsigned char *recv_buf, char *cFromIP,
		int length, int m_Socket);
//��Ԫ�ſڻ���Χǽ���������ſڻ����ͺ�����Ƭ->��������
void RecvCapture_Send_Data_Func(unsigned char *recv_buf, char *cFromIP,
		int length, int m_Socket);
//��Ԫ�ſڻ���Χǽ���������ſڻ����ͺ�����Ƭ->���ͽ������ɹ���
void RecvCapture_Send_Succ_Func(unsigned char *recv_buf, char *cFromIP,
		int length, int m_Socket);
//��Ԫ�ſڻ���Χǽ���������ſڻ����ͺ�����Ƭ->���ͽ�����ʧ�ܣ�
void RecvCapture_Send_Fail_Func(unsigned char *recv_buf, char *cFromIP,
		int length, int m_Socket);
#endif

//����Զ�̵�����Ϣ
void RecvRemoteDebugInfo_Func(unsigned char *recv_buf, char *cFromIP,
		int length, int m_Socket);

#ifdef _REMOTECALLTEST  //Զ�̺��в���
//����Զ�̺��в���
void RecvRemoteCallTest_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket);
#endif

//����
void RecvNSAsk_Func(unsigned char *recv_buf, char *cFromIP, int m_Socket); //��������
void RecvNSReply_Func(unsigned char *recv_buf, char *cFromIP, int m_Socket); //����Ӧ��
//����
void RecvWatchCall_Func(unsigned char *recv_buf, char *cFromIP); //���Ӻ���
void RecvWatchLineUse_Func(unsigned char *recv_buf, char *cFromIP); //����ռ��Ӧ��
void RecvWatchCallAnswer_Func(unsigned char *recv_buf, char *cFromIP); //���Ӻ���Ӧ��
void RecvWatchCallConfirm_Func(unsigned char *recv_buf, char *cFromIP); //ͨ������ȷ��
void RecvWatchCallEnd_Func(unsigned char *recv_buf, char *cFromIP); //���Ӻ��н���
void RecvWatchZoomOut_Func(unsigned char *recv_buf, char *cFromIP); //�Ŵ�(720*480)
void RecvWatchZoomIn_Func(unsigned char *recv_buf, char *cFromIP); //��С(352*240)
void RecvWatchCallUpDown_Func(unsigned char *recv_buf, char *cFromIP,
		int length); //��������
//�Խ�
void RecvTalkCall_Func(unsigned char *recv_buf, char *cFromIP); //�Խ�����
void RecvTalkLineUse_Func(unsigned char *recv_buf, char *cFromIP); //�Խ�ռ��Ӧ��
void RecvTalkCallAnswer_Func(unsigned char *recv_buf, char *cFromIP); //�Խ�����Ӧ��
void RecvTalkCallConfirm_Func(unsigned char *recv_buf, char *cFromIP); //�Խ�����ȷ��
void RecvTalkOpenLock_Func(unsigned char *recv_buf, char *cFromIP); //�Խ�Զ�̿���
void RecvTalkCallAsk_Func(unsigned char *recv_buf, char *cFromIP); //�Խ���ʼͨ��ѯ��
void RecvTalkCallStart_Func(unsigned char *recv_buf, char *cFromIP); //�Խ���ʼͨ��
void RecvTalkCallEnd_Func(unsigned char *recv_buf, char *cFromIP); //�Խ����н���
void RecvTalkZoomOut_Func(unsigned char *recv_buf, char *cFromIP); //�Ŵ�(720*480)
void RecvTalkZoomIn_Func(unsigned char *recv_buf, char *cFromIP); //��С(352*240)
void RecvTalkCallUpDown_Func(unsigned char *recv_buf, char *cFromIP, int length); //�Խ�����
void TalkEnd_ClearStatus(void); //�Խ���������״̬�͹ر�����Ƶ
void RecvForceIFrame_Func(unsigned char *recv_buf, char *cFromIP); //�Խ�ǿ��I֡
void ForceIFrame_Func(void); //ǿ��I֡

#ifdef _MULTI_MACHINE_SUPPORT  //��ֻ�֧��
void RecvTalkJoinGroup_Func(unsigned char *recv_buf, char *cFromIP); //�����鲥�飨���з�->���з������з�Ӧ��
void RecvTalkLeaveGroup_Func(unsigned char *recv_buf, char *cFromIP); //�˳��鲥�飨���з�->���з������з�Ӧ��
void ExitGroup(unsigned char *buf); //���������з��˳��鲥������
void SingleExitGroup(unsigned char *buf); //�򵥸����з��˳��鲥������
void RecvTalkTurnTalk_Func(unsigned char *recv_buf, char *cFromIP); //ת�ӣ����з�->���з������з�Ӧ��

void TalkTurnOn_Status(void); //����ת����
#endif

int downbuflen;
char downip[20];
unsigned char *downbuf;
pthread_t download_image_thread; //����ϵͳӳ���߳�
void download_image_thread_func(void);
int downloaded_flag[2000]; //�����ر�־
int OldPackage;

void SendUdpOne(unsigned char Order, unsigned char SonOrder,
		unsigned char PerCent, char *cFromIP);
//---------------------------------------------------------------------------
int InitUdpSocket(short lPort)
{
	struct sockaddr_in s_addr;
	int nZero = 0;
	int iLen;
	int m_Socket;
	int nYes;
	int ret;
	int rc;
	int ttl; //����TTLֵ

	/* ���� socket , �ؼ�������� SOCK_DGRAM */
	if ((m_Socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		P_Debug("Create socket error\r\n");
		return 0;
	}
	else
		P_Debug("create socket.\n\r");

	if (m_EthSocket == 0)
		m_EthSocket = socket(AF_INET, SOCK_DGRAM, 0);

	memset(&s_addr, 0, sizeof(struct sockaddr_in));
	/* ���õ�ַ�Ͷ˿���Ϣ */
	s_addr.sin_family = AF_INET;
	s_addr.sin_port = htons(lPort);
	s_addr.sin_addr.s_addr = INADDR_ANY; //inet_addr(LocalIP);//INADDR_ANY;

	iLen = sizeof(nZero); //  SO_SNDBUF
	nZero = SndBufLen; //128K
	setsockopt(m_Socket, SOL_SOCKET, SO_SNDBUF, (char*) &nZero,
			sizeof((char*) &nZero));
	nZero = RcvBufLen; //128K
	setsockopt(m_Socket, SOL_SOCKET, SO_RCVBUF, (char*) &nZero,
			sizeof((char*) &nZero));

	ttl = MULTITTL; //����TTLֵ
	rc = setsockopt(m_Socket, IPPROTO_IP, IP_MULTICAST_TTL, (char *) &ttl,
			sizeof(ttl));
//  rc = getsockopt(m_Socket, IPPROTO_IP, IP_TTL, (char *)&ttl, sizeof(ttl));

	//����Ƶ�˿ڣ������ͺͽ��չ㲥
	//20100914  DataPort Ҳ�ɷ��͹㲥
	//if(lPort == LocalVideoPort)
	{
		nYes = 1;
		if (setsockopt(m_Socket, SOL_SOCKET, SO_BROADCAST, (char *) &nYes,
				sizeof((char *) &nYes)) == -1)
		{
			P_Debug("set broadcast error.\n\r");
			return 0;
		}
	}

	/* �󶨵�ַ�Ͷ˿���Ϣ */
	if ((bind(m_Socket, (struct sockaddr *) &s_addr, sizeof(s_addr))) == -1)
	{
		perror("bind error");
		exit(errno);
		return 0;
	}
	else
		P_Debug("bind address to socket.\n\r");
	if (lPort == LocalDataPort)
	{
		m_DataSocket = m_Socket;
		//����UDP�����߳�
		CreateUdpDataRcvThread();
	}
	if (lPort == LocalVideoPort)
	{
		m_VideoSocket = m_Socket;
		//����UDP�����߳�
		CreateUdpVideoRcvThread();
	}
	return 1;
}
//---------------------------------------------------------------------------
void CloseUdpSocket(void)
{
	UdpRecvFlag = 0;
	pthread_cancel(udpdatarcvid);
	pthread_cancel(udpvideorcvid);
	close(m_DataSocket);
	close(m_VideoSocket);

	if (m_EthSocket > 0)
		close(m_EthSocket);
}
//---------------------------------------------------------------------------
#define ISSETUPPACKETSIZE   //������С��
#define SMALLESTSIZE  512    //UDP��С����С
int UdpSendBuff(int m_Socket, char *RemoteHost, int RemotePort,
		unsigned char *buf, int nlength)
{
	struct sockaddr_in To;
	int nSize;
	To.sin_family = AF_INET;
	To.sin_port = htons(RemotePort);
	To.sin_addr.s_addr = inet_addr(RemoteHost);
#ifdef ISSETUPPACKETSIZE
	if (nlength < SMALLESTSIZE)
		nlength = SMALLESTSIZE;
#endif
	//printf("RemoteHost = %s, nlength = %d\n", RemoteHost, nlength);
	nSize = sendto(m_Socket, buf, nlength, 0, (struct sockaddr*) &To,
			sizeof(struct sockaddr));
	return nSize;
}
//---------------------------------------------------------------------------
void CreateUdpDataRcvThread()
{
	int i, ret;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	ret = pthread_create(&udpdatarcvid, &attr, (void *) UdpDataRcvThread, NULL);
	pthread_attr_destroy(&attr);
	if (DebugMode == 1)
		P_Debug("Create UDP data pthread!\n");
	if (ret != 0)
	{
		P_Debug("Create data pthread error!\n");
		exit(1);
	}
}
//---------------------------------------------------------------------------
void CreateUdpVideoRcvThread()
{
	int i, ret;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	ret = pthread_create(&udpvideorcvid, &attr, (void *) UdpVideoRcvThread,
			NULL);
	pthread_attr_destroy(&attr);
	if (DebugMode == 1)
		P_Debug("Create UDP video pthread!\n");
	if (ret != 0)
	{
		P_Debug("Create video pthread error!\n");
		exit(1);
	}
}
//---------------------------------------------------------------------------
void AddMultiGroup(int m_Socket, char *McastAddr) //�����鲥��
{
// Winsock1.0
	struct ip_mreq mcast; // Winsock1.0
//  memset(&mcast, 0, sizeof(struct ip_mreq));
	mcast.imr_multiaddr.s_addr = inet_addr(McastAddr);
	mcast.imr_interface.s_addr = INADDR_ANY;
	if (setsockopt(m_Socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mcast,
			sizeof(mcast)) == -1)
	{
		P_Debug("set multicast error.\n\r");
		return;
	}
}
//---------------------------------------------------------------------------
void DropMultiGroup(int m_Socket, char *McastAddr) //�˳��鲥��
{
// Winsock1.0
	struct ip_mreq mcast; // Winsock1.0
	char IP_Group[20];
	//�鿴�Ƿ��������鲥����
	if (Local.IP_Group[0] != 0)
	{
		sprintf(IP_Group, "%d.%d.%d.%d\0", Local.IP_Group[0], Local.IP_Group[1],
				Local.IP_Group[2], Local.IP_Group[3]);
		Local.IP_Group[0] = 0; //�鲥��ַ
		Local.IP_Group[1] = 0;
		Local.IP_Group[2] = 0;
		Local.IP_Group[3] = 0;
		//  memset(&mcast, 0, sizeof(struct ip_mreq));
		mcast.imr_multiaddr.s_addr = inet_addr(IP_Group);
		mcast.imr_interface.s_addr = INADDR_ANY;
		if (setsockopt(m_Socket, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char*) &mcast,
				sizeof(mcast)) == -1)
		{
			P_Debug("drop multicast error.\n\r");
			return;
		}
	}
}
//---------------------------------------------------------------------------
void DropNsMultiGroup(int m_Socket, char *McastAddr) //�˳�NS�鲥��
{
// Winsock1.0
	struct ip_mreq mcast; // Winsock1.0

	//  memset(&mcast, 0, sizeof(struct ip_mreq));
	mcast.imr_multiaddr.s_addr = inet_addr(McastAddr);
	mcast.imr_interface.s_addr = INADDR_ANY;
	if (setsockopt(m_Socket, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char*) &mcast,
			sizeof(mcast)) == -1)
	{
		P_Debug("drop multicast error.\n\r");
		return;
	}
}
//---------------------------------------------------------------------------
void RefreshNetSetup(int cType) //ˢ����������  0 -- δ����  1 -- ������
{
	char SystemOrder[100];
	//����������
	pthread_mutex_lock(&Local.udp_lock);
	//�˳�NS�鲥��
	if (cType == 1)
	{
		DropNsMultiGroup(m_VideoSocket, NSMULTIADDR);
		DropNsMultiGroup(m_DataSocket, NSMULTIADDR);
	}
	//����MAC��ַ
	system("ifconfig eth0 down");
	sprintf(SystemOrder,
			"ifconfig eth0 hw ether %02X:%02X:%02X:%02X:%02X:%02X\0",
			LocalCfg.Mac_Addr[0], LocalCfg.Mac_Addr[1], LocalCfg.Mac_Addr[2],
			LocalCfg.Mac_Addr[3], LocalCfg.Mac_Addr[4], LocalCfg.Mac_Addr[5]);
	system(SystemOrder);
	system("ifconfig eth0 up");
	//����IP��ַ����������
	sprintf(SystemOrder, "ifconfig eth0 %d.%d.%d.%d netmask %d.%d.%d.%d\0",
			LocalCfg.IP[0], LocalCfg.IP[1], LocalCfg.IP[2], LocalCfg.IP[3],
			LocalCfg.IP_Mask[0], LocalCfg.IP_Mask[1], LocalCfg.IP_Mask[2],
			LocalCfg.IP_Mask[3]);
	system(SystemOrder);
	//��������
	sprintf(SystemOrder, "route add default gw %d.%d.%d.%d\0",
			LocalCfg.IP_Gate[0], LocalCfg.IP_Gate[1], LocalCfg.IP_Gate[2],
			LocalCfg.IP_Gate[3]);
	system(SystemOrder);
	//����NS�鲥��
	if (cType == 1)
	{
		AddMultiGroup(m_VideoSocket, NSMULTIADDR);
		AddMultiGroup(m_DataSocket, NSMULTIADDR);
	}
	//�򿪻�����
	pthread_mutex_unlock(&Local.udp_lock);
}
//---------------------------------------------------------------------------
void UdpDataRcvThread(void) //UDP�����̺߳���
{
	/* ѭ���������� */
//  int oldframeno=0;
	unsigned char send_b[1520];
	int sendlength;
	char FromIP[20];
	int newframeno;
	int currpackage;
	int i, j;
	int sub;
	short PackIsExist; //���ݰ��ѽ��ձ�־
	short FrameIsNew; //���ݰ��Ƿ�����֡�Ŀ�ʼ
	struct sockaddr_in c_addr;
	socklen_t addr_len;
	int len;
	int tmp;
	unsigned char buff[8096];

	char tmpAddr[21];
	int isAddrOK;
	if (DebugMode == 1)
		P_Debug("This is udp pthread.\n");
	UdpRecvFlag = 1;

	addr_len = sizeof(c_addr);
	while (UdpRecvFlag == 1)
	{
		len = recvfrom(m_DataSocket, buff, sizeof(buff) - 1, 0,
				(struct sockaddr *) &c_addr, &addr_len);
		if (len < 0)
		{
			perror("recvfrom");
			exit(errno);
		}
		buff[len] = '\0';
//    printf("�յ�����%s:%d����Ϣ:%s\n\r",
//            inet_ntoa(c_addr.sin_addr), ntohs(c_addr.sin_port), buff);
		strcpy(FromIP, inet_ntoa(c_addr.sin_addr));
		//if(DebugMode == 1)
		//  printf("FromIP is %s\n",FromIP);
		if ((buff[0] == UdpPackageHead[0]) && (buff[1] == UdpPackageHead[1])
				&& (buff[2] == UdpPackageHead[2])
				&& (buff[3] == UdpPackageHead[3])
				&& (buff[4] == UdpPackageHead[4])
				&& (buff[5] == UdpPackageHead[5]))
		{
			switch (buff[6])
			{
			case ALARM: //����
				RecvAlarm_Func(buff, FromIP, len, m_DataSocket);
				break;
			case SENDMESSAGE: //��Ϣ
				RecvMessage_Func(buff, FromIP, len, m_DataSocket);
				break;
			case REPORTSTATUS: //�豸��ʱ����״̬
#ifdef ISSETUPPACKETSIZE
				if (len == SMALLESTSIZE)
#else
				if(len == 40)
#endif
				{
					RecvReportStatus_Func(buff, FromIP, len, m_DataSocket);
				}
				else
				{
					if (DebugMode == 1)
						P_Debug("�豸��ʱ����״̬Ӧ�𳤶��쳣\n");
				}
				break;
			case QUERYSTATUS: //�������Ĳ�ѯ�豸״̬
#ifdef ISSETUPPACKETSIZE
				if (len == SMALLESTSIZE)
#else
				if(len == 40)
#endif
				{
					RecvQueryStatus_Func(buff, FromIP, len, m_DataSocket);
				}
				else
				{
					if (DebugMode == 1)
						P_Debug("�������Ĳ�ѯ�豸״̬�����쳣\n");
				}
				break;
			case WRITEADDRESS: //д��ַ����
#ifdef ISSETUPPACKETSIZE
				if (len == SMALLESTSIZE)
#else
				if(len == 72)
#endif
				{
					RecvWriteAddress_Func(buff, FromIP, len, m_DataSocket);
				}
				else
				{
					if (DebugMode == 1)
						P_Debug("д��ַ���ó����쳣\n");
				}
				break;
			case READADDRESS: //����ַ����
#ifdef ISSETUPPACKETSIZE
				if (len == SMALLESTSIZE)
#else
				if(len == 28)
#endif
				{
					RecvReadAddress_Func(buff, FromIP, len, m_DataSocket);
				}
				else
				{
					if (DebugMode == 1)
						P_Debug("����ַ���ó����쳣\n");
				}
				break;
			case SEARCHALLEQUIP: //252  //���������豸���������ģ����豸��
#ifdef ISSETUPPACKETSIZE
				if (len == SMALLESTSIZE)
#else
				if(len == 28)
#endif
				{
					RecvSearchAllEquip_Func(buff, FromIP, len, m_DataSocket);
				}
				else
				{
					if (DebugMode == 1)
						P_Debug("���������豸�����쳣\n");
				}
				break;
			case WRITEEQUIPADDR: //254  //д�豸��ַ���������ģ����豸��
#ifdef ISSETUPPACKETSIZE
				if (len == SMALLESTSIZE)
#else
				if(len == 94)
#endif
				{
					RecvWriteEquipAddr_Func(buff, FromIP, len, m_DataSocket);
				}
				else
				{
					if (DebugMode == 1)
						P_Debug("д�豸��ַ�����쳣\n");
				}
				break;
			case WRITEHELP: //д������Ϣ
				RecvWriteHelp_Func(buff, FromIP, len, m_DataSocket);
				break;
			case READHELP: //��������Ϣ
#ifdef ISSETUPPACKETSIZE
				if (len == SMALLESTSIZE)
#else
				if(len == 28)
#endif
				{
					RecvReadHelp_Func(buff, FromIP, len, m_DataSocket);
				}
				else
				{
					if (DebugMode == 1)
						P_Debug("��������Ϣ�����쳣\n");
				}
				break;
			case WRITESETUP: //д������Ϣ
#ifdef ISSETUPPACKETSIZE
				if (len == SMALLESTSIZE)
#else
				if(len == 54)
#endif
				{
					RecvWriteSetup_Func(buff, FromIP, len, m_DataSocket);
				}
				else
				{
					if (DebugMode == 1)
						P_Debug("д������Ϣ�����쳣\n");
				}
				break;
			case READSETUP: //��������Ϣ
#ifdef ISSETUPPACKETSIZE
				if (len == SMALLESTSIZE)
#else
				if(len == 28)
#endif
				{
					RecvReadSetup_Func(buff, FromIP, len, m_DataSocket);
				}
				else
				{
					if (DebugMode == 1)
						P_Debug("��������Ϣ�����쳣\n");
				}
				break;
#ifdef _BRUSHIDCARD_SUPPORT   //ˢ������֧��
			case WRITEIDCARD: //дID����Ϣ
				RecvWriteIDCard_Func(buff, FromIP, len, m_DataSocket);
				break;
			case READIDCARD: //��ID����Ϣ
				RecvReadIDCard_Func(buff, FromIP, len, m_DataSocket);
				break;
			case BRUSHIDCARD: //ˢ����¼
				RecvBrushIDCard_Func(buff, FromIP, len, m_DataSocket);
				break;
#endif
//        case NSOrder:   //�����������������ڹ㲥��
			case NSSERVERORDER: //����������(NS������)
				switch (buff[7])
				{
				case 1: //����
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
					if(len == 56)
#endif
					{
						RecvNSAsk_Func(buff, FromIP, m_DataSocket);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("�����������ݳ����쳣\n");
					}
					break;
				case 2: //������Ӧ
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
					if(len >= 57)
#endif
					{
						RecvNSReply_Func(buff, FromIP, m_DataSocket);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("����Ӧ�����ݳ����쳣\n");
					}
					break;
				}
				break;
			case DOWNLOADFILE: //����Ӧ�ó���
				if (len >= (9 + sizeof(struct downfile1)))
				{
					RecvDownLoadFile_Func(buff, FromIP, len, m_DataSocket);
				}
				else
				{
					if (DebugMode == 1)
						P_Debug("����Ӧ�ó��򳤶��쳣\n");
				}
				break;
			case DOWNLOADIMAGE: //����ϵͳӳ��
				if (len >= (9 + sizeof(struct downfile1)))
				{
					RecvDownLoadImage_Func(buff, FromIP, len, m_DataSocket);
				}
				else
				{
					if (DebugMode == 1)
						P_Debug("����ϵͳӳ�񳤶��쳣\n");
				}
				break;
#ifdef _CAPTUREPIC_TO_CENTER  //����ͼƬ��������
			case CAPTUREPIC_SEND_START: //��Ԫ�ſڻ���Χǽ���������ſڻ����ͺ�����Ƭ->���Ϳ�ʼ
				if (len >= 67)
				{
					RecvCapture_Send_Start_Func(buff, FromIP, len,
							m_DataSocket);
				}
				else
				{
					P_Debug("���ͺ�����Ƭ->���Ϳ�ʼ�����쳣\n");
				}
				break;
			case CAPTUREPIC_SEND_DATA: //��Ԫ�ſڻ���Χǽ���������ſڻ����ͺ�����Ƭ->��������
				if (len >= 58)
				{
					RecvCapture_Send_Data_Func(buff, FromIP, len, m_DataSocket);
				}
				else
				{
					P_Debug("���ͺ�����Ƭ->���Ϳ�ʼ�����쳣\n");
				}
				break;
			case CAPTUREPIC_SEND_SUCC: //��Ԫ�ſڻ���Χǽ���������ſڻ����ͺ�����Ƭ->���ͽ������ɹ���
				if (len >= 42)
				{
					RecvCapture_Send_Succ_Func(buff, FromIP, len, m_DataSocket);
				}
				else
				{
#ifdef _DEBUG
					P_Debug("���ͺ�����Ƭ->���Ϳ�ʼ�����쳣\n");
#endif
				}
				break;
			case CAPTUREPIC_SEND_FAIL: //��Ԫ�ſڻ���Χǽ���������ſڻ����ͺ�����Ƭ->���ͽ�����ʧ�ܣ�
				if (len >= 42)
				{
					RecvCapture_Send_Fail_Func(buff, FromIP, len, m_DataSocket);
				}
				else
				{
#ifdef _DEBUG
					P_Debug("���ͺ�����Ƭ->���Ϳ�ʼ�����쳣\n");
#endif
				}
				break;
#endif
			case REMOTEDEBUGINFO: //����Զ�̵�����Ϣ
				if (len == 29)
				{
					RecvRemoteDebugInfo_Func(buff, FromIP, len, m_DataSocket);
				}
				else
				{
#ifdef _DEBUG
					P_Debug("����Զ�̵�����Ϣ�����쳣\n");
#endif
				}
				break;
#ifdef _REMOTECALLTEST  //Զ�̺��в���
				case REMOTETEST: //����Զ�̺��в���
				if(len == 49)
				{
					RecvRemoteCallTest_Func(buff, FromIP, len, m_DataSocket);
				}
				else
				{
#ifdef _DEBUG
					P_Debug("����Զ�̺��в��Գ����쳣\n");
#endif
				}
				break;
#endif
			}
		}
		if (strcmp(buff, "exit") == 0)
		{
			P_Debug("recvfrom888888888\n");
			UdpRecvFlag = 0;
			//   break;
		}
	}
}
//---------------------------------------------------------------------------
//����
void RecvAlarm_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket)
{
	int i, j;
	int isAddrOK;
	i = 0;
	isAddrOK = 1;
	for (j = 8; j < 8 + Local.AddrLen; j++)
		if (LocalCfg.Addr[j - 8] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	//��ַƥ��
	if (isAddrOK == 1)
	{
		//����������
		pthread_mutex_lock(&Local.udp_lock);
		if ((recv_buf[7] & 0x03) == REPLY
		) //Ӧ��
			for (i = 0; i < UDPSENDMAX; i++)
				if (Multi_Udp_Buff[i].isValid == 1)
					if (Multi_Udp_Buff[i].m_Socket == m_DataSocket)
						if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM
						)
							if (Multi_Udp_Buff[i].buf[6] == ALARM
							)
								if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK
								)
									if (strcmp(Multi_Udp_Buff[i].RemoteHost,
											cFromIP) == 0)
									{
										Multi_Udp_Buff[i].isValid = 0;
										if (DebugMode == 1)
											P_Debug("�յ�����Ӧ��\n");
										break;
									}
		//�򿪻�����
		pthread_mutex_unlock(&Local.udp_lock);
	}
}
//---------------------------------------------------------------------------
//��Ϣ
void RecvMessage_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket)
{

}
//---------------------------------------------------------------------------
//�豸��ʱ����״̬
void RecvReportStatus_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket)
{
	int i, j;
	int newlength;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	//ʱ��
	time_t t;

	i = 0;
	isAddrOK = 1;
	for (j = 8; j < 8 + Local.AddrLen; j++)
		if (LocalCfg.Addr[j - 8] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	//��ַƥ��
	if (isAddrOK == 1)
	{
		//����������
		pthread_mutex_lock(&Local.udp_lock);
		if ((recv_buf[7] & 0x03) == REPLY
		) //Ӧ��
			for (i = 0; i < UDPSENDMAX; i++)
				if (Multi_Udp_Buff[i].isValid == 1)
					if (Multi_Udp_Buff[i].m_Socket == m_DataSocket)
						if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM
						)
							if (Multi_Udp_Buff[i].buf[6] == REPORTSTATUS
							)
								if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK
								)
									if (strcmp(Multi_Udp_Buff[i].RemoteHost,
											cFromIP) == 0)
									{
										Multi_Udp_Buff[i].isValid = 0;
#ifdef _CAPTUREPIC_TO_CENTER  //����ͼƬ��������
										Local.ConnectCentered = 1; //����������״̬
#endif
										if (((recv_buf[29] << 8) + recv_buf[28])
												!= LocalCfg.ReportTime)
										{
											LocalCfg.ReportTime = (recv_buf[29]
													<< 8) + recv_buf[28];
											Save_Setup(); //��Flash�д洢�ļ�
										}
										//У׼ʱ��
										if (((curr_tm_t->tm_year + 1900)
												!= ((recv_buf[30] << 8)
														+ recv_buf[31]))
												|| ((curr_tm_t->tm_mon + 1)
														!= recv_buf[32])
												|| (curr_tm_t->tm_mday
														!= recv_buf[33])
												|| (curr_tm_t->tm_hour
														!= recv_buf[34])
												|| (curr_tm_t->tm_min
														!= recv_buf[35])
												|| (curr_tm_t->tm_sec
														!= recv_buf[36]))
										{
											curr_tm_t->tm_year = (recv_buf[30]
													<< 8) + recv_buf[31] - 1900;
											curr_tm_t->tm_mon = recv_buf[32]
													- 1;
											curr_tm_t->tm_mday = recv_buf[33];
											curr_tm_t->tm_hour = recv_buf[34];
											curr_tm_t->tm_min = recv_buf[35];
											curr_tm_t->tm_sec = recv_buf[36];
											t = mktime(curr_tm_t);
											stime((long*) &t);
										}
										//����Ԥ��
										if ((Local.Weather[0] != recv_buf[37])
												|| (Local.Weather[1]
														!= recv_buf[38])
												|| (Local.Weather[2]
														!= recv_buf[39]))
										{
											Local.Weather[0] = recv_buf[37];
											Local.Weather[1] = recv_buf[38];
											Local.Weather[2] = recv_buf[39];
											if ((Local.Weather[0] == 0)
													&& (Local.Weather[1] == 0)
													&& (Local.Weather[2] == 0))
											{
											}
											else
											{
											}
										}
										//if(DebugMode == 1)
										//  P_Debug("�յ��豸��ʱ����״̬Ӧ��\n");
										break;
									}
		//�򿪻�����
		pthread_mutex_unlock(&Local.udp_lock);
	}
}
//---------------------------------------------------------------------------
//�������Ĳ�ѯ�豸״̬
void RecvQueryStatus_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket)
{
	int i, j;
	int newlength;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	//ʱ��
	time_t t;

	i = 0;
	isAddrOK = 1;
	for (j = 8; j < 8 + Local.AddrLen; j++)
		if (LocalCfg.Addr[j - 8] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	//��ַƥ��
	if (isAddrOK == 1)
	{
#ifdef _CAPTUREPIC_TO_CENTER  //����ͼƬ��������
		Local.ConnectCentered = 1; //����������״̬
#endif
		memcpy(send_b, UdpPackageHead, 6);
		send_b[6] = QUERYSTATUS;
		send_b[7] = REPLY; //Ӧ��
		memcpy(send_b + 8, LocalCfg.Addr, 20);
		memcpy(send_b + 28, LocalCfg.Mac_Addr, 6);
		sendlength = 34;
		if (m_Socket == m_DataSocket)
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);
		if (m_Socket == m_VideoSocket)
			UdpSendBuff(m_Socket, cFromIP, RemoteVideoPort, send_b, sendlength);
	}
}
//---------------------------------------------------------------------------
//д��ַ����
void RecvWriteAddress_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket)
{
	int i, j;
	int newlength;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;

	i = 0;
	isAddrOK = 1;
#if 0
	for(j=8; j<8+Local.AddrLen; j++)
	if(LocalCfg.Addr[j-8] != recv_buf[j])
	{
		isAddrOK = 0;
		break;
	}
#endif
	//��ַƥ��
	if (isAddrOK == 1)
	{
		memcpy(send_b, recv_buf, length);
		send_b[7] = REPLY; //Ӧ��
		sendlength = length;
		if (m_Socket == m_DataSocket)
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);
		if (m_Socket == m_VideoSocket)
			UdpSendBuff(m_Socket, cFromIP, RemoteVideoPort, send_b, sendlength);

		//д��ַ����
		if (recv_buf[28] & 0x01) //��ַ����
			memcpy(LocalCfg.Addr, recv_buf + 30, 20);
		if (recv_buf[28] & 0x02) //������ַ
		{
			memcpy(LocalCfg.Mac_Addr, recv_buf + 50, 6);
		}
		if (recv_buf[28] & 0x04) //IP��ַ
		{
			memcpy(LocalCfg.IP, recv_buf + 56, 4);
		}
		if (recv_buf[28] & 0x08) //��������
		{
			memcpy(LocalCfg.IP_Mask, recv_buf + 60, 4);
		}
		if (recv_buf[28] & 0x10) //���ص�ַ
		{
			memcpy(LocalCfg.IP_Gate, recv_buf + 64, 4);
		}
		if (recv_buf[28] & 0x20) //��������ַ
			memcpy(LocalCfg.IP_Server, recv_buf + 68, 4);

		Save_Setup(); //��Flash�д洢�ļ�
		RefreshNetSetup(1); //ˢ����������
	}
}
//---------------------------------------------------------------------------
//����ַ����
void RecvReadAddress_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket)
{
	int i, j;
	int newlength;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;

	i = 0;
	isAddrOK = 1;
#if 0
	for(j=8; j<8+Local.AddrLen; j++)
	if(LocalCfg.Addr[j-8] != recv_buf[j])
	{
		isAddrOK = 0;
		break;
	}
#endif
	//��ַƥ��
	if (isAddrOK == 1)
	{
		memcpy(send_b, recv_buf, length);
		send_b[7] = REPLY; //Ӧ��

		send_b[28] = 0;
		send_b[29] = 0;

		//��ַ����
		memcpy(send_b + 30, LocalCfg.Addr, 20);
		//������ַ
		memcpy(send_b + 50, LocalCfg.Mac_Addr, 6);
		//IP��ַ
		memcpy(send_b + 56, LocalCfg.IP, 4);
		//��������
		memcpy(send_b + 60, LocalCfg.IP_Mask, 4);
		//���ص�ַ
		memcpy(send_b + 64, LocalCfg.IP_Gate, 4);
		//��������ַ
		memcpy(send_b + 68, LocalCfg.IP_Server, 4);

		sendlength = 72;
		if (m_Socket == m_DataSocket)
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);
		if (m_Socket == m_VideoSocket)
			UdpSendBuff(m_Socket, cFromIP, RemoteVideoPort, send_b, sendlength);
	}
}
//---------------------------------------------------------------------------
//���������豸Ӧ��
void RecvSearchAllEquip_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket)
{
	int i, j;
	int newlength;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;

	i = 0;
	isAddrOK = 1;
	printf("RecvSearchAllEquip_Func1\n");
	if (recv_buf[7] == REPLY
	)
		return;
#if 0
	for(j=8; j<8+Local.AddrLen; j++)
	if(LocalCfg.Addr[j-8] != recv_buf[j])
	{
		isAddrOK = 0;
		break;
	}
#endif
	//strcpy(cFromIP, "192.168.68.255");
	strcpy(cFromIP, "255.255.255.255");

	//��ַƥ��
	if (isAddrOK == 1)
	{
		printf("RecvSearchAllEquip_Func2\n");

		memcpy(send_b, recv_buf, length);
		send_b[7] = REPLY; //Ӧ��

		send_b[8] = 0x05;
		send_b[9] = 0;

		//��ַ����
		memcpy(send_b + 10, LocalCfg.Addr, 20);
		//������ַ
		memcpy(send_b + 30, LocalCfg.Mac_Addr, 6);
		//IP��ַ
		memcpy(send_b + 36, LocalCfg.IP, 4);
		//��������
		memcpy(send_b + 40, LocalCfg.IP_Mask, 4);
		//���ص�ַ
		memcpy(send_b + 44, LocalCfg.IP_Gate, 4);
		//��������ַ
		memcpy(send_b + 48, LocalCfg.IP_Server, 4);
		//�汾��
		memcpy(send_b + 52, SERIALNUM, 16);

		sendlength = 68;
		if (m_Socket == m_DataSocket)
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);
		if (m_Socket == m_VideoSocket)
			UdpSendBuff(m_Socket, cFromIP, RemoteVideoPort, send_b, sendlength);
	}
}
//---------------------------------------------------------------------------
//д�豸��ַӦ��
void RecvWriteEquipAddr_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket)
{
	int i, j;
	int newlength;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;

	if (recv_buf[7] == REPLY
	)
		return;

	printf("RecvWriteEquipAddr_Func \n");

	i = 0;
	isAddrOK = 1;
#if 1
	for (j = 10; j < 10 + Local.AddrLen; j++)
		if (LocalCfg.Addr[j - 10] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}

	for (j = 30; j < 30 + 6; j++)
		if (LocalCfg.Mac_Addr[j - 30] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}

	for (j = 36; j < 36 + 4; j++)
		if (LocalCfg.IP[j - 36] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
#endif

	strcpy(cFromIP, "255.255.255.255");
	//��ַƥ��
	if (isAddrOK == 1)
	{
		memcpy(send_b, recv_buf, length);
		send_b[7] = REPLY; //Ӧ��
		sendlength = length;
		if (m_Socket == m_DataSocket)
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);
		if (m_Socket == m_VideoSocket)
			UdpSendBuff(m_Socket, cFromIP, RemoteVideoPort, send_b, sendlength);

		//д��ַ����
		memcpy(LocalCfg.Addr, recv_buf + 52, 20);
		memcpy(LocalCfg.Mac_Addr, recv_buf + 72, 6);
		memcpy(LocalCfg.IP, recv_buf + 78, 4);
		memcpy(LocalCfg.IP_Mask, recv_buf + 82, 4);
		memcpy(LocalCfg.IP_Gate, recv_buf + 86, 4);
		memcpy(LocalCfg.IP_Server, recv_buf + 90, 4);

		Save_Setup(); //��Flash�д洢�ļ�
		RefreshNetSetup(1); //ˢ����������
	}
}
//---------------------------------------------------------------------------
//д������Ϣ
void RecvWriteHelp_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket)
{
	int i, j;
	int helplength;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;

	i = 0;
	isAddrOK = 1;
	for (j = 8; j < 8 + Local.AddrLen; j++)
		if (LocalCfg.Addr[j - 8] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	//��ַƥ��
	if (isAddrOK == 1)
	{
		memcpy(send_b, recv_buf, length);
		send_b[7] = REPLY; //Ӧ��
		sendlength = length;
		if (m_Socket == m_DataSocket)
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);
		if (m_Socket == m_VideoSocket)
			UdpSendBuff(m_Socket, cFromIP, RemoteVideoPort, send_b, sendlength);

		helplength = (recv_buf[29] << 8) + recv_buf[28];
		if (helplength > 200)
			helplength = 200;
		memcpy(LocalCfg.HelpInfo, recv_buf + 30, helplength);
		LocalCfg.HelpInfo[helplength] = '\0';

		Save_Setup(); //��Flash�д洢�ļ�
	}
}
//---------------------------------------------------------------------------
//��������Ϣ
void RecvReadHelp_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket)
{
	int i, j;
	short helplength;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	//ʱ��
	time_t t;

	i = 0;
	isAddrOK = 1;
	for (j = 8; j < 8 + Local.AddrLen; j++)
		if (LocalCfg.Addr[j - 8] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	//��ַƥ��
	if (isAddrOK == 1)
	{
		memcpy(send_b, UdpPackageHead, 6);
		send_b[6] = READHELP;
		send_b[7] = REPLY; //Ӧ��
		memcpy(send_b + 8, LocalCfg.Addr, 20);
		helplength = strlen(LocalCfg.HelpInfo);
		if (helplength > 200)
			helplength = 200;
		memcpy(recv_buf + 28, &helplength, sizeof(helplength));
		memcpy(recv_buf + 30, LocalCfg.HelpInfo, helplength);

		sendlength = 28 + helplength;
		if (m_Socket == m_DataSocket)
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);
		if (m_Socket == m_VideoSocket)
			UdpSendBuff(m_Socket, cFromIP, RemoteVideoPort, send_b, sendlength);
	}
}
//---------------------------------------------------------------------------
//д������Ϣ
void RecvWriteSetup_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket)
{
	int i, j;
	int newlength;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;

	i = 0;
	isAddrOK = 1;
	for (j = 8; j < 8 + Local.AddrLen; j++)
		if (LocalCfg.Addr[j - 8] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	//��ַƥ��
	if (isAddrOK == 1)
	{
		memcpy(send_b, recv_buf, length);
		send_b[7] = REPLY; //Ӧ��
		sendlength = length;
		if (m_Socket == m_DataSocket)
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);
		if (m_Socket == m_VideoSocket)
			UdpSendBuff(m_Socket, cFromIP, RemoteVideoPort, send_b, sendlength);

		//д������Ϣ
		if (recv_buf[28] & 0x01) //��������
			LocalCfg.LockType = recv_buf[30];
		if (recv_buf[28] & 0x02) //����ʱ��
			LocalCfg.OpenLockTime = recv_buf[31];

		Save_Setup(); //��Flash�д洢�ļ�
	}
}
//---------------------------------------------------------------------------
//��������Ϣ
void RecvReadSetup_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket)
{
	int i, j;
	int newlength;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;

	i = 0;
	isAddrOK = 1;
	for (j = 8; j < 8 + Local.AddrLen; j++)
		if (LocalCfg.Addr[j - 8] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	//��ַƥ��
	if (isAddrOK == 1)
	{
		memcpy(send_b, recv_buf, length);
		send_b[7] = REPLY; //Ӧ��

		send_b[28] = 0;
		send_b[29] = 0;

		//��������
		send_b[30] = LocalCfg.LockType;
		//����ʱ��
		send_b[31] = LocalCfg.OpenLockTime;

		sendlength = 54;
		if (m_Socket == m_DataSocket)
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);
		if (m_Socket == m_VideoSocket)
			UdpSendBuff(m_Socket, cFromIP, RemoteVideoPort, send_b, sendlength);
	}
}
//---------------------------------------------------------------------------
#ifdef _BRUSHIDCARD_SUPPORT   //ˢ������֧��
//дID����Ϣ
void RecvWriteIDCard_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket)
{
	int i, j;
	int newlength;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
//  uint32_t tmpserialnum;
//  uint32_t tmpcurrnum;
//  uint32_t tmpcurrpack;
	int RecvOk;
	//��дID�����ݽṹ
	struct iddata1 iddata;

	i = 0;
	isAddrOK = 1;
	for (j = 8; j < 8 + Local.AddrLen; j++)
		if (LocalCfg.Addr[j - 8] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	sprintf(Local.DebugInfo, "isAddrOK = %d\n", isAddrOK);
	P_Debug(Local.DebugInfo);
	//��ַƥ��
	if (isAddrOK == 1)
	{
		memcpy(send_b, recv_buf, length);
		send_b[7] = REPLY; //Ӧ��
		sendlength = length;
		if (m_Socket == m_DataSocket)
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);
		if (m_Socket == m_VideoSocket)
			UdpSendBuff(m_Socket, cFromIP, RemoteVideoPort, send_b, sendlength);

		memcpy(&iddata, recv_buf + 8, sizeof(iddata));
		//printf("RecvIDCardNo.isNewWriteFlag = %d, iddata.serialnum = %d, IDCardNo.serialnum = %d\n",
		//        RecvIDCardNo.isNewWriteFlag, iddata.serialnum, IDCardNo.serialnum);

		//�鿴�Ƿ���дID����Ϣ״̬
		if (RecvIDCardNo.isNewWriteFlag == 1)
		{
			//�ϴ�дδ��ɣ��¿�ʼд
			if (iddata.serialnum != RecvIDCardNo.serialnum)
			{
				RecvIDCardNo.isNewWriteFlag = 1;
				//������
				RecvIDCardNo.Num = iddata.Num;
				//�ܰ���
				RecvIDCardNo.PackNum = iddata.TotalPackage;
				//���ѽ��ձ�־
				for (i = 0; i < RecvIDCardNo.PackNum; i++)
					RecvIDCardNo.PackIsRecved[i] = 0;
				//���
				RecvIDCardNo.serialnum = iddata.serialnum;
			}
			//��ǰ���ݰ�������
			if (iddata.CurrNum > CARDPERPACK
			)
				iddata.CurrNum = CARDPERPACK;
			//��ǰ��
			if (iddata.CurrPackage > RecvIDCardNo.PackNum)
				iddata.CurrPackage = RecvIDCardNo.PackNum;
			RecvIDCardNo.PackIsRecved[iddata.CurrPackage - 1] = 1;
			memcpy(
					RecvIDCardNo.SN
							+ CARDPERPACK * (iddata.CurrPackage - 1) * BYTEPERSN
							, recv_buf + 48, BYTEPERSN * iddata.CurrNum);

			//printf("RecvIDCardNo.PackNum = %d, iddata.CurrPackage = %d, iddata.serialnum = %d\n", RecvIDCardNo.PackNum, iddata.CurrPackage, iddata.serialnum);
			//���ѽ��ձ�־���鿴�Ƿ�ȫ������
			RecvOk = 1;
			for (i = 0; i < RecvIDCardNo.PackNum; i++)
			{
				//printf("RecvIDCardNo.PackIsRecved[%d] = %d\n", i, RecvIDCardNo.PackIsRecved[i]);
				if (RecvIDCardNo.PackIsRecved[i] == 0)
				{
					RecvOk = 0;
					break;
				}
			}
			if (RecvOk == 1)
				SaveWriteIDCard_Func();
		}
		else if (iddata.serialnum != IDCardNo.serialnum)
		{
			RecvIDCardNo.isNewWriteFlag = 1;
			//������
			RecvIDCardNo.Num = iddata.Num;
			//�ܰ���
			RecvIDCardNo.PackNum = iddata.TotalPackage;
			//���ѽ��ձ�־
			for (i = 0; i < RecvIDCardNo.PackNum; i++)
				RecvIDCardNo.PackIsRecved[i] = 0;
			//���
			RecvIDCardNo.serialnum = iddata.serialnum;

			RecvIDCardNo.PackIsRecved[iddata.CurrPackage - 1] = 1;
			memcpy(
					RecvIDCardNo.SN
							+ CARDPERPACK * (iddata.CurrPackage - 1) * BYTEPERSN
							, recv_buf + 48, BYTEPERSN * iddata.CurrNum);

			//printf("iddata.CurrPackage = %d\n", iddata.CurrPackage);
			//printf("RecvIDCardNo.PackNum1 = %d\n", RecvIDCardNo.PackNum);
			//���ѽ��ձ�־���鿴�Ƿ�ȫ������
			RecvOk = 1;
			for (i = 0; i < RecvIDCardNo.PackNum; i++)
				if (RecvIDCardNo.PackIsRecved[i] == 0)
				{
					RecvOk = 0;
					break;
				}
			sprintf(Local.DebugInfo,
					"RecvIDCardNo.PackNum = %d, iddata.Num = %d\n",
					RecvIDCardNo.PackNum, iddata.Num);
			P_Debug(Local.DebugInfo);

			//printf("recv iddata ----------------------------------------------------------------\n");
			//for(i=0; i < iddata.Num; i++)
			//  printf("%p  %p  %p  %p\n", RecvIDCardNo.SN[0 + BYTEPERSN*i], RecvIDCardNo.SN[1 + BYTEPERSN*i], RecvIDCardNo.SN[2 + BYTEPERSN*i], RecvIDCardNo.SN[3 + BYTEPERSN*i]);

			if (RecvOk == 1)
				SaveWriteIDCard_Func();
		}

	}
}
//---------------------------------------------------------------------------
//�洢ID����Ϣ
void SaveWriteIDCard_Func(void)
{
	int i;
	IDCardNo.Num = RecvIDCardNo.Num;
	IDCardNo.serialnum = RecvIDCardNo.serialnum;
	memcpy(IDCardNo.SN, RecvIDCardNo.SN, BYTEPERSN * IDCardNo.Num);

//  for(i=0; i<IDCardNo.Num; i++)
//    memcpy(IDCardNo.SN[i], RecvIDCardNo.SN[i], 4);
	RecvIDCardNo.serialnum = 0;

	RecvIDCardNo.isNewWriteFlag = 0;
	//����������
	pthread_mutex_lock(&Local.save_lock);
	//���ҿ��ô洢���岢���
	for (i = 0; i < SAVEMAX; i++)
		if (Save_File_Buff[i].isValid == 0)
		{
			Save_File_Buff[i].Type = 5;
			Save_File_Buff[i].isValid = 1;
			sem_post(&save_file_sem);
			break;
		}
	//�򿪻�����
	pthread_mutex_unlock(&Local.save_lock);

}
//---------------------------------------------------------------------------
//��ID����Ϣ
void RecvReadIDCard_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket)
{
	int i, j;
	int newlength;
	int isAddrOK;
	i = 0;
	isAddrOK = 1;
	for (j = 8; j < 8 + Local.AddrLen; j++)
		if (LocalCfg.Addr[j - 8] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	sprintf(Local.DebugInfo, "isAddrOK = %d\n", isAddrOK);
	P_Debug(Local.DebugInfo);
	//��ַƥ��
	if (isAddrOK == 1)
	{
		//����������
		pthread_mutex_lock(&Local.dispart_send_lock);
		//���ҿ��ô洢���岢���
		for (i = 0; i < DISPARTMAX; i++)
			if (Dispart_Send_Buff[i].isValid == 0)
			{
				Dispart_Send_Buff[i].Type = 10;
				strcpy(Dispart_Send_Buff[i].cFromIP, cFromIP);
				Dispart_Send_Buff[i].isValid = 1;
				break;
			}
		//�򿪻�����
		pthread_mutex_unlock(&Local.dispart_send_lock);
		sem_post(&dispart_send_sem);
	}
}
//---------------------------------------------------------------------------
//ˢ����¼
void RecvBrushIDCard_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket)
{
	int i, j;
	int newlength;
	int isAddrOK;
	unsigned char send_b[1520];
	unsigned char tmpbrushinfo[IDCARDBRUSHNUM * 11];int
	tmpnum;
	int sendlength;

	i = 0;
	isAddrOK = 1;
	for (j = 8; j < 8 + Local.AddrLen; j++)
		if (LocalCfg.Addr[j - 8] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	//��ַƥ��
	if (isAddrOK == 1)
	{
		//����������
		pthread_mutex_lock(&Local.udp_lock);
		if ((recv_buf[7] & 0x03) == REPLY
		) //Ӧ��
			for (i = 0; i < UDPSENDMAX; i++)
				if (Multi_Udp_Buff[i].isValid == 1)
					if (Multi_Udp_Buff[i].m_Socket == m_DataSocket)
						if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM
						)
							if (Multi_Udp_Buff[i].buf[6] == BRUSHIDCARD
							)
								if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK
								)
									if (strcmp(Multi_Udp_Buff[i].RemoteHost,
											cFromIP) == 0)
									{
										tmpnum = (recv_buf[29] << 8)
												+ recv_buf[28];
										if (tmpnum <= BrushIDCard.Num)
										{
											//����������
											pthread_mutex_lock(
													&Local.idcard_lock);
											memcpy(
													tmpbrushinfo,
													BrushIDCard.Info
															+ tmpnum * 11,
													(IDCARDBRUSHNUM - tmpnum)
															* 11);
											memcpy(
													BrushIDCard.Info,
													tmpbrushinfo,
													(IDCARDBRUSHNUM - tmpnum)
															* 11);
											BrushIDCard.Num -= tmpnum;
											//�򿪻�����
											pthread_mutex_unlock(
													&Local.idcard_lock);
										}
										if (BrushIDCard.Num == 0)
											Multi_Udp_Buff[i].isValid = 0;
										else
										{
											//����
											if (BrushIDCard.Num > 120)
												tmpnum = 120;
											else
												tmpnum = BrushIDCard.Num;
											Multi_Udp_Buff[i].buf[29] = (tmpnum
													& 0xFF00) >> 8;
											Multi_Udp_Buff[i].buf[28] = (tmpnum
													& 0x00FF);

											memcpy(Multi_Udp_Buff[i].buf + 30,
													BrushIDCard.Info,
													11 * tmpnum);

											Multi_Udp_Buff[i].nlength = 30
													+ 11 * tmpnum;
											Multi_Udp_Buff[i].DelayTime = 100;
											Multi_Udp_Buff[i].SendDelayTime = 0; //���͵ȴ�ʱ�����  20101112
											Multi_Udp_Buff[i].isValid = 1;
											sem_post(&multi_send_sem);
										}

#ifdef _DEBUG
										P_Debug("�յ�ˢ����¼Ӧ��\n");
#endif
										break;
									}
		//�򿪻�����
		pthread_mutex_unlock(&Local.udp_lock);
	}
}
#endif
//---------------------------------------------------------------------------
//����Ӧ�ó���
void RecvDownLoadFile_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket)
{
	int i;
	unsigned char send_b[8096];
	int PackDataLen = 8000;
	int sendlength;
	FILE *pic_fd;
	DIR *dirp;
	char picname[80];
	char filename[20] = "door26";
	struct downfile1 DownData;
	char systemtext[50];
	int DownOK;
	memcpy(&DownData, recv_buf + 9, sizeof(struct downfile1));
	if ((strcmp(DownData.FlagText, FLAGTEXT) == 0)
			&& (strcmp(DownData.FileName, filename) == 0))
	{
		switch (recv_buf[8])
		{
		case STARTDOWN: //��ʼ����
			memcpy(send_b, recv_buf, length);
			send_b[7] = REPLY; //Ӧ��
			sendlength = length;
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);

			Local.DownProgramOK = 0; //����Ӧ�ó������
			printf("��ʼ����, DownData.Filelen = %d\n", DownData.Filelen);
			if (downbuf == NULL
			)
				downbuf = (unsigned char *) malloc(DownData.Filelen);
			for (i = 0; i < DownData.TotalPackage; i++)
				downloaded_flag[i] = 0;
			break;
		case DOWN: //����
			downloaded_flag[DownData.CurrPackage] = 1;
			memcpy(downbuf + DownData.CurrPackage * PackDataLen,
					recv_buf + 9 + sizeof(struct downfile1), DownData.Datalen);

			/*    if(DownData.CurrPackage != 0)
			 if(DownData.CurrPackage != (OldPackage + 1))
			 printf("CurrPackage = %d, package lost %d, length = %d\n", DownData.CurrPackage , OldPackage + 1, length);
			 if(OldPackage != DownData.CurrPackage)
			 OldPackage = DownData.CurrPackage;    */

			sendlength = 9 + sizeof(struct downfile1);
			memcpy(send_b, recv_buf, sendlength);
			send_b[7] = REPLY; //Ӧ��
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);
			break;
		case DOWNFINISHONE: //�������һ��
			memcpy(send_b, recv_buf, length);
			send_b[7] = REPLY; //Ӧ��
			sendlength = length;
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);

			P_Debug("�������һ��\n");
			DownOK = 1;
			for (i = 0; i < DownData.TotalPackage; i++)
				if (downloaded_flag[i] == 0)
				{
					printf("��ʧ���ݰ���i = %d\n", i);
					DownOK = 0;
				}
			if (DownOK == 1)
			{
				if (Local.DownProgramOK == 0)
				{
					Local.DownProgramOK = 1; //����Ӧ�ó������
					sprintf(picname, "/mnt/mtd/%s\0", DownData.FileName);
					if ((pic_fd = fopen(picname, "wb")) == NULL
					)
						P_Debug("�޷�����Ӧ�ó����ļ�\n");
					else
					{
						fwrite(downbuf, DownData.Filelen, 1, pic_fd);
						fclose(pic_fd);
					}
					usleep(200 * 1000);
					sprintf(systemtext, "chmod 777 %s\0", picname);
					system(systemtext);
					free(downbuf);
					downbuf = NULL;

					usleep(200 * 1000);
					sync();
					usleep(200 * 1000);
					system("reboot");
				}
			}
			else
			{
				free(downbuf);
				downbuf = NULL;
				SendUdpOne(DOWNLOADIMAGE, DOWNFAIL, 0, cFromIP); //ʧ��
			}
			break;
		case STOPDOWN: //ֹͣ����
			memcpy(send_b, recv_buf, length);
			send_b[7] = REPLY; //Ӧ��
			sendlength = length;
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);

			P_Debug("ֹͣ����\n");
			if (downbuf != NULL)
			{
				free(downbuf);
				downbuf = NULL;
			}
			break;
		case DOWNFINISHALL: //ȫ���������
			memcpy(send_b, recv_buf, length);
			send_b[7] = REPLY; //Ӧ��
			sendlength = length;
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);

			P_Debug("�������\n");
			break;
		}
	}
}
//---------------------------------------------------------------------------
//����ϵͳӳ��
void RecvDownLoadImage_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket)
{
	int i;
	pthread_attr_t attr;
	unsigned char send_b[8096];
	int PackDataLen = 8000;
	int sendlength;
	FILE *pic_fd;
	DIR *dirp;
	char picname[80];
	char filename[20] = "mdoor26pImage";
	struct downfile1 DownData;
	char systemtext[50];
	int DownOK;
	memcpy(&DownData, recv_buf + 9, sizeof(struct downfile1));
	if ((strcmp(DownData.FlagText, FLAGTEXT) == 0)
			&& (strcmp(DownData.FileName, filename) == 0))
	{
		switch (recv_buf[8])
		{
		case STARTDOWN: //��ʼ����
			memcpy(send_b, recv_buf, length);
			send_b[7] = REPLY; //Ӧ��
			sendlength = length;
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);

			printf("��ʼ����, DownData.Filelen = %d\n", DownData.Filelen);
			if (downbuf == NULL
			)
				downbuf = (unsigned char *) malloc(DownData.Filelen);
			for (i = 0; i < DownData.TotalPackage; i++)
				downloaded_flag[i] = 0;
			break;
		case DOWN: //����
			downloaded_flag[DownData.CurrPackage] = 1;
			memcpy(downbuf + DownData.CurrPackage * PackDataLen,
					recv_buf + 9 + sizeof(struct downfile1), DownData.Datalen);

			/*if(DownData.CurrPackage != 0)
			 if(DownData.CurrPackage != (OldPackage + 1))
			 printf("CurrPackage = %d, package lost %d, length = %d\n", DownData.CurrPackage , OldPackage + 1, length);
			 if(OldPackage != DownData.CurrPackage)
			 OldPackage = DownData.CurrPackage;  */
			memcpy(send_b, recv_buf, length);
			send_b[7] = REPLY; //Ӧ��
			sendlength = 9 + sizeof(struct downfile1);
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);
			break;
		case DOWNFINISHONE: //�������һ��
			memcpy(send_b, recv_buf, length);
			send_b[7] = REPLY; //Ӧ��
			sendlength = length;
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);

			P_Debug("�������һ��\n");
			DownOK = 1;
			for (i = 0; i < DownData.TotalPackage; i++)
				if (downloaded_flag[i] == 0)
				{
					printf("��ʧ���ݰ���i = %d\n", i);
					DownOK = 0;
				}
			if (DownOK == 1)
			{
				downbuflen = DownData.Filelen;
				strcpy(downip, cFromIP);
				if (Local.download_image_flag == 0)
				{
					Local.download_image_flag = 1;
					pthread_attr_init(&attr);
					pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
					pthread_create(&download_image_thread, &attr,
							(void *) download_image_thread_func, NULL);
					pthread_attr_destroy(&attr);
					if (download_image_thread == 0)
					{
						P_Debug("�޷�ϵͳӳ�������߳�\n");
						free(downbuf);
						downbuf = NULL;
					}
				}
				else
					P_Debug("�ظ�����ϵͳӳ�������߳�\n");
			}
			else
			{
				free(downbuf);
				downbuf = NULL;
				SendUdpOne(DOWNLOADIMAGE, DOWNFAIL, 0, cFromIP); //ʧ��
			}
			break;
		case STOPDOWN: //ֹͣ����
			memcpy(send_b, recv_buf, length);
			send_b[7] = REPLY; //Ӧ��
			sendlength = length;
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);

			P_Debug("ֹͣ����\n");
			if (downbuf != NULL)
			{
				free(downbuf);
				downbuf = NULL;
			}
			break;
		case DOWNFINISHALL: //ȫ���������
			memcpy(send_b, recv_buf, length);
			send_b[7] = REPLY; //Ӧ��
			sendlength = length;
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);

			P_Debug("�������\n");
			break;
		}
	}
}
//---------------------------------------------------------------------------
//ϵͳӳ�������߳�
void download_image_thread_func(void)
{
	FILE *image_fd;
	char SysOrder[100];

	if (DebugMode == 1)
		P_Debug("����ϵͳӳ�������߳�\n");

	if ((image_fd = fopen(mtdimage_name, "wb")) == NULL
	)
		P_Debug("�޷�����ϵͳӳ���ļ�\n");
	else
	{
		fwrite(downbuf, downbuflen, 1, image_fd);
		fclose(image_fd);
	}
	free(downbuf);
	downbuf = NULL;
#if 1
	SendUdpOne(DOWNLOADIMAGE, WRITEFLASH, 20, downip); //����дFlash
	sprintf(SysOrder, "/bin/gm_spi_write -x %s /dev/mtd0\0", mtdimage_name);
	system(SysOrder);
	//flashcp(downbuf, downbuflen, downip);
#else
	system("sync");
	gm_spi_write(mtdimage_name, "/dev/mtd0", downip);
#endif

	Local.download_image_flag = 0;

	if (DebugMode == 1)
		P_Debug("����ϵͳӳ�������߳�\n");

	SendUdpOne(DOWNLOADIMAGE, ENDFLASH, 0, downip); //���дImage

	sprintf(SysOrder, "rm %s\0", mtdexe_name);
	system(SysOrder);

	strcpy(SysOrder, "rm /mnt/mtd/isp_ov9710.cfg\0");
	system(SysOrder);

	strcpy(SysOrder, "rm /mnt/mtd/ipc.cfg\0");
	system(SysOrder);

	strcpy(SysOrder, "rm /mnt/mtd/config_8126\0");
	system(SysOrder);

	strcpy(SysOrder, "rm /mnt/mtd/gmdvr_mem.cfg\0");
	system(SysOrder);

	sprintf(SysOrder, "rm %s\0", mtdimage_name);
	system(SysOrder);

	system("sync");
	system("reboot");

}
//---------------------------------------------------------------------------
void SendUdpOne(unsigned char Order, unsigned char SonOrder,
		unsigned char PerCent, char *cFromIP)
{
	unsigned char send_b[1500];
	int sendlength;
	memcpy(send_b, UdpPackageHead, 6);
	send_b[6] = Order;
	send_b[7] = ASK; //����
	send_b[8] = SonOrder; //������
	memcpy(send_b + 9, FLAGTEXT, 20); //��־
	send_b[29] = PerCent;
	sendlength = 30;
	UdpSendBuff(m_DataSocket, cFromIP, RemoteDataPort, send_b, sendlength);
}
//---------------------------------------------------------------------------
//����Զ�̵�����Ϣ
int SountPlayFlag = 0;
void RecvRemoteDebugInfo_Func(unsigned char *recv_buf, char *cFromIP,
		int length, int m_Socket)
{
	int i;
	unsigned char send_b[8096];
	int sendlength;
	char FlagText[50];
	memcpy(FlagText, recv_buf + 9, 20);
	if (strcmp(FlagText, FLAGTEXT) == 0)
	{
		memcpy(send_b, recv_buf, length);
		send_b[7] = REPLY; //Ӧ��
		sendlength = length;
		UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);
		switch (recv_buf[8])
		{
		case 0: //ֹͣ
			Local.RemoteDebugInfo = 0; //����Զ�̵�����Ϣ
			P_Debug("����Զ�̵�����Ϣ ֹͣ\n");
			break;
		case 1: //��ʼ
			Local.RemoteDebugInfo = 1; //����Զ�̵�����Ϣ
			strcpy(Local.DebugIP, cFromIP);
			sprintf(Local.DebugInfo, "����Զ�̵�����Ϣ ��ʼ, %s, Local.DebugIP = %s\n",
					LocalCfg.Addr, Local.DebugIP);
			P_Debug(Local.DebugInfo);
			break;
		}
	}
}
//---------------------------------------------------------------------------
#ifdef _REMOTECALLTEST  //Զ�̺��в���
//����Զ�̺��в���
void RecvRemoteCallTest_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket)
{
	int i;
	unsigned char send_b[8096];
	int sendlength;
	char RemoteAddr[20];
	char FlagText[50];
	time_t t;
	struct tm *curr_tm_t;
	time(&t);
	curr_tm_t=localtime(&t);
	memcpy(FlagText, recv_buf + 9, 20);
	if(strcmp(FlagText, FLAGTEXT) == 0)
	{
		memcpy(RemoteAddr, recv_buf + 29 + 1, 19);
		RemoteAddr[19] = '\0';
		printf("����Զ�̺��в��� %s, %04d-%02d-%02d %02d:%02d:%02d\n", RemoteAddr,
				curr_tm_t->tm_year + 1900, curr_tm_t->tm_mon + 1, curr_tm_t->tm_mday, curr_tm_t->tm_hour, curr_tm_t->tm_min, curr_tm_t->tm_sec);
		if(Local.Status == 0)
		{
			Call_Func(2, RemoteAddr); //����ס��
		}
	}
}
#endif
//---------------------------------------------------------------------------
#ifdef _CAPTUREPIC_TO_CENTER  //����ͼƬ��������
//��Ԫ�ſڻ���Χǽ���������ſڻ����ͺ�����Ƭ->���Ϳ�ʼ
void RecvCapture_Send_Start_Func(unsigned char *recv_buf, char *cFromIP,
		int length, int m_Socket)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	int Start_OK;
	int CurrNo;
	Start_OK = 0;

	//printf("RecvCapture_Send_Start_Func1 \n");
	i = 0;
	isAddrOK = 1;
	for (j = 8; j < 8 + Local.AddrLen; j++)
		if (LocalCfg.Addr[j - 8] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	//printf("isAddrOK  = %d\n", isAddrOK);
	//��ַƥ��
	if (isAddrOK == 1)
	{
		//����������
		pthread_mutex_lock(&Local.udp_lock);
		if ((recv_buf[7] & 0x03) == REPLY
		) //Ӧ��
			for (i = 0; i < UDPSENDMAX; i++)
				if (Multi_Udp_Buff[i].isValid == 1)
					if (Multi_Udp_Buff[i].m_Socket == m_DataSocket)
						if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM
						)
							if (Multi_Udp_Buff[i].buf[6]
									== CAPTUREPIC_SEND_START
									)
								if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK
								)
									if (strcmp(Multi_Udp_Buff[i].RemoteHost,
											cFromIP) == 0)
									{
										Multi_Udp_Buff[i].isValid = 0;

										Start_OK = 1;
										memcpy(&CurrNo, recv_buf + 48,
												sizeof(CurrNo));

#ifdef _DEBUG
										P_Debug("�յ�������Ƭ->���Ϳ�ʼӦ��\n");
#endif
										break;
									}
		//�򿪻�����
		pthread_mutex_unlock(&Local.udp_lock);
		//printf("Start_OK  = %d\n", Start_OK);
		if (Start_OK == 1)
			SendCapturePicData(CurrNo);
	}
}
//---------------------------------------------------------------------------
//��Ԫ�ſڻ���Χǽ���������ſڻ����ͺ�����Ƭ->��������
void RecvCapture_Send_Data_Func(unsigned char *recv_buf, char *cFromIP,
		int length, int m_Socket)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	int Finish_OK;
	int CurrNo;
	short CurrPack;

	i = 0;
	isAddrOK = 1;
	for (j = 8; j < 8 + Local.AddrLen; j++)
		if (LocalCfg.Addr[j - 8] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	//printf("RecvCapture_Send_Data_Func   isAddrOK = %d\n", isAddrOK);
	//��ַƥ��
	if (isAddrOK == 1)
	{
		//����������
		pthread_mutex_lock(&Local.udp_lock);
		if ((recv_buf[7] & 0x03) == REPLY
		) //Ӧ��
			for (i = 0; i < UDPSENDMAX; i++)
				if (Multi_Udp_Buff[i].isValid == 1)
					if (Multi_Udp_Buff[i].m_Socket == m_DataSocket)
						if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM
						)
							if (Multi_Udp_Buff[i].buf[6] == CAPTUREPIC_SEND_DATA
							)
								if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK
								)
									if ((Multi_Udp_Buff[i].buf[56]
											== recv_buf[56])
											&& (Multi_Udp_Buff[i].buf[57]
													== recv_buf[57]))
										if (strcmp(Multi_Udp_Buff[i].RemoteHost,
												cFromIP) == 0)
										{
											Multi_Udp_Buff[i].isValid = 0;

											memcpy(&CurrNo, recv_buf + 48,
													sizeof(CurrNo));
											memcpy(&CurrPack, recv_buf + 56,
													sizeof(CurrPack));
											if (CurrPack >= 2000)
											{
												printf("CurrPack = %d\n",
														CurrPack);
												CurrPack = 0;
											}
											Capture_Send_Flag[CurrPack] = 1;

#ifdef _DEBUG
											//sprintf(Local.DebugInfo, "�յ�������Ƭ->��������Ӧ��, %d\n", CurrPack);
											//P_Debug(Local.DebugInfo);
#endif
											break;
										}
		//�򿪻�����
		pthread_mutex_unlock(&Local.udp_lock);

		Finish_OK = 1;
		for (j = 0; j < Capture_Total_Package; j++)
			if (Capture_Send_Flag[j] == 0)
			{
				Finish_OK = 0;
				break;
			}
		if (Finish_OK == 1)
			SendCapturePicFinish(CurrNo, 1);
	}
}
//---------------------------------------------------------------------------
//��Ԫ�ſڻ���Χǽ���������ſڻ����ͺ�����Ƭ->���ͽ������ɹ���
void RecvCapture_Send_Succ_Func(unsigned char *recv_buf, char *cFromIP,
		int length, int m_Socket)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	int Start_OK;
	int CurrNo;
	Start_OK = 0;

	i = 0;
	isAddrOK = 1;
	for (j = 8; j < 8 + Local.AddrLen; j++)
		if (LocalCfg.Addr[j - 8] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	//��ַƥ��
	if (isAddrOK == 1)
	{
		//����������
		pthread_mutex_lock(&Local.udp_lock);
		if ((recv_buf[7] & 0x03) == REPLY
		) //Ӧ��
			for (i = 0; i < UDPSENDMAX; i++)
				if (Multi_Udp_Buff[i].isValid == 1)
					if (Multi_Udp_Buff[i].m_Socket == m_DataSocket)
						if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM
						)
							if (Multi_Udp_Buff[i].buf[6] == CAPTUREPIC_SEND_SUCC
							)
								if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK
								)
									if (strcmp(Multi_Udp_Buff[i].RemoteHost,
											cFromIP) == 0)
									{
										Multi_Udp_Buff[i].isValid = 0;

										memcpy(&CurrNo, recv_buf + 48,
												sizeof(CurrNo));
										Capture_Pic_Center[CurrNo].isVilid = 0;
										if (Capture_Pic_Center[CurrNo].jpeg_pic
												!= NULL)
										{
											free(
													Capture_Pic_Center[CurrNo].jpeg_pic);
											Capture_Pic_Center[CurrNo].jpeg_pic =
													NULL;
										}
										Local.SendToCetnering = 0; //���ڴ�������

#ifdef _DEBUG
										P_Debug("�յ�������Ƭ->���ͽ������ɹ���Ӧ��\n");
#endif
										break;
									}
		//�򿪻�����
		pthread_mutex_unlock(&Local.udp_lock);
	}
}
//---------------------------------------------------------------------------
//��Ԫ�ſڻ���Χǽ���������ſڻ����ͺ�����Ƭ->���ͽ�����ʧ�ܣ�
void RecvCapture_Send_Fail_Func(unsigned char *recv_buf, char *cFromIP,
		int length, int m_Socket)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	int Start_OK;
	int CurrNo;
	Start_OK = 0;

	i = 0;
	isAddrOK = 1;
	for (j = 8; j < 8 + Local.AddrLen; j++)
		if (LocalCfg.Addr[j - 8] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	//��ַƥ��
	if (isAddrOK == 1)
	{
		//����������
		pthread_mutex_lock(&Local.udp_lock);
		if ((recv_buf[7] & 0x03) == REPLY
		) //Ӧ��
			for (i = 0; i < UDPSENDMAX; i++)
				if (Multi_Udp_Buff[i].isValid == 1)
					if (Multi_Udp_Buff[i].m_Socket == m_DataSocket)
						if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM
						)
							if (Multi_Udp_Buff[i].buf[6] == CAPTUREPIC_SEND_FAIL
							)
								if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK
								)
									if (strcmp(Multi_Udp_Buff[i].RemoteHost,
											cFromIP) == 0)
									{
										Multi_Udp_Buff[i].isValid = 0;
										Local.SendToCetnering = 0; //���ڴ�������

#ifdef _DEBUG
										P_Debug("�յ�������Ƭ->���ͽ�����ʧ�ܣ�Ӧ��\n");
#endif
										break;
									}
		//�򿪻�����
		pthread_mutex_unlock(&Local.udp_lock);
	}
}
#endif
//---------------------------------------------------------------------------
void UdpVideoRcvThread(void) //UDP�����̺߳���
{
	/* ѭ���������� */
//  int oldframeno=0;
	unsigned char send_b[1520];
	int sendlength;
	char FromIP[20];
	int newframeno;
	int currpackage;
	int i, j;
	int sub;
	short PackIsExist; //���ݰ��ѽ��ձ�־
	short FrameIsNew; //���ݰ��Ƿ�����֡�Ŀ�ʼ
	struct sockaddr_in c_addr;
	socklen_t addr_len;
	int len;
	int tmp;
	unsigned char buff[8096];

	int isAddrOK;
	if (DebugMode == 1)
		P_Debug("This is udp video pthread.\n");
	UdpRecvFlag = 1;

	addr_len = sizeof(c_addr);
	while (UdpRecvFlag == 1)
	{
		len = recvfrom(m_VideoSocket, buff, sizeof(buff) - 1, 0,
				(struct sockaddr *) &c_addr, &addr_len);
		if (len < 0)
		{
			perror("recvfrom");
			exit(errno);
		}
		buff[len] = '\0';
//    printf("�յ�����%s:%d����Ϣ:%s\n\r",
//            inet_ntoa(c_addr.sin_addr), ntohs(c_addr.sin_port), buff);
		strcpy(FromIP, inet_ntoa(c_addr.sin_addr));
		//if(DebugMode == 1)
		//   printf("FromIP is %s\n",FromIP);
		if ((buff[0] == UdpPackageHead[0]) && (buff[1] == UdpPackageHead[1])
				&& (buff[2] == UdpPackageHead[2])
				&& (buff[3] == UdpPackageHead[3])
				&& (buff[4] == UdpPackageHead[4])
				&& (buff[5] == UdpPackageHead[5]))
		{
			switch (buff[6])
			{
			case NSORDER: //�����������������ڹ㲥��
				//    case NSSERVERORDER:  //����������(NS������)
				switch (buff[7])
				{
				case 1: //����
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
					if(len >= 56)
#endif
					{
						RecvNSAsk_Func(buff, FromIP, m_VideoSocket);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("�����������ݳ����쳣\n");
					}
					break;
				case 2: //������Ӧ
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
					if(len >= 57)
#endif
					{
						RecvNSReply_Func(buff, FromIP, m_VideoSocket);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("����Ӧ�����ݳ����쳣\n");
					}
					break;
				}
				break;
			case VIDEOTALK: //���������ӶԽ�
			case VIDEOTALKTRANS: //���������ӶԽ���ת����
				switch (buff[8])
				{
#ifdef _MULTI_MACHINE_SUPPORT  //��ֻ�֧��
				case JOINGROUP: //�����鲥�飨���з�->���з������з�Ӧ��
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
					if(len >= 61)
#endif
					{
						RecvTalkJoinGroup_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("�Խ������鲥�����ݳ����쳣\n");
					}
					break;
				case LEAVEGROUP: //�˳��鲥�飨���з�->���з������з�Ӧ��
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
					if(len >= 61)
#endif
					{
						RecvTalkLeaveGroup_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("�Խ��˳��鲥�����ݳ����쳣\n");
					}
					break;
#endif
				case CALL: //�Է�����Խ�
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
					if(len >= 62)
#endif
					{
						RecvTalkCall_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("�Խ��������ݳ����쳣\n");
					}
					break;
				case LINEUSE: //�Է���æ
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
					if(len == 57)
#endif
					{
						RecvTalkLineUse_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("ռ��Ӧ�����ݳ����쳣\n");
					}
					break;
				case CALLANSWER: //����Ӧ��

#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
					if(len == 62)
#endif
					{
						RecvTalkCallAnswer_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("�Խ�����Ӧ�����ݳ����쳣\n");
					}
					break;
				case CALLSTART: //���з���ʼͨ��
					//printf("FromIP is %s\n",FromIP);
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
					if(len == 57)
#endif
					{
						RecvTalkCallStart_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("�Խ���ʼͨ�����ݳ����쳣\n");
					}
					break;
				case CALLCONFIRM: //ͨ������ȷ��
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
					if(len == 61)
#endif
					{
						RecvTalkCallConfirm_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("�Խ�ͨ������ȷ�����ݳ����쳣\n");
					}
					break;
				case REMOTEOPENLOCK: //���з�Զ�̿���
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
					if(len == 57)
#endif
					{
						RecvTalkOpenLock_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("Զ�̿������ݳ����쳣\n");
					}
					break;
				case CALLEND: //ͨ������
					//����������Ƶ
					//��Ϊ�Է���������Ӧ��
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
					if(len == 57)
#endif
					{
						RecvTalkCallEnd_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("�����Խ����ݳ����쳣\n");
					}
					break;
				case FORCEIFRAME: //ǿ��I֡����
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
					if(len == 57)
#endif
					{
						RecvForceIFrame_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("ǿ��I֡�������ݳ����쳣\n");
					}
					break;
				case ZOOMOUT: //�Ŵ�(720*480)
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
					if(len == 57)
#endif
					{
						RecvTalkZoomOut_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("�Ŵ�(720*480)���ݳ����쳣");
					}
					break;
				case ZOOMIN: //��С(352*240)
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
					if(len == 57)
#endif
					{
						RecvTalkZoomIn_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("��С(352*240)���ݳ����쳣");
					}
					break;
				case CALLUP: //ͨ������
				case CALLDOWN: //ͨ������
					RecvTalkCallUpDown_Func(buff, FromIP, len);
					break;
#ifdef _MULTI_MACHINE_SUPPORT  //��ֻ�֧��
				case TURNTALK: //ת�ӣ����з�->���з������з�Ӧ��
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
					if(len == 81)
#endif
					{
						RecvTalkTurnTalk_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("ת�����ݳ����쳣\n");
					}
					break;
#endif
				}
				break;
			case VIDEOWATCH: //���������
			case VIDEOWATCHTRANS: //�����������ת����
				switch (buff[8])
				{
				case CALL: //�Է��������
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
					if(len == 62)
#endif
					{
						RecvWatchCall_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("���Ӻ������ݳ����쳣\n");
					}
					break;
				case LINEUSE: //�Է���æ
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
					if(len == 57)
#endif
					{
						RecvWatchLineUse_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("ռ��Ӧ�����ݳ����쳣\n");
					}
					break;
				case CALLANSWER: //����Ӧ��
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
					if(len == 57)
#endif
					{
						RecvWatchCallAnswer_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("���Ӻ���Ӧ�����ݳ����쳣\n");
					}
					break;
				case CALLCONFIRM: //ͨ������ȷ��
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
					if(len == 61)
#endif
					{
						RecvWatchCallConfirm_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("����ͨ������ȷ�����ݳ����쳣\n");
					}
					break;
				case CALLEND: //ͨ������
					//����������Ƶ
					//��Ϊ�Է���������Ӧ��
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
					if(len == 57)
#endif
					{
						RecvWatchCallEnd_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("�����������ݳ����쳣\n");
					}
					break;
				case FORCEIFRAME: //ǿ��I֡����
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
					if(len == 57)
#endif
					{
						RecvForceIFrame_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("ǿ��I֡�������ݳ����쳣\n");
					}
					break;
				case ZOOMOUT: //�Ŵ�(720*480)
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
					if(len == 57)
#endif
					{
						RecvWatchZoomOut_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("�Ŵ�(720*480)���ݳ����쳣");
					}
					break;
				case ZOOMIN: //��С(352*240)
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
					if(len == 57)
#endif
					{
						RecvWatchZoomIn_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("��С(352*240)���ݳ����쳣");
					}
					break;
				case CALLUP: //ͨ������
				case CALLDOWN: //ͨ������
					RecvWatchCallUpDown_Func(buff, FromIP, len);
					break;
				}
				break;
			}
		}

		if (strcmp(buff, "exit") == 0)
		{
			P_Debug("recvfrom888888888\n");
			UdpRecvFlag = 0;
			//   break;
		}
	}
}
//-----------------------------------------------------------------------
//��������
void RecvNSAsk_Func(unsigned char *recv_buf, char *cFromIP, int m_Socket)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	isAddrOK = 1;

	for (j = 8; j < 8 + Local.AddrLen; j++)
		if (LocalCfg.Addr[j - 8] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}

	//sprintf(Local.DebugInfo, "cFromIP = %s, isAddrOK = %d, Local.AddrLen = %d, LocalCfg.Addr = %s\n", cFromIP, isAddrOK, Local.AddrLen, LocalCfg.Addr);
	//P_Debug(Local.DebugInfo);
	//���Ǳ�������
	if (isAddrOK == 0)
	{
		isAddrOK = 1;
		for (j = 32; j < 32 + Local.AddrLen; j++)
			if (LocalCfg.Addr[j - 32] != recv_buf[j])
			{
				isAddrOK = 0;
				break;
			}
		//Ҫ��������Ǳ�����ַ
		if (isAddrOK == 1)
		{
			//   if((LocalCfg.Addr[0] == 'S')||(LocalCfg.Addr[0] == 'B'))

			Local.DenNum = 0; //��������
			memcpy(send_b, recv_buf, 32);
			send_b[7] = REPLY; //Ӧ��

			send_b[32] = Local.DenNum + 1; //��ַ����

			memcpy(send_b + 33, LocalCfg.Addr, 20);
			memcpy(send_b + 53, LocalCfg.IP, 4);
			for (i = 0; i < Local.DenNum; i++)
			{
				memcpy(send_b + 57 + 24 * i, Local.DenAddr[i], 20);
				memcpy(send_b + 57 + 20 + 24 * i, Local.DenIP[i], 4);
			}
			sendlength = 57 + 24 * Local.DenNum;
			if (m_Socket == m_DataSocket)
				UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b,
						sendlength);
			if (m_Socket == m_VideoSocket)
				UdpSendBuff(m_Socket, cFromIP, RemoteVideoPort, send_b,
						sendlength);
		}
	}
}
//-----------------------------------------------------------------------
//����Ӧ��
void RecvNSReply_Func(unsigned char *recv_buf, char *cFromIP, int m_Socket)
{
	int i, j, k;
	int CurrOrder;
	int isAddrOK;

	//��ʱ�رձ��ؽ������������������
	if (Local._TESTNSSERVER == 1)
		if (m_Socket == m_VideoSocket)
			return;

	//����������
	pthread_mutex_lock(&Local.udp_lock);

	printf("RecvNSReply_Func 1\n");
	for (i = 0; i < UDPSENDMAX; i++)
		if (Multi_Udp_Buff[i].isValid == 1)
			//  if(Multi_Udp_Buff[i].m_Socket == m_VideoSocket)
			if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM)
				if ((Multi_Udp_Buff[i].buf[6] == NSORDER)
						|| (Multi_Udp_Buff[i].buf[6] == NSSERVERORDER))
					if ((Multi_Udp_Buff[i].buf[7] == ASK) && (recv_buf[32] > 0))
					{
						//�жϱ�����ַ�Ƿ�ƥ��
						isAddrOK = 1;
						for (j = 32; j < 32 + Local.AddrLen; j++)
							if (Multi_Udp_Buff[i].buf[j] != recv_buf[j + 1])
							{
								isAddrOK = 0;
								break;
							}
						printf(
								"RecvNSReply_Func isAddrOK = %d, NSReplyFlag = %d\n",
								isAddrOK, Local.NSReplyFlag);
						if (isAddrOK == 1)
						{
							CurrOrder = Multi_Udp_Buff[i].CurrOrder;
							Multi_Udp_Buff[i].isValid = 0;
							Multi_Udp_Buff[i].SendNum = 0;
							break;
						}
					}
	//�򿪻�����
	pthread_mutex_unlock(&Local.udp_lock);

	if ((isAddrOK == 1) && (Local.NSReplyFlag == 0))
	{ //�յ���ȷ�Ľ�����Ӧ
		Local.NSReplyFlag = 1; //NSӦ�����־
		if (CurrOrder == 255) //�����򸱻�����
		{
			// j = recv_buf[33+11] - '0';
			// if(j > Local.DenNum)
			//   Local.DenNum = j;
			// memcpy(Local.DenIP[j-1], recv_buf + 53, 4);
			// memcpy(Local.DenAddr[j-1], recv_buf + 33, 20);
		}
		else
		{
			Remote.DenNum = recv_buf[32];
			if ((Remote.DenNum >= 1) && (Remote.DenNum <= 10))
			{
				//           if(Remote.DenNum == 1) //����
				{
					for (j = 0; j < Remote.DenNum; j++)
					{
						Remote.IP[j][0] = recv_buf[53 + 24 * j];
						Remote.IP[j][1] = recv_buf[54 + 24 * j];
						Remote.IP[j][2] = recv_buf[55 + 24 * j];
						Remote.IP[j][3] = recv_buf[56 + 24 * j];
						for (k = 0; k < 20; k++)
							Remote.Addr[j][k] = recv_buf[33 + 24 * j + k];
						Remote.GroupIP[0] = 236;
						Remote.GroupIP[1] = LocalCfg.IP[1];
						Remote.GroupIP[2] = LocalCfg.IP[2];
						Remote.GroupIP[3] = LocalCfg.IP[3];

						//����������
						pthread_mutex_lock(&Local.udp_lock);

						for (i = 0; i < UDPSENDMAX; i++)
							if (Multi_Udp_Buff[i].isValid == 0)
							{
								Multi_Udp_Buff[i].SendNum = 0;
								Multi_Udp_Buff[i].m_Socket = m_VideoSocket;
								Multi_Udp_Buff[i].RemotePort = RemoteVideoPort;
								sprintf(Multi_Udp_Buff[i].RemoteHost,
										"%d.%d.%d.%d\0", Remote.IP[j][0],
										Remote.IP[j][1], Remote.IP[j][2],
										Remote.IP[j][3]);
								sprintf(Multi_Udp_Buff[i].RemoteIP,
										"%d.%d.%d.%d\0", Remote.IP[j][0],
										Remote.IP[j][1], Remote.IP[j][2],
										Remote.IP[j][3]);
								if (DebugMode == 1)
								{
									sprintf(Local.DebugInfo,
											"������ַ�ɹ�,���ں���, Multi_Udp_Buff[i].RemoteHost is %s\n",
											Multi_Udp_Buff[i].RemoteHost);
									P_Debug(Local.DebugInfo);
								}
								//ͷ��
								memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead,6);
								//����
								Multi_Udp_Buff[i].buf[6] = CurrOrder;
								Multi_Udp_Buff[i].buf[7] = ASK; //����
								// ������
								Multi_Udp_Buff[i].buf[8] = CALL;

								memcpy(Multi_Udp_Buff[i].buf + 9, LocalCfg.Addr,20);
								memcpy(Multi_Udp_Buff[i].buf + 29, LocalCfg.IP,4);
								memcpy(Multi_Udp_Buff[i].buf + 33,Remote.Addr[j], 20);
								memcpy(Multi_Udp_Buff[i].buf + 53, Remote.IP[j],4);
								if (Remote.DenNum == 1)
									Multi_Udp_Buff[i].buf[57] = 0; //����
								else
									Multi_Udp_Buff[i].buf[57] = 1; //�鲥
								//�鲥��ַ
								Multi_Udp_Buff[i].buf[58] = Remote.GroupIP[0];
								Multi_Udp_Buff[i].buf[59] = Remote.GroupIP[1];
								Multi_Udp_Buff[i].buf[60] = Remote.GroupIP[2];
								Multi_Udp_Buff[i].buf[61] = Remote.GroupIP[3];

								Multi_Udp_Buff[i].nlength = 62;
								Multi_Udp_Buff[i].DelayTime = DIRECTCALLTIME;
								Multi_Udp_Buff[i].SendDelayTime = 0; //���͵ȴ�ʱ�����  20101112
								Multi_Udp_Buff[i].isValid = 1;
								sem_post(&multi_send_sem);
								break;
							}
						//�򿪻�����
						pthread_mutex_unlock(&Local.udp_lock);
					}
				}
			}
		}
	}
}
//-----------------------------------------------------------------------
//���Ӻ���
void RecvWatchCall_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;
	uint32_t Ip_Int;

	if (Local._TESTTRANS == 1)
		if (recv_buf[6] == VIDEOWATCH
		)
			return;

	//�鿴Ŀ���ַ�Ƿ��Ǳ���
	isAddrOK = 1;
	for (j = 33; j < 33 + Local.AddrLen; j++)
		if (LocalCfg.Addr[j - 33] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	if (isAddrOK == 0)
		return;

	//����״̬Ϊ����
	if (Local.Status == 0)
	{
		Ip_Int = inet_addr(cFromIP);
		memcpy(Remote.DenIP, &Ip_Int, 4);
		printf("Remote.DenIP, %d.%d.%d.%d\0", Remote.DenIP[0], Remote.DenIP[1],
				Remote.DenIP[2], Remote.DenIP[3]);
		//��ȡ�Է���ַ
		memcpy(Remote.Addr[0], recv_buf + 9, 20);
		memcpy(Remote.IP[0], recv_buf + 29, 4);
		printf("Remote.IP[0], %d.%d.%d.%d\0", Remote.IP[0][0], Remote.IP[0][1],
				Remote.IP[0][2], Remote.IP[0][3]);
		Remote.DenNum = 1;

		if (recv_buf[6] == VIDEOWATCH)
		{
			Remote.isDirect = 0;
			RemotePort = RemoteVideoPort;
			if (DebugMode == 1)
				P_Debug("�Է�����ֱͨ���Ӻ���\n");
		}
		else
		{
			Remote.isDirect = 1;
			RemotePort = RemoteVideoServerPort;
			if (DebugMode == 1)
				P_Debug("�Է�������ת���Ӻ���\n");
		}

		memcpy(send_b, recv_buf, 57);
		send_b[7] = ASK; //����
		send_b[8] = CALLANSWER; //����Ӧ��
		sendlength = 57;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);

		Local.Status = 4; //״̬Ϊ������
		//��ʼ¼����Ƶ
		StartRecVideo(CIF_W, CIF_H);
		Local.CallConfirmFlag = 1; //�������߱�־
		Local.Timer1Num = 0;
		Local.TimeOut = 0; //���ӳ�ʱ,  ͨ����ʱ,  ���г�ʱ�����˽���
		Local.OnlineNum = 0; //����ȷ�����
		Local.OnlineFlag = 1;

		DisplayWatchWindow(0);

		//���ڼ�����
	}
	//����Ϊæ
	else
	{
		if (recv_buf[6] == VIDEOWATCH)
		{
			RemotePort = RemoteVideoPort;
		}
		else
		{
			RemotePort = RemoteVideoServerPort;
		}
		memcpy(send_b, recv_buf, 57);
		send_b[7] = ASK; //����
		send_b[8] = LINEUSE; //ռ��Ӧ��
		sendlength = 57;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);
		if (DebugMode == 1)
			P_Debug("�Է�������Ӻ���\n");
	}
}
//-----------------------------------------------------------------------
//����ռ��Ӧ��
void RecvWatchLineUse_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i, j;
	int isAddrOK;

	//����������
	pthread_mutex_lock(&Local.udp_lock);
	if ((recv_buf[7] & 0x03) == ASK
	) //Ӧ��
		for (i = 0; i < UDPSENDMAX; i++)
			if (Multi_Udp_Buff[i].isValid == 1)
				if (Multi_Udp_Buff[i].m_Socket == m_VideoSocket)
					if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM
					)
						if ((Multi_Udp_Buff[i].buf[6] == VIDEOWATCH)
								|| (Multi_Udp_Buff[i].buf[6] == VIDEOWATCHTRANS))
							if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK
							)
								if (Multi_Udp_Buff[i].buf[8] == CALL
								)
									if (strcmp(Multi_Udp_Buff[i].RemoteHost,
											cFromIP) == 0)
									{
										Multi_Udp_Buff[i].isValid = 0;
										Local.Status = 0; //״̬��Ϊ����
										if (DebugMode == 1)
											P_Debug("�յ�����ռ��Ӧ��\n");
										break;
									}
	//�򿪻�����
	pthread_mutex_unlock(&Local.udp_lock);
}
//-----------------------------------------------------------------------
//���Ӻ���Ӧ��
void RecvWatchCallAnswer_Func(unsigned char *recv_buf, char *cFromIP)
{
}
//-----------------------------------------------------------------------
//��������ȷ��
void RecvWatchCallConfirm_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;

	//��������    ������
	if ((Local.Status == 4) && ((recv_buf[7] & 0x03) == ASK))
	{
		memcpy(send_b, recv_buf, 61);
		send_b[7] = REPLY; //Ӧ��
		sendlength = 61;
		RemotePort = RemoteVideoPort;
		if (recv_buf[6] == VIDEOWATCH
		) //ֱͨ����
			RemotePort = RemoteVideoPort;
		else
			//��ת����
			RemotePort = RemoteVideoServerPort;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);
		Local.CallConfirmFlag = 1;
		//if(DebugMode == 1)
		//printf("�յ���������ȷ��\n");
	}
	else //��������
	if (Local.Status == 3)
	{
		Local.CallConfirmFlag = 1;
		if (DebugMode == 1)
			P_Debug("�յ��Է�Ӧ�𱾻���������ȷ��\n");
	}
}
//-----------------------------------------------------------------------
//���Ӻ��н���
void RecvWatchCallEnd_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;

	//��������    ������
	if ((Local.Status == 4) && ((recv_buf[7] & 0x03) == ASK))
	{
		Local.OnlineFlag = 0;
		Local.CallConfirmFlag = 0; //�������߱�־
		memcpy(send_b, recv_buf, 57);
		send_b[7] = REPLY; //Ӧ��
		sendlength = 57;
		RemotePort = RemoteVideoPort;
		if (recv_buf[6] == VIDEOWATCH
		) //ֱͨ����
			RemotePort = RemoteVideoPort;
		else
			//��ת����
			RemotePort = RemoteVideoServerPort;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);

		StopRecVideo();
		if (Local.CurrentWindow == 4)
		{
			//��ʱ����ʾ��Ϣ��־
			PicStatBuf.Type = 4;
			PicStatBuf.Time = 0;
			PicStatBuf.Flag = 1;
		}
		//���ӽ���
		if (DebugMode == 1)
			P_Debug("�Է���������\n");
	}
	else //��������
	if ((recv_buf[7] & 0x03) == REPLY)
	{
		Local.OnlineFlag = 0;
		Local.CallConfirmFlag = 0; //�������߱�־

		//����������
		pthread_mutex_lock(&Local.udp_lock);
		//��������
		if ((recv_buf[7] & 0x03) == REPLY
		)
			for (i = 0; i < UDPSENDMAX; i++)
				if (Multi_Udp_Buff[i].isValid == 1)
					if (Multi_Udp_Buff[i].m_Socket == m_VideoSocket)
						if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM
						)
							if (Multi_Udp_Buff[i].buf[6] == VIDEOWATCH
							)
								if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK
								)
									if (Multi_Udp_Buff[i].buf[8] == CALLEND
									)
										if (strcmp(Multi_Udp_Buff[i].RemoteHost,
												cFromIP) == 0)
										{
											Multi_Udp_Buff[i].isValid = 0;
											Local.Status = 0; //״̬Ϊ����
											if (DebugMode == 1)
												P_Debug("�Է�Ӧ�𱾻���������\n");
											break;
										}
		//�򿪻�����
		pthread_mutex_unlock(&Local.udp_lock);
	}
}
//-----------------------------------------------------------------------
//�Ŵ�(720*480)
void RecvWatchZoomOut_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;

	//��������    ������
	if ((Local.Status == 4) && ((recv_buf[7] & 0x03) == ASK))
	{
		memcpy(send_b, recv_buf, 57);
		send_b[7] = REPLY; //Ӧ��
		sendlength = 57;
		RemotePort = RemoteVideoPort;
		if (recv_buf[6] == VIDEOWATCH
		) //ֱͨ����
			RemotePort = RemoteVideoPort;
		else
			//��ת����
			RemotePort = RemoteVideoServerPort;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);
		if (Local.RecPicSize == 1)
		{
			StopRecVideo(); //352*240
			StartRecVideo(D1_W, D1_H); //720*480
		}
		if (DebugMode == 1)
			P_Debug("�Է��Ŵ�ͼ��\n");
	}
	else //��������
	if (Local.Status == 3)
	{
		//����������
		pthread_mutex_lock(&Local.udp_lock);
		//��������
		if ((recv_buf[7] & 0x03) == REPLY
		)
			for (i = 0; i < UDPSENDMAX; i++)
				if (Multi_Udp_Buff[i].isValid == 1)
					if (Multi_Udp_Buff[i].m_Socket == m_VideoSocket)
						if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM
						)
							if ((Multi_Udp_Buff[i].buf[6] == VIDEOWATCH)
									|| (Multi_Udp_Buff[i].buf[6]
											== VIDEOWATCHTRANS))
								if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK
								)
									if (Multi_Udp_Buff[i].buf[8] == ZOOMOUT
									)
										if (strcmp(Multi_Udp_Buff[i].RemoteHost,
												cFromIP) == 0)
										{
											Multi_Udp_Buff[i].isValid = 0;
											if (DebugMode == 1)
												P_Debug("�Է�Ӧ�𱾻��Ŵ�ͼ��\n");
											break;
										}
		//�򿪻�����
		pthread_mutex_unlock(&Local.udp_lock);
	}
}
//-----------------------------------------------------------------------
//��С(352*240)
void RecvWatchZoomIn_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;

	//��������    ������
	if ((Local.Status == 4) && ((recv_buf[7] & 0x03) == ASK))
	{
		memcpy(send_b, recv_buf, 57);
		send_b[7] = REPLY; //Ӧ��
		sendlength = 57;
		RemotePort = RemoteVideoPort;
		if (recv_buf[6] == VIDEOWATCH
		) //ֱͨ����
			RemotePort = RemoteVideoPort;
		else
			//��ת����
			RemotePort = RemoteVideoServerPort;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);
		if (Local.RecPicSize == 2)
		{
			StopRecVideo(); //720*480
			StartRecVideo(CIF_W, CIF_H); //352*288
		}
		if (DebugMode == 1)
			P_Debug("�Է���Сͼ��\n");
	}
	else //��������
	if (Local.Status == 3)
	{
		//����������
		pthread_mutex_lock(&Local.udp_lock);
		//��������
		if ((recv_buf[7] & 0x03) == REPLY
		)
			for (i = 0; i < UDPSENDMAX; i++)
				if (Multi_Udp_Buff[i].isValid == 1)
					if (Multi_Udp_Buff[i].m_Socket == m_VideoSocket)
						if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM
						)
							if ((Multi_Udp_Buff[i].buf[6] == VIDEOWATCH)
									|| (Multi_Udp_Buff[i].buf[6]
											== VIDEOWATCHTRANS))
								if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK
								)
									if (Multi_Udp_Buff[i].buf[8] == ZOOMIN
									)
										if (strcmp(Multi_Udp_Buff[i].RemoteHost,
												cFromIP) == 0)
										{
											Multi_Udp_Buff[i].isValid = 0;
											if (DebugMode == 1)
												P_Debug("�Է�Ӧ�𱾻���Сͼ��\n");
											break;
										}
		//�򿪻�����
		pthread_mutex_unlock(&Local.udp_lock);
	}
}
//-----------------------------------------------------------------------
//��������
void RecvWatchCallUpDown_Func(unsigned char *recv_buf, char *cFromIP,
		int length)
{
}
//-----------------------------------------------------------------------
#ifdef _MULTI_MACHINE_SUPPORT  //��ֻ�֧��
//�����鲥�飨���з�->���з������з�Ӧ��
void RecvTalkJoinGroup_Func(unsigned char *recv_buf, char *cFromIP)
{
}
//-----------------------------------------------------------------------
//�˳��鲥�飨���з�->���з������з�Ӧ��
void RecvTalkLeaveGroup_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;

	//����Ϊ���з� ���жԽ�
	if ((Local.Status == 2) && ((recv_buf[7] & 0x03) == ASK))
	{
		memcpy(send_b, recv_buf, 61);
		send_b[7] = REPLY; //Ӧ��
		sendlength = 61;
		RemotePort = RemoteVideoPort;
		if (recv_buf[6] == VIDEOTALK
		) //ֱͨ����
			RemotePort = RemoteVideoPort;
		else
			//��ת����
			RemotePort = RemoteVideoServerPort;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);
		//�뿪�鲥��
		DropMultiGroup(m_VideoSocket, NULL);

		TalkEnd_ClearStatus();

		sprintf(Local.DebugInfo, "�Է�Ҫ���뿪�鲥��, %s\n", cFromIP);
		P_Debug(Local.DebugInfo);
	}
	else //����Ϊ���з� ����
	if ((recv_buf[7] & 0x03) == REPLY)
	{
		//����������
		pthread_mutex_lock(&Local.udp_lock);
		//��������
		if ((recv_buf[7] & 0x03) == REPLY
		)
			for (i = 0; i < UDPSENDMAX; i++)
				if (Multi_Udp_Buff[i].isValid == 1)
					if (Multi_Udp_Buff[i].m_Socket == m_VideoSocket)
						if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM
						)
							if ((Multi_Udp_Buff[i].buf[6] == VIDEOTALK)
									|| (Multi_Udp_Buff[i].buf[6]
											== VIDEOTALKTRANS))
								if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK
								)
									if (Multi_Udp_Buff[i].buf[8] == LEAVEGROUP
									)
										if (strcmp(Multi_Udp_Buff[i].RemoteHost,
												cFromIP) == 0)
										{
											Multi_Udp_Buff[i].isValid = 0;
											sprintf(Local.DebugInfo,
													"�Է�Ӧ�𱾻��뿪�鲥��, %s\n",
													cFromIP);
											P_Debug(Local.DebugInfo);
											break;
										}
		//�򿪻�����
		pthread_mutex_unlock(&Local.udp_lock);
	}
}
//-----------------------------------------------------------------------
//ת�ӣ����з�->���з������з�Ӧ��
void RecvTalkTurnTalk_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i;
	int isAddrOK;
	int sendlength;
	unsigned char send_b[1520];
	int RemotePort;
	char tmp_text[20];

	if (recv_buf[7] != ASK
	)
		return;

	if ((Local.Status == 5) || (Local.Status == 6))
	{
		//�жϱ�����ַ�Ƿ�ƥ��
		isAddrOK = 1;
		for (i = 9; i < 9 + Local.AddrLen; i++)
			if (LocalCfg.Addr[i - 9] != recv_buf[i])
			{
				isAddrOK = 0;
				break;
			}
		memcpy(tmp_text, recv_buf + 9, Local.AddrLen);
		sprintf(
				Local.DebugInfo,
				"RecvTalkTurnTalk_Func Local.AddrLen = %d, isAddrOK = %d, tmp_text = %s\n",
				Local.AddrLen, isAddrOK, tmp_text);
		P_Debug(Local.DebugInfo);
		if (isAddrOK == 1)
		{
			memcpy(send_b, recv_buf, 81);
			send_b[7] = REPLY; //Ӧ��
			sendlength = 81;
			if (Remote.isDirect == 0) //ֱͨ����
			{
				memcpy(Remote.DenIP, recv_buf + 77, 4); //ֱͨ������Ŀ���ַ
				RemotePort = RemoteVideoPort;
			}
			else //��ת����
			{ //��ת��������Ŀ���ַ
				RemotePort = RemoteVideoServerPort;
			}
			UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);

			Remote.DenNum = 1;
			memcpy(Remote.IP[0], recv_buf + 77, 4);
			memcpy(Remote.Addr[0], recv_buf + 57, 20);

			TalkTurnOn_Status();

			//����������
			pthread_mutex_lock(&Local.udp_lock);

			for (i = 0; i < UDPSENDMAX; i++)
				if (Multi_Udp_Buff[i].isValid == 0)
				{
					Multi_Udp_Buff[i].SendNum = 0;
					Multi_Udp_Buff[i].m_Socket = m_VideoSocket;
					Multi_Udp_Buff[i].RemotePort = RemotePort;
					sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0",
							Remote.DenIP[0], Remote.DenIP[1], Remote.DenIP[2],
							Remote.DenIP[3]);
					sprintf(Multi_Udp_Buff[i].RemoteIP, "%d.%d.%d.%d\0",
							Remote.IP[0][0], Remote.IP[0][1], Remote.IP[0][2],
							Remote.IP[0][3]);
					if (DebugMode == 1)
						printf("ת��,���ں��� %d.%d.%d.%d\n", Remote.DenIP[0],
								Remote.DenIP[1], Remote.DenIP[2],
								Remote.DenIP[3]);
					//ͷ��
					memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
					//����
					if (Remote.isDirect == 0) //ֱͨ����
						Multi_Udp_Buff[i].buf[6] = VIDEOTALK;
					else
						//��ת����
						Multi_Udp_Buff[i].buf[6] = VIDEOTALKTRANS;
					Multi_Udp_Buff[i].buf[7] = ASK; //����
					// ������
					Multi_Udp_Buff[i].buf[8] = CALL;

					memcpy(Multi_Udp_Buff[i].buf + 9, LocalCfg.Addr, 20);
					memcpy(Multi_Udp_Buff[i].buf + 29, LocalCfg.IP, 4);
					memcpy(Multi_Udp_Buff[i].buf + 33, Remote.Addr[0], 20);
					memcpy(Multi_Udp_Buff[i].buf + 53, Remote.IP[0], 4);
					if (Remote.DenNum == 1)
						Multi_Udp_Buff[i].buf[57] = 0; //����
					else
						Multi_Udp_Buff[i].buf[57] = 1; //�鲥
					//�鲥��ַ
					Multi_Udp_Buff[i].buf[58] = Remote.GroupIP[0];
					Multi_Udp_Buff[i].buf[59] = Remote.GroupIP[1];
					Multi_Udp_Buff[i].buf[60] = Remote.GroupIP[2];
					Multi_Udp_Buff[i].buf[61] = Remote.GroupIP[3];

					Multi_Udp_Buff[i].nlength = 62;
					Multi_Udp_Buff[i].DelayTime = 800;
					Multi_Udp_Buff[i].SendDelayTime = 0; //���͵ȴ�ʱ�����  20101112
					Multi_Udp_Buff[i].isValid = 1;
					sem_post(&multi_send_sem);
					break;
				}
			//�򿪻�����
			pthread_mutex_unlock(&Local.udp_lock);
		}
	}
}
//-----------------------------------------------------------------------
//����ת����
void TalkTurnOn_Status(void)
{
	StopPlayWavFile(); //�رջ�����
	WaitAudioUnuse(2000);
	//�鿴�Ƿ��������鲥����
	DropMultiGroup(m_VideoSocket, NULL);

	//StopRecVideo();
	StopRecAudio();
	StopPlayAudio();
	Local.Status = 0; //״̬Ϊ����

	//����ת����, �˴��ᵼ�� mega 64 ���һ�������¹һ�
//    SendHostOrder(0x68, Local.DoorNo, NULL); //������������  �Է��һ�
#ifdef _DEBUG
	P_Debug("�Է������Խ�, ת��\n");
#endif

	Local.OnlineFlag = 0;
	Local.CallConfirmFlag = 0; //�������߱�־
}
#endif
//-----------------------------------------------------------------------
//�Խ�����
void RecvTalkCall_Func(unsigned char *recv_buf, char *cFromIP)
{
}
//-----------------------------------------------------------------------
//�Խ�ռ��Ӧ��
void RecvTalkLineUse_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i, j;
	int isAddrOK;
	char wavFile[80];

	//����������
	pthread_mutex_lock(&Local.udp_lock);
	if ((recv_buf[7] & 0x03) == ASK
	) //Ӧ��
		for (i = 0; i < UDPSENDMAX; i++)
			if (Multi_Udp_Buff[i].isValid == 1)
				if (Multi_Udp_Buff[i].m_Socket == m_VideoSocket)
					if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM
					)
						if ((Multi_Udp_Buff[i].buf[6] == VIDEOTALK)
								|| (Multi_Udp_Buff[i].buf[6] == VIDEOTALKTRANS))
							if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK
							)
								if (Multi_Udp_Buff[i].buf[8] == CALL
								)
									if (strcmp(Multi_Udp_Buff[i].RemoteHost,
											cFromIP) == 0)
									{
										Multi_Udp_Buff[i].isValid = 0;
										if (Remote.DenNum == 1)
										{
											Local.Status = 0; //��״̬Ϊ����
											StopPlayWavFile();
											WaitAudioUnuse(2000);
											sprintf(wavFile, "%s/inuse.wav\0",
													wavPath);
											StartPlayWav(wavFile, 0);
										}

										//�Է�ռ��

										if (DebugMode == 1)
										{
											sprintf(
													Local.DebugInfo,
													"�յ��Խ�ռ��Ӧ�� i = %d,%d.%d.%d.%d\n",
													i, recv_buf[53],
													recv_buf[54], recv_buf[55],
													recv_buf[56]);
											P_Debug(Local.DebugInfo);
										}
										break;
									}
	//�򿪻�����
	pthread_mutex_unlock(&Local.udp_lock);
}

//-----------------------------------------------------------------------
//�Խ�����Ӧ��
void RecvTalkCallAnswer_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i, j;
	int isAddrOK;
	uint32_t Ip_Int;
	char wavFile[80];
	char RemoteIP[20];

	sprintf(RemoteIP, "%d.%d.%d.%d\0", recv_buf[53], recv_buf[54], recv_buf[55],
			recv_buf[56]);

	//����������
	pthread_mutex_lock(&Local.udp_lock);
	if ((recv_buf[7] & 0x03) == ASK) //Ӧ��
		for (i = 0; i < UDPSENDMAX; i++)
			if (Multi_Udp_Buff[i].isValid == 1)
				if (Multi_Udp_Buff[i].m_Socket == m_VideoSocket)
					if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM )
						if ((Multi_Udp_Buff[i].buf[6] == VIDEOTALK)
								|| (Multi_Udp_Buff[i].buf[6] == VIDEOTALKTRANS))
							if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK )
								if (Multi_Udp_Buff[i].buf[8] == CALL )
									if (strcmp(Multi_Udp_Buff[i].RemoteHost, cFromIP) == 0)
										if (strcmp(Multi_Udp_Buff[i].RemoteIP, RemoteIP) == 0)
										{
											Multi_Udp_Buff[i].isValid = 0;
											//  printf("Remote.DenNum = %d, Remote.isDirect = %d\n", Remote.DenNum, Remote.isDirect);
											if (Remote.isDirect == 0) //ֱͨ����
											{
												if ((LocalCfg.Addr[0] == 'S')
														|| (LocalCfg.Addr[0]
																== 'B')
														|| (LocalCfg.Addr[0]
																== 'Z')
														|| (Remote.DenNum == 1))
												{
													Ip_Int = inet_addr(cFromIP);
													memcpy(Remote.DenIP,
															&Ip_Int, 4);
												}
												else if (Remote.DenNum > 1)
												{
													Remote.DenIP[0] = Remote.GroupIP[0];
													Remote.DenIP[1] = Remote.GroupIP[1];
													Remote.DenIP[2] = Remote.GroupIP[2];
													Remote.DenIP[3] = Remote.GroupIP[3];
												}
											}
											else //��ת����
											{
												Remote.DenIP[0] =
														Local.IP_VideoServer[0];
												Remote.DenIP[1] =
														Local.IP_VideoServer[1];
												Remote.DenIP[2] =
														Local.IP_VideoServer[2];
												Remote.DenIP[3] =
														Local.IP_VideoServer[3];
											}

											//��ʼ¼����Ƶ
											StartRecVideo(CIF_W, CIF_H);


											//��������ǰ
											WaitAudioUnuse(1000);
											//���ź����������ں�����
											switch (LocalCfg.CallWaitRing)
											{
											case 0:
												sprintf(wavFile,
														"%s/cartoon.wav\0",
														wavPath);
												break;
											default:
												sprintf(wavFile,
														"%s/cartoon.wav\0",
														wavPath);
												break;
											}
											StartPlayWav(wavFile, 1);

											Local.Status = 1; //״̬Ϊ���жԽ�
											DisplayTalkConnectWindow(); //��ʾ�Խ� �������Ӵ���
											Local.CallConfirmFlag = 1; //�������߱�־
											Local.Timer1Num = 0;
											Local.TimeOut = 0; //���ӳ�ʱ,  ͨ����ʱ,  ���г�ʱ�����˽���
											Local.OnlineNum = 0; //����ȷ�����
											Local.OnlineFlag = 1;

#ifdef _CAPTUREPIC_TO_CENTER  //����ͼƬ��������
											Local.RecordPic = 1; //ͨ������Ƭ
											Local.IFrameCount = 0; //I֡����
											Local.HavePicRecorded = 0;
											memcpy(Local.RemoteAddr, Remote.Addr[0], 20);
											Local.RemoteAddr[12] = '\0';
#endif

											printf("Local.Status = %d\n", Local.Status);
											sprintf(Local.DebugInfo, "�յ��Խ�����Ӧ��, i = %d, %d.%d.%d.%d\n",
													i, recv_buf[53],
													recv_buf[54], recv_buf[55],
													recv_buf[56]);
											P_Debug(Local.DebugInfo);
											break;
										}
	//�򿪻�����
	pthread_mutex_unlock(&Local.udp_lock);
}
//-----------------------------------------------------------------------
//�Խ���ʼͨ��  �ɱ��з��������з�Ӧ��
void RecvTalkCallStart_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;
	uint32_t Ip_Int;
	printf("Remote.DenNum = %d, Remote.isDirect = %d\n", Remote.DenNum,
			Remote.isDirect);

	//******************************add by xuqd************************
	if (INVALID_SIP_CALL_ID != g_sipCallID)
	{
		TerminateLpCall(g_sipCallID);
		printf("******RecvTalkCallStart_Func()**begin to stop sip************\n");
	}
	//***********************************end***************************

	//����Ϊ���з� Ӧ��  ����ӰԤ������Ӱ״̬ �û�����ͨ��
	if (((Local.Status == 1) || (Local.Status == 7) || (Local.Status == 9))
			&& ((recv_buf[7] & 0x03) == ASK))
	{
		memcpy(send_b, recv_buf, 57);
		send_b[7] = REPLY | 0x04; //Ӧ��     �������ж��Ƿ��ǡ�8126
		sendlength = 57;
		RemotePort = RemoteVideoPort;
		if (recv_buf[6] == VIDEOTALK) //ֱͨ����
			RemotePort = RemoteVideoPort;
		else
			//��ת����
			RemotePort = RemoteVideoServerPort;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);
#ifdef _MULTI_MACHINE_SUPPORT  //��ֻ�֧��
		ExitGroup(recv_buf); //���������з��˳��鲥������
#endif

		//��ȡ���з���ַ
		memcpy(Remote.Addr[0], recv_buf + 33, 20);
		memcpy(Remote.IP[0], recv_buf + 53, 4);
		Remote.DenNum = 1;
		if (recv_buf[6] == VIDEOTALK) //ֱͨ����
			memcpy(Remote.DenIP, Remote.IP[0], 4);

		StopPlayWavFile(); //�رջ�����
		WaitAudioUnuse(2000); //�ȴ���������    //��ʱ��Ϊ�˵ȴ�����������ɣ������������

		StartRecAudio();
		StartPlayAudio();

		Local.Status = 5; //״̬Ϊ����ͨ��
		DisplayTalkStartWindow(); //��ʾ�Խ� ͨ������
		Local.TimeOut = 0; //���ӳ�ʱ,  ͨ����ʱ,  ���г�ʱ�����˽���

		//ͨ��ָʾ��
//    ioctl(gpio_fd, IO_CLEAR, 1);
		//����ͨ����
		if (DebugMode == 1)
			P_Debug("�Է���ʼͨ��\n");
	}
}
//-----------------------------------------------------------------------
//�Խ�����ȷ��
void RecvTalkCallConfirm_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;

	//����Ϊ���з�
	if (((Local.Status == 1) || (Local.Status == 5) || (Local.Status == 7)
			|| (Local.Status == 9)) && ((recv_buf[7] & 0x03) == ASK))
	{
		memcpy(send_b, recv_buf, 61);
		send_b[7] = REPLY; //Ӧ��
		sendlength = 61;
		RemotePort = RemoteVideoPort;
		if (recv_buf[6] == VIDEOTALK) //ֱͨ����
			RemotePort = RemoteVideoPort;
		else
			//��ת����
			RemotePort = RemoteVideoServerPort;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);
		Local.CallConfirmFlag = 1;
#ifdef _MULTI_MACHINE_SUPPORT  //��ֻ�֧��
		//������ͨ���У����յ���Ϊͨ����������ȷ�ϣ����˳��鲥������
		if (Local.Status == 5)
			if ((Remote.IP[0][0] != recv_buf[53])
					|| (Remote.IP[0][1] != recv_buf[54])
					|| (Remote.IP[0][2] != recv_buf[55])
					|| (Remote.IP[0][3] != recv_buf[56]))
			{
				sprintf(
						Local.DebugInfo,
						"�������з��˳��鲥��: Remote.IP = %d.%d.%d.%d, recv_buf = %d.%d.%d.%d, cFromIP = %s\n",
						Remote.IP[0][0], Remote.IP[0][1], Remote.IP[0][2],
						Remote.IP[0][3], recv_buf[53], recv_buf[54],
						recv_buf[55], recv_buf[56], cFromIP);
				P_Debug(Local.DebugInfo);
				SingleExitGroup(recv_buf); //�򵥸����з��˳��鲥������
			}
#endif
	}
	else //����Ϊ���з�
	if (((Local.Status == 2) || (Local.Status == 6) || (Local.Status == 8)
			|| (Local.Status == 10)) && ((recv_buf[7] & 0x03) == REPLY))
	{
		Local.CallConfirmFlag = 1;
		//if(DebugMode == 1)
		//   printf("�յ��Է�Ӧ�𱾻��Խ�����ȷ��\n");
	}
}
//-----------------------------------------------------------------------
//�Խ�Զ�̿���
void RecvTalkOpenLock_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;

	//��������
	if ((/*(Local.Status == 1)||(Local.Status == 2)||*/(Local.Status == 5)
			|| (Local.Status == 6)) && ((recv_buf[7] & 0x03) == ASK))
	{
		memcpy(send_b, recv_buf, 57);
		send_b[7] = REPLY; //Ӧ��
		sendlength = 57;
		RemotePort = RemoteVideoPort;
		if (recv_buf[6] == VIDEOTALK
		) //ֱͨ����
			RemotePort = RemoteVideoPort;
		else
			//��ת����
			RemotePort = RemoteVideoServerPort;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);

		if(Local.cLockFlg == 0)
		{
			OpenLock_Func(); //��������

			WaitAudioUnuse(2000); //�ȴ���������
			sprintf(wavFile, "%s/dooropen.wav\0", wavPath);
			StartPlayWav(wavFile, 0);

//			door_openlock.xLeft = 258;
//			door_openlock.yTop = 300;
			DisplayImage(&door_openlock, 0);

			if (DebugMode == 1)
				P_Debug("�Է�Զ�̿���\n");
			Local.cLockFlg = 1;
		}
	}
}
//-----------------------------------------------------------------------
//�Խ����н���
void RecvTalkCallEnd_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;

	//��������
	if (((Local.Status == 1) || (Local.Status == 2) || (Local.Status == 5)
			|| (Local.Status == 6) || (Local.Status == 7) || (Local.Status == 8)
			|| (Local.Status == 9) || (Local.Status == 10))
			&& ((recv_buf[7] & 0x03) == ASK))
	{
		Local.OnlineFlag = 0;
		Local.CallConfirmFlag = 0; //�������߱�־

		memcpy(send_b, recv_buf, 57);
		send_b[7] = REPLY; //Ӧ��
		sendlength = 57;
		RemotePort = RemoteVideoPort;
		if (recv_buf[6] == VIDEOTALK
		) //ֱͨ����
			RemotePort = RemoteVideoPort;
		else
			//��ת����
			RemotePort = RemoteVideoServerPort;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);
		//  if(Local.Status == 10)
		//    ShowStatusText(50, 380 , 3, cBlack, 1, 1, "������Ӱ", 0);
		//  else
		//    ShowStatusText(50, 380 , 3, cBlack, 1, 1, "�Է������Խ�", 0);
#ifdef _MULTI_MACHINE_SUPPORT  //��ֻ�֧��
		ExitGroup(recv_buf); //���������з��˳��鲥������
#endif

		//*************add by xuqd*********************
		printf("******RecvTalkCallEnd_Func()**g_sipCallID is %d***wait for close!!!!!!****\n",
				g_sipCallID);
		if (INVALID_SIP_CALL_ID != g_sipCallID)
		{
			TerminateLpCall(g_sipCallID);
		}
		TalkEnd_ClearStatus();
		printf(
				"******RecvTalkCallEnd_Func()**g_sipCallID is %d***TalkEnd_ClearStatus()****\n",
				g_sipCallID);
		//*********************************************

		//���н���
#ifdef _DEBUG
		sprintf(Local.DebugInfo, "�Է������Խ�, %s\n", cFromIP);
		P_Debug(Local.DebugInfo);
#endif
	}
	else //��������
//   if((Local.Status == 1)||(Local.Status == 2)||(Local.Status == 5)||(Local.Status == 6)
//     ||(Local.Status == 7)||(Local.Status == 8)||(Local.Status == 9)||(Local.Status == 10))
	{
		Local.OnlineFlag = 0;
		Local.CallConfirmFlag = 0; //�������߱�־

		//����������
		pthread_mutex_lock(&Local.udp_lock);
		//��������
		if ((recv_buf[7] & 0x03) == REPLY)
			for (i = 0; i < UDPSENDMAX; i++)
				if (Multi_Udp_Buff[i].isValid == 1)
					if (Multi_Udp_Buff[i].m_Socket == m_VideoSocket)
						if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM
						)
							if ((Multi_Udp_Buff[i].buf[6] == VIDEOTALK)
									|| (Multi_Udp_Buff[i].buf[6]
											== VIDEOTALKTRANS))
								if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK
								)
									if (Multi_Udp_Buff[i].buf[8] == CALLEND
									)
										if (strcmp(Multi_Udp_Buff[i].RemoteHost,cFromIP) == 0)
										{
											Multi_Udp_Buff[i].isValid = 0;

											printf("***RecvTalkCallEnd_Func()**g_sipCallID is %d***********!\n",
													g_sipCallID);

											//*************add by xuqd*********************
											//if(INVALID_SIP_CALL_ID == g_sipCallID)
											{
												TalkEnd_ClearStatus();
											}
											//*********************************************

											//TalkEnd_ClearStatus();

											//���н���

											//     ShowStatusText(50, 380 , 3, cBlack, 1, 1, "���������Խ�", 0);
											if (DebugMode == 1)
												P_Debug("�Է�Ӧ�𱾻������Խ�\n");
											break;
										}
		//�򿪻�����
		pthread_mutex_unlock(&Local.udp_lock);
		//20101112 �������������� �Է� ExitGroup
		//ExitGroup(recv_buf);      //���������з��˳��鲥������
	}
}
//-----------------------------------------------------------------------
//�Խ���������״̬�͹ر�����Ƶ
void TalkEnd_ClearStatus(void)
{
	StopPlayWavFile(); //�ر�����
	WaitAudioUnuse(2000);
	printf("Local.Status=%d\n", Local.Status);
	//�鿴�Ƿ��������鲥����
	DropMultiGroup(m_VideoSocket, NULL);
	switch (Remote.Addr[0][0])
	{
	case 'Z': //���Ļ�����
	case 'W': //Χǽ������
		Remote.Addr[0][5] = '\0';
		break;
	case 'H': //�����ſڻ�����
	case 'B': //���������ڻ�����
		Remote.Addr[0][6] = '\0';
		break;
	case 'M': //��Ԫ�ſڻ�����
		Remote.Addr[0][8] = '\0';
		break;
	case 'S': //���ڻ�����
		Remote.Addr[0][12] = '\0';
		break;
	}
	//ͨ��ָʾ��
//    ioctl(gpio_fd, IO_PUT, 1);
	switch (Local.Status)
	{
	case 1: //��������
		printf(
				"****TalkEnd_ClearStatus()***case 1**,StopRecVideo(),StopPlayWavFile()****\n");
		StopRecVideo();
		StopPlayWavFile(); //�رջ�����
		printf("****TalkEnd_ClearStatus()***case 1**,Local.Status is %d****\n",
				Local.Status);

#ifdef _CAPTUREPIC_TO_CENTER  //����ͼƬ��������
		if (Local.HavePicRecorded == 1) //����Ƭ��¼��
		{
			Local.HavePicRecorded = 0;
			//SaveToFlash(3);       //δ����  modified by xuqd
		}
#endif
		break;
	case 5: //��������ͨ��
		printf("****TalkEnd_ClearStatus()***case 1**,StopRecVideo(),StopPlayWavFile(),StopPlayAudio()****\n");
		StopRecVideo();
		StopRecAudio();
		StopPlayAudio();
		printf("****TalkEnd_ClearStatus()***case 2**,Local.Status is %d****\n", Local.Status);
#ifdef _CAPTUREPIC_TO_CENTER  //����ͼƬ��������
		if (Local.HavePicRecorded == 1) //����Ƭ��¼��
		{
			Local.HavePicRecorded = 0;
			//SaveToFlash(3);       //δ���� modified by xuqd
		}
#endif
		break;
	}

	//Local.Status = 0;  // add by xuqd
	// �����ڻ��ҵ�ʱ��g_sipCallIDӦ��Ϊ0
	// û��sip�ն˵�ʱ��g_sipCallIDҲӦ��Ϊ0
	// ����sip�ն��Ƚ���������idu�һ���ʱ��g_sipCallID��Ϊ0����ʱ����������״̬
	if (g_sipCallID == INVALID_SIP_CALL_ID)
	{
		Local.OnlineFlag = 0;
		Local.CallConfirmFlag = 0; //�������߱�־

		Local.TalkFinishFlag = 1; //ͨ��������־     20101029  xu
		Local.TalkFinishTime = 0; //ͨ����������

		PicStatBuf.Type = Local.CurrentWindow;
		PicStatBuf.Time = 0;
		PicStatBuf.Flag = 1;
	}
}
//-----------------------------------------------------------------------
//�Խ�ǿ��I֡
void RecvForceIFrame_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i, j;
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;

	//��������
	if ((recv_buf[7] & 0x03) == ASK)
	{
		memcpy(send_b, recv_buf, 57);
		send_b[7] = REPLY; //Ӧ��
		sendlength = 57;
		if (Remote.isDirect == 0) //ֱͨ����
			RemotePort = RemoteVideoPort;
		else
			//��ת����
			RemotePort = RemoteVideoServerPort;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);

		Local.ForceIFrame = 1;

		if (DebugMode == 1)
			P_Debug("�յ�ǿ��I֡����\n");
	}
	else
	{
		//����������
		pthread_mutex_lock(&Local.udp_lock);
		//��������
		if ((recv_buf[7] & 0x03) == REPLY
		)
			for (i = 0; i < UDPSENDMAX; i++)
				if (Multi_Udp_Buff[i].isValid == 1)
					if (Multi_Udp_Buff[i].m_Socket == m_VideoSocket)
						if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM
						)
							if ((Multi_Udp_Buff[i].buf[6] == VIDEOTALK)
									|| (Multi_Udp_Buff[i].buf[6]
											== VIDEOTALKTRANS))
								if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK
								)
									if (Multi_Udp_Buff[i].buf[8] == FORCEIFRAME
									)
										if (strcmp(Multi_Udp_Buff[i].RemoteHost,
												cFromIP) == 0)
										{
											Multi_Udp_Buff[i].isValid = 0;
											if (DebugMode == 1)
												P_Debug("�Է�Ӧ��ǿ��I֡����\n");
											break;
										}
		//�򿪻�����
		pthread_mutex_unlock(&Local.udp_lock);
	}
}
//-----------------------------------------------------------------------
//�Ŵ�(720*480)
void RecvTalkZoomOut_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;

	//��������
	if ((/*(Local.Status == 1)||(Local.Status == 2)||*/(Local.Status == 5)
			|| (Local.Status == 6)) && ((recv_buf[7] & 0x03) == ASK))
	{
		memcpy(send_b, recv_buf, 57);
		send_b[7] = REPLY; //Ӧ��
		sendlength = 57;
		RemotePort = RemoteVideoPort;
		if (recv_buf[6] == VIDEOTALK
		) //ֱͨ����
			RemotePort = RemoteVideoPort;
		else
			//��ת����
			RemotePort = RemoteVideoServerPort;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);
		if (Local.RecPicSize == 1)
		{
			StopRecVideo(); //352*240
			StartRecVideo(D1_W, D1_H); //720*480
		}
		if (DebugMode == 1)
			P_Debug("�Է��Ŵ�ͼ��\n");
	}
	else //��������
	if (/*(Local.Status == 1)||(Local.Status == 2)||*/(Local.Status == 5)
			|| (Local.Status == 6))
	{
		//����������
		pthread_mutex_lock(&Local.udp_lock);
		//��������
		if ((recv_buf[7] & 0x03) == REPLY
		)
			for (i = 0; i < UDPSENDMAX; i++)
				if (Multi_Udp_Buff[i].isValid == 1)
					if (Multi_Udp_Buff[i].m_Socket == m_VideoSocket)
						if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM
						)
							if ((Multi_Udp_Buff[i].buf[6] == VIDEOTALK)
									|| (Multi_Udp_Buff[i].buf[6]
											== VIDEOTALKTRANS))
								if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK
								)
									if (Multi_Udp_Buff[i].buf[8] == ZOOMOUT
									)
										if (strcmp(Multi_Udp_Buff[i].RemoteHost,
												cFromIP) == 0)
										{
											Multi_Udp_Buff[i].isValid = 0;
											if (DebugMode == 1)
												P_Debug("�Է�Ӧ�𱾻��Ŵ�ͼ��\n");
											break;
										}
		//�򿪻�����
		pthread_mutex_unlock(&Local.udp_lock);
	}
}
//-----------------------------------------------------------------------
//��С(352*288)
void RecvTalkZoomIn_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;

	//��������
	if ((/*(Local.Status == 1)||(Local.Status == 2)||*/(Local.Status == 5)
			|| (Local.Status == 6)) && ((recv_buf[7] & 0x03) == ASK))
	{
		memcpy(send_b, recv_buf, 57);
		send_b[7] = REPLY; //Ӧ��
		sendlength = 57;
		RemotePort = RemoteVideoPort;
		if (recv_buf[6] == VIDEOTALK
		) //ֱͨ����
			RemotePort = RemoteVideoPort;
		else
			//��ת����
			RemotePort = RemoteVideoServerPort;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);
		if (Local.RecPicSize == 2)
		{
			StopRecVideo(); //720*480
			StartRecVideo(CIF_W, CIF_H); //352*288
		}
		if (DebugMode == 1)
			P_Debug("�Է���Сͼ��\n");
	}
	else //��������
	if (/*(Local.Status == 1)||(Local.Status == 2)||*/(Local.Status == 5)
			|| (Local.Status == 6))
	{
		//����������
		pthread_mutex_lock(&Local.udp_lock);
		//��������
		if ((recv_buf[7] & 0x03) == REPLY
		)
			for (i = 0; i < UDPSENDMAX; i++)
				if (Multi_Udp_Buff[i].isValid == 1)
					if (Multi_Udp_Buff[i].m_Socket == m_VideoSocket)
						if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM
						)
							if ((Multi_Udp_Buff[i].buf[6] == VIDEOTALK)
									|| (Multi_Udp_Buff[i].buf[6]
											== VIDEOTALKTRANS))
								if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK
								)
									if (Multi_Udp_Buff[i].buf[8] == ZOOMIN
									)
										if (strcmp(Multi_Udp_Buff[i].RemoteHost,
												cFromIP) == 0)
										{
											Multi_Udp_Buff[i].isValid = 0;
											if (DebugMode == 1)
												P_Debug("�Է�Ӧ�𱾻���Сͼ��\n");
											break;
										}
		//�򿪻�����
		pthread_mutex_unlock(&Local.udp_lock);
	}
}
//-----------------------------------------------------------------------
//�Խ�����
void RecvTalkCallUpDown_Func(unsigned char *recv_buf, char *cFromIP, int length)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	short PackIsExist; //���ݰ��ѽ��ձ�־
	TempAudioNode1 * tmp_audionode;
	int isFull;
	struct talkdata1 talkdata;
	if ((Local.Status == 1) || (Local.Status == 2) || (Local.Status == 5)
			|| (Local.Status == 6) || (Local.Status == 7) || (Local.Status == 8)
			|| (Local.Status == 9) || (Local.Status == 10)) //״̬Ϊ�Խ�
		switch (recv_buf[61])
		{
		case 1: //��Ƶ
#if 1
			//֡���
			//       if(AudioMuteFlag == 0)
			//д��Ӱ���ݺ���, ����  ����  ���� 1--��Ƶ  2--��ƵI  3--��ƵP
			if ((Local.Status == 5) || (Local.Status == 6)
					|| (Local.Status == 10))
			{
				memcpy(&talkdata, recv_buf + 9, sizeof(talkdata));
				//       printf("talkdata.Frameno = %d, talkdata.timestamp = %d\n", talkdata.Frameno, talkdata.timestamp);
				if (temp_audio_n >= G711NUM)
				{
					temp_audio_n = G711NUM;
					if (DebugMode == 1)
						P_Debug("temp_audio is full\n");
				}
				else
				{
					tmp_audionode = (TempAudioNode1 *) find_audionode(
							TempAudioNode_h, talkdata.Frameno,
							talkdata.CurrPackage);
					if (tmp_audionode == NULL)
					{
						isFull = creat_audionode(TempAudioNode_h, talkdata,
								recv_buf, length);
						PackIsExist = 0;
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("audio pack exist\n");
						PackIsExist = 1;
					}

					if (PackIsExist == 0)
					{
						TimeStamp.OldCurrAudio = TimeStamp.CurrAudio; //��һ�ε�ǰ��Ƶʱ��
						TimeStamp.CurrAudio = talkdata.timestamp;

						//   temp_audio_n ++;
						temp_audio_n = length_audionode(TempAudioNode_h);
						if (temp_audio_n >= 4) //VPLAYNUM/2 4֡ 128ms
						{
							sem_post(&audiorec2playsem);
						}
					}
				}
			}
#endif           
			break;
		case 2: //��Ƶ  I֡  352*288
		case 3: //��Ƶ  P֡  352*288
		case 4: //��Ƶ  I֡  720*480
		case 5: //��Ƶ  P֡  720*480
			P_Debug("video data recv\n");
			break;

		}
}
//-----------------------------------------------------------------------
void ForceIFrame_Func(void) //ǿ��I֡
{
	int i;
	int RemotePort;

	for (i = 0; i < UDPSENDMAX; i++)
		if (Multi_Udp_Buff[i].isValid == 0)
		{
			//����������
			pthread_mutex_lock(&Local.udp_lock);
			//ֻ����һ��
			Multi_Udp_Buff[i].SendNum = 5;
			Multi_Udp_Buff[i].m_Socket = m_VideoSocket;
			if (Remote.isDirect == 0) //ֱͨ����
				RemotePort = RemoteVideoPort;
			else
				//��ת����
				RemotePort = RemoteVideoServerPort;
			Multi_Udp_Buff[i].RemotePort = RemotePort;
			Multi_Udp_Buff[i].CurrOrder = 0;
			sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0",
					Remote.DenIP[0], Remote.DenIP[1], Remote.DenIP[2],
					Remote.DenIP[3]);
			//ͷ��
			memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
			//����  ,�����㲥����
			if ((Local.Status == 1) || (Local.Status == 2)
					|| (Local.Status == 5) || (Local.Status == 6)
					|| (Local.Status == 7) || (Local.Status == 8)
					|| (Local.Status == 9) || (Local.Status == 10))
			{
				if (Remote.isDirect == 0) //ֱͨ����
					Multi_Udp_Buff[i].buf[6] = VIDEOTALK;
				else
					//��ת����
					Multi_Udp_Buff[i].buf[6] = VIDEOTALKTRANS;
			}
			if ((Local.Status == 3) || (Local.Status == 4))
			{
				if (Remote.isDirect == 0) //ֱͨ����
					Multi_Udp_Buff[i].buf[6] = VIDEOWATCH;
				else
					//��ת����
					Multi_Udp_Buff[i].buf[6] = VIDEOWATCHTRANS;
			}
			Multi_Udp_Buff[i].buf[7] = ASK; //����
			Multi_Udp_Buff[i].buf[8] = FORCEIFRAME; //FORCEIFRAME

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
//-----------------------------------------------------------------------
void ExitGroup(unsigned char *buf) //���������з��˳��鲥������
{
	int i, j;
	int isIP;
	int RemotePort;
	char tmp_ip[20];
	//�˳��鲥��
	if (Remote.DenNum > 1)
	{
		//����������
		pthread_mutex_lock(&Local.udp_lock);
		for (j = 0; j < Remote.DenNum; j++)
		{
			Remote.Added[j] = 0;
			if (DebugMode == 1)
				printf("%d.%d.%d.%d  %d.%d.%d.%d\n", Remote.IP[j][0],
						Remote.IP[j][1], Remote.IP[j][2], Remote.IP[j][3],
						buf[53], buf[54], buf[55], buf[56]);
			//��Ϊ��ʼͨ������һ��
			if ((Remote.IP[j][0] == buf[53]) && (Remote.IP[j][1] == buf[54])
					&& (Remote.IP[j][2] == buf[55])
					&& (Remote.IP[j][3] == buf[56]))
				isIP = 1;
			else
				isIP = 0;

			//����Ƿ��Ѿ��ڷ��ͻ�������   //xu 20101109
			sprintf(tmp_ip, "%d.%d.%d.%d\0", Remote.IP[j][0], Remote.IP[j][1],
					Remote.IP[j][2], Remote.IP[j][3]);
			for (i = 0; i < UDPSENDMAX; i++)
				if (Multi_Udp_Buff[i].isValid == 1)
				{
					if ((strcmp(Multi_Udp_Buff[i].RemoteHost, tmp_ip) == 0)
							&& (Multi_Udp_Buff[i].buf[8] == LEAVEGROUP))
					{
						isIP = 1;
						sprintf(Local.DebugInfo, "�˳��鲥���������ڻ�����, %s\n", tmp_ip);
						P_Debug(Local.DebugInfo);
						break;
					}
				}

			if (isIP == 0)
				for (i = 0; i < UDPSENDMAX; i++)
					if (Multi_Udp_Buff[i].isValid == 0)
					{
						Multi_Udp_Buff[i].SendNum = 0;
						Multi_Udp_Buff[i].m_Socket = m_VideoSocket;
						if (Remote.isDirect == 0) //ֱͨ����
						{
							RemotePort = RemoteVideoPort;
							Multi_Udp_Buff[i].buf[6] = VIDEOTALK;
							sprintf(Multi_Udp_Buff[i].RemoteHost,
									"%d.%d.%d.%d\0", Remote.IP[j][0],
									Remote.IP[j][1], Remote.IP[j][2],
									Remote.IP[j][3]);
						}
						else //��ת����
						{
							RemotePort = RemoteVideoServerPort;
							Multi_Udp_Buff[i].buf[6] = VIDEOTALKTRANS;
							sprintf(Multi_Udp_Buff[i].RemoteHost,
									"%d.%d.%d.%d\0", Remote.DenIP[0],
									Remote.DenIP[1], Remote.DenIP[2],
									Remote.DenIP[3]);
						}
						Multi_Udp_Buff[i].RemotePort = RemotePort;
						if (DebugMode == 1)
						{
							sprintf(
									Local.DebugInfo,
									"����Ҫ���˳��鲥��, Multi_Udp_Buff[i].RemoteHost is %s\n",
									Multi_Udp_Buff[i].RemoteHost);
							P_Debug(Local.DebugInfo);
						}
						//ͷ��
						memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);

						Multi_Udp_Buff[i].buf[7] = ASK; //����
						// ������
						Multi_Udp_Buff[i].buf[8] = LEAVEGROUP;

						memcpy(Multi_Udp_Buff[i].buf + 9, LocalCfg.Addr, 20);
						memcpy(Multi_Udp_Buff[i].buf + 29, LocalCfg.IP, 4);
						memcpy(Multi_Udp_Buff[i].buf + 33, Remote.Addr[j], 20);
						memcpy(Multi_Udp_Buff[i].buf + 53, Remote.IP[j], 4);

						Multi_Udp_Buff[i].buf[57] = Remote.GroupIP[0];
						Multi_Udp_Buff[i].buf[58] = Remote.GroupIP[1];
						Multi_Udp_Buff[i].buf[59] = Remote.GroupIP[2];
						Multi_Udp_Buff[i].buf[60] = Remote.GroupIP[3];

						Multi_Udp_Buff[i].nlength = 61;
						Multi_Udp_Buff[i].DelayTime = 100; //600;  20101112 ��Ϊ 100
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
//-----------------------------------------------------------------------
void SingleExitGroup(unsigned char *buf) //�򵥸����з��˳��鲥������
{
	int i, j;
	int RemotePort;
	//����������
	pthread_mutex_lock(&Local.udp_lock);
	for (i = 0; i < UDPSENDMAX; i++)
		if (Multi_Udp_Buff[i].isValid == 0)
		{
			Multi_Udp_Buff[i].DelayTime = 100;
			Multi_Udp_Buff[i].SendDelayTime = 0; //���͵ȴ�ʱ�����  20101112
			Multi_Udp_Buff[i].isValid = 1;
			Multi_Udp_Buff[i].SendNum = 0;
			Multi_Udp_Buff[i].m_Socket = m_VideoSocket;
			if (Remote.isDirect == 0) //ֱͨ����
			{
				RemotePort = RemoteVideoPort;
				Multi_Udp_Buff[i].buf[6] = VIDEOTALK;
				sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0", buf[53],
						buf[54], buf[55], buf[56]);
			}
			else //��ת����
			{
				RemotePort = RemoteVideoServerPort;
				Multi_Udp_Buff[i].buf[6] = VIDEOTALKTRANS;
				sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0",
						Remote.DenIP[0], Remote.DenIP[1], Remote.DenIP[2],
						Remote.DenIP[3]);
			}
			if (DebugMode == 1)
			{
				printf("Multi_Udp_Buff[i].RemoteHost is %s\n",
						Multi_Udp_Buff[i].RemoteHost);
				P_Debug("����Ҫ���˳��鲥��\n");
			}
			//ͷ��
			memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);

			Multi_Udp_Buff[i].buf[7] = ASK; //����
			// ������
			Multi_Udp_Buff[i].buf[8] = LEAVEGROUP;

			memcpy(Multi_Udp_Buff[i].buf + 9, LocalCfg.Addr, 20);
			memcpy(Multi_Udp_Buff[i].buf + 29, LocalCfg.IP, 4);
			memcpy(Multi_Udp_Buff[i].buf + 33, buf + 33, 20);
			memcpy(Multi_Udp_Buff[i].buf + 53, buf + 53, 4);

			Multi_Udp_Buff[i].buf[57] = Remote.GroupIP[0];
			Multi_Udp_Buff[i].buf[58] = Remote.GroupIP[1];
			Multi_Udp_Buff[i].buf[59] = Remote.GroupIP[2];
			Multi_Udp_Buff[i].buf[60] = Remote.GroupIP[3];

			Multi_Udp_Buff[i].nlength = 61;
			Multi_Udp_Buff[i].DelayTime = 100;
			Multi_Udp_Buff[i].SendDelayTime = 0; //���͵ȴ�ʱ�����  20101112
			Multi_Udp_Buff[i].isValid = 1;

			sem_post(&multi_send_sem);
			break;
		}
	//�򿪻�����
	pthread_mutex_unlock(&Local.udp_lock);
}
//-----------------------------------------------------------------------
