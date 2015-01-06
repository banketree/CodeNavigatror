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
 
int DebugMode = 0;           //调试模式

pthread_t idbt_notify_delegate_pthread;
pthread_t cc_pthread;

pthread_mutex_t audio_open_lock;
pthread_mutex_t audio_close_lock;
pthread_mutex_t audio_lock;

extern int InitArpSocket(void);
extern void AddMultiGroup(int m_Socket, char *McastAddr);
extern void InitRecVideo(void);
//******************************end****************************

#ifdef  _MESSAGE_SUPPORT  //消息机制
#include "type.h"
#endif

#define idcard_name "/mnt/mtd/config/idcardcfg"

char sZkPath[80] = "/usr/zk/";
void ReadCfgFile(void);
//写本地设置文件
void WriteCfgFile(void);

#if 0//def _BRUSHIDCARD_SUPPORT   //刷卡功能支持
//读ID卡文件

void ReadIDCardCfgFile(void);

//写ID卡文件

void WriteIDCardFile(void);

//读ID卡应答

void ReadIDCardReply(char *cFromIP);

#endif

extern struct timeval ref_time; //基准时间,音频或视频第一帧

//---------------------------------------------------------------------------
#ifdef _CAPTUREPIC_TO_CENTER  //捕获图片传到中心
//处理捕获的图片
void Deal_CapturePic_Center(void);
#endif
void P_Debug(char *DebugInfo); //调试信息
#ifdef _ECHO_STATE_SUPPORT  //回声消除支持
extern int echo_num;
#endif 
//---------------------------------------------------------------------------
void P_Debug(char *DebugInfo) //调试信息
{
	unsigned char send_b[1500];
	int sendlength;
	int infolen;
	printf(DebugInfo);
	LOG_RUNLOG_DEBUG("LAN_Debug %s", DebugInfo);

	if (Local.RemoteDebugInfo == 1) //发送远程调试信息
	{
		memcpy(send_b, UdpPackageHead, 6);
		send_b[6] = REMOTEDEBUGINFO;
		send_b[7] = ASK; //主叫
		send_b[8] = 2; //子命令
		memcpy(send_b + 9, FLAGTEXT, 20); //标志
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
	int programrun; //程序运行标志
 	int i, j;
	uint32_t Ip_Int;

	DebugMode = 1;

	ref_time.tv_sec = 0; //初始时间戳
	ref_time.tv_usec = 0;
	TimeStamp.OldCurrVideo = 0; //上一次当前视频时间
	TimeStamp.CurrVideo = 0;
	TimeStamp.OldCurrAudio = 0; //上一次当前音频时间
	TimeStamp.CurrAudio = 0;

	//初始化链表头结点
	if (TempAudioNode_h == NULL)
		TempAudioNode_h = (TempAudioNode1 *) init_audionode();

	RemoteDataPort = 8300;
	RemoteVideoPort = 8302;
	RemoteVideoServerPort = 8306; //视频服务器音视频UDP端口
	strcpy(RemoteHost, "192.168.0.88");
	LocalDataPort = 8300; //命令及数据UDP端口
	LocalVideoPort = 8302; //音视频UDP端口
	Local.IP_VideoServer[0] = 192; //视频服务器地址
	Local.IP_VideoServer[1] = 168;
	Local.IP_VideoServer[2] = 68;
	Local.IP_VideoServer[3] = 192;

	Local.NetStatus = 0; //网络状态 0 断开  1 接通
	Local.OldNetSpeed = 100; //网络速度
 
  	Local.LoginServer = 0; //是否登录服务器

	//读配置文件
  	GetCfg();
	//ReadCfgFile();
	if (LocalCfg.Addr[0] == 'M')
	{
		Local.AddrLen = 8; //地址长度  S 12  B 6 M 8 H 6
		memcpy(LocalCfg.Addr + 8, "0000", 4);

	}
	if (LocalCfg.Addr[0] == 'W')
	{
		Local.AddrLen = 5; //地址长度  S 12  B 6 M 8 H 6
		memcpy(LocalCfg.Addr + 5, "0000000", 7);
	}
	LocalCfg.Addr[12] = '\0';


#if 0//def _BRUSHIDCARD_SUPPORT   //刷卡功能支持
	//读ID卡文件
	ReadIDCardCfgFile();
	sprintf(Local.DebugInfo, "IDCardNo.Num = %d\n", IDCardNo.Num);
	P_Debug(Local.DebugInfo);

	//写ID卡信息
	RecvIDCardNo.isNewWriteFlag = 0;
	RecvIDCardNo.Num = 0;
	RecvIDCardNo.serialnum = IDCardNo.serialnum;
#endif

 	Local._TESTNSSERVER = 0; //测试服务器解析模式
	Local._TESTTRANS = 0; //测试视频中转模式
 
	Local.DownProgramOK = 0; //下载应用程序完成
	Local.download_image_flag = 0; //下载系统映像

 	//刷ID卡记录
#if 0//def _BRUSHIDCARD_SUPPORT   //刷卡功能支持
	BrushIDCard.Num = 0;
#endif

	DeltaLen = 9 + sizeof(struct talkdata1); //数据包有效数据偏移量

	strcpy(UdpPackageHead, "XXXCID");
	for (i = 0; i < 20; i++)
		NullAddr[i] = '0';
	NullAddr[20] = '\0';

	Local.Status = 0; //状态为空闲
	Local.NSReplyFlag = 0; //NS应答处理标志
	Local.RecPicSize = 1; //默认视频大小为352*288
	Local.PlayPicSize = 1; //默认视频大小为352*288

	Ip_Int = inet_addr("192.168.68.88");
	memcpy(Remote.IP, &Ip_Int, 4);
	memcpy(Remote.Addr[0], NullAddr, 20);
	memcpy(Remote.Addr[0], "S00010101010", 12); //

	//InitVideoParam();   //初始化视频参数

	//InitAudioParam(); //初始化音频参数
	//InitRecVideo();

	Local.RemoteDebugInfo = 0; //发送远程调试信息

#ifdef _CAPTUREPIC_TO_CENTER  //捕获图片传到中心
	Capture_Pic_Num = 0;
	for (i = 0; i < MAX_CAPTUREPIC_NUM; i++)
	{
		Capture_Pic_Center[i].isVilid = 0;
		Capture_Pic_Center[i].jpeg_pic = NULL;
	}
	Local.RecordPic = 0;
	Local.IFrameNo = 25; //75;    //留第几个I帧
	Local.IFrameCount = 0; //帧计数
	Local.Pic_Width = CIF_W;
	Local.Pic_Height = CIF_H;
	Local.HavePicRecorded = 0; //有照片已录制
	Local.SendToCetnering = 0; //正在传给中心
	Local.ConnectCentered = 0; //与中心连接状态
#endif

	//检测网络连接
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

#if 0//def _BRUSHIDCARD_SUPPORT   //刷卡功能支持
	//将拆分包缓冲置为无效
	for (i = 0; i < DISPARTMAX; i++)
		Dispart_Send_Buff[i].isValid = 0;
	//拆分包发送线程
	sem_init(&dispart_send_sem, 0, 0);
	dispart_send_flag = 1;
	//创建互斥锁
	pthread_mutex_init(&Local.dispart_send_lock, NULL);
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&dispart_send_thread, &attr, (void *) dispart_send_thread_func, NULL);
	pthread_attr_destroy(&attr);
	if (dispart_send_thread == 0)
	{
		P_Debug("无法创建拆分包发送线程\n");
		return FALSE;
	}
#endif

#ifdef _MESSAGE_SUPPORT  //消息机制
	InitMsgQueue();
#endif

//将UDP发送缓冲置为无效
	for (i = 0; i < UDPSENDMAX; i++)
	{
		Multi_Udp_Buff[i].isValid = 0;
		Multi_Udp_Buff[i].SendNum = 0;
		Multi_Udp_Buff[i].DelayTime = 100;
		Multi_Udp_Buff[i].SendDelayTime = 0; //发送等待时间计数  20101112
	}

//主动命令数据发送线程：终端主动发送命令，如延时一段没收到回应，则多次发送
//用于UDP和Comm通信
	sem_init(&multi_send_sem, 0, 0);
	multi_send_flag = 1;
	//创建互斥锁
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

	//主动命令数据发送线程：终端主动发送命令，如延时一段没收到回应，则多次发送

#if 0//def _BRUSHIDCARD_SUPPORT   //刷卡功能支持
	pthread_mutex_init(&Local.idcard_lock, NULL);

#endif

	//初始化ARP Socket
	//InitArpSocket();
	//加入NS组播组
	AddMultiGroup(m_VideoSocket, NSMULTIADDR);
	AddMultiGroup(m_DataSocket, NSMULTIADDR);

	sprintf(Local.DebugInfo, "LocalCfg.IP=%d.%d.%d.%d, LocalCfg.Addr = %s\n",
			LocalCfg.IP[0], LocalCfg.IP[1], LocalCfg.IP[2], LocalCfg.IP[3], LocalCfg.Addr);
	P_Debug(Local.DebugInfo);

	//发送免费ARP
//  SendFreeArp();

	Init_Timer(); //初始化定时器

	programrun = 1;
	ch = ' ';
	P_Debug("Please select: 'q' is exit!\n");

	g_iRunLANTaskFlag = TRUE;
	//Call_Func(2, "0105", "");
	while(g_iRunLANTaskFlag)
	{
//		sleep(20);//rectify by xuqd 前面改成休眠20秒的兄弟注意了，这个时间设置为30秒，这个模块退出的时候最长等待时间也是30秒
//		printf("LAN thread is alive!\n");
		usleep(10000);
	}

	Uninit_Timer(); //释放定时器

	//退出NS组播组
	DropNsMultiGroup(m_VideoSocket, NSMULTIADDR);
	DropNsMultiGroup(m_DataSocket, NSMULTIADDR);

#if 0//def _BRUSHIDCARD_SUPPORT   //刷卡功能支持
	//拆分包发送线程
	dispart_send_flag = 0;
	usleep(40 * 1000);
	pthread_cancel(dispart_send_thread);
	sem_destroy(&dispart_send_sem);
	pthread_mutex_destroy(&Local.dispart_send_lock); //删除互斥锁
#endif

	//UDP主动命令数据发送线程
	multi_send_flag = 0;
	usleep(40 * 1000);
	//pthread_cancel(multi_send_thread);
	sem_destroy(&multi_send_sem);
	pthread_mutex_destroy(&Local.udp_lock); //删除互斥锁

	//COMM主动命令数据发送线程
	multi_comm_send_flag = 0;
	usleep(40 * 1000);
	//pthread_cancel(multi_comm_send_thread);
	sem_destroy(&multi_comm_send_sem);
	pthread_mutex_destroy(&Local.comm_lock); //删除互斥锁

#if 0//def _BRUSHIDCARD_SUPPORT   //刷卡功能支持
	pthread_mutex_destroy(&Local.idcard_lock); //删除互斥锁
#endif

#ifdef _MESSAGE_SUPPORT  //消息机制
	DestroyMsgQueue();
#endif

	pthread_mutex_destroy(&audio_open_lock);
	pthread_mutex_destroy(&audio_close_lock);
	pthread_mutex_destroy(&audio_lock);

 	//关闭ARP
	CloseArpSocket();
	//关闭UDP及接收线程
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
	//系统初始化标志
	//InitSuccFlag = 1;

	while (save_file_flag == 1)
	{
		sem_wait(&save_file_sem);
		printf("save_file_thread_func \n");

		if (Local.Save_File_Run_Flag == 0)
			Local.Save_File_Run_Flag = 1;
		else
		{
			//锁定互斥锁
			pthread_mutex_lock(&Local.save_lock);
			P_Debug("save thread is run\n");
			for (i = 0; i < SAVEMAX; i++)
				if (Save_File_Buff[i].isValid == 1)
				{
					switch (Save_File_Buff[i].Type)
					{
					case 1: //一类信息
						break;
					case 2: //单个信息
						break;
					case 3:
#ifdef _CAPTUREPIC_TO_CENTER  //捕获图片传到中心
						Deal_CapturePic_Center();
#endif
						break;
					case 4: //本地设置
						WriteCfgFile();
						break;
					case 5: //ID卡信息
#if 0//def _BRUSHIDCARD_SUPPORT   //刷卡功能支持
						WriteIDCardFile();
#endif
						break;
					}

					Save_File_Buff[i].isValid = 0;
					system("sync");
					break;
				}
			//打开互斥锁
			pthread_mutex_unlock(&Local.save_lock);
		}
	}
#endif   
}
//---------------------------------------------------------------------------
#ifdef _CAPTUREPIC_TO_CENTER  //捕获图片传到中心
//处理捕获的图片
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
#if 0//def _BRUSHIDCARD_SUPPORT   //刷卡功能支持
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
			//锁定互斥锁
			pthread_mutex_lock(&Local.dispart_send_lock);
			P_Debug("dispart thread is run\n");
			for (i = 0; i < DISPARTMAX; i++)
				if (Dispart_Send_Buff[i].isValid == 1)
				{
					switch (Dispart_Send_Buff[i].Type)
					{
					case 10: //ID卡信息  ReadIDCardReply
						ReadIDCardReply(Dispart_Send_Buff[i].cFromIP);
						break;
					}

					Dispart_Send_Buff[i].isValid = 0;

				}
			//打开互斥锁
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
		//等待有按键按下的信号
		sem_wait(&multi_send_sem);
		HaveDataSend = 1;
		while (HaveDataSend)
		{
			//锁定互斥锁
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
							ArpSendBuff(); //免费ARP发送
						else
						{
							//UDP发送
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
						//20101112 发送等待时间计数, 避免发送时间太长，引起混乱
						Multi_Udp_Buff[i].SendDelayTime += 100;
						if (Multi_Udp_Buff[i].SendDelayTime >= Multi_Udp_Buff[i].DelayTime)
						{
							Multi_Udp_Buff[i].SendDelayTime = 0;
							Multi_Udp_Buff[i].SendNum++;
						}
					}
					if (Multi_Udp_Buff[i].SendNum >= MAXSENDNUM)
					{
						//锁定互斥锁
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
#ifdef _CAPTUREPIC_TO_CENTER  //捕获图片传到中心
							case REPORTSTATUS:
								Multi_Udp_Buff[i].isValid = 0;
								Local.ConnectCentered = 0; //与中心连接状态
								Local.LoginServer = 0; //是否登录服务器
								//if(DebugMode == 1)
								// P_Debug("设备定时报告状态发送失败\n");
								break;
							case CAPTUREPIC_SEND_START: //单元门口机、围墙机、二次门口机发送呼叫照片->发送开始
								Multi_Udp_Buff[i].isValid = 0;
								break;
							case CAPTUREPIC_SEND_DATA: //单元门口机、围墙机、二次门口机发送呼叫照片->发送数据
								Multi_Udp_Buff[i].isValid = 0;
								memcpy(&CurrNo, Multi_Udp_Buff[i].buf + 48,
										sizeof(CurrNo));
								memcpy(&CurrPack,
										Multi_Udp_Buff[i].buf + 56,
										sizeof(CurrPack));
								SendCapturePicFailFlag = 1;
								Local.SendToCetnering = 0; //正在传给中心
								break;
							case CAPTUREPIC_SEND_SUCC: //单元门口机、围墙机、二次门口机发送呼叫照片->发送结束（成功）
								Multi_Udp_Buff[i].isValid = 0;
								Local.SendToCetnering = 0; //正在传给中心
								break;
							case CAPTUREPIC_SEND_FAIL: //单元门口机、围墙机、二次门口机发送呼叫照片->发送结束（失败）
								Multi_Udp_Buff[i].isValid = 0;
								Local.SendToCetnering = 0; //正在传给中心
								break;
#endif
							case NSORDER:
								if (Multi_Udp_Buff[i].CurrOrder == 255) //主机向副机解析
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
							case NSSERVERORDER: //服务器解析
								Multi_Udp_Buff[i].isValid = 0;
#ifdef _DEBUG
								printf("service resolved fail\n");
#endif

								//呼叫窗口
								/*if (Local.CurrentWindow == 0)
								{
									FindError = 1;
									if (Remote.Addr[0][0] == 'Z')
									{
										if (Remote.Addr[0][4] != '1')
										{
											WaitAudioUnuse(200); //等待声卡空闲
											sprintf(wavFile,"%s/inuse.wav\0",wavPath);
											StartPlayWav(wavFile, 0);
											FindError = 0;
										}
									}
									if (FindError == 1)
									{
										{
											WaitAudioUnuse(200); //等待声卡空闲
											sprintf(wavFile,"%s/callfail.wav\0",wavPath);
											StartPlayWav(wavFile, 0);
										}
									}
									ClearTalkEdit(); //清 文本框
								}*/

								break;
							case VIDEOTALK: //局域网可视对讲
							case VIDEOTALKTRANS: //局域网可视对讲中转服务
								switch (Multi_Udp_Buff[i].buf[8])
								{
								case JOINGROUP: //加入组播组（主叫方->被叫方，被叫方应答）
									Multi_Udp_Buff[i].isValid = 0;
#ifdef _DEBUG
									printf("add multicast fail, %d\n", Multi_Udp_Buff[i].buf[6]);
#endif
									break;
								case CALL:
									if (Multi_Udp_Buff[i].buf[6] == VIDEOTALK)
									{
										//有主副机，清其它的呼叫
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
								case CALLEND: //通话结束
									Multi_Udp_Buff[i].isValid = 0;
									Local.OnlineFlag = 0;
									Local.CallConfirmFlag = 0; //设置在线标志
									printf("multi_send_thread_func :: TalkEnd_ClearStatus()\n");
									//对讲结束，清状态和关闭音视频
									TalkEnd_ClearStatus();
									break;
								default: //为其它命令，本次通信结束
									Multi_Udp_Buff[i].isValid = 0;
#ifdef _DEBUG
									printf("communicate fail 1, %d\n", Multi_Udp_Buff[i].buf[6]);
#endif
									break;
								}
								break;
							case VIDEOWATCH: //局域网监控
							case VIDEOWATCHTRANS: //局域网监控中转服务
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
								case CALLEND: //通话结束
									Multi_Udp_Buff[i].isValid = 0;
									Local.OnlineFlag = 0;
									Local.CallConfirmFlag = 0; //设置在线标志

									switch (Local.Status)
									{
									case 3: //本机监视
										break;
									case 4: //本机被监视
										Local.Status = 0; //状态为空闲
										MsgLAN2CCCallHangOff();
										//	StopRecVideo();
										break;
									}
									break;
								default: //为其它命令，本次通信结束
									Multi_Udp_Buff[i].isValid = 0;
#ifdef _DEBUG
									printf("communicate fail 2, %d\n", Multi_Udp_Buff[i].buf[6]);
#endif
									break;
								}
								break;
							default: //为其它命令，本次通信结束
								Multi_Udp_Buff[i].isValid = 0;
								//     Local.Status = 0;
#ifdef _DEBUG
								printf("communicate fail 3, %d\n", Multi_Udp_Buff[i].buf[6]);
#endif
								break;
							}
						}
						//打开互斥锁
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

#ifdef _CAPTUREPIC_TO_CENTER  //捕获图片传到中心
			if (SendCapturePicFailFlag == 1)
			{
				//printf("3333333333, CurrNo = %d, CurrPack = %d\n", CurrNo, CurrPack);
				SendCapturePicFinish(CurrNo, 0);
			}
			SendCapturePicFailFlag = 0;
#endif

			//判断数据是否全部发送完，若是，线程终止
			HaveDataSend = 0;
			for (i = 0; i < UDPSENDMAX; i++)
				if (Multi_Udp_Buff[i].isValid == 1)
				{
					HaveDataSend = 1;
					break;
				}
			//打开互斥锁
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

	//地址编码
	memcpy(LocalCfg.Addr, NullAddr, 20);
	memcpy(LocalCfg.Addr, g_stIdbtCfg.acPositionCode, 12);
	//网卡地址
	LocalCfg.Mac_Addr[0] = 0x00;
	LocalCfg.Mac_Addr[1] = 0x19;
	LocalCfg.Mac_Addr[2] = 0xA6;
	LocalCfg.Mac_Addr[3] = 0x44;
	LocalCfg.Mac_Addr[4] = 0x55;
	LocalCfg.Mac_Addr[5] = 0x88;
	//IP地址
	NMGetIpAddress2(LocalCfg.IP);
	//子网掩码
	Ip_Int = inet_addr("255.255.255.0");
	memcpy(LocalCfg.IP_Mask, &Ip_Int, 4);
	//网关地址
	Ip_Int = inet_addr("192.168.68.1");
	memcpy(LocalCfg.IP_Gate, &Ip_Int, 4);
	//NS（名称解析）服务器地址
	Ip_Int = inet_addr("192.168.68.1");
	memcpy(LocalCfg.IP_NS, &Ip_Int, 4);
	//主服务器地址
	Ip_Int = inet_addr("192.168.68.1");
	memcpy(LocalCfg.IP_Server, &Ip_Int, 4);
	//广播地址
	for (i = 0; i < 4; i++)
	{
		if (LocalCfg.IP_Mask[i] != 0)
			LocalCfg.IP_Broadcast[i] = LocalCfg.IP_Mask[i] & LocalCfg.IP[i];
		else
			LocalCfg.IP_Broadcast[i] = 0xFF;
	}

	//设备定时报告状态时间
	LocalCfg.ReportTime = 10;

	//密码开锁
	LocalCfg.PassOpenLock = 1;
	//刷卡开锁
	LocalCfg.CardOpenLock = 1;
	//房号类型
	LocalCfg.RoomType = 0;
	//呼叫回铃
	LocalCfg.CallWaitRing = 0;

	//语音提示  0 关闭  1  打开
	LocalCfg.VoiceHint = 1;
	//按键音    0 关闭  1  打开
	LocalCfg.KeyVoice = 1;
}
//读配置文件
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
	//查找配置文件目录是否存在
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
		//打开配置文件
		//  if(access(cfg_name, R_OK|W_OK) == NULL)
		if ((cfg_fd = fopen(cfg_name, "rb")) == NULL)
		{
			P_Debug("config file is not exist! create new config file \n");
			// if((cfg_fd = open(cfg_name, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) == -1)
			if ((cfg_fd = fopen(cfg_name, "wb")) == NULL)
				P_Debug("don't create config file!\n");
			else
			{
				//地址编码
				memcpy(LocalCfg.Addr, NullAddr, 20);
				memcpy(LocalCfg.Addr, "M00010100000", 12);

				//网卡地址
				LocalCfg.Mac_Addr[0] = 0x00;
				LocalCfg.Mac_Addr[1] = 0x19;
				LocalCfg.Mac_Addr[2] = 0xA6;
				LocalCfg.Mac_Addr[3] = 0x44;
				LocalCfg.Mac_Addr[4] = 0x55;
				LocalCfg.Mac_Addr[5] = 0x88;
				//IP地址
				Ip_Int = inet_addr("192.168.68.180");
				memcpy(LocalCfg.IP, &Ip_Int, 4);
				//子网掩码
				Ip_Int = inet_addr("255.255.255.0");
				memcpy(LocalCfg.IP_Mask, &Ip_Int, 4);
				//网关地址
				Ip_Int = inet_addr("192.168.68.1");
				memcpy(LocalCfg.IP_Gate, &Ip_Int, 4);
				//NS（名称解析）服务器地址
				Ip_Int = inet_addr("192.168.68.1");
				memcpy(LocalCfg.IP_NS, &Ip_Int, 4);
				//主服务器地址
				Ip_Int = inet_addr("192.168.68.1");
				memcpy(LocalCfg.IP_Server, &Ip_Int, 4);
				//广播地址
				Ip_Int = inet_addr("192.168.68.255");
				memcpy(LocalCfg.IP_Broadcast, &Ip_Int, 4);

				//设备定时报告状态时间
				LocalCfg.ReportTime = 10;

				//电锁类型  0 －－ 电控锁  1 －－ 电磁锁
				LocalCfg.LockType = 0;
				//开锁时间  电控锁 0 ――100ms  1――200ms
				//          电磁锁 0 ――5s  1――10s
				LocalCfg.OpenLockTime = 0;
				//延时开锁  0 0s  1 3s  2 5s  3 10s
				LocalCfg.DelayLockTime = 0;
				//密码开锁
				LocalCfg.PassOpenLock = 1;
				//刷卡开锁
				LocalCfg.CardOpenLock = 1;
				//刷卡通信
				LocalCfg.CardComm = 1;
				//门磁检测
				LocalCfg.DoorDetect = 1;
				//房号类型
				LocalCfg.RoomType = 0;
				//呼叫回铃
				LocalCfg.CallWaitRing = 0;

				//工程密码
				strcpy(LocalCfg.EngineerPass, "888888");
				strcpy(LocalCfg.OpenLockPass, "123456");
				//语音提示  0 关闭  1  打开
				LocalCfg.VoiceHint = 1;
				//按键音    0 关闭  1  打开
				LocalCfg.KeyVoice = 1;
				//触摸屏
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
//        printf("无法打开配置文件\n");
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
					RefreshNetSetup(0); //刷新网络设置
					//广播地址
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
//写本地设置文件
void WriteCfgFile(void)
{
	FILE *cfg_fd;
	int j;
	uint8_t isValid;
	if ((cfg_fd = fopen(cfg_name, "wb")) == NULL)
		P_Debug("don't create config file \n");
	else
	{
		//重写本地设置文件
		fwrite(&LocalCfg, sizeof(LocalCfg), 1, cfg_fd);
		fclose(cfg_fd);
		P_Debug("save local set succeed \n");
	}
}
//---------------------------------------------------------------------------
#if 0//def _BRUSHIDCARD_SUPPORT   //刷卡功能支持
//读ID卡文件
void ReadIDCardCfgFile(void)
{
	FILE *idcardcfg_fd;
	int bytes_write;
	int bytes_read;
	int ReadOk;
	DIR *dirp;
	int i, j;
	//查找配置文件目录是否存在
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
		//打开配置文件
		//  if(access(cfg_name, R_OK|W_OK) == NULL)
		if ((idcardcfg_fd = fopen(idcard_name, "rb")) == NULL)
		{
			P_Debug("ID卡配置文件不存在，创建新文件\n");
			// if((cfg_fd = open(cfg_name, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) == -1)
			if ((idcardcfg_fd = fopen(idcard_name, "wb")) == NULL)
				P_Debug("无法创建ID卡配置文件\n");
			else
			{
				//数量
				IDCardNo.Num = 0;
				//序号
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
				P_Debug("读取ID卡配置文件失败,以默认方式重建ID卡配置文件\n");
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
//写ID卡文件
void WriteIDCardFile(void)
{
	FILE *idcardcfg_fd;
	int j;
	uint8_t isValid;
	if ((idcardcfg_fd = fopen(idcard_name, "wb")) == NULL
		)
		P_Debug("无法创建IDCard文件\n");
	else
	{
		//重写IDCard文件
		fwrite(&IDCardNo.Num, sizeof(IDCardNo.Num), 1, idcardcfg_fd);
		fwrite(&IDCardNo.serialnum, sizeof(IDCardNo.serialnum), 1,
				idcardcfg_fd);
		fwrite(IDCardNo.SN, BYTEPERSN * IDCardNo.Num, 1, idcardcfg_fd);
		fclose(idcardcfg_fd);
		P_Debug("存储IDCard文件成功\n");
	}
}
//---------------------------------------------------------------------------
//读ID卡应答
void ReadIDCardReply(char *cFromIP)
{
	unsigned char send_b[1520];
	int sendlength;
	int i;
	//读写ID卡数据结构
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
		//头部
		memcpy(send_b, UdpPackageHead, 6);
		//命令
		send_b[6] = READIDCARD;
		send_b[7] = REPLY; //应答

		memcpy(iddata.Addr, LocalCfg.Addr, 20);
		//序号
		iddata.serialnum = IDCardNo.serialnum;

		//卡总数
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
	//打开互斥锁
}
#endif

