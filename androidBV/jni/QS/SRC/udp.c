//UDP
#include <stdio.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <semaphore.h>       //sem_t
#include <dirent.h>
#include <LANIntfLayer.h>
#include <LOGIntfLayer.h>

#define CommonH
#include "common.h"

#ifdef  _MESSAGE_SUPPORT  //消息机制
#include "../type.h"
#endif

//UDP
int SndBufLen = 1024 * 128;
int RcvBufLen = 1024 * 128;

extern int DebugMode;           //调试模式

#include <LANIntfLayer.h>

short UdpRecvFlag;
pthread_t udpdatarcvid;
pthread_t udpvideorcvid;
int InitUdpSocket(short lPort);
void CloseUdpSocket(void);
int UdpSendBuff(int m_Socket, char *RemoteHost, int RemotePort, unsigned char *buf, int nlength);
void CreateUdpDataRcvThread(void);
void CreateUdpVideoRcvThread(void);
void UdpDataRcvThread(void); //UDP数据接收线程函数
void UdpVideoRcvThread(void); //UDP音视频接收线程函数
void AddMultiGroup(int m_Socket, char *McastAddr); //加入组播组
void DropMultiGroup(int m_Socket, char *McastAddr); //退出组播组
void DropNsMultiGroup(int m_Socket, char *McastAddr); //退出NS组播组
void RefreshNetSetup(int cType); //刷新网络设置  0 -- 未启动  1 -- 已启动

extern sem_t audiorec2playsem;

extern sem_t videorec2playsem;

int AudioMuteFlag; //静音标志
char watchRecvBuf[1024];

//接收线程内部处理函数
//报警
void RecvAlarm_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket);
//信息
void RecvMessage_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket); //信息
//设备定时报告状态
void RecvReportStatus_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket);
//管理中心查询设备状态
void RecvQueryStatus_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket);
//写地址设置
void RecvWriteAddress_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket);
//读地址设置
void RecvReadAddress_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket);
//搜索所有设备应答
void RecvSearchAllEquip_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket);
//写设备地址应答
void RecvWriteEquipAddr_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket);
//写帮助信息
void RecvWriteHelp_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket);
//读帮助信息
void RecvReadHelp_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket);
//写设置信息
void RecvWriteSetup_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket);
//读设置信息
void RecvReadSetup_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket);

#ifdef _BRUSHIDCARD_SUPPORT   //刷卡功能支持
//写ID卡信息
void RecvWriteIDCard_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket);
//存储ID卡信息
void SaveWriteIDCard_Func(void);
//读ID卡信息
void RecvReadIDCard_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket);
//刷卡记录
void RecvBrushIDCard_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket);
#endif

//下载应用程序
void RecvDownLoadFile_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket);
//下载系统映像
void RecvDownLoadImage_Func(unsigned char *recv_buf, char *cFromIP, int length,
		int m_Socket);

#ifdef _CAPTUREPIC_TO_CENTER  //捕获图片传到中心
//单元门口机、围墙机、二次门口机发送呼叫照片->发送开始
void RecvCapture_Send_Start_Func(unsigned char *recv_buf, char *cFromIP,
		int length, int m_Socket);
//单元门口机、围墙机、二次门口机发送呼叫照片->发送数据
void RecvCapture_Send_Data_Func(unsigned char *recv_buf, char *cFromIP,
		int length, int m_Socket);
//单元门口机、围墙机、二次门口机发送呼叫照片->发送结束（成功）
void RecvCapture_Send_Succ_Func(unsigned char *recv_buf, char *cFromIP,
		int length, int m_Socket);
//单元门口机、围墙机、二次门口机发送呼叫照片->发送结束（失败）
void RecvCapture_Send_Fail_Func(unsigned char *recv_buf, char *cFromIP,
		int length, int m_Socket);
#endif

//发送远程调试信息
void RecvRemoteDebugInfo_Func(unsigned char *recv_buf, char *cFromIP,
		int length, int m_Socket);

#ifdef _REMOTECALLTEST  //远程呼叫测试
//接收远程呼叫测试
void RecvRemoteCallTest_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket);
#endif

//解析
void RecvNSAsk_Func(unsigned char *recv_buf, char *cFromIP, int m_Socket); //解析请求
void RecvNSReply_Func(unsigned char *recv_buf, char *cFromIP, int m_Socket); //解析应答
//监视
void RecvWatchCall_Func(unsigned char *recv_buf, char *cFromIP); //监视呼叫
void RecvWatchLineUse_Func(unsigned char *recv_buf, char *cFromIP); //监视占线应答
void RecvWatchCallAnswer_Func(unsigned char *recv_buf, char *cFromIP); //监视呼叫应答
void RecvWatchCallConfirm_Func(unsigned char *recv_buf, char *cFromIP); //通话在线确认
void RecvWatchCallEnd_Func(unsigned char *recv_buf, char *cFromIP); //监视呼叫结束
void RecvWatchZoomOut_Func(unsigned char *recv_buf, char *cFromIP); //放大(720*480)
void RecvWatchZoomIn_Func(unsigned char *recv_buf, char *cFromIP); //缩小(352*240)
void RecvWatchCallUpDown_Func(unsigned char *recv_buf, char *cFromIP, int length); //监视数据
//对讲
void RecvTalkCall_Func(unsigned char *recv_buf, char *cFromIP); //对讲呼叫
void RecvTalkLineUse_Func(unsigned char *recv_buf, char *cFromIP); //对讲占线应答
void RecvTalkCallAnswer_Func(unsigned char *recv_buf, char *cFromIP); //对讲呼叫应答
void RecvTalkCallConfirm_Func(unsigned char *recv_buf, char *cFromIP); //对讲在线确认
void RecvTalkOpenLock_Func(unsigned char *recv_buf, char *cFromIP); //对讲远程开锁
void RecvTalkCallStart_Func(unsigned char *recv_buf, char *cFromIP); //对讲开始通话
void RecvTalkCallEnd_Func(unsigned char *recv_buf, char *cFromIP); //对讲呼叫结束
void RecvTalkZoomOut_Func(unsigned char *recv_buf, char *cFromIP); //放大(720*480)
void RecvTalkZoomIn_Func(unsigned char *recv_buf, char *cFromIP); //缩小(352*240)
void RecvTalkCallUpDown_Func(unsigned char *recv_buf, char *cFromIP, int length); //对讲数据
void TalkEnd_ClearStatus(void); //对讲结束，清状态和关闭音视频
void RecvForceIFrame_Func(unsigned char *recv_buf, char *cFromIP); //对讲强制I帧
void ForceIFrame_Func(void); //强制I帧

#ifdef _MULTI_MACHINE_SUPPORT  //多分机支持
void RecvTalkJoinGroup_Func(unsigned char *recv_buf, char *cFromIP); //加入组播组（主叫方->被叫方，被叫方应答）
void RecvTalkLeaveGroup_Func(unsigned char *recv_buf, char *cFromIP); //退出组播组（主叫方->被叫方，被叫方应答）
void ExitGroup(unsigned char *buf); //向其它被叫方退出组播组命令
void SingleExitGroup(unsigned char *buf); //向单个被叫方退出组播组命令
void RecvTalkTurnTalk_Func(unsigned char *recv_buf, char *cFromIP); //转接（被叫方->主叫方，主叫方应答）

void TalkTurnOn_Status(void); //正在转接中
#endif

int downbuflen;
char downip[20];
unsigned char *downbuf;
pthread_t download_image_thread; //下载系统映像线程
void download_image_thread_func(void);
int downloaded_flag[2000]; //已下载标志
int OldPackage;

void SendUdpOne(unsigned char Order, unsigned char SonOrder, unsigned char PerCent, char *cFromIP);
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
	int ttl; //设置TTL值

	/* 创建 socket , 关键在于这个 SOCK_DGRAM */
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
	/* 设置地址和端口信息 */
	s_addr.sin_family = AF_INET;
	s_addr.sin_port = htons(lPort);
	s_addr.sin_addr.s_addr = INADDR_ANY; //inet_addr(LocalIP);//INADDR_ANY;

	iLen = sizeof(nZero); //  SO_SNDBUF
	nZero = SndBufLen; //128K
	setsockopt(m_Socket, SOL_SOCKET, SO_SNDBUF, (char*) &nZero, sizeof((char*) &nZero));
	nZero = RcvBufLen; //128K
	setsockopt(m_Socket, SOL_SOCKET, SO_RCVBUF, (char*) &nZero, sizeof((char*) &nZero));

	ttl = MULTITTL; //设置TTL值
	rc = setsockopt(m_Socket, IPPROTO_IP, IP_MULTICAST_TTL, (char *) &ttl, sizeof(ttl));
	//rc = getsockopt(m_Socket, IPPROTO_IP, IP_TTL, (char *)&ttl, sizeof(ttl));

	//音视频端口，允许发送和接收广播
	//20100914  DataPort 也可发送广播
	//if(lPort == LocalVideoPort)
	{
		nYes = 1;
		if (setsockopt(m_Socket, SOL_SOCKET, SO_BROADCAST, (char *)&nYes, sizeof((char *) &nYes)) == -1)
		{
			P_Debug("set broadcast error.\n\r");
			return 0;
		}
	}

	/* 绑定地址和端口信息 */
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
		//创建UDP接收线程
		CreateUdpDataRcvThread();
	}
	if (lPort == LocalVideoPort)
	{
		m_VideoSocket = m_Socket;
		//创建UDP接收线程
		CreateUdpVideoRcvThread();
	}
	return 1;
}
//---------------------------------------------------------------------------
void CloseUdpSocket(void)
{
	UdpRecvFlag = 0;
	//pthread_cancel(udpdatarcvid);
	//pthread_cancel(udpvideorcvid);
	close(m_DataSocket);
	close(m_VideoSocket);

	if (m_EthSocket > 0)
		close(m_EthSocket);
}
//---------------------------------------------------------------------------
//#define ISSETUPPACKETSIZE   //限制最小包
//#define ISSENDPACKETSIZE   //限制SEND最小包
#define SMALLESTSIZE  512    //UDP最小包大小

int UdpSendBuff(int m_Socket, char *RemoteHost, int RemotePort, unsigned char *buf, int nlength)
{
	struct sockaddr_in To;
	int nSize;
	To.sin_family = AF_INET;
	To.sin_port = htons(RemotePort);
	To.sin_addr.s_addr = inet_addr(RemoteHost);
#ifdef ISSENDPACKETSIZE
	if (nlength < SMALLESTSIZE)
		nlength = SMALLESTSIZE;
#endif
	nSize = sendto(m_Socket, buf, nlength, 0, (struct sockaddr*) &To, sizeof(struct sockaddr));
#ifdef _DEBUG
	sprintf(Local.DebugInfo, "*** SEND &&& m_Socket = %d, RemoteHost = %s, RemotePort = %d, buf[%d] = %s, nSize = %d, buf[6] = %d, buf[7] = %d, buf[8] = %d, buf[61] = %d %d \n",
			m_Socket, RemoteHost, RemotePort, nlength, buf, nSize, buf[6], buf[7], buf[8], buf[61], buf[62]);
	P_Debug(Local.DebugInfo);
#endif
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
#ifdef _DEBUG
		P_Debug("Create UDP data pthread!\n");
#endif
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
	ret = pthread_create(&udpvideorcvid, &attr, (void *) UdpVideoRcvThread, NULL);
	pthread_attr_destroy(&attr);
#ifdef _DEBUG
		P_Debug("Create UDP video pthread!\n");
#endif
	if (ret != 0)
	{
		P_Debug("Create video pthread error!\n");
		exit(1);
	}
}
//---------------------------------------------------------------------------
void AddMultiGroup(int m_Socket, char *McastAddr) //加入组播组
{
// Winsock1.0
	struct ip_mreq mcast; // Winsock1.0
//  memset(&mcast, 0, sizeof(struct ip_mreq));
	mcast.imr_multiaddr.s_addr = inet_addr(McastAddr);
	mcast.imr_interface.s_addr = INADDR_ANY;
	if (setsockopt(m_Socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mcast, sizeof(mcast)) == -1)
	{
		P_Debug("set multicast error.\n\r");
		return;
	}
#ifdef _DEBUG
	P_Debug("AddMultiGroup \n\r");
#endif
}
//---------------------------------------------------------------------------
void DropMultiGroup(int m_Socket, char *McastAddr) //退出组播组
{
// Winsock1.0
	struct ip_mreq mcast; // Winsock1.0
	char IP_Group[20];
	//查看是否在其它组播组内
	if (Local.IP_Group[0] != 0)
	{
		sprintf(IP_Group, "%d.%d.%d.%d\0", Local.IP_Group[0], Local.IP_Group[1], Local.IP_Group[2], Local.IP_Group[3]);
		Local.IP_Group[0] = 0; //组播地址
		Local.IP_Group[1] = 0;
		Local.IP_Group[2] = 0;
		Local.IP_Group[3] = 0;
		//  memset(&mcast, 0, sizeof(struct ip_mreq));
		mcast.imr_multiaddr.s_addr = inet_addr(IP_Group);
		mcast.imr_interface.s_addr = INADDR_ANY;
		if (setsockopt(m_Socket, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char*) &mcast, sizeof(mcast)) < 0)
		{
			P_Debug("drop multicast error.\n\r");
			return;
		}
	}
}
//---------------------------------------------------------------------------
void DropNsMultiGroup(int m_Socket, char *McastAddr) //退出NS组播组
{
// Winsock1.0
	struct ip_mreq mcast; // Winsock1.0

	//  memset(&mcast, 0, sizeof(struct ip_mreq));
	mcast.imr_multiaddr.s_addr = inet_addr(McastAddr);
	mcast.imr_interface.s_addr = inet_addr("172.16.9.231");//INADDR_ANY;
	if (setsockopt(m_Socket, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char*) &mcast, sizeof(mcast)) == -1)
	{
		P_Debug("drop multicast error.\n\r");
		return;
	}
}
//---------------------------------------------------------------------------
void RefreshNetSetup(int cType) //刷新网络设置  0 -- 未启动  1 -- 已启动
{
	/*char SystemOrder[100];
	//锁定互斥锁
	pthread_mutex_lock(&Local.udp_lock);
	//退出NS组播组
	if (cType == 1)
	{
		DropNsMultiGroup(m_VideoSocket, NSMULTIADDR);
		DropNsMultiGroup(m_DataSocket, NSMULTIADDR);
	}
	//设置MAC地址
	system("ifconfig eth0 down");
	sprintf(SystemOrder, "ifconfig eth0 hw ether %02X:%02X:%02X:%02X:%02X:%02X\0",
			LocalCfg.Mac_Addr[0], LocalCfg.Mac_Addr[1], LocalCfg.Mac_Addr[2],
			LocalCfg.Mac_Addr[3], LocalCfg.Mac_Addr[4], LocalCfg.Mac_Addr[5]);
	system(SystemOrder);
	system("ifconfig eth0 up");
	//设置IP地址和子网掩码
	sprintf(SystemOrder, "ifconfig eth0 %d.%d.%d.%d netmask %d.%d.%d.%d\0",
			LocalCfg.IP[0], LocalCfg.IP[1], LocalCfg.IP[2], LocalCfg.IP[3],
			LocalCfg.IP_Mask[0], LocalCfg.IP_Mask[1], LocalCfg.IP_Mask[2], LocalCfg.IP_Mask[3]);
	system(SystemOrder);
	//设置网关
	sprintf(SystemOrder, "route add default gw %d.%d.%d.%d\0",
			LocalCfg.IP_Gate[0], LocalCfg.IP_Gate[1], LocalCfg.IP_Gate[2], LocalCfg.IP_Gate[3]);
	system(SystemOrder);
	//加入NS组播组
	if (cType == 1)
	{
		AddMultiGroup(m_VideoSocket, NSMULTIADDR);
		AddMultiGroup(m_DataSocket, NSMULTIADDR);
	}
	//打开互斥锁
	pthread_mutex_unlock(&Local.udp_lock);*/
}
//---------------------------------------------------------------------------
void UdpDataRcvThread(void) //UDP接收线程函数
{
	/* 循环接收数据 */
//  int oldframeno=0;
	unsigned char send_b[1520];
	int sendlength;
	char FromIP[20];
	int newframeno;
	int currpackage;
	int i, j;
	int sub;
	short PackIsExist; //数据包已接收标志
	short FrameIsNew; //数据包是否是新帧的开始
	struct sockaddr_in c_addr;
	socklen_t addr_len;
	int len;
	int tmp;
	unsigned char buff[8096];

	char tmpAddr[21];
	int isAddrOK;
#ifdef _DEBUG
		P_Debug("This is udp pthread.\n");
#endif
	UdpRecvFlag = 1;

	addr_len = sizeof(c_addr);
	while (UdpRecvFlag == 1)
	{
		len = recvfrom(m_DataSocket, buff, sizeof(buff) - 1, 0, (struct sockaddr *) &c_addr, &addr_len);
		if (len < 0)
		{
			perror("recvfrom");
			exit(errno);
		}
		buff[len] = '\0';
#ifdef _DEBUG
		sprintf(Local.DebugInfo, "&&& RECV DATA &&& len=%d addr=%s:%d:%s\n\r", len, inet_ntoa(c_addr.sin_addr), ntohs(c_addr.sin_port), buff);
		P_Debug(Local.DebugInfo);
		sprintf(Local.DebugInfo, "&&& RECV DATA &&& [6]%d, [7]%d, [8]%d, [32]%d, [33]%d, [34]%d, [35]%d\n\r", buff[6], buff[7], buff[8], buff[32], buff[33], buff[34], buff[35]);
		P_Debug(Local.DebugInfo);
#endif
		strcpy(FromIP, inet_ntoa(c_addr.sin_addr));
		//if(DebugMode == 1)
		//  printf("FromIP is %s\n",FromIP);
		if ((buff[0] == UdpPackageHead[0]) && (buff[1] == UdpPackageHead[1]) && (buff[2] == UdpPackageHead[2])
				&& (buff[3] == UdpPackageHead[3]) && (buff[4] == UdpPackageHead[4]) && (buff[5] == UdpPackageHead[5]))
		{
			switch (buff[6])
			{
			case ALARM: //报警
				RecvAlarm_Func(buff, FromIP, len, m_DataSocket);
				break;
			case SENDMESSAGE: //信息
				RecvMessage_Func(buff, FromIP, len, m_DataSocket);
				break;
			case REPORTSTATUS: //设备定时报告状态
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
						P_Debug("len of device time report status reply is unusual \n");
				}
				break;
			case QUERYSTATUS: //管理中心查询设备状态
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
						P_Debug("len of administration center reach device status is unusual \n");
				}
				break;
			case WRITEADDRESS: //写地址设置
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
						P_Debug("len of write address is unusual \n");
				}
				break;
			case READADDRESS: //读地址设置
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
						P_Debug("len of read address set is unusual \n");
				}
				break;
			case SEARCHALLEQUIP: //252  //搜索所有设备（管理中心－＞设备）
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
						P_Debug("len of search all device is unusual \n");
				}
				break;
			case WRITEEQUIPADDR: //254  //写设备地址（管理中心－＞设备）
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
						P_Debug("len of write device address is unusual \n");
				}
				break;
			case WRITEHELP: //写帮助信息
				RecvWriteHelp_Func(buff, FromIP, len, m_DataSocket);
				break;
			case READHELP: //读帮助信息
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
						P_Debug("len of read help information is unusual \n");
				}
				break;
			case WRITESETUP: //写设置信息
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
						P_Debug("len of write setup information is unusual \n");
				}
				break;
			case READSETUP: //读设置信息
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
						P_Debug("len of read setup information is unusual \n");
				}
				break;
#ifdef _BRUSHIDCARD_SUPPORT   //刷卡功能支持
			case WRITEIDCARD: //写ID卡信息
				RecvWriteIDCard_Func(buff, FromIP, len, m_DataSocket);
				break;
			case READIDCARD: //读ID卡信息
				RecvReadIDCard_Func(buff, FromIP, len, m_DataSocket);
				break;
			case BRUSHIDCARD: //刷卡记录
				RecvBrushIDCard_Func(buff, FromIP, len, m_DataSocket);
				break;
#endif
//        case NSOrder:   //主机名解析（子网内广播）
			case NSSERVERORDER: //主机名解析(NS服务器)
				switch (buff[7])
				{
				case ASK: //解析
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
							P_Debug("len of resolved quest is unusual \n");
					}
					break;
				case REPLY: //解析回应
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
							P_Debug("len of reply resolved is unusual \n");
					}
					break;
				}
				break;
			case DOWNLOADFILE: //下载应用程序
				if (len >= (9 + sizeof(struct downfile1)))
				{
					RecvDownLoadFile_Func(buff, FromIP, len, m_DataSocket);
				}
				else
				{
					if (DebugMode == 1)
						P_Debug("len of download application is unusual \n");
				}
				break;
			case DOWNLOADIMAGE: //下载系统映像
				if (len >= (9 + sizeof(struct downfile1)))
				{
					RecvDownLoadImage_Func(buff, FromIP, len, m_DataSocket);
				}
				else
				{
					if (DebugMode == 1)
						P_Debug("len of download system zImage is unusual \n");
				}
				break;
#ifdef _CAPTUREPIC_TO_CENTER  //捕获图片传到中心
			case CAPTUREPIC_SEND_START: //单元门口机、围墙机、二次门口机发送呼叫照片->发送开始
				if (len >= 67)
				{
					RecvCapture_Send_Start_Func(buff, FromIP, len, m_DataSocket);
				}
				else
				{
					P_Debug("len of capture send start is unusual \n");
				}
				break;
			case CAPTUREPIC_SEND_DATA: //单元门口机、围墙机、二次门口机发送呼叫照片->发送数据
				if (len >= 58)
				{
					RecvCapture_Send_Data_Func(buff, FromIP, len, m_DataSocket);
				}
				else
				{
					P_Debug("len of capture send data is unusual \n");
				}
				break;
			case CAPTUREPIC_SEND_SUCC: //单元门口机、围墙机、二次门口机发送呼叫照片->发送结束（成功）
				if (len >= 42)
				{
					RecvCapture_Send_Succ_Func(buff, FromIP, len, m_DataSocket);
				}
				else
				{
#ifdef _DEBUG
					P_Debug("len of caputre send succeed is unusual \n");
#endif
				}
				break;
			case CAPTUREPIC_SEND_FAIL: //单元门口机、围墙机、二次门口机发送呼叫照片->发送结束（失败）
				if (len >= 42)
				{
					RecvCapture_Send_Fail_Func(buff, FromIP, len, m_DataSocket);
				}
				else
				{
#ifdef _DEBUG
					P_Debug("len of capture send fail is unusual \n");
#endif
				}
				break;
#endif
			case REMOTEDEBUGINFO: //发送远程调试信息
				if (len == 29)
				{
					RecvRemoteDebugInfo_Func(buff, FromIP, len, m_DataSocket);
				}
				else
				{
#ifdef _DEBUG
					P_Debug("len of remote bug information is unusual \n");
#endif
				}
				break;
#ifdef _REMOTECALLTEST  //远程呼叫测试
				case REMOTETEST: //接收远程呼叫测试
				if(len == 49)
				{
					RecvRemoteCallTest_Func(buff, FromIP, len, m_DataSocket);
				}
				else
				{
#ifdef _DEBUG
					P_Debug("接收远程呼叫测试长度异常\n");
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
//报警
void RecvAlarm_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket)
{
	int i, j;
	int isAddrOK;
	i = 0;
	isAddrOK = 1;
	for (j = 8; j < 8 + Local.AddrLen; j++)
	{
		if (LocalCfg.Addr[j - 8] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	}
	//地址匹配
	if (isAddrOK == 1)
	{
		//锁定互斥锁
		pthread_mutex_lock(&Local.udp_lock);
		if ((recv_buf[7] & 0x03) == REPLY) //应答
		{
			for (i = 0; i < UDPSENDMAX; i++)
			{
				if (Multi_Udp_Buff[i].isValid == 1)
				{
					if (Multi_Udp_Buff[i].m_Socket == m_DataSocket)
					{
						if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM)
						{
							if (Multi_Udp_Buff[i].buf[6] == ALARM)
							{
								if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK)
								{
									if (strcmp(Multi_Udp_Buff[i].RemoteHost, cFromIP) == 0)
									{
										Multi_Udp_Buff[i].isValid = 0;
										if (DebugMode == 1)
											P_Debug("receive alarm reply \n");
										break;
									}
								}
							}
						}
					}
				}
			}
		}
		//打开互斥锁
		pthread_mutex_unlock(&Local.udp_lock);
	}
}
//---------------------------------------------------------------------------
//信息
void RecvMessage_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket)
{

}
//---------------------------------------------------------------------------
//设备定时报告状态
void RecvReportStatus_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	//时钟
	time_t t;

	i = 0;
	isAddrOK = 1;
	for (j = 8; j < 8 + Local.AddrLen; j++)
	{
		if (LocalCfg.Addr[j - 8] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	}
	//地址匹配
	if (isAddrOK == 1)
	{
		//锁定互斥锁
		pthread_mutex_lock(&Local.udp_lock);
		if ((recv_buf[7] & 0x03) == REPLY) //应答
		{
			for (i = 0; i < UDPSENDMAX; i++)
			{
				if (Multi_Udp_Buff[i].isValid == 1)
				{
					if (Multi_Udp_Buff[i].m_Socket == m_DataSocket)
					{
						if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM)
						{
							if (Multi_Udp_Buff[i].buf[6] == REPORTSTATUS)
							{
								if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK)
								{
									if (strcmp(Multi_Udp_Buff[i].RemoteHost, cFromIP) == 0)
									{
										Multi_Udp_Buff[i].isValid = 0;
#ifdef _CAPTUREPIC_TO_CENTER  //捕获图片传到中心
										Local.ConnectCentered = 1; //与中心连接状态
#endif
										if (((recv_buf[29] << 8) + recv_buf[28]) != LocalCfg.ReportTime)
										{
											LocalCfg.ReportTime = (recv_buf[29] << 8) + recv_buf[28];
											//Save_Setup(); //向Flash中存储文件
										}
										//校准时钟
										if (((curr_tm_t->tm_year + 1900) != ((recv_buf[30] << 8) + recv_buf[31])) || ((curr_tm_t->tm_mon + 1) != recv_buf[32])
												|| (curr_tm_t->tm_mday != recv_buf[33]) || (curr_tm_t->tm_hour != recv_buf[34])
												|| (curr_tm_t->tm_min != recv_buf[35]) || (curr_tm_t->tm_sec != recv_buf[36]))
										{
											curr_tm_t->tm_year = (recv_buf[30] << 8) + recv_buf[31] - 1900;
											curr_tm_t->tm_mon = recv_buf[32] - 1;
											curr_tm_t->tm_mday = recv_buf[33];
											curr_tm_t->tm_hour = recv_buf[34];
											curr_tm_t->tm_min = recv_buf[35];
											curr_tm_t->tm_sec = recv_buf[36];
											t = mktime(curr_tm_t);
											//stime((long*) &t);
										}
										//天气预报
										if ((Local.Weather[0] != recv_buf[37]) || (Local.Weather[1] != recv_buf[38]) || (Local.Weather[2] != recv_buf[39]))
										{
											Local.Weather[0] = recv_buf[37];
											Local.Weather[1] = recv_buf[38];
											Local.Weather[2] = recv_buf[39];
											if ((Local.Weather[0] == 0) && (Local.Weather[1] == 0) && (Local.Weather[2] == 0))
											{
											}
											else
											{
											}
										}
										//if(DebugMode == 1)
										//  P_Debug("收到设备定时报告状态应答\n");
										break;
									}
								}
							}
						}
					}
				}
			}
		}
		//打开互斥锁
		pthread_mutex_unlock(&Local.udp_lock);
	}
}
//---------------------------------------------------------------------------
//管理中心查询设备状态
void RecvQueryStatus_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;

	i = 0;
	isAddrOK = 1;
	for (j = 8; j < 8 + Local.AddrLen; j++)
	{
		if (LocalCfg.Addr[j - 8] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	}
	//地址匹配
	if (isAddrOK == 1)
	{
#ifdef _CAPTUREPIC_TO_CENTER  //捕获图片传到中心
		Local.ConnectCentered = 1; //与中心连接状态
#endif
		memcpy(send_b, UdpPackageHead, 6);
		send_b[6] = QUERYSTATUS;
		send_b[7] = REPLY; //应答
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
//写地址设置
void RecvWriteAddress_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket)
{
	int i, j;
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
	//地址匹配
	if (isAddrOK == 1)
	{
		memcpy(send_b, recv_buf, length);
		send_b[7] = REPLY; //应答
		sendlength = length;
		if (m_Socket == m_DataSocket)
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);
		if (m_Socket == m_VideoSocket)
			UdpSendBuff(m_Socket, cFromIP, RemoteVideoPort, send_b, sendlength);

		//写地址设置
		if (recv_buf[28] & 0x01) //地址编码
			memcpy(LocalCfg.Addr, recv_buf + 30, 20);
		if (recv_buf[28] & 0x02) //网卡地址
		{
			memcpy(LocalCfg.Mac_Addr, recv_buf + 50, 6);
		}
		if (recv_buf[28] & 0x04) //IP地址
		{
			memcpy(LocalCfg.IP, recv_buf + 56, 4);
		}
		if (recv_buf[28] & 0x08) //子网掩码
		{
			memcpy(LocalCfg.IP_Mask, recv_buf + 60, 4);
		}
		if (recv_buf[28] & 0x10) //网关地址
		{
			memcpy(LocalCfg.IP_Gate, recv_buf + 64, 4);
		}
		if (recv_buf[28] & 0x20) //服务器地址
			memcpy(LocalCfg.IP_Server, recv_buf + 68, 4);

		//Save_Setup(); //向Flash中存储文件
		RefreshNetSetup(1); //刷新网络设置
	}
}
//---------------------------------------------------------------------------
//读地址设置
void RecvReadAddress_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket)
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
	//地址匹配
	if (isAddrOK == 1)
	{
		memcpy(send_b, recv_buf, length);
		send_b[7] = REPLY; //应答

		send_b[28] = 0;
		send_b[29] = 0;

		//地址编码
		memcpy(send_b + 30, LocalCfg.Addr, 20);
		//网卡地址
		memcpy(send_b + 50, LocalCfg.Mac_Addr, 6);
		//IP地址
		memcpy(send_b + 56, LocalCfg.IP, 4);
		//子网掩码
		memcpy(send_b + 60, LocalCfg.IP_Mask, 4);
		//网关地址
		memcpy(send_b + 64, LocalCfg.IP_Gate, 4);
		//服务器地址
		memcpy(send_b + 68, LocalCfg.IP_Server, 4);

		sendlength = 72;
		if (m_Socket == m_DataSocket)
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);
		if (m_Socket == m_VideoSocket)
			UdpSendBuff(m_Socket, cFromIP, RemoteVideoPort, send_b, sendlength);
	}
}
//---------------------------------------------------------------------------
//搜索所有设备应答
void RecvSearchAllEquip_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket)
{
	int i, j;
	int newlength;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;

	i = 0;
	isAddrOK = 1;
	P_Debug("RecvSearchAllEquip_Func1\n");
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

	//地址匹配
	if (isAddrOK == 1)
	{
		P_Debug("RecvSearchAllEquip_Func2\n");

		memcpy(send_b, recv_buf, length);
		send_b[7] = REPLY; //应答

		send_b[8] = 0x05;
		send_b[9] = 0;

		//地址编码
		memcpy(send_b + 10, LocalCfg.Addr, 20);
		//网卡地址
		memcpy(send_b + 30, LocalCfg.Mac_Addr, 6);
		//IP地址
		memcpy(send_b + 36, LocalCfg.IP, 4);
		//子网掩码
		memcpy(send_b + 40, LocalCfg.IP_Mask, 4);
		//网关地址
		memcpy(send_b + 44, LocalCfg.IP_Gate, 4);
		//服务器地址
		memcpy(send_b + 48, LocalCfg.IP_Server, 4);
		//版本号
		memcpy(send_b + 52, SERIALNUM, 16);

		sendlength = 68;
		if (m_Socket == m_DataSocket)
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);
		if (m_Socket == m_VideoSocket)
			UdpSendBuff(m_Socket, cFromIP, RemoteVideoPort, send_b, sendlength);
	}
}
//---------------------------------------------------------------------------
//写设备地址应答
void RecvWriteEquipAddr_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket)
{
	int i, j;
	int newlength;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;

	if (recv_buf[7] == REPLY
	)
		return;

	P_Debug("RecvWriteEquipAddr_Func \n");

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
	//地址匹配
	if (isAddrOK == 1)
	{
		memcpy(send_b, recv_buf, length);
		send_b[7] = REPLY; //应答
		sendlength = length;
		if (m_Socket == m_DataSocket)
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);
		if (m_Socket == m_VideoSocket)
			UdpSendBuff(m_Socket, cFromIP, RemoteVideoPort, send_b, sendlength);

		//写地址设置
		memcpy(LocalCfg.Addr, recv_buf + 52, 20);
		memcpy(LocalCfg.Mac_Addr, recv_buf + 72, 6);
		memcpy(LocalCfg.IP, recv_buf + 78, 4);
		memcpy(LocalCfg.IP_Mask, recv_buf + 82, 4);
		memcpy(LocalCfg.IP_Gate, recv_buf + 86, 4);
		memcpy(LocalCfg.IP_Server, recv_buf + 90, 4);

		//Save_Setup(); //向Flash中存储文件
		RefreshNetSetup(1); //刷新网络设置
	}
}
//---------------------------------------------------------------------------
//写帮助信息
void RecvWriteHelp_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket)
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
	//地址匹配
	if (isAddrOK == 1)
	{
		memcpy(send_b, recv_buf, length);
		send_b[7] = REPLY; //应答
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

		//Save_Setup(); //向Flash中存储文件
	}
}
//---------------------------------------------------------------------------
//读帮助信息
void RecvReadHelp_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket)
{
	int i, j;
	short helplength;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;

	i = 0;
	isAddrOK = 1;
	for (j = 8; j < 8 + Local.AddrLen; j++)
	{
		if (LocalCfg.Addr[j - 8] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	}
	//地址匹配
	if (isAddrOK == 1)
	{
		memcpy(send_b, UdpPackageHead, 6);
		send_b[6] = READHELP;
		send_b[7] = REPLY; //应答
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
//写设置信息
void RecvWriteSetup_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket)
{
	int i, j;
	int newlength;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;

	i = 0;
	isAddrOK = 1;
	for (j = 8; j < 8 + Local.AddrLen; j++)
	{
		if (LocalCfg.Addr[j - 8] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	}
	//地址匹配
	if (isAddrOK == 1)
	{
		memcpy(send_b, recv_buf, length);
		send_b[7] = REPLY; //应答
		sendlength = length;
		if (m_Socket == m_DataSocket)
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);
		if (m_Socket == m_VideoSocket)
			UdpSendBuff(m_Socket, cFromIP, RemoteVideoPort, send_b, sendlength);

		//写设置信息
		if (recv_buf[28] & 0x01) //电锁类型
			LocalCfg.LockType = recv_buf[30];
		if (recv_buf[28] & 0x02) //开锁时间
			LocalCfg.OpenLockTime = recv_buf[31];

		//Save_Setup(); //向Flash中存储文件
	}
}
//---------------------------------------------------------------------------
//读设置信息
void RecvReadSetup_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;

	i = 0;
	isAddrOK = 1;
	for (j = 8; j < 8 + Local.AddrLen; j++)
	{
		if (LocalCfg.Addr[j - 8] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	}
	//地址匹配
	if (isAddrOK == 1)
	{
		memcpy(send_b, recv_buf, length);
		send_b[7] = REPLY; //应答

		send_b[28] = 0;
		send_b[29] = 0;

		//电锁类型
		send_b[30] = LocalCfg.LockType;
		//开锁时间
		send_b[31] = LocalCfg.OpenLockTime;

		sendlength = 54;
		if (m_Socket == m_DataSocket)
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);
		if (m_Socket == m_VideoSocket)
			UdpSendBuff(m_Socket, cFromIP, RemoteVideoPort, send_b, sendlength);
	}
}
//---------------------------------------------------------------------------
#ifdef _BRUSHIDCARD_SUPPORT   //刷卡功能支持
//写ID卡信息
void RecvWriteIDCard_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket)
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
	//读写ID卡数据结构
	struct iddata1 iddata;

	i = 0;
	isAddrOK = 1;
	for (j = 8; j < 8 + Local.AddrLen; j++)
	{
		if (LocalCfg.Addr[j - 8] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	}
	sprintf(Local.DebugInfo, "isAddrOK = %d\n", isAddrOK);
	P_Debug(Local.DebugInfo);
	//地址匹配
	if (isAddrOK == 1)
	{
		memcpy(send_b, recv_buf, length);
		send_b[7] = REPLY; //应答
		sendlength = length;
		if (m_Socket == m_DataSocket)
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);
		if (m_Socket == m_VideoSocket)
			UdpSendBuff(m_Socket, cFromIP, RemoteVideoPort, send_b, sendlength);

		memcpy(&iddata, recv_buf + 8, sizeof(iddata));
		//printf("RecvIDCardNo.isNewWriteFlag = %d, iddata.serialnum = %d, IDCardNo.serialnum = %d\n",
		//        RecvIDCardNo.isNewWriteFlag, iddata.serialnum, IDCardNo.serialnum);

		//查看是否在写ID卡信息状态
		if (RecvIDCardNo.isNewWriteFlag == 1)
		{
			//上次写未完成，新开始写
			if (iddata.serialnum != RecvIDCardNo.serialnum)
			{
				RecvIDCardNo.isNewWriteFlag = 1;
				//卡数量
				RecvIDCardNo.Num = iddata.Num;
				//总包数
				RecvIDCardNo.PackNum = iddata.TotalPackage;
				//包已接收标志
				for (i = 0; i < RecvIDCardNo.PackNum; i++)
					RecvIDCardNo.PackIsRecved[i] = 0;
				//序号
				RecvIDCardNo.serialnum = iddata.serialnum;
			}
			//当前数据包卡数量
			if (iddata.CurrNum > CARDPERPACK
			)
				iddata.CurrNum = CARDPERPACK;
			//当前包
			if (iddata.CurrPackage > RecvIDCardNo.PackNum)
				iddata.CurrPackage = RecvIDCardNo.PackNum;
			RecvIDCardNo.PackIsRecved[iddata.CurrPackage - 1] = 1;
			memcpy(RecvIDCardNo.SN + CARDPERPACK * (iddata.CurrPackage - 1) * BYTEPERSN,
					recv_buf + 48, BYTEPERSN * iddata.CurrNum);

			//printf("RecvIDCardNo.PackNum = %d, iddata.CurrPackage = %d, iddata.serialnum = %d\n", RecvIDCardNo.PackNum, iddata.CurrPackage, iddata.serialnum);
			//包已接收标志，查看是否全部接收
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
			//卡数量
			RecvIDCardNo.Num = iddata.Num;
			//总包数
			RecvIDCardNo.PackNum = iddata.TotalPackage;
			//包已接收标志
			for (i = 0; i < RecvIDCardNo.PackNum; i++)
				RecvIDCardNo.PackIsRecved[i] = 0;
			//序号
			RecvIDCardNo.serialnum = iddata.serialnum;

			RecvIDCardNo.PackIsRecved[iddata.CurrPackage - 1] = 1;
			memcpy(
					RecvIDCardNo.SN
							+ CARDPERPACK * (iddata.CurrPackage - 1) * BYTEPERSN
							, recv_buf + 48, BYTEPERSN * iddata.CurrNum);

			//printf("iddata.CurrPackage = %d\n", iddata.CurrPackage);
			//printf("RecvIDCardNo.PackNum1 = %d\n", RecvIDCardNo.PackNum);
			//包已接收标志，查看是否全部接收
			RecvOk = 1;
			for (i = 0; i < RecvIDCardNo.PackNum; i++)
			{
				if (RecvIDCardNo.PackIsRecved[i] == 0)
				{
					RecvOk = 0;
					break;
				}
			}
			sprintf(Local.DebugInfo, "RecvIDCardNo.PackNum = %d, iddata.Num = %d\n", RecvIDCardNo.PackNum, iddata.Num);
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
//存储ID卡信息
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
	//锁定互斥锁
	pthread_mutex_lock(&Local.save_lock);
	//查找可用存储缓冲并填空
	for (i = 0; i < SAVEMAX; i++)
	{
		if (Save_File_Buff[i].isValid == 0)
		{
			Save_File_Buff[i].Type = 5;
			Save_File_Buff[i].isValid = 1;
			sem_post(&save_file_sem);
			break;
		}
	}
	//打开互斥锁
	pthread_mutex_unlock(&Local.save_lock);

}
//---------------------------------------------------------------------------
//读ID卡信息
void RecvReadIDCard_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket)
{
	int i, j;
	int newlength;
	int isAddrOK;
	i = 0;
	isAddrOK = 1;
	for (j = 8; j < 8 + Local.AddrLen; j++)
	{
		if (LocalCfg.Addr[j - 8] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	}
	sprintf(Local.DebugInfo, "isAddrOK = %d\n", isAddrOK);
	P_Debug(Local.DebugInfo);
	//地址匹配
	if (isAddrOK == 1)
	{
		//锁定互斥锁
		pthread_mutex_lock(&Local.dispart_send_lock);
		//查找可用存储缓冲并填空
		for (i = 0; i < DISPARTMAX; i++)
		{
			if (Dispart_Send_Buff[i].isValid == 0)
			{
				Dispart_Send_Buff[i].Type = 10;
				strcpy(Dispart_Send_Buff[i].cFromIP, cFromIP);
				Dispart_Send_Buff[i].isValid = 1;
				break;
			}
		}
		//打开互斥锁
		pthread_mutex_unlock(&Local.dispart_send_lock);
		sem_post(&dispart_send_sem);
	}
}
//---------------------------------------------------------------------------
//刷卡记录
void RecvBrushIDCard_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket)
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
	{
		if (LocalCfg.Addr[j - 8] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	}
	//地址匹配
	if (isAddrOK == 1)
	{
		//锁定互斥锁
		pthread_mutex_lock(&Local.udp_lock);
		if ((recv_buf[7] & 0x03) == REPLY) //应答
		{
			for (i = 0; i < UDPSENDMAX; i++)
			{
				if (Multi_Udp_Buff[i].isValid == 1)
				{
					if (Multi_Udp_Buff[i].m_Socket == m_DataSocket)
					{
						if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM)
						{
							if (Multi_Udp_Buff[i].buf[6] == BRUSHIDCARD)
							{
								if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK)
								{
									if (strcmp(Multi_Udp_Buff[i].RemoteHost, cFromIP) == 0)
									{
										tmpnum = (recv_buf[29] << 8) + recv_buf[28];
										if (tmpnum <= BrushIDCard.Num)
										{
											//锁定互斥锁
											pthread_mutex_lock(&Local.idcard_lock);
											memcpy(tmpbrushinfo, BrushIDCard.Info + tmpnum * 11,
													(IDCARDBRUSHNUM - tmpnum) * 11);
											memcpy(BrushIDCard.Info, tmpbrushinfo, (IDCARDBRUSHNUM - tmpnum) * 11);
											BrushIDCard.Num -= tmpnum;
											//打开互斥锁
											pthread_mutex_unlock(&Local.idcard_lock);
										}
										if (BrushIDCard.Num == 0)
											Multi_Udp_Buff[i].isValid = 0;
										else
										{
											//数量
											if (BrushIDCard.Num > 120)
												tmpnum = 120;
											else
												tmpnum = BrushIDCard.Num;
											Multi_Udp_Buff[i].buf[29] = (tmpnum & 0xFF00) >> 8;
											Multi_Udp_Buff[i].buf[28] = (tmpnum & 0x00FF);

											memcpy(Multi_Udp_Buff[i].buf + 30, BrushIDCard.Info, 11 * tmpnum);
											Multi_Udp_Buff[i].nlength = 30 + 11 * tmpnum;
											Multi_Udp_Buff[i].DelayTime = 100;
											Multi_Udp_Buff[i].SendDelayTime = 0; //发送等待时间计数  20101112
											Multi_Udp_Buff[i].isValid = 1;
											sem_post(&multi_send_sem);
										}
#ifdef _DEBUG
										P_Debug("receive BrushIDCard record reply \n");
#endif
										break;
									}
								}
							}
						}
					}
				}
			}
		}
		//打开互斥锁
		pthread_mutex_unlock(&Local.udp_lock);
	}
}
#endif
//---------------------------------------------------------------------------
//下载应用程序
void RecvDownLoadFile_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket)
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
	if ((strcmp(DownData.FlagText, FLAGTEXT) == 0) && (strcmp(DownData.FileName, filename) == 0))
	{
		switch (recv_buf[8])
		{
		case STARTDOWN: //开始下载
			memcpy(send_b, recv_buf, length);
			send_b[7] = REPLY; //应答
			sendlength = length;
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);

			Local.DownProgramOK = 0; //下载应用程序完成
			sprintf(Local.DebugInfo, "start download, DownData.Filelen = %d\n", DownData.Filelen);
			P_Debug(Local.DebugInfo);
			if (downbuf == NULL)
				downbuf = (unsigned char *) malloc(DownData.Filelen);
			for (i = 0; i < DownData.TotalPackage; i++)
				downloaded_flag[i] = 0;
			break;
		case DOWN: //下载
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
			send_b[7] = REPLY; //应答
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);
			break;
		case DOWNFINISHONE: //下载完成一个
			memcpy(send_b, recv_buf, length);
			send_b[7] = REPLY; //应答
			sendlength = length;
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);

			P_Debug("download finish one \n");
			DownOK = 1;
			for (i = 0; i < DownData.TotalPackage; i++)
			{
				if (downloaded_flag[i] == 0)
				{
					sprintf(Local.DebugInfo, "lost data packet，i = %d\n", i);
					P_Debug(Local.DebugInfo);
					DownOK = 0;
				}
			}
			if (DownOK == 1)
			{
				if (Local.DownProgramOK == 0)
				{
					Local.DownProgramOK = 1; //下载应用程序完成
					sprintf(picname, "/mnt/mtd/%s\0", DownData.FileName);
					if ((pic_fd = fopen(picname, "wb")) == NULL
					)
						P_Debug("don't create application file \n");
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
				SendUdpOne(DOWNLOADIMAGE, DOWNFAIL, 0, cFromIP); //失败
			}
			break;
		case STOPDOWN: //停止下载
			memcpy(send_b, recv_buf, length);
			send_b[7] = REPLY; //应答
			sendlength = length;
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);

			P_Debug("stop download \n");
			if (downbuf != NULL)
			{
				free(downbuf);
				downbuf = NULL;
			}
			break;
		case DOWNFINISHALL: //全部完成下载
			memcpy(send_b, recv_buf, length);
			send_b[7] = REPLY; //应答
			sendlength = length;
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);

			P_Debug("download finish\n");
			break;
		}
	}
}
//---------------------------------------------------------------------------
//下载系统映像
void RecvDownLoadImage_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket)
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
	if ((strcmp(DownData.FlagText, FLAGTEXT) == 0) && (strcmp(DownData.FileName, filename) == 0))
	{
		switch (recv_buf[8])
		{
		case STARTDOWN: //开始下载
			memcpy(send_b, recv_buf, length);
			send_b[7] = REPLY; //应答
			sendlength = length;
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);

			sprintf(Local.DebugInfo, "start download, DownData.Filelen = %d\n", DownData.Filelen);
			P_Debug(Local.DebugInfo);
			if (downbuf == NULL)
				downbuf = (unsigned char *) malloc(DownData.Filelen);
			for (i = 0; i < DownData.TotalPackage; i++)
				downloaded_flag[i] = 0;
			break;
		case DOWN: //下载
			downloaded_flag[DownData.CurrPackage] = 1;
			memcpy(downbuf + DownData.CurrPackage * PackDataLen,
					recv_buf + 9 + sizeof(struct downfile1), DownData.Datalen);

			/*if(DownData.CurrPackage != 0)
			 if(DownData.CurrPackage != (OldPackage + 1))
			 printf("CurrPackage = %d, package lost %d, length = %d\n", DownData.CurrPackage , OldPackage + 1, length);
			 if(OldPackage != DownData.CurrPackage)
			 OldPackage = DownData.CurrPackage;  */
			memcpy(send_b, recv_buf, length);
			send_b[7] = REPLY; //应答
			sendlength = 9 + sizeof(struct downfile1);
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);
			break;
		case DOWNFINISHONE: //下载完成一个
			memcpy(send_b, recv_buf, length);
			send_b[7] = REPLY; //应答
			sendlength = length;
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);

			P_Debug("download finish one \n");
			DownOK = 1;
			for (i = 0; i < DownData.TotalPackage; i++)
			{
				if (downloaded_flag[i] == 0)
				{
					sprintf(Local.DebugInfo, "lost data packet，i = %d\n", i);
					P_Debug(Local.DebugInfo);
					DownOK = 0;
				}
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
					pthread_create(&download_image_thread, &attr, (void *) download_image_thread_func, NULL);
					pthread_attr_destroy(&attr);
					if (download_image_thread == 0)
					{
						P_Debug("don't download image thread \n");
						free(downbuf);
						downbuf = NULL;
					}
				}
				else
					P_Debug("recreate download image thread \n");
			}
			else
			{
				free(downbuf);
				downbuf = NULL;
				SendUdpOne(DOWNLOADIMAGE, DOWNFAIL, 0, cFromIP); //失败
			}
			break;
		case STOPDOWN: //停止下载
			memcpy(send_b, recv_buf, length);
			send_b[7] = REPLY; //应答
			sendlength = length;
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);

			P_Debug("stop download \n");
			if (downbuf != NULL)
			{
				free(downbuf);
				downbuf = NULL;
			}
			break;
		case DOWNFINISHALL: //全部完成下载
			memcpy(send_b, recv_buf, length);
			send_b[7] = REPLY; //应答
			sendlength = length;
			UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);

			P_Debug("download finish \n");
			break;
		}
	}
}
//---------------------------------------------------------------------------
//系统映像下载线程
void download_image_thread_func(void)
{
	FILE *image_fd;
	char SysOrder[100];

	if (DebugMode == 1)
		P_Debug("create download image thread \n");

	if ((image_fd = fopen(mtdimage_name, "wb")) == NULL
	)
		P_Debug("don't create image file \n");
	else
	{
		fwrite(downbuf, downbuflen, 1, image_fd);
		fclose(image_fd);
	}
	free(downbuf);
	downbuf = NULL;
#if 1
	SendUdpOne(DOWNLOADIMAGE, WRITEFLASH, 20, downip); //正在写Flash
	sprintf(SysOrder, "/bin/gm_spi_write -x %s /dev/mtd0\0", mtdimage_name);
	system(SysOrder);
	//flashcp(downbuf, downbuflen, downip);
#else
	system("sync");
	gm_spi_write(mtdimage_name, "/dev/mtd0", downip);
#endif

	Local.download_image_flag = 0;

	if (DebugMode == 1)
		P_Debug("end download image thread \n");

	SendUdpOne(DOWNLOADIMAGE, ENDFLASH, 0, downip); //完成写Image

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
	send_b[7] = ASK; //主叫
	send_b[8] = SonOrder; //子命令
	memcpy(send_b + 9, FLAGTEXT, 20); //标志
	send_b[29] = PerCent;
	sendlength = 30;
	UdpSendBuff(m_DataSocket, cFromIP, RemoteDataPort, send_b, sendlength);
}
//---------------------------------------------------------------------------
//发送远程调试信息
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
		send_b[7] = REPLY; //应答
		sendlength = length;
		UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);
		switch (recv_buf[8])
		{
		case 0: //停止
			Local.RemoteDebugInfo = 0; //发送远程调试信息
			P_Debug("send remote debug information, stop \n");
			break;
		case 1: //开始
			Local.RemoteDebugInfo = 1; //发送远程调试信息
			strcpy(Local.DebugIP, cFromIP);
			sprintf(Local.DebugInfo, "send remote debug information, start, %s, Local.DebugIP = %s\n", LocalCfg.Addr, Local.DebugIP);
			P_Debug(Local.DebugInfo);
			break;
		}
	}
}
//---------------------------------------------------------------------------
#ifdef _REMOTECALLTEST  //远程呼叫测试
//接收远程呼叫测试
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
		printf("接收远程呼叫测试 %s, %04d-%02d-%02d %02d:%02d:%02d\n", RemoteAddr,
				curr_tm_t->tm_year + 1900, curr_tm_t->tm_mon + 1, curr_tm_t->tm_mday, curr_tm_t->tm_hour, curr_tm_t->tm_min, curr_tm_t->tm_sec);
		if(Local.Status == 0)
		{
			Call_Func(2, RemoteAddr); //呼叫住户
		}
	}
}
#endif
//---------------------------------------------------------------------------
#ifdef _CAPTUREPIC_TO_CENTER  //捕获图片传到中心
//单元门口机、围墙机、二次门口机发送呼叫照片->发送开始
void RecvCapture_Send_Start_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket)
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
	{
		if (LocalCfg.Addr[j - 8] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	}
	//地址匹配
	if (isAddrOK == 1)
	{
		//锁定互斥锁
		pthread_mutex_lock(&Local.udp_lock);
		if ((recv_buf[7] & 0x03) == REPLY) //应答
		{
			for (i = 0; i < UDPSENDMAX; i++)
			{
				if (Multi_Udp_Buff[i].isValid == 1)
				{
					if (Multi_Udp_Buff[i].m_Socket == m_DataSocket)
					{
						if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM)
						{
							if (Multi_Udp_Buff[i].buf[6] == CAPTUREPIC_SEND_START)
							{
								if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK)
								{
									if (strcmp(Multi_Udp_Buff[i].RemoteHost, cFromIP) == 0)
									{
										Multi_Udp_Buff[i].isValid = 0;

										Start_OK = 1;
										memcpy(&CurrNo, recv_buf + 48, sizeof(CurrNo));

#ifdef _DEBUG
										P_Debug("receive reply of capture send start \n");
#endif
										break;
									}
								}
							}
						}
					}
				}
			}
		}
		//打开互斥锁
		pthread_mutex_unlock(&Local.udp_lock);
		if (Start_OK == 1)
			SendCapturePicData(CurrNo);
	}
}
//---------------------------------------------------------------------------
//单元门口机、围墙机、二次门口机发送呼叫照片->发送数据
void RecvCapture_Send_Data_Func(unsigned char *recv_buf, char *cFromIP,
		int length, int m_Socket)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int Finish_OK;
	int CurrNo;
	short CurrPack;

	i = 0;
	isAddrOK = 1;
	for (j = 8; j < 8 + Local.AddrLen; j++)
	{
		if (LocalCfg.Addr[j - 8] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	}
	//printf("RecvCapture_Send_Data_Func   isAddrOK = %d\n", isAddrOK);
	//地址匹配
	if (isAddrOK == 1)
	{
		//锁定互斥锁
		pthread_mutex_lock(&Local.udp_lock);
		if ((recv_buf[7] & 0x03) == REPLY) //应答
		{
			for (i = 0; i < UDPSENDMAX; i++)
			{
				if (Multi_Udp_Buff[i].isValid == 1)
				{
					if (Multi_Udp_Buff[i].m_Socket == m_DataSocket)
					{
						if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM)
						{
							if (Multi_Udp_Buff[i].buf[6] == CAPTUREPIC_SEND_DATA)
							{
								if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK)
								{
									if ((Multi_Udp_Buff[i].buf[56] == recv_buf[56]) && (Multi_Udp_Buff[i].buf[57] == recv_buf[57]))
									{
										if (strcmp(Multi_Udp_Buff[i].RemoteHost, cFromIP) == 0)
										{
											Multi_Udp_Buff[i].isValid = 0;

											memcpy(&CurrNo, recv_buf + 48, sizeof(CurrNo));
											memcpy(&CurrPack, recv_buf + 56, sizeof(CurrPack));
											if (CurrPack >= 2000)
											{
												sprintf(Local.DebugInfo, "CurrPack = %d\n", CurrPack);
												P_Debug(Local.DebugInfo);
												CurrPack = 0;
											}
											Capture_Send_Flag[CurrPack] = 1;

#ifdef _DEBUG
											//sprintf(Local.DebugInfo, "收到呼叫照片->发送数据应答, %d\n", CurrPack);
											//P_Debug(Local.DebugInfo);
#endif
											break;
										}
									}
								}
							}
						}
					}
				}
			}
		}
		//打开互斥锁
		pthread_mutex_unlock(&Local.udp_lock);

		Finish_OK = 1;
		for (j = 0; j < Capture_Total_Package; j++)
		{
			if (Capture_Send_Flag[j] == 0)
			{
				Finish_OK = 0;
				break;
			}
		}
		if (Finish_OK == 1)
			SendCapturePicFinish(CurrNo, 1);
	}
}
//---------------------------------------------------------------------------
//单元门口机、围墙机、二次门口机发送呼叫照片->发送结束（成功）
void RecvCapture_Send_Succ_Func(unsigned char *recv_buf, char *cFromIP,
		int length, int m_Socket)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int Start_OK;
	int CurrNo;
	Start_OK = 0;

	i = 0;
	isAddrOK = 1;
	for (j = 8; j < 8 + Local.AddrLen; j++)
	{
		if (LocalCfg.Addr[j - 8] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	}
	//地址匹配
	if (isAddrOK == 1)
	{
		//锁定互斥锁
		pthread_mutex_lock(&Local.udp_lock);
		if ((recv_buf[7] & 0x03) == REPLY) //应答
		{
			for (i = 0; i < UDPSENDMAX; i++)
			{
				if (Multi_Udp_Buff[i].isValid == 1)
				{
					if (Multi_Udp_Buff[i].m_Socket == m_DataSocket)
					{
						if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM)
						{
							if (Multi_Udp_Buff[i].buf[6] == CAPTUREPIC_SEND_SUCC)
							{
								if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK)
								{
									if (strcmp(Multi_Udp_Buff[i].RemoteHost, cFromIP) == 0)
									{
										Multi_Udp_Buff[i].isValid = 0;

										memcpy(&CurrNo, recv_buf + 48, sizeof(CurrNo));
										Capture_Pic_Center[CurrNo].isVilid = 0;
										if (Capture_Pic_Center[CurrNo].jpeg_pic != NULL)
										{
											free(Capture_Pic_Center[CurrNo].jpeg_pic);
											Capture_Pic_Center[CurrNo].jpeg_pic =
													NULL;
										}
										Local.SendToCetnering = 0; //正在传给中心

#ifdef _DEBUG
										P_Debug("receive reply of capture send succeed \n");
#endif
										break;
									}
								}
							}
						}
					}
				}
			}
		}
		//打开互斥锁
		pthread_mutex_unlock(&Local.udp_lock);
	}
}
//---------------------------------------------------------------------------
//单元门口机、围墙机、二次门口机发送呼叫照片->发送结束（失败）
void RecvCapture_Send_Fail_Func(unsigned char *recv_buf, char *cFromIP,
		int length, int m_Socket)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int Start_OK;
	int CurrNo;
	Start_OK = 0;

	i = 0;
	isAddrOK = 1;
	for (j = 8; j < 8 + Local.AddrLen; j++)
	{
		if (LocalCfg.Addr[j - 8] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	}
	//地址匹配
	if (isAddrOK == 1)
	{
		//锁定互斥锁
		pthread_mutex_lock(&Local.udp_lock);
		if ((recv_buf[7] & 0x03) == REPLY) //应答
		{
			for (i = 0; i < UDPSENDMAX; i++)
			{
				if (Multi_Udp_Buff[i].isValid == 1)
				{
					if (Multi_Udp_Buff[i].m_Socket == m_DataSocket)
					{
						if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM)
						{
							if (Multi_Udp_Buff[i].buf[6] == CAPTUREPIC_SEND_FAIL)
							{
								if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK)
								{
									if (strcmp(Multi_Udp_Buff[i].RemoteHost, cFromIP) == 0)
									{
										Multi_Udp_Buff[i].isValid = 0;
										Local.SendToCetnering = 0; //正在传给中心

#ifdef _DEBUG
										P_Debug("receive reply of capture send failed \n");
#endif
										break;
									}
								}
							}
						}
					}
				}
			}
		}
		//打开互斥锁
		pthread_mutex_unlock(&Local.udp_lock);
	}
}
#endif
//---------------------------------------------------------------------------
void UdpVideoRcvThread(void) //UDP接收线程函数
{
	/* 循环接收数据 */
//  int oldframeno=0;
	unsigned char send_b[1520];
	int sendlength;
	char FromIP[20];
	int newframeno;
	int currpackage;
	int i, j;
	int sub;
	short PackIsExist; //数据包已接收标志
	short FrameIsNew; //数据包是否是新帧的开始
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
		len = recvfrom(m_VideoSocket, buff, sizeof(buff) - 1, 0, (struct sockaddr *)&c_addr, &addr_len);
		if (len < 0)
		{
			perror("recvfrom");
			exit(errno);
		}
		buff[len] = '\0';
#ifdef _DEBUG
		sprintf(Local.DebugInfo, "&&& RECV VIDEO &&& len=%d addr=%s:%d:%s\n\r", len, inet_ntoa(c_addr.sin_addr), ntohs(c_addr.sin_port), buff);
		P_Debug(Local.DebugInfo);
		sprintf(Local.DebugInfo,"&&& RECV VIDEO &&& [6]%d, [7]%d, [8]%d, [32]%d, [33]%d, [34]%d, [35]%d\n\r", buff[6], buff[7], buff[8], buff[32], buff[33], buff[34], buff[35]);
		P_Debug(Local.DebugInfo);
#endif
		strcpy(FromIP, inet_ntoa(c_addr.sin_addr));
		//if(DebugMode == 1)
		//   printf("FromIP is %s\n",FromIP);
		if ((buff[0] == UdpPackageHead[0]) && (buff[1] == UdpPackageHead[1]) && (buff[2] == UdpPackageHead[2])
				&& (buff[3] == UdpPackageHead[3]) && (buff[4] == UdpPackageHead[4]) && (buff[5] == UdpPackageHead[5]))
		{
			switch (buff[6])
			{
			case NSORDER: //主机名解析（子网内广播）
				//    case NSSERVERORDER:  //主机名解析(NS服务器)
				switch (buff[7] & 0x03)
				{
				case ASK: //解析
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
							P_Debug("len of resolved request data is unusual\n");
					}
					break;
				case REPLY: //解析回应
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
							P_Debug("len of resolved reply data is unusual\n");
					}
					break;
				}
				break;
			case VIDEOTALK: //局域网可视对讲
			case VIDEOTALKTRANS: //局域网可视对讲中转服务
				switch (buff[8])
				{
#ifdef _MULTI_MACHINE_SUPPORT  //多分机支持
				case JOINGROUP: //加入组播组（主叫方->被叫方，被叫方应答）
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
							P_Debug("len of talk join group is unusual \n");
					}
					break;
				case LEAVEGROUP: //退出组播组（主叫方->被叫方，被叫方应答）
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
							P_Debug("len of talk leave group is unusual \n");
					}
					break;
#endif
				case CALL: //对方发起对讲
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
							P_Debug("len of call is unusual\n");
					}
					break;
				case LINEUSE: //对方正忙
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
//					if(len == 57 || len == 58)
					if(len >= 57)
#endif
					{
						RecvTalkLineUse_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("len of lineuse reply is unusual\n");
					}
					break;
				case CALLANSWER: //呼叫应答
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
//					if(len == 61 || len == 62)
					if(len >= 61)
#endif
					{
//						P_Debug("UdpVideoRcvThread, 2222\n");
						RecvTalkCallAnswer_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("len of call reply is unusual\n");
					}
					break;
				case CALLSTART: //被叫方开始通话
//					P_Debug("FromIP is %s\n",FromIP);
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
//					if(len == 57 || len == 58)
					if(len >= 57)
#endif
					{
						RecvTalkCallStart_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("len of talk-start is unusual\n");
					}
					break;
				case CALLCONFIRM: //通话在线确认
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
//					if(len == 61 || len == 62)
					if(len >= 61)
#endif
					{
//						P_Debug("UdpVideoRcvThread, 4444\n");
						RecvTalkCallConfirm_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("len of call-confirm is unusual\n");
					}
					break;
				case REMOTEOPENLOCK: //被叫方远程开锁
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
//					if(len == 57 || len == 58)
					if(len >= 57)
#endif
					{
						RecvTalkOpenLock_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("len of remote open lock is unusual\n");
					}
					break;
				case CALLEND: //通话结束
					//结束播放视频
					//如为对方结束，需应答
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
//					if(len == 57 || len == 58)
					if(len >= 57)
#endif
					{
						RecvTalkCallEnd_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("len of call end is unusual\n");
					}
					break;
				case FORCEIFRAME: //强制I帧请求
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
//					if(len == 57 || len == 58)
					if(len >= 57)
#endif
					{
						RecvForceIFrame_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("len of force Iframe is unusual\n");
					}
					break;
				case ZOOMOUT: //放大(720*480)
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
//					if(len == 57 || len == 58)
					if(len >= 57)
#endif
					{
						RecvTalkZoomOut_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("len of zoom out(720*480) is unusual");
					}
					break;
				case ZOOMIN: //缩小(352*240)
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
//					if(len == 57 || len == 58)
					if(len >= 57)
#endif
					{
						RecvTalkZoomIn_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("len of zoom in(352*240) is unusual");
					}
					break;
				case CALLUP: //通话上行
				case CALLDOWN: //通话下行
					RecvTalkCallUpDown_Func(buff, FromIP, len);
					break;
#ifdef _MULTI_MACHINE_SUPPORT  //多分机支持
				case TURNTALK: //转接（被叫方->主叫方，主叫方应答）
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
							P_Debug("len of turn talk is unusual\n");
					}
					break;
#endif
				}
				break;
			case VIDEOWATCH: //局域网监控
			case VIDEOWATCHTRANS: //局域网监控中转服务
				switch (buff[8])
				{
				case CALL: //对方发起监视
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
//					if(len == 61 || len == 62)
					if(len >= 61)
#endif
					{
						RecvWatchCall_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("len of watch is unusual\n");
					}
					break;
				case LINEUSE: //对方正忙
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
//					if(len == 57 || len == 58)
					if(len >= 57)
#endif
					{
						RecvWatchLineUse_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("len of lineuse reply is unusual\n");
					}
					break;
				case CALLANSWER: //呼叫应答
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
//					if(len == 57 || len == 58)
					if(len >= 57)
#endif
					{
						RecvWatchCallAnswer_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("len of watch reply is unusual\n");
					}
					break;
				case CALLCONFIRM: //通话在线确认
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
//					if(len == 61 || len == 62)
					if(len >= 61)
#endif
					{
						RecvWatchCallConfirm_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("len of watch confirm is unusual\n");
					}
					break;
				case CALLEND: //通话结束
					//结束播放视频
					//如为对方结束，需应答
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
//					if(len == 57 || len == 58)
					if(len >= 57)
#endif
					{
						RecvWatchCallEnd_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("len of watch end is unusual\n");
					}
					break;
				case FORCEIFRAME: //强制I帧请求
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
//					if(len == 57 || len == 58)
					if(len >= 57)
#endif
					{
						RecvForceIFrame_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("len of force Iframe is unusual\n");
					}
					break;
				case ZOOMOUT: //放大(720*480)
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
//					if(len == 57 || len == 58)
					if(len >= 57)
#endif
					{
						RecvWatchZoomOut_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("len of zoom out(720*480) is unusual");
					}
					break;
				case ZOOMIN: //缩小(352*240)
#ifdef ISSETUPPACKETSIZE
					if (len == SMALLESTSIZE)
#else
//					if(len == 57 || len == 58)
					if(len >= 57)
#endif
					{
						RecvWatchZoomIn_Func(buff, FromIP);
					}
					else
					{
						if (DebugMode == 1)
							P_Debug("len of zoom in(352*240) is unusual");
					}
					break;
				case CALLUP: //通话上行
				case CALLDOWN: //通话下行
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
//解析请求
void RecvNSAsk_Func(unsigned char *recv_buf, char *cFromIP, int m_Socket)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	isAddrOK = 1;

	for (j = 8; j < 8 + Local.AddrLen; j++)
	{
		if (LocalCfg.Addr[j - 8] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	}

	//sprintf(Local.DebugInfo, "cFromIP = %s, isAddrOK = %d, Local.AddrLen = %d, LocalCfg.Addr = %s\n", cFromIP, isAddrOK, Local.AddrLen, LocalCfg.Addr);
	//P_Debug(Local.DebugInfo);
	//不是本机所发
	if (isAddrOK == 0)
	{
		isAddrOK = 1;
		for (j = 32; j < 32 + Local.AddrLen; j++)
		{
			if (LocalCfg.Addr[j - 32] != recv_buf[j])
			{
				isAddrOK = 0;
				break;
			}
		}
		//要求解析的是本机地址
		if (isAddrOK == 1)
		{
			//   if((LocalCfg.Addr[0] == 'S')||(LocalCfg.Addr[0] == 'B'))

			Local.DenNum = 0; //副机数量
			memcpy(send_b, recv_buf, 32);
			send_b[7] = REPLY; //应答

			send_b[32] = Local.DenNum + 1; //地址个数

			memcpy(send_b + 33, LocalCfg.Addr, 20);
			memcpy(send_b + 53, LocalCfg.IP, 4);
			for (i = 0; i < Local.DenNum; i++)
			{
				memcpy(send_b + 57 + 24 * i, Local.DenAddr[i], 20);
				memcpy(send_b + 57 + 20 + 24 * i, Local.DenIP[i], 4);
			}
			sendlength = 57 + 24 * Local.DenNum;
			if (m_Socket == m_DataSocket)
				UdpSendBuff(m_Socket, cFromIP, RemoteDataPort, send_b, sendlength);
			if (m_Socket == m_VideoSocket)
				UdpSendBuff(m_Socket, cFromIP, RemoteVideoPort, send_b, sendlength);
		}
	}
}
//-----------------------------------------------------------------------
//解析应答
void RecvNSReply_Func(unsigned char *recv_buf, char *cFromIP, int m_Socket)
{
	int i, j, k;
	int CurrOrder;
	int isAddrOK;
	int AddrLen;

	LOG_RUNLOG_DEBUG("RecvNSReply_Func 2  recv_buf:%s", recv_buf);

	//暂时关闭本地解析，试验服务器解析
	if (Local._TESTNSSERVER == 1)
	{
		if (m_Socket == m_VideoSocket)
			return;
	}

	//锁定互斥锁
	pthread_mutex_lock(&Local.udp_lock);

	P_Debug("RecvNSReply_Func 1\n");
	for (i = 0; i < UDPSENDMAX; i++)
	{
		if (Multi_Udp_Buff[i].isValid == 1)
		{
			//  if(Multi_Udp_Buff[i].m_Socket == m_VideoSocket)
			if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM)
			{
				if ((Multi_Udp_Buff[i].buf[6] == NSORDER) || (Multi_Udp_Buff[i].buf[6] == NSSERVERORDER))
				{
					//LOG_RUNLOG_DEBUG("RecvNSReply_Func 2 Multi_Udp_Buff[i].buf[7]:%d recv_buf:%s", Multi_Udp_Buff[i].buf[7], recv_buf[32]);
					if ((Multi_Udp_Buff[i].buf[7] == ASK) && (recv_buf[32] > 0))
					{
						//判断本机地址是否匹配
						isAddrOK = 1;
						if(Multi_Udp_Buff[i].buf[32] == 'Z')
							AddrLen = 4;
						else
							AddrLen = Local.AddrLen;
						//LOG_RUNLOG_DEBUG("RecvNSReply_Func 3 Local.AddrLen:%d recv_buf:%s", Local.AddrLen, recv_buf);
						for (j = 32; j < 32 + AddrLen; j++)
						{
							sprintf(Local.DebugInfo, "Multi_Udp_Buff = %d, recv_buf = %d\n", Multi_Udp_Buff[i].buf[j], recv_buf[j + 1]);
							P_Debug(Local.DebugInfo);
							if (Multi_Udp_Buff[i].buf[j] != recv_buf[j + 1])
							{
								if((recv_buf[j + 1] - Multi_Udp_Buff[i].buf[j]) == 32)
								{
									isAddrOK = 1;
								}
								else
								{
									isAddrOK = 0;
									break;
								}
							}
						}
						sprintf(Local.DebugInfo, "RecvNSReply_Func isAddrOK = %d, NSReplyFlag = %d\n", isAddrOK, Local.NSReplyFlag);
						P_Debug(Local.DebugInfo);
						if (isAddrOK == 1)
						{
							CurrOrder = Multi_Udp_Buff[i].CurrOrder;
							Multi_Udp_Buff[i].isValid = 0;
							Multi_Udp_Buff[i].SendNum = 0;
							break;
						}
					}
				}
			}
		}
	}
	//打开互斥锁
	pthread_mutex_unlock(&Local.udp_lock);

	if ((isAddrOK == 1) && (Local.NSReplyFlag == 0))
	{ //收到正确的解析回应
		Local.NSReplyFlag = 1; //NS应答处理标志
		if (CurrOrder == 255) //主机向副机解析
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
				//           if(Remote.DenNum == 1) //单播
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

						//锁定互斥锁
						pthread_mutex_lock(&Local.udp_lock);

						for (i = 0; i < UDPSENDMAX; i++)
						{
							if (Multi_Udp_Buff[i].isValid == 0)
							{
								Multi_Udp_Buff[i].SendNum = 0;
								Multi_Udp_Buff[i].m_Socket = m_VideoSocket;
								Multi_Udp_Buff[i].RemotePort = RemoteVideoPort;
								sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0", Remote.IP[j][0],
										Remote.IP[j][1], Remote.IP[j][2], Remote.IP[j][3]);
								sprintf(Multi_Udp_Buff[i].RemoteIP, "%d.%d.%d.%d\0", Remote.IP[j][0],
										Remote.IP[j][1], Remote.IP[j][2], Remote.IP[j][3]);
								if (DebugMode == 1)
								{
									sprintf(Local.DebugInfo, "resolved address succeed, calling, Multi_Udp_Buff[i].RemoteHost is %s\n",
											Multi_Udp_Buff[i].RemoteHost);
									P_Debug(Local.DebugInfo);
								}
								//头部
								memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead,6);
								//命令
								Multi_Udp_Buff[i].buf[6] = CurrOrder;
								Multi_Udp_Buff[i].buf[7] = ASK; //主叫
								// 子命令
								Multi_Udp_Buff[i].buf[8] = CALL;

								memcpy(Multi_Udp_Buff[i].buf + 9, LocalCfg.Addr,20);
								memcpy(Multi_Udp_Buff[i].buf + 29, LocalCfg.IP,4);
								memcpy(Multi_Udp_Buff[i].buf + 33,Remote.Addr[j], 20);
								memcpy(Multi_Udp_Buff[i].buf + 53, Remote.IP[j],4);
								if (Remote.DenNum == 1)
									Multi_Udp_Buff[i].buf[57] = 0; //单播
								else
									Multi_Udp_Buff[i].buf[57] = 1; //组播
								//组播地址
								Multi_Udp_Buff[i].buf[58] = Remote.GroupIP[0];
								Multi_Udp_Buff[i].buf[59] = Remote.GroupIP[1];
								Multi_Udp_Buff[i].buf[60] = Remote.GroupIP[2];
								Multi_Udp_Buff[i].buf[61] = Remote.GroupIP[3];

								Multi_Udp_Buff[i].nlength = 62;
								Multi_Udp_Buff[i].DelayTime = DIRECTCALLTIME;
								Multi_Udp_Buff[i].SendDelayTime = 0; //发送等待时间计数  20101112
								Multi_Udp_Buff[i].isValid = 1;
								sem_post(&multi_send_sem);
								break;
							}
						}
						//打开互斥锁
						pthread_mutex_unlock(&Local.udp_lock);
					}
				}
			}
		}
	}
}
//-----------------------------------------------------------------------
//监视呼叫
void RecvWatchCall_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;
	uint32_t Ip_Int;

	if (Local._TESTTRANS == 1)
		if (recv_buf[6] == VIDEOWATCH)
			return;

	//查看目标地址是否是本机
	isAddrOK = 1;
	for (j = 33; j < 33 + Local.AddrLen; j++)
	{
		if (LocalCfg.Addr[j - 33] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	}
	sprintf(Local.DebugInfo, "RecvWatchCall isAddrOK = %d status = %d \n", isAddrOK, Local.Status);
	P_Debug(Local.DebugInfo);
	if (isAddrOK == 0)
		return;

	//本机状态为空闲
	/*if (Local.Status == 0)
	{
		Ip_Int = inet_addr(cFromIP);
		memcpy(Remote.DenIP, &Ip_Int, 4);
		sprintf(Local.DebugInfo, "Remote.DenIP, %d.%d.%d.%d\0", Remote.DenIP[0], Remote.DenIP[1], Remote.DenIP[2], Remote.DenIP[3]);
		P_Debug(Local.DebugInfo);
		//获取对方地址
		memcpy(Remote.Addr[0], recv_buf + 9, 20);
		memcpy(Remote.IP[0], recv_buf + 29, 4);
		sprintf(Local.DebugInfo, "Remote.IP[0], %d.%d.%d.%d\0", Remote.IP[0][0], Remote.IP[0][1], Remote.IP[0][2], Remote.IP[0][3]);
		P_Debug(Local.DebugInfo);
		Remote.DenNum = 1;

		if (recv_buf[6] == VIDEOWATCH)
		{
			Remote.isDirect = 0;
			RemotePort = RemoteVideoPort;
			if (DebugMode == 1)
				P_Debug("other start direct watch call \n");
		}
		else
		{
			Remote.isDirect = 1;
			RemotePort = RemoteVideoServerPort;
			if (DebugMode == 1)
				P_Debug("other start trunon watch call \n");
		}

		memcpy(send_b, recv_buf, 57);
		send_b[7] = ASK; //主叫
		send_b[8] = CALLANSWER; //监视应答
		sendlength = 57;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);

		Local.Status = 4; //状态为被监视
		//开始录制视频
		StartRecVideo(CIF_W, CIF_H);
		Local.CallConfirmFlag = 1; //设置在线标志
		Local.Timer1Num = 0;
		Local.TimeOut = 0; //监视超时,  通话超时,  呼叫超时，无人接听
		Local.OnlineNum = 0; //在线确认序号
		Local.OnlineFlag = 1;

		//正在监视中
	}
	else//本机为忙
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
		send_b[7] = ASK; //主叫
		send_b[8] = LINEUSE; //占线应答
		sendlength = 57;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);
		if (DebugMode == 1)
			P_Debug("other start watch call \n");
	}*/
	memcpy(watchRecvBuf, recv_buf, strlen(recv_buf));
	MsgLAN2CCWatch(cFromIP);//发送监视消息
}

//监视应答
void WatchRespond_Func(char *cFromIP)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;
	uint32_t Ip_Int;

	//本机状态为空闲
	if (Local.Status == 0)
	{
		Ip_Int = inet_addr(cFromIP);
		memcpy(Remote.DenIP, &Ip_Int, 4);
		sprintf(Local.DebugInfo, "Remote.DenIP, %d.%d.%d.%d\0", Remote.DenIP[0], Remote.DenIP[1], Remote.DenIP[2], Remote.DenIP[3]);
		P_Debug(Local.DebugInfo);
		//获取对方地址
		memcpy(Remote.Addr[0], watchRecvBuf + 9, 20);
		memcpy(Remote.IP[0], watchRecvBuf + 29, 4);
		sprintf(Local.DebugInfo, "Remote.IP[0], %d.%d.%d.%d\0", Remote.IP[0][0], Remote.IP[0][1], Remote.IP[0][2], Remote.IP[0][3]);
		P_Debug(Local.DebugInfo);
		Remote.DenNum = 1;

		if (watchRecvBuf[6] == VIDEOWATCH)
		{
			Remote.isDirect = 0;
			RemotePort = RemoteVideoPort;
			if (DebugMode == 1)
				P_Debug("other start direct watch call \n");
		}
		else
		{
			Remote.isDirect = 1;
			RemotePort = RemoteVideoServerPort;
			if (DebugMode == 1)
				P_Debug("other start trunon watch call \n");
		}

		memcpy(send_b, watchRecvBuf, 57);
		send_b[7] = ASK; //主叫
		send_b[8] = CALLANSWER; //监视应答
		sendlength = 57;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);

		Local.Status = 4; //状态为被监视
		//开始录制视频
//		StartRecVideo(640, 480);//(CIF_W, CIF_H);
		StartRecVideo(CIF_W, CIF_H);
		Local.CallConfirmFlag = 1; //设置在线标志
		Local.Timer1Num = 0;
		Local.TimeOut = 0; //监视超时,  通话超时,  呼叫超时，无人接听
		Local.OnlineNum = 0; //在线确认序号
		Local.OnlineFlag = 1;

		//正在监视中
	}
	else//本机为忙
	{
		if (watchRecvBuf[6] == VIDEOWATCH)
		{
			RemotePort = RemoteVideoPort;
		}
		else
		{
			RemotePort = RemoteVideoServerPort;
		}
		memcpy(send_b, watchRecvBuf, 57);
		send_b[7] = ASK; //主叫
		send_b[8] = LINEUSE; //占线应答
		sendlength = 57;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);
		if (DebugMode == 1)
			P_Debug("other start watch call \n");
	}

}
//监视占线
void WatchLineUse_Func(char *cFromIP)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;
	uint32_t Ip_Int;

	//本机为忙
	if (Local.Status != 0)
	{
		if (watchRecvBuf[6] == VIDEOWATCH)
		{
			RemotePort = RemoteVideoPort;
		}
		else
		{
			RemotePort = RemoteVideoServerPort;
		}
		memcpy(send_b, watchRecvBuf, 57);
		send_b[7] = ASK; //主叫
		send_b[8] = LINEUSE; //占线应答
		sendlength = 57;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);
		if (DebugMode == 1)
			P_Debug("other start watch call \n");
	}
}
//-----------------------------------------------------------------------
//监视占线应答
void RecvWatchLineUse_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i, j;
	int isAddrOK;

	//锁定互斥锁
	pthread_mutex_lock(&Local.udp_lock);
	if ((recv_buf[7] & 0x03) == ASK) //应答
	{
		for (i = 0; i < UDPSENDMAX; i++)
		{
			if (Multi_Udp_Buff[i].isValid == 1)
			{
				if (Multi_Udp_Buff[i].m_Socket == m_VideoSocket)
				{
					if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM)
					{
						if ((Multi_Udp_Buff[i].buf[6] == VIDEOWATCH) || (Multi_Udp_Buff[i].buf[6] == VIDEOWATCHTRANS))
						{
							if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK)
							{
								if (Multi_Udp_Buff[i].buf[8] == CALL)
								{
									if (strcmp(Multi_Udp_Buff[i].RemoteHost, cFromIP) == 0)
									{
										Multi_Udp_Buff[i].isValid = 0;
										Local.Status = 0; //状态设为空闲
										MsgLAN2CCCallLineuse();//发送占线消息
										if (DebugMode == 1)
											P_Debug("receive reply of watch lineuse \n");
										break;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	//打开互斥锁
	pthread_mutex_unlock(&Local.udp_lock);
}
//-----------------------------------------------------------------------
//监视呼叫应答
void RecvWatchCallAnswer_Func(unsigned char *recv_buf, char *cFromIP)
{
}
//-----------------------------------------------------------------------
//监视在线确认
void RecvWatchCallConfirm_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;

	//本机被动    被监视
	if ((Local.Status == 4) && ((recv_buf[7] & 0x03) == ASK))
	{
		memcpy(send_b, recv_buf, 61);
		send_b[7] = REPLY; //应答
		sendlength = 61;
		RemotePort = RemoteVideoPort;
		if (recv_buf[6] == VIDEOWATCH) //直通呼叫
			RemotePort = RemoteVideoPort;
		else
			//中转呼叫
			RemotePort = RemoteVideoServerPort;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);
		Local.CallConfirmFlag = 1;
		//if(DebugMode == 1)
		//printf("收到监视在线确认\n");
	}
	else //本机监视
	if (Local.Status == 3)
	{
		Local.CallConfirmFlag = 1;
		if (DebugMode == 1)
			P_Debug("receive reply of other watch call confirm \n");
	}
}
//-----------------------------------------------------------------------
//监视呼叫结束
void RecvWatchCallEnd_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;

	//本机被动    被监视
	if ((Local.Status == 4) && ((recv_buf[7] & 0x03) == ASK))
	{
		Local.OnlineFlag = 0;
		Local.CallConfirmFlag = 0; //设置在线标志
		memcpy(send_b, recv_buf, 57);
		send_b[7] = REPLY; //应答
		sendlength = 57;
		RemotePort = RemoteVideoPort;
		if (recv_buf[6] == VIDEOWATCH) //直通呼叫
			RemotePort = RemoteVideoPort;
		else
			//中转呼叫
			RemotePort = RemoteVideoServerPort;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);

		StopRecVideo();

		TalkEnd_ClearStatus();//add by zlin

		//监视结束
		if (DebugMode == 1)
			P_Debug("other end watch \n");
	}
	else //本机主动
	if ((recv_buf[7] & 0x03) == REPLY)
	{
		Local.OnlineFlag = 0;
		Local.CallConfirmFlag = 0; //设置在线标志

		//锁定互斥锁
		pthread_mutex_lock(&Local.udp_lock);
		//本机主动
		if ((recv_buf[7] & 0x03) == REPLY)
		{
			for (i = 0; i < UDPSENDMAX; i++)
			{
				if (Multi_Udp_Buff[i].isValid == 1)
				{
					if (Multi_Udp_Buff[i].m_Socket == m_VideoSocket)
					{
						if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM)
						{
							if (Multi_Udp_Buff[i].buf[6] == VIDEOWATCH)
							{
								if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK)
								{
									if (Multi_Udp_Buff[i].buf[8] == CALLEND)
									{
										if (strcmp(Multi_Udp_Buff[i].RemoteHost, cFromIP) == 0)
										{
											Multi_Udp_Buff[i].isValid = 0;
											Local.Status = 0; //状态为空闲
											MsgLAN2CCCallHangOff();
											if (DebugMode == 1)
												P_Debug("reply of other end watch \n");
											break;
										}
									}
								}
							}
						}
					}
				}
			}
		}
		//打开互斥锁
		pthread_mutex_unlock(&Local.udp_lock);
	}
}
//-----------------------------------------------------------------------
//放大(720*480)
void RecvWatchZoomOut_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;

	//本机被动    被监视
	if ((Local.Status == 4) && ((recv_buf[7] & 0x03) == ASK))
	{
		memcpy(send_b, recv_buf, 57);
		send_b[7] = REPLY; //应答
		sendlength = 57;
		RemotePort = RemoteVideoPort;
		if (recv_buf[6] == VIDEOWATCH) //直通呼叫
			RemotePort = RemoteVideoPort;
		else
			//中转呼叫
			RemotePort = RemoteVideoServerPort;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);
		if (Local.RecPicSize == 1)
		{
			StopRecVideo(); //352*240
			StartRecVideo(D1_W, D1_H); //720*480
		}
		if (DebugMode == 1)
			P_Debug("other zoomout capture \n");
	}
	else //本机监视
	if (Local.Status == 3)
	{
		//锁定互斥锁
		pthread_mutex_lock(&Local.udp_lock);
		//本机主动
		if ((recv_buf[7] & 0x03) == REPLY)
		{
			for (i = 0; i < UDPSENDMAX; i++)
			{
				if (Multi_Udp_Buff[i].isValid == 1)
				{
					if (Multi_Udp_Buff[i].m_Socket == m_VideoSocket)
					{
						if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM)
						{
							if ((Multi_Udp_Buff[i].buf[6] == VIDEOWATCH) || (Multi_Udp_Buff[i].buf[6] == VIDEOWATCHTRANS))
							{
								if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK)
								{
									if (Multi_Udp_Buff[i].buf[8] == ZOOMOUT)
									{
										if (strcmp(Multi_Udp_Buff[i].RemoteHost, cFromIP) == 0)
										{
											Multi_Udp_Buff[i].isValid = 0;
											if (DebugMode == 1)
												P_Debug("reply of ohter zoomout capture \n");
											break;
										}
									}
								}
							}
						}
					}
				}
			}
		}
		//打开互斥锁
		pthread_mutex_unlock(&Local.udp_lock);
	}
}
//-----------------------------------------------------------------------
//缩小(352*240)
void RecvWatchZoomIn_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;

	//本机被动    被监视
	if ((Local.Status == 4) && ((recv_buf[7] & 0x03) == ASK))
	{
		memcpy(send_b, recv_buf, 57);
		send_b[7] = REPLY; //应答
		sendlength = 57;
		RemotePort = RemoteVideoPort;
		if (recv_buf[6] == VIDEOWATCH) //直通呼叫
			RemotePort = RemoteVideoPort;
		else
			//中转呼叫
			RemotePort = RemoteVideoServerPort;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);
		if (Local.RecPicSize == 2)
		{
			StopRecVideo(); //720*480
			StartRecVideo(CIF_W, CIF_H); //352*288
		}
		if (DebugMode == 1)
			P_Debug("other zoomin capture \n");
	}
	else //本机监视
	if (Local.Status == 3)
	{
		//锁定互斥锁
		pthread_mutex_lock(&Local.udp_lock);
		//本机主动
		if ((recv_buf[7] & 0x03) == REPLY)
		{
			for (i = 0; i < UDPSENDMAX; i++)
			{
				if (Multi_Udp_Buff[i].isValid == 1)
				{
					if (Multi_Udp_Buff[i].m_Socket == m_VideoSocket)
					{
						if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM)
						{
							if ((Multi_Udp_Buff[i].buf[6] == VIDEOWATCH) || (Multi_Udp_Buff[i].buf[6] == VIDEOWATCHTRANS))
							{
								if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK)
								{
									if (Multi_Udp_Buff[i].buf[8] == ZOOMIN)
									{
										if (strcmp(Multi_Udp_Buff[i].RemoteHost, cFromIP) == 0)
										{
											Multi_Udp_Buff[i].isValid = 0;
											if (DebugMode == 1)
												P_Debug("reply of other zoomin capture \n");
											break;
										}
									}
								}
							}
						}
					}
				}
			}
		}
		//打开互斥锁
		pthread_mutex_unlock(&Local.udp_lock);
	}
}
//-----------------------------------------------------------------------
//监视数据
void RecvWatchCallUpDown_Func(unsigned char *recv_buf, char *cFromIP, int length)
{
}
//-----------------------------------------------------------------------
#ifdef _MULTI_MACHINE_SUPPORT  //多分机支持
//加入组播组（主叫方->被叫方，被叫方应答）
void RecvTalkJoinGroup_Func(unsigned char *recv_buf, char *cFromIP)
{
}
//-----------------------------------------------------------------------
//退出组播组（主叫方->被叫方，被叫方应答）
void RecvTalkLeaveGroup_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;

	//本机为被叫方 被叫对讲
	if ((Local.Status == 2) && ((recv_buf[7] & 0x03) == ASK))
	{
		memcpy(send_b, recv_buf, 61);
		send_b[7] = REPLY; //应答
		sendlength = 61;
		RemotePort = RemoteVideoPort;
		if (recv_buf[6] == VIDEOTALK) //直通呼叫
			RemotePort = RemoteVideoPort;
		else
			//中转呼叫
			RemotePort = RemoteVideoServerPort;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);
		//离开组播组
		DropMultiGroup(m_VideoSocket, NULL);

		TalkEnd_ClearStatus();

		sprintf(Local.DebugInfo, "other require leave multicast group, %s\n", cFromIP);
		P_Debug(Local.DebugInfo);
	}
	else //本机为主叫方 发起
	if ((recv_buf[7] & 0x03) == REPLY)
	{
		//锁定互斥锁
		pthread_mutex_lock(&Local.udp_lock);
		//本机主动
		if ((recv_buf[7] & 0x03) == REPLY)
		{
			for (i = 0; i < UDPSENDMAX; i++)
			{
				if (Multi_Udp_Buff[i].isValid == 1)
				{
					if (Multi_Udp_Buff[i].m_Socket == m_VideoSocket)
					{
						if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM)
						{
							if ((Multi_Udp_Buff[i].buf[6] == VIDEOTALK) || (Multi_Udp_Buff[i].buf[6] == VIDEOTALKTRANS))
							{
								if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK)
								{
									if (Multi_Udp_Buff[i].buf[8] == LEAVEGROUP)
									{
										if (strcmp(Multi_Udp_Buff[i].RemoteHost, cFromIP) == 0)
										{
											Multi_Udp_Buff[i].isValid = 0;
											sprintf(Local.DebugInfo, "reply of other leave multicast group, %s\n", cFromIP);
											P_Debug(Local.DebugInfo);
											break;
										}
									}
								}
							}
						}
					}
				}
			}
		}
		//打开互斥锁
		pthread_mutex_unlock(&Local.udp_lock);
	}
}
//-----------------------------------------------------------------------
//转接（被叫方->主叫方，主叫方应答）
void RecvTalkTurnTalk_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i;
	int isAddrOK;
	int sendlength;
	unsigned char send_b[1520];
	int RemotePort;
	char tmp_text[20];

	if (recv_buf[7] != ASK)
		return;

	if ((Local.Status == 5) || (Local.Status == 6))
	{
		//判断本机地址是否匹配
		isAddrOK = 1;
		for (i = 9; i < 9 + Local.AddrLen; i++)
		{
			if (LocalCfg.Addr[i - 9] != recv_buf[i])
			{
				isAddrOK = 0;
				break;
			}
		}
		memcpy(tmp_text, recv_buf + 9, Local.AddrLen);
		sprintf(Local.DebugInfo, "RecvTalkTurnTalk_Func Local.AddrLen = %d, isAddrOK = %d, tmp_text = %s\n",
				Local.AddrLen, isAddrOK, tmp_text);
		P_Debug(Local.DebugInfo);
		if (isAddrOK == 1)
		{
			memcpy(send_b, recv_buf, 81);
			send_b[7] = REPLY; //应答
			sendlength = 81;
			if (Remote.isDirect == 0) //直通呼叫
			{
				memcpy(Remote.DenIP, recv_buf + 77, 4); //直通，更改目标地址
				RemotePort = RemoteVideoPort;
			}
			else //中转呼叫
			{ //中转，不更改目标地址
				RemotePort = RemoteVideoServerPort;
			}
			UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);

			Remote.DenNum = 1;
			memcpy(Remote.IP[0], recv_buf + 77, 4);
			memcpy(Remote.Addr[0], recv_buf + 57, 20);

			TalkTurnOn_Status();

			//锁定互斥锁
			pthread_mutex_lock(&Local.udp_lock);

			for (i = 0; i < UDPSENDMAX; i++)
			{
				if (Multi_Udp_Buff[i].isValid == 0)
				{
					Multi_Udp_Buff[i].SendNum = 0;
					Multi_Udp_Buff[i].m_Socket = m_VideoSocket;
					Multi_Udp_Buff[i].RemotePort = RemotePort;
					sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0",
							Remote.DenIP[0], Remote.DenIP[1], Remote.DenIP[2], Remote.DenIP[3]);
					sprintf(Multi_Udp_Buff[i].RemoteIP, "%d.%d.%d.%d\0",
							Remote.IP[0][0], Remote.IP[0][1], Remote.IP[0][2], Remote.IP[0][3]);
					if (DebugMode == 1)
					{
						sprintf(Local.DebugInfo, "trun on, calling %d.%d.%d.%d\n", Remote.DenIP[0], Remote.DenIP[1], Remote.DenIP[2], Remote.DenIP[3]);
						P_Debug(Local.DebugInfo);
					}
					//头部
					memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
					//命令
					if (Remote.isDirect == 0) //直通呼叫
						Multi_Udp_Buff[i].buf[6] = VIDEOTALK;
					else
						//中转呼叫
						Multi_Udp_Buff[i].buf[6] = VIDEOTALKTRANS;
					Multi_Udp_Buff[i].buf[7] = ASK; //主叫
					// 子命令
					Multi_Udp_Buff[i].buf[8] = CALL;

					memcpy(Multi_Udp_Buff[i].buf + 9, LocalCfg.Addr, 20);
					memcpy(Multi_Udp_Buff[i].buf + 29, LocalCfg.IP, 4);
					memcpy(Multi_Udp_Buff[i].buf + 33, Remote.Addr[0], 20);
					memcpy(Multi_Udp_Buff[i].buf + 53, Remote.IP[0], 4);
					if (Remote.DenNum == 1)
						Multi_Udp_Buff[i].buf[57] = 0; //单播
					else
						Multi_Udp_Buff[i].buf[57] = 1; //组播
					//组播地址
					Multi_Udp_Buff[i].buf[58] = Remote.GroupIP[0];
					Multi_Udp_Buff[i].buf[59] = Remote.GroupIP[1];
					Multi_Udp_Buff[i].buf[60] = Remote.GroupIP[2];
					Multi_Udp_Buff[i].buf[61] = Remote.GroupIP[3];

					Multi_Udp_Buff[i].nlength = 62;
					Multi_Udp_Buff[i].DelayTime = 800;
					Multi_Udp_Buff[i].SendDelayTime = 0; //发送等待时间计数  20101112
					Multi_Udp_Buff[i].isValid = 1;
					sem_post(&multi_send_sem);
					break;
				}
			}
			//打开互斥锁
			pthread_mutex_unlock(&Local.udp_lock);
		}
	}
}
//-----------------------------------------------------------------------
//正在转接中
void TalkTurnOn_Status(void)
{
	//查看是否在其它组播组内
	DropMultiGroup(m_VideoSocket, NULL);

	//StopRecVideo();
	StopRecAudio();
	StopPlayAudio();
	Local.Status = 0; //状态为空闲

	//正在转接中, 此处会导致 mega 64 发挂机命令，导致挂机
//    SendHostOrder(0x68, Local.DoorNo, NULL); //发送主动命令  对方挂机
#ifdef _DEBUG
	P_Debug("other end talk, turnon \n");
#endif

	Local.OnlineFlag = 0;
	Local.CallConfirmFlag = 0; //设置在线标志
}
#endif
//-----------------------------------------------------------------------
//对讲呼叫
void RecvTalkCall_Func(unsigned char *recv_buf, char *cFromIP)
{
}
//-----------------------------------------------------------------------
//对讲占线应答
void RecvTalkLineUse_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i, j;
	int isAddrOK;
	char wavFile[80];

	//锁定互斥锁
	pthread_mutex_lock(&Local.udp_lock);
	if ((recv_buf[7] & 0x03) == ASK) //应答
	{
		for (i = 0; i < UDPSENDMAX; i++)
		{
			if (Multi_Udp_Buff[i].isValid == 1)
			{
				if (Multi_Udp_Buff[i].m_Socket == m_VideoSocket)
				{
					if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM)
					{
						if ((Multi_Udp_Buff[i].buf[6] == VIDEOTALK) || (Multi_Udp_Buff[i].buf[6] == VIDEOTALKTRANS))
						{
							if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK)
							{
								if (Multi_Udp_Buff[i].buf[8] == CALL)
								{
									if (strcmp(Multi_Udp_Buff[i].RemoteHost, cFromIP) == 0)
									{
										Multi_Udp_Buff[i].isValid = 0;
										if (Remote.DenNum == 1)
										{
											Local.Status = 0; //设状态为空闲
											MsgLAN2CCCallLineuse();//发送占线消息
										}

										//对方占线

										if (DebugMode == 1)
										{
											sprintf(Local.DebugInfo, "receive reply of lineuse i = %d,%d.%d.%d.%d\n",
													i, recv_buf[53], recv_buf[54], recv_buf[55], recv_buf[56]);
											P_Debug(Local.DebugInfo);
										}
										break;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	//打开互斥锁
	pthread_mutex_unlock(&Local.udp_lock);
}

//-----------------------------------------------------------------------
//对讲呼叫应答
void RecvTalkCallAnswer_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i, j;
	int isAddrOK;
	uint32_t Ip_Int;
	char wavFile[80];
	char RemoteIP[20];

	sprintf(RemoteIP, "%d.%d.%d.%d\0", recv_buf[53], recv_buf[54], recv_buf[55], recv_buf[56]);

	//锁定互斥锁
	pthread_mutex_lock(&Local.udp_lock);
	if ((recv_buf[7] & 0x03) == ASK) //应答
	{
		for (i = 0; i < UDPSENDMAX; i++)
		{
			if (Multi_Udp_Buff[i].isValid == 1)
			{
				if (Multi_Udp_Buff[i].m_Socket == m_VideoSocket)
				{
					if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM )
					{
						if ((Multi_Udp_Buff[i].buf[6] == VIDEOTALK) || (Multi_Udp_Buff[i].buf[6] == VIDEOTALKTRANS))
						{
							if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK)
							{
								if (Multi_Udp_Buff[i].buf[8] == CALL )
								{
									if (strcmp(Multi_Udp_Buff[i].RemoteHost, cFromIP) == 0)
									{
										if (strcmp(Multi_Udp_Buff[i].RemoteIP, RemoteIP) == 0)
										{
											Multi_Udp_Buff[i].isValid = 0;
											//  printf("Remote.DenNum = %d, Remote.isDirect = %d\n", Remote.DenNum, Remote.isDirect);
											if (Remote.isDirect == 0) //直通呼叫
											{
												if ((LocalCfg.Addr[0] == 'S') || (LocalCfg.Addr[0] == 'B')
														|| (LocalCfg.Addr[0] == 'Z') || (Remote.DenNum == 1))
												{
													Ip_Int = inet_addr(cFromIP);
													memcpy(Remote.DenIP, &Ip_Int, 4);
												}
												else if (Remote.DenNum > 1)
												{
													Remote.DenIP[0] = Remote.GroupIP[0];
													Remote.DenIP[1] = Remote.GroupIP[1];
													Remote.DenIP[2] = Remote.GroupIP[2];
													Remote.DenIP[3] = Remote.GroupIP[3];
												}
											}
											else //中转呼叫
											{
												Remote.DenIP[0] = Local.IP_VideoServer[0];
												Remote.DenIP[1] = Local.IP_VideoServer[1];
												Remote.DenIP[2] = Local.IP_VideoServer[2];
												Remote.DenIP[3] = Local.IP_VideoServer[3];
											}

											//开始录制视频
											MsgLAN2CCCallRespond();
//											StartRecVideo(CIF_W, 480);//(CIF_W, CIF_H);
											StartRecVideo(CIF_W, CIF_H);
											Local.Status = 1; //状态为主叫对讲
											Local.CallConfirmFlag = 1; //设置在线标志
											Local.Timer1Num = 0;
											Local.TimeOut = 0; //监视超时,  通话超时,  呼叫超时，无人接听
											Local.OnlineNum = 0; //在线确认序号
											Local.OnlineFlag = 1;

#ifdef _CAPTUREPIC_TO_CENTER  //捕获图片传到中心
											Local.RecordPic = 1; //通话留照片
											Local.IFrameCount = 0; //I帧计数
											Local.HavePicRecorded = 0;
											memcpy(Local.RemoteAddr, Remote.Addr[0], 20);
											Local.RemoteAddr[12] = '\0';
#endif
											sprintf(Local.DebugInfo, "Local.Status = %d\n", Local.Status);
											P_Debug(Local.DebugInfo);
											sprintf(Local.DebugInfo, "receive reply of call, i = %d, %d.%d.%d.%d\n",
													i, recv_buf[53], recv_buf[54], recv_buf[55], recv_buf[56]);
											P_Debug(Local.DebugInfo);
											break;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	//打开互斥锁
	pthread_mutex_unlock(&Local.udp_lock);
}
//-----------------------------------------------------------------------
//对讲开始通话  由被叫方发起，主叫方应答
void RecvTalkCallStart_Func(unsigned char *recv_buf, char *cFromIP)
{
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;
//	sprintf(Local.DebugInfo, "Local.Status: %d, recv_buf[7]: %d, Remote.DenNum = %d, Remote.isDirect = %d\n",Local.Status, recv_buf[7], Remote.DenNum, Remote.isDirect);
//	P_Debug(Local.DebugInfo);

	//本机为主叫方 应答  在留影预备或留影状态 用户可以通话
	if (((Local.Status == 1) || (Local.Status == 7) || (Local.Status == 9)) && ((recv_buf[7] & 0x03) == ASK))
	{
		memcpy(send_b, recv_buf, 57);
//		send_b[7] = REPLY | 0x04; //应答     由室内判断是否是　8126
		send_b[7] = REPLY;
		sendlength = 57;
		RemotePort = RemoteVideoPort;
		if (recv_buf[6] == VIDEOTALK) //直通呼叫
			RemotePort = RemoteVideoPort;
		else
			//中转呼叫
			RemotePort = RemoteVideoServerPort;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);
#ifdef _MULTI_MACHINE_SUPPORT  //多分机支持
		ExitGroup(recv_buf); //向其它被叫方退出组播组命令
#endif

		//获取被叫方地址
		memcpy(Remote.Addr[0], recv_buf + 33, 20);
		memcpy(Remote.IP[0], recv_buf + 53, 4);
		Remote.DenNum = 1;
		if (recv_buf[6] == VIDEOTALK) //直通呼叫
			memcpy(Remote.DenIP, Remote.IP[0], 4);

		MsgLAN2CCCallPickUp();//发送接通消息

		//StopPlayWavFile(); //关闭回铃音
		//WaitAudioUnuse(2000); //等待声卡空闲    //延时是为了等待声音播放完成，否则会有杂音
#ifdef	_SND_RECORD_
		StartRecAudio();
#endif
#ifdef	_SND_PLAY_
		StartPlayAudio();
#endif

		Local.Status = 5; //状态为主叫通话
		Local.TimeOut = 0; //监视超时,  通话超时,  呼叫超时，无人接听

		//正在通话中
		if (DebugMode == 1)
			P_Debug("other start talk \n");
	}
}
//-----------------------------------------------------------------------
//对讲在线确认
void RecvTalkCallConfirm_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;

	//本机为主叫方
	if (((Local.Status == 1) || (Local.Status == 5) || (Local.Status == 7) || (Local.Status == 9)) && ((recv_buf[7] & 0x03) == ASK))
	{
		memcpy(send_b, recv_buf, 61);
		send_b[7] = REPLY; //应答
		sendlength = 61;
		RemotePort = RemoteVideoPort;
		if (recv_buf[6] == VIDEOTALK) //直通呼叫
			RemotePort = RemoteVideoPort;
		else
			//中转呼叫
			RemotePort = RemoteVideoServerPort;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);
		Local.CallConfirmFlag = 1;
#ifdef _MULTI_MACHINE_SUPPORT  //多分机支持
		//如已在通话中，但收到不为通话方的在线确认，则发退出组播组命令
		if (Local.Status == 5)
		{
			if ((Remote.IP[0][0] != recv_buf[53]) || (Remote.IP[0][1] != recv_buf[54])
					|| (Remote.IP[0][2] != recv_buf[55]) || (Remote.IP[0][3] != recv_buf[56]))
			{
				sprintf(Local.DebugInfo, "called exit multicast group: Remote.IP = %d.%d.%d.%d, recv_buf = %d.%d.%d.%d, cFromIP = %s\n",
						Remote.IP[0][0], Remote.IP[0][1], Remote.IP[0][2], Remote.IP[0][3], recv_buf[53], recv_buf[54],
						recv_buf[55], recv_buf[56], cFromIP);
				P_Debug(Local.DebugInfo);
				SingleExitGroup(recv_buf); //向单个被叫方退出组播组命令
			}
		}
#endif
	}
	else //本机为被叫方
	if (((Local.Status == 2) || (Local.Status == 6) || (Local.Status == 8) || (Local.Status == 10)) && ((recv_buf[7] & 0x03) == REPLY))
	{
		Local.CallConfirmFlag = 1;
		//if(DebugMode == 1)
		//   printf("收到对方应答本机对讲在线确认\n");
	}
}
//-----------------------------------------------------------------------
//对讲远程开锁
void RecvTalkOpenLock_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;

	//本机被动
	if (((Local.Status == 1) || (Local.Status == 2) || (Local.Status == 5) || (Local.Status == 6)) && ((recv_buf[7] & 0x03) == ASK))
	{
		memcpy(send_b, recv_buf, 57);
		send_b[7] = REPLY; //应答
		sendlength = 57;
		RemotePort = RemoteVideoPort;
		if (recv_buf[6] == VIDEOTALK) //直通呼叫
			RemotePort = RemoteVideoPort;
		else
			//中转呼叫
			RemotePort = RemoteVideoServerPort;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);

		MsgLAN2CCCallOpendoor();//发送开锁消息

		if (DebugMode == 1)
			P_Debug("other remote open lock\n");
	}
}
//-----------------------------------------------------------------------
//对讲呼叫结束
void RecvTalkCallEnd_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i, j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;

	//本机被动
	if (((Local.Status == 1) || (Local.Status == 2) || (Local.Status == 5) || (Local.Status == 6) || (Local.Status == 7) || (Local.Status == 8)
			|| (Local.Status == 9) || (Local.Status == 10)) && ((recv_buf[7] & 0x03) == ASK))
	{
		Local.OnlineFlag = 0;
		Local.CallConfirmFlag = 0; //设置在线标志

		memcpy(send_b, recv_buf, 57);
		send_b[7] = REPLY; //应答
		sendlength = 57;
		RemotePort = RemoteVideoPort;
		if (recv_buf[6] == VIDEOTALK) //直通呼叫
			RemotePort = RemoteVideoPort;
		else
			//中转呼叫
			RemotePort = RemoteVideoServerPort;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);
		//  if(Local.Status == 10)
		//    ShowStatusText(50, 380 , 3, cBlack, 1, 1, "结束留影", 0);
		//  else
		//    ShowStatusText(50, 380 , 3, cBlack, 1, 1, "对方结束对讲", 0);
#ifdef _MULTI_MACHINE_SUPPORT  //多分机支持
		ExitGroup(recv_buf); //向其它被叫方退出组播组命令
#endif

		TalkEnd_ClearStatus();

		//呼叫结束
#ifdef _DEBUG
		sprintf(Local.DebugInfo, "other end talk, %s\n", cFromIP);
		P_Debug(Local.DebugInfo);
#endif
	}
	/*else if(Local.Status == 0)//add by zlin
	{
		return;
	}*/
	else //本机主动
		//if((Local.Status == 1)||(Local.Status == 2)||(Local.Status == 5)||(Local.Status == 6)
		//		||(Local.Status == 7)||(Local.Status == 8)||(Local.Status == 9)||(Local.Status == 10))
	{
		Local.OnlineFlag = 0;
		Local.CallConfirmFlag = 0; //设置在线标志

		//锁定互斥锁
		pthread_mutex_lock(&Local.udp_lock);
		//本机主动
		if ((recv_buf[7] & 0x03) == REPLY)
		{
			for (i = 0; i < UDPSENDMAX; i++)
			{
				if (Multi_Udp_Buff[i].isValid == 1)
				{
					if (Multi_Udp_Buff[i].m_Socket == m_VideoSocket)
					{
						if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM)
						{
							if ((Multi_Udp_Buff[i].buf[6] == VIDEOTALK) || (Multi_Udp_Buff[i].buf[6] == VIDEOTALKTRANS))
							{
								if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK)
								{
									if (Multi_Udp_Buff[i].buf[8] == CALLEND)
									{
										if (strcmp(Multi_Udp_Buff[i].RemoteHost,cFromIP) == 0)
										{
											Multi_Udp_Buff[i].isValid = 0;

											//TalkEnd_ClearStatus();//del by zlin

											//呼叫结束

											if (DebugMode == 1)
												P_Debug("other reply talk end\n");
											break;
										}
									}
								}
							}
						}
					}
				}
			}
		}
		//打开互斥锁
		pthread_mutex_unlock(&Local.udp_lock);
		//20101112 本机主动，无需 对方 ExitGroup
		//ExitGroup(recv_buf);      //向其它被叫方退出组播组命令
	}
}
//-----------------------------------------------------------------------
//对讲结束，清状态和关闭音视频
void TalkEnd_ClearStatus(void)
{
#ifdef _DEBUG
	sprintf(Local.DebugInfo, "Local.Status=%d\n", Local.Status);
	P_Debug(Local.DebugInfo);
#endif
	//查看是否在其它组播组内
	DropMultiGroup(m_VideoSocket, NULL);
	switch (Remote.Addr[0][0])
	{
	case 'Z': //中心机呼叫
	case 'W': //围墙机呼叫
		Remote.Addr[0][5] = '\0';
		break;
	case 'H': //别墅门口机呼叫
	case 'B': //别墅型室内机呼叫
		Remote.Addr[0][6] = '\0';
		break;
	case 'M': //单元门口机呼叫
		Remote.Addr[0][8] = '\0';
		break;
	case 'S': //室内机呼叫
		Remote.Addr[0][12] = '\0';
		break;
	}
	switch (Local.Status)
	{
	case 1: //本机主叫
		P_Debug("****TalkEnd_ClearStatus()***case 1**,StopRecVideo(),StopPlayWavFile()****\n");
		StopRecVideo();
		sprintf(Local.DebugInfo, "****TalkEnd_ClearStatus()***case 1**,Local.Status is %d****\n", Local.Status);
		P_Debug(Local.DebugInfo);

#ifdef _CAPTUREPIC_TO_CENTER  //捕获图片传到中心
		if (Local.HavePicRecorded == 1) //有照片已录制
		{
			Local.HavePicRecorded = 0;
			//SaveToFlash(3);       //未接听  modified by xuqd
		}
#endif
		break;
	case 5: //本机主叫通话
		LOG_RUNLOG_DEBUG("AndroidAudio StopRecVideo 4148");
		P_Debug("****TalkEnd_ClearStatus()***case 1**,StopRecVideo(),StopPlayWavFile(),StopPlayAudio()****\n");
		StopRecVideo();
#ifdef	_SND_RECORD_
		StopRecAudio();
#endif
#ifdef	_SND_PLAY_
		StopPlayAudio();
#endif
		sprintf(Local.DebugInfo,"****TalkEnd_ClearStatus()***case 2**,Local.Status is %d****\n", Local.Status);
		P_Debug(Local.DebugInfo);
#ifdef _CAPTUREPIC_TO_CENTER  //捕获图片传到中心
		if (Local.HavePicRecorded == 1) //有照片已录制
		{
			Local.HavePicRecorded = 0;
			//SaveToFlash(3);       //未接听 modified by xuqd
		}
#endif
		break;
	}

	Local.Status = 0;
	// 被室内机挂的时候，g_sipCallID应该为0
	// 没有sip终端的时候，g_sipCallID也应该为0
	// 由于sip终端先接听而导致idu挂机的时候，g_sipCallID不为0，此时不消除以下状态
	Local.OnlineFlag = 0;
	Local.CallConfirmFlag = 0; //设置在线标志
	MsgLAN2CCCallHangOff();

}
//-----------------------------------------------------------------------
//对讲强制I帧
void RecvForceIFrame_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i;
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;

	//本机被动
	if ((recv_buf[7] & 0x03) == ASK)
	{
		memcpy(send_b, recv_buf, 57);
		send_b[7] = REPLY; //应答
		sendlength = 57;
		if (Remote.isDirect == 0) //直通呼叫
			RemotePort = RemoteVideoPort;
		else
			//中转呼叫
			RemotePort = RemoteVideoServerPort;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);

		Local.ForceIFrame = 1;

		if (DebugMode == 1)
			P_Debug("receive force IFrame request \n");
	}
	else
	{
		//锁定互斥锁
		pthread_mutex_lock(&Local.udp_lock);
		//本机主动
		if ((recv_buf[7] & 0x03) == REPLY)
		{
			for (i = 0; i < UDPSENDMAX; i++)
			{
				if (Multi_Udp_Buff[i].isValid == 1)
				{
					if (Multi_Udp_Buff[i].m_Socket == m_VideoSocket)
					{
						if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM)
						{
							if ((Multi_Udp_Buff[i].buf[6] == VIDEOTALK) || (Multi_Udp_Buff[i].buf[6] == VIDEOTALKTRANS))
							{
								if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK)
								{
									if (Multi_Udp_Buff[i].buf[8] == FORCEIFRAME)
									{
										if (strcmp(Multi_Udp_Buff[i].RemoteHost, cFromIP) == 0)
										{
											Multi_Udp_Buff[i].isValid = 0;
											if (DebugMode == 1)
												P_Debug("other reply of force IFrame request \n");
											break;
										}
									}
								}
							}
						}
					}
				}
			}
		}
		//打开互斥锁
		pthread_mutex_unlock(&Local.udp_lock);
	}
}
//-----------------------------------------------------------------------
//放大(720*480)
void RecvTalkZoomOut_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i;
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;

	//本机被动
	if ((/*(Local.Status == 1)||(Local.Status == 2)||*/(Local.Status == 5)
			|| (Local.Status == 6)) && ((recv_buf[7] & 0x03) == ASK))
	{
		memcpy(send_b, recv_buf, 57);
		send_b[7] = REPLY; //应答
		sendlength = 57;
		RemotePort = RemoteVideoPort;
		if (recv_buf[6] == VIDEOTALK
		) //直通呼叫
			RemotePort = RemoteVideoPort;
		else
			//中转呼叫
			RemotePort = RemoteVideoServerPort;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);
		if (Local.RecPicSize == 1)
		{
			StopRecVideo(); //352*240
			StartRecVideo(D1_W, D1_H); //720*480
		}
		if (DebugMode == 1)
			P_Debug("other zoomout capture \n");
	}
	else //本机主动
	if (/*(Local.Status == 1)||(Local.Status == 2)||*/(Local.Status == 5)
			|| (Local.Status == 6))
	{
		//锁定互斥锁
		pthread_mutex_lock(&Local.udp_lock);
		//本机主动
		if ((recv_buf[7] & 0x03) == REPLY)
		{
			for (i = 0; i < UDPSENDMAX; i++)
			{
				if (Multi_Udp_Buff[i].isValid == 1)
				{
					if (Multi_Udp_Buff[i].m_Socket == m_VideoSocket)
					{
						if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM)
						{
							if ((Multi_Udp_Buff[i].buf[6] == VIDEOTALK) || (Multi_Udp_Buff[i].buf[6] == VIDEOTALKTRANS))
							{
								if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK)
								{
									if (Multi_Udp_Buff[i].buf[8] == ZOOMOUT)
									{
										if (strcmp(Multi_Udp_Buff[i].RemoteHost, cFromIP) == 0)
										{
											Multi_Udp_Buff[i].isValid = 0;
											if (DebugMode == 1)
												P_Debug("other reply of zoomout capture \n");
											break;
										}
									}
								}
							}
						}
					}
				}
			}
		}
		//打开互斥锁
		pthread_mutex_unlock(&Local.udp_lock);
	}
}
//-----------------------------------------------------------------------
//缩小(352*288)
void RecvTalkZoomIn_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i;
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;

	//本机被动
	if ((/*(Local.Status == 1)||(Local.Status == 2)||*/(Local.Status == 5)
			|| (Local.Status == 6)) && ((recv_buf[7] & 0x03) == ASK))
	{
		memcpy(send_b, recv_buf, 57);
		send_b[7] = REPLY; //应答
		sendlength = 57;
		RemotePort = RemoteVideoPort;
		if (recv_buf[6] == VIDEOTALK) //直通呼叫
			RemotePort = RemoteVideoPort;
		else
			//中转呼叫
			RemotePort = RemoteVideoServerPort;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);
		if (Local.RecPicSize == 2)
		{
			StopRecVideo(); //720*480
			StartRecVideo(CIF_W, CIF_H); //352*288
		}
		if (DebugMode == 1)
			P_Debug("other zoomin capture \n");
	}
	else //本机主动
	if (/*(Local.Status == 1)||(Local.Status == 2)||*/(Local.Status == 5) || (Local.Status == 6))
	{
		//锁定互斥锁
		pthread_mutex_lock(&Local.udp_lock);
		//本机主动
		if ((recv_buf[7] & 0x03) == REPLY)
		{
			for (i = 0; i < UDPSENDMAX; i++)
			{
				if (Multi_Udp_Buff[i].isValid == 1)
				{
					if (Multi_Udp_Buff[i].m_Socket == m_VideoSocket)
					{
						if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM)
						{
							if ((Multi_Udp_Buff[i].buf[6] == VIDEOTALK) || (Multi_Udp_Buff[i].buf[6] == VIDEOTALKTRANS))
							{
								if ((Multi_Udp_Buff[i].buf[7] & 0x03) == ASK)
								{
									if (Multi_Udp_Buff[i].buf[8] == ZOOMIN)
									{
										if (strcmp(Multi_Udp_Buff[i].RemoteHost, cFromIP) == 0)
										{
											Multi_Udp_Buff[i].isValid = 0;
											if (DebugMode == 1)
												P_Debug("other reply of zoomin capture \n");
											break;
										}
									}
								}
							}
						}
					}
				}
			}
		}
		//打开互斥锁
		pthread_mutex_unlock(&Local.udp_lock);
	}
}
//-----------------------------------------------------------------------
//对讲数据
void RecvTalkCallUpDown_Func(unsigned char *recv_buf, char *cFromIP, int length)
{
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	short PackIsExist; //数据包已接收标志
	TempAudioNode1 * tmp_audionode;
	int isFull;
	struct talkdata1 talkdata;
	if ((Local.Status == 1) || (Local.Status == 2) || (Local.Status == 5) || (Local.Status == 6)
			|| (Local.Status == 7) || (Local.Status == 8) || (Local.Status == 9) || (Local.Status == 10)) //状态为对讲
	{
//		P_Debug(Local.DebugInfo, "&&&&&& recv_buf[61] = %d \n", recv_buf[61]);
//		P_Debug(Local.DebugInfo);
		switch (recv_buf[61])
		{
		case 1: //音频
#if 1
			//帧序号
			//       if(AudioMuteFlag == 0)
			//写留影数据函数, 数据  长度  类型 1--音频  2--视频I  3--视频P
			if ((Local.Status == 5) || (Local.Status == 6) || (Local.Status == 10))
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
					tmp_audionode = (TempAudioNode1 *) find_audionode(TempAudioNode_h, talkdata.Frameno, talkdata.CurrPackage);
					if (tmp_audionode == NULL)
					{
						isFull = creat_audionode(TempAudioNode_h, talkdata, recv_buf, length);
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
						TimeStamp.OldCurrAudio = TimeStamp.CurrAudio; //上一次当前音频时间
						TimeStamp.CurrAudio = talkdata.timestamp;

						//   temp_audio_n ++;
						temp_audio_n = length_audionode(TempAudioNode_h);
						if (temp_audio_n >= 4) //VPLAYNUM/2 4帧 128ms
						{
//							P_Debug("&&&&&& audio data recv\n");
							sem_post(&audiorec2playsem);
						}
					}
				}
			}
#endif           
			break;
		case 2: //视频  I帧  352*288
		case 3: //视频  P帧  352*288
		case 4: //视频  I帧  720*480
		case 5: //视频  P帧  720*480
			P_Debug("video data recv\n");
			break;

		}
	}
}
//-----------------------------------------------------------------------
void ForceIFrame_Func(void) //强制I帧
{
	int i;
	int RemotePort;

	for (i = 0; i < UDPSENDMAX; i++)
	{
		if (Multi_Udp_Buff[i].isValid == 0)
		{
			//锁定互斥锁
			pthread_mutex_lock(&Local.udp_lock);
			//只发送一次
			Multi_Udp_Buff[i].SendNum = 5;
			Multi_Udp_Buff[i].m_Socket = m_VideoSocket;
			if (Remote.isDirect == 0) //直通呼叫
				RemotePort = RemoteVideoPort;
			else
				//中转呼叫
				RemotePort = RemoteVideoServerPort;
			Multi_Udp_Buff[i].RemotePort = RemotePort;
			Multi_Udp_Buff[i].CurrOrder = 0;
			sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0",
					Remote.DenIP[0], Remote.DenIP[1], Remote.DenIP[2],
					Remote.DenIP[3]);
			//头部
			memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
			//命令  ,子网广播解析
			if ((Local.Status == 1) || (Local.Status == 2)
					|| (Local.Status == 5) || (Local.Status == 6)
					|| (Local.Status == 7) || (Local.Status == 8)
					|| (Local.Status == 9) || (Local.Status == 10))
			{
				if (Remote.isDirect == 0) //直通呼叫
					Multi_Udp_Buff[i].buf[6] = VIDEOTALK;
				else
					//中转呼叫
					Multi_Udp_Buff[i].buf[6] = VIDEOTALKTRANS;
			}
			if ((Local.Status == 3) || (Local.Status == 4))
			{
				if (Remote.isDirect == 0) //直通呼叫
					Multi_Udp_Buff[i].buf[6] = VIDEOWATCH;
				else
					//中转呼叫
					Multi_Udp_Buff[i].buf[6] = VIDEOWATCHTRANS;
			}
			Multi_Udp_Buff[i].buf[7] = ASK; //主叫
			Multi_Udp_Buff[i].buf[8] = FORCEIFRAME; //FORCEIFRAME

			memcpy(Multi_Udp_Buff[i].buf + 9, LocalCfg.Addr, 20);
			memcpy(Multi_Udp_Buff[i].buf + 29, LocalCfg.IP, 4);
			memcpy(Multi_Udp_Buff[i].buf + 33, Remote.Addr[0], 20);
			memcpy(Multi_Udp_Buff[i].buf + 53, Remote.IP[0], 4);

			Multi_Udp_Buff[i].nlength = 57;
			Multi_Udp_Buff[i].DelayTime = 100;
			Multi_Udp_Buff[i].SendDelayTime = 0; //发送等待时间计数  20101112
			Multi_Udp_Buff[i].isValid = 1;

			//打开互斥锁
			pthread_mutex_unlock(&Local.udp_lock);

			sem_post(&multi_send_sem);
			break;
		}
	}
}
//-----------------------------------------------------------------------
void ExitGroup(unsigned char *buf) //向其它被叫方退出组播组命令
{
	int i, j;
	int isIP;
	int RemotePort;
	char tmp_ip[20];
	//退出组播组
	if (Remote.DenNum > 1)
	{
		//锁定互斥锁
		pthread_mutex_lock(&Local.udp_lock);
		for (j = 0; j < Remote.DenNum; j++)
		{
			Remote.Added[j] = 0;
			if (DebugMode == 1)
			{
				sprintf(Local.DebugInfo, "%d.%d.%d.%d  %d.%d.%d.%d\n", Remote.IP[j][0], Remote.IP[j][1],
						Remote.IP[j][2], Remote.IP[j][3], buf[53], buf[54], buf[55], buf[56]);
				P_Debug(Local.DebugInfo);
			}
			//不为开始通话的那一方
			if ((Remote.IP[j][0] == buf[53]) && (Remote.IP[j][1] == buf[54])
					&& (Remote.IP[j][2] == buf[55]) && (Remote.IP[j][3] == buf[56]))
				isIP = 1;
			else
				isIP = 0;

			//检查是否已经在发送缓冲区内   //xu 20101109
			sprintf(tmp_ip, "%d.%d.%d.%d\0", Remote.IP[j][0], Remote.IP[j][1], Remote.IP[j][2], Remote.IP[j][3]);
			for (i = 0; i < UDPSENDMAX; i++)
			{
				if (Multi_Udp_Buff[i].isValid == 1)
				{
					if ((strcmp(Multi_Udp_Buff[i].RemoteHost, tmp_ip) == 0)
							&& (Multi_Udp_Buff[i].buf[8] == LEAVEGROUP))
					{
						isIP = 1;
						sprintf(Local.DebugInfo, "exit command of multicast group in buffer, %s\n", tmp_ip);
						P_Debug(Local.DebugInfo);
						break;
					}
				}
			}

			if (isIP == 0)
			{
				for (i = 0; i < UDPSENDMAX; i++)
				{
					if (Multi_Udp_Buff[i].isValid == 0)
					{
						Multi_Udp_Buff[i].SendNum = 0;
						Multi_Udp_Buff[i].m_Socket = m_VideoSocket;
						if (Remote.isDirect == 0) //直通呼叫
						{
							RemotePort = RemoteVideoPort;
							Multi_Udp_Buff[i].buf[6] = VIDEOTALK;
							sprintf(Multi_Udp_Buff[i].RemoteHost,
									"%d.%d.%d.%d\0", Remote.IP[j][0],
									Remote.IP[j][1], Remote.IP[j][2],
									Remote.IP[j][3]);
						}
						else //中转呼叫
						{
							RemotePort = RemoteVideoServerPort;
							Multi_Udp_Buff[i].buf[6] = VIDEOTALKTRANS;
							sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0", Remote.DenIP[0],
									Remote.DenIP[1], Remote.DenIP[2], Remote.DenIP[3]);
						}
						Multi_Udp_Buff[i].RemotePort = RemotePort;
						if (DebugMode == 1)
						{
							sprintf(Local.DebugInfo, "requiring exit multicast group, Multi_Udp_Buff[i].RemoteHost is %s\n",
									Multi_Udp_Buff[i].RemoteHost);
							P_Debug(Local.DebugInfo);
						}
						//头部
						memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);

						Multi_Udp_Buff[i].buf[7] = ASK; //主叫
						// 子命令
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
						Multi_Udp_Buff[i].DelayTime = 100; //600;  20101112 改为 100
						Multi_Udp_Buff[i].SendDelayTime = 0; //发送等待时间计数  20101112
						Multi_Udp_Buff[i].isValid = 1;

						sem_post(&multi_send_sem);
						break;
					}
				}
			}
		}
		//打开互斥锁
		pthread_mutex_unlock(&Local.udp_lock);
	}
}
//-----------------------------------------------------------------------
void SingleExitGroup(unsigned char *buf) //向单个被叫方退出组播组命令
{
	int i, j;
	int RemotePort;
	//锁定互斥锁
	pthread_mutex_lock(&Local.udp_lock);
	for (i = 0; i < UDPSENDMAX; i++)
	{
		if (Multi_Udp_Buff[i].isValid == 0)
		{
			Multi_Udp_Buff[i].DelayTime = 100;
			Multi_Udp_Buff[i].SendDelayTime = 0; //发送等待时间计数  20101112
			Multi_Udp_Buff[i].isValid = 1;
			Multi_Udp_Buff[i].SendNum = 0;
			Multi_Udp_Buff[i].m_Socket = m_VideoSocket;
			if (Remote.isDirect == 0) //直通呼叫
			{
				RemotePort = RemoteVideoPort;
				Multi_Udp_Buff[i].buf[6] = VIDEOTALK;
				sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0", buf[53],
						buf[54], buf[55], buf[56]);
			}
			else //中转呼叫
			{
				RemotePort = RemoteVideoServerPort;
				Multi_Udp_Buff[i].buf[6] = VIDEOTALKTRANS;
				sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0",
						Remote.DenIP[0], Remote.DenIP[1], Remote.DenIP[2],
						Remote.DenIP[3]);
			}
			if (DebugMode == 1)
			{
				sprintf(Local.DebugInfo, "Multi_Udp_Buff[i].RemoteHost is %s\n", Multi_Udp_Buff[i].RemoteHost);
				P_Debug(Local.DebugInfo);
				P_Debug("requiring exit multicast group \n");
			}
			//头部
			memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);

			Multi_Udp_Buff[i].buf[7] = ASK; //主叫
			// 子命令
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
			Multi_Udp_Buff[i].SendDelayTime = 0; //发送等待时间计数  20101112
			Multi_Udp_Buff[i].isValid = 1;

			sem_post(&multi_send_sem);
			break;
		}
	}
	//打开互斥锁
	pthread_mutex_unlock(&Local.udp_lock);
}
//-----------------------------------------------------------------------
