/************************************************************
 *  File:
 *  Author:
 *  Version :
 *  Date:
 *  Description:
 *  History:
 *            <chensq>  <2012-03-31>  <增加未操作超时休眠以及唤醒休眠功能>
*            <chensq>  <2012-04-01>  <增加SIP注册状态检测与界面显示功能>
 ***********************************************************/

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/ipc.h>
#include "LPIntfLayer.h"

typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned char u8;

//***************************add by xuqd*****************
#include <XTIntfLayer.h>
//*****************************end***********************


//#include <linux/ethtool.h>
#include <net/if.h>
#include <linux/sockios.h>
#define ETHTOOL_GLINK		0x0000000a /* Get link status (ethtool_value) */
#define ETHTOOL_GSET		0x00000001 /* Get settings. */
#define ETHTOOL_SSET		0x00000002 /* Set 功能settings, privileged. */
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

//检测网络信号
void CheckEth(void);
int detect_ethtool(int skfd, char *ifname);
int get_ethtool_speed(int skfd, char *ifname); //获取网卡速率
int set_ethtool_speed(int skfd, char *ifname, int speed, int duplex,
		int autoneg); //设置网卡速率

//检测IP地址冲突
void CheckIPConflict(void);

void OnlineCheckFunc(void); //在线确认检测函数
void TimeReportStatusFunc(void); //设备定时报告状态函数

void ScreenOperFunc(void); //无操作退回主界面或关屏
void ClearTipFunc(void); //清状态提示信息函数

void ResetParam(void); //恢复设置

int CursorFlag;
int ShowCursor; //是否显示光标
void WriteCursor(struct TEdit *t_edit, int t_Flag, int t_show, int PageNo); //写光标函数   t_Flag 0--清除  1--写
void ShowCursorFunc(void); //显示输入光标函数

void TalkCtrlFunc(void); //对讲控制，显示通话时间和判断超时
void OpenDoorFunc(void); //开锁函数

extern struct audiobuf1 recbuf; //音频采集环形缓冲
extern struct videobuf1 videorecbuf; //视频采集环形缓冲  环形?

#ifdef _CAPTUREPIC_TO_CENTER  //捕获图片传到中心
//检查捕获图片传到中心
void CheckCapturePicCenter(void);
void SendCapturePicData(int CurrNo);
void SendCapturePicFinish(int CurrNo, int Result);
#endif

//timer 定时器
int timer_fd;
int timer_thread_flag;
pthread_t timer_thread;
void timer_thread_func(void);
//char cRegOldFlg = 0;

int Init_Timer(void); //初始化定时器
int Uninit_Timer(void); //释放定时器

void CheckMainFbPage(void); //检查是否在framebuffer 的第0页
//---------------------------------------------------------------------------
int Init_Timer(void) {
	pthread_attr_t attr;
	timer_thread_flag = 1;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&timer_thread, &attr, (void *) timer_thread_func, NULL);
	pthread_attr_destroy(&attr);
	if (timer_thread == 0) {
		printf("无法创建timer处理线程\n");
		return;
	}
}
//---------------------------------------------------------------------------
int Uninit_Timer(void) {
	//timer
	timer_thread_flag = 0;
	usleep(40 * 1000);
	pthread_cancel(timer_thread);
}
//---------------------------------------------------------------------------
void timer_thread_func(void) {
#if 1
	int timenum; //计数
	int old_sd_status;
	int new_sd_status;
	int sendlength;
	unsigned char send_b[1520];
	char RemoteHost[20];

	int i;
	//时钟
	time_t t;
	struct tm *tm_t;
	char strtime[30];
	int tmpnum;
	DIR *dirp;

	static int flag = 0;


	P_Debug("创建timer线程：0.001\n");

	timenum = 0;

	while (timer_thread_flag) {
		//    gettimeofday(&tv1, NULL);

//		if ((timenum % (INTRPERSEC)) == 0) {//20*50=*2=2s 1S
//			//add by chensq 20120401 SIP注册成功与失败提示
//			SipStatusChangeFunc();
//		}

			if ((timenum % (INTRPERSEC*5)) == 0) {//20*50=*5=5s
				//add by chensq 20120401 SIP注册成功与失败提示
				//printf("******************timer check sip status:%d\n", Local.cRegFlg);
//				printf("********************************** %d\n", LPGetSipRegStatus());
				DisplaySipStatusWindow();
			}

		if ((timenum % (INTRPERSEC * 2)) == 0) {
			//检测网络信号
			CheckEth();    // add by xuqd
		}

		if(Local.cLockFlg == 1)
		{
			if ((timenum % (INTRPERSEC * 5)) == 0)
			{
				Local.cLockFlg = 0;
			}
		}

		//连续6秒没有收到在线确认，则认为断线
		if (Local.OnlineFlag == 1) {
			OnlineCheckFunc();
		}

		//检测IP地址冲突
		CheckIPConflict();

		//连续6秒没有收到在线确认，则认为断线
		if (Local.OnlineFlag == 1) {
			OnlineCheckFunc();
		}
		//开锁
		if (Local.OpenDoorFlag > 0) {
			OpenDoorFunc();
		}

		ScreenOperFunc(); //无操作退回主界面或关屏
		//清状态提示信息函数
		ClearTipFunc();

		//设备定时报告状态函数
		if (LocalCfg.ReportTime != 0) {
			if (Local.ReportSend == 1)
				if (Local.ReportTimeNum >= (LocalCfg.ReportTime * INTRPERSEC)) {
					Local.RandReportTime = (int) (1.0 * LocalCfg.ReportTime
							* rand() / (RAND_MAX + 1.0)) + 1;
					Local.ReportSend = 0;
					Local.ReportTimeNum = 0;

					//sprintf(Local.DebugInfo, "LocalCfg.ReportTime = %d, Local.RandReportTime = %d\n",
					//        LocalCfg.ReportTime, Local.RandReportTime);
					//P_Debug(Local.DebugInfo);
				}
			if (Local.ReportSend == 0)
				if (Local.ReportTimeNum
						>= (Local.RandReportTime * INTRPERSEC)) {
					CheckRecVideoStatus(); //检查 视频录制状态，在gpio.c 里面检查
					Local.ReportSend = 1;
					TimeReportStatusFunc();
				}
			Local.ReportTimeNum++;
		}

		if (Local.CallKeyFlag == 1) //呼叫键标志     20110901  xu
				{
			if ((timenum % (INTRPERSEC)) == 0) {
				Local.CallKeyTime++; //呼叫键计数
			}
			if (Local.CallKeyTime >= 2) {
				Local.CallKeyFlag = 0; //呼叫键标志     20110901  xu
				Local.CallKeyTime = 0; //呼叫键计数
			}
		}

		if (Local.TalkFinishFlag == 1) //通话结束标志     20101029  xu
				{
			if ((timenum % (INTRPERSEC)) == 0) {
				Local.TalkFinishTime++; //通话结束计数
				sprintf(Local.DebugInfo, "正在通话结束延时中...  %d\n",
						Local.TalkFinishTime);
				P_Debug(Local.DebugInfo);
			}
			if (Local.TalkFinishTime >= 3) {
				Local.TalkFinishFlag = 0; //通话结束标志     20101029  xu
				Local.TalkFinishTime = 0; //通话结束计数
			}
		}

		if (Local.CaptureVideoStartFlag == 1) //捕获视频开始标志     20101109  xu
				{
			if ((timenum % 4) == 0) {
				Local.CaptureVideoStartTime++; //捕获视频开始计数
				Local.ForceIFrame = 1;
			}
			if (Local.CaptureVideoStartTime >= 10) {
				Local.CaptureVideoStartFlag = 0; //捕获视频开始标志     20101109  xu
				Local.CaptureVideoStartTime = 0; //捕获视频开始计数
			}
		}

		//写共享内存到守护进程
		if ((timenum % (INTRPERSEC * 6)) == 0)

		{

#ifdef _CAPTUREPIC_TO_CENTER  //捕获图片传到中心
			//检查捕获图片传到中心
			CheckCapturePicCenter();
#endif
		}

//		if ((timenum % (INTRPERSEC * 6)) == 0)
//			SendOneOrder(_M8_WATCHDOG_START, 0, NULL); //启动 M8 看门狗
		//清WatchDog标志
		//ClearWatchFlag();

		timenum++;

		if (timenum > 0xFFFFFF)

			timenum = 0;

		usleep((INTRTIME - 10) * 1000); //40ms
	}
#endif
}
//---------------------------------------------------------------------------
void ResetParam(void) //恢复设置
{
	uint32_t Ip_Int;

	//IP地址
	Ip_Int = inet_addr("192.168.68.170");
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

	Save_Setup(); //存储更改
	RefreshNetSetup(1); //刷新网络设置
}
//---------------------------------------------------------------------------
#ifdef _CAPTUREPIC_TO_CENTER  //捕获图片传到中心
//检查捕获图片传到中心
void CheckCapturePicCenter(void) {
	int i, j, k;
	//sprintf(Local.DebugInfo, "Local.ConnectCentered = %d, Local.SendToCetnering = %d\n", Local.ConnectCentered, Local.SendToCetnering);
	//P_Debug(Local.DebugInfo);
	if (Local.ConnectCentered == 0) //与中心连接状态
		return;
	if (Local.SendToCetnering == 1) //正在传给中心
		return;
	for (j = 0; j < MAX_CAPTUREPIC_NUM; j++)
		if (Capture_Pic_Center[j].isVilid == 1) {
			//锁定互斥锁
			pthread_mutex_lock(&Local.udp_lock);
			//查找可用发送缓冲并填空
			for (i = 0; i < UDPSENDMAX; i++)
				if (Multi_Udp_Buff[i].isValid == 0) {
					sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0",
							LocalCfg.IP_Server[0], LocalCfg.IP_Server[1],
							LocalCfg.IP_Server[2], LocalCfg.IP_Server[3]);

					Multi_Udp_Buff[i].SendNum = 0; //最多发6次
					Multi_Udp_Buff[i].m_Socket = m_DataSocket;
					Multi_Udp_Buff[i].RemotePort = RemoteDataPort;
					Multi_Udp_Buff[i].CurrOrder = 0;
					//头部
					memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
					//命令
					Multi_Udp_Buff[i].buf[6] = CAPTUREPIC_SEND_START;
					Multi_Udp_Buff[i].buf[7] = ASK; //主叫

					memcpy(Multi_Udp_Buff[i].buf + 8, LocalCfg.Addr, 20);
					memcpy(Multi_Udp_Buff[i].buf + 28,
							Capture_Pic_Center[j].RemoteAddr, 20); //目标地址
					memcpy(Multi_Udp_Buff[i].buf + 48, &j, sizeof(j)); //序列号

					Multi_Udp_Buff[i].buf[52] =
							(Capture_Pic_Center[j].recpic_tm_t->tm_year + 1900)
									/ 256;
					Multi_Udp_Buff[i].buf[53] =
							(Capture_Pic_Center[j].recpic_tm_t->tm_year + 1900)
									% 256;
					Multi_Udp_Buff[i].buf[54] =
							Capture_Pic_Center[j].recpic_tm_t->tm_mon + 1;
					Multi_Udp_Buff[i].buf[55] =
							Capture_Pic_Center[j].recpic_tm_t->tm_mday;
					Multi_Udp_Buff[i].buf[56] =
							Capture_Pic_Center[j].recpic_tm_t->tm_hour;
					Multi_Udp_Buff[i].buf[57] =
							Capture_Pic_Center[j].recpic_tm_t->tm_min;
					Multi_Udp_Buff[i].buf[58] =
							Capture_Pic_Center[j].recpic_tm_t->tm_sec;

					sprintf(Local.DebugInfo,
							"Capture_Pic_Center[%d].jpegsize = %d\n", j,
							Capture_Pic_Center[j].jpegsize);
					P_Debug(Local.DebugInfo);

					memcpy(Multi_Udp_Buff[i].buf + 59,
							&Capture_Pic_Center[j].jpegsize,
							sizeof(Capture_Pic_Center[j].jpegsize)); //文件长度
					if ((Capture_Pic_Center[j].jpegsize % PACKDATALEN) == 0)
						Capture_Total_Package = Capture_Pic_Center[j].jpegsize
								/ PACKDATALEN;
					else
						Capture_Total_Package = Capture_Pic_Center[j].jpegsize
								/ PACKDATALEN + 1;
					memcpy(Multi_Udp_Buff[i].buf + 63, &Capture_Total_Package,
							sizeof(Capture_Total_Package)); //总包数
					for (k = 0; k < Capture_Total_Package; k++)
						Capture_Send_Flag[k] = 0;
					Multi_Udp_Buff[i].nlength = 67;
					Multi_Udp_Buff[i].DelayTime = 100;
					Multi_Udp_Buff[i].SendDelayTime = 0; //发送等待时间计数  20101112
					Multi_Udp_Buff[i].isValid = 1;
					sem_post(&multi_send_sem);
					break;
				}
			//打开互斥锁
			pthread_mutex_unlock(&Local.udp_lock);
			break;
		}
}
//---------------------------------------------------------------------------
void SendCapturePicData(int CurrNo) {
	int i, j, k;
	int CurrSize;
	short CurrPack;
	if (Local.ConnectCentered == 0)
		return;
	//P_Debug("SendCapturePicData 1\n");
	Local.SendToCetnering = 1; //正在传给中心
	for (j = 0; j < Capture_Total_Package; j++)
		if (Capture_Send_Flag[j] == 0) {
			//锁定互斥锁
			pthread_mutex_lock(&Local.udp_lock);
			//查找可用发送缓冲并填空
			for (i = 0; i < UDPSENDMAX; i++)
				if (Multi_Udp_Buff[i].isValid == 0) {
					sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0",
							LocalCfg.IP_Server[0], LocalCfg.IP_Server[1],
							LocalCfg.IP_Server[2], LocalCfg.IP_Server[3]);
					Multi_Udp_Buff[i].SendNum = 0; //最多发6次
					Multi_Udp_Buff[i].m_Socket = m_DataSocket;
					Multi_Udp_Buff[i].RemotePort = RemoteDataPort;
					Multi_Udp_Buff[i].CurrOrder = 0;
					//头部
					memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
					//命令
					Multi_Udp_Buff[i].buf[6] = CAPTUREPIC_SEND_DATA;
					Multi_Udp_Buff[i].buf[7] = ASK; //主叫

					memcpy(Multi_Udp_Buff[i].buf + 8, LocalCfg.Addr, 20);
					memcpy(Multi_Udp_Buff[i].buf + 28,
							Capture_Pic_Center[CurrNo].RemoteAddr, 20); //目标地址
					memcpy(Multi_Udp_Buff[i].buf + 48, &CurrNo, sizeof(CurrNo)); //序列号

					//sprintf(Local.DebugInfo, "SendCapturePicData j = %d, Capture_Total_Package = %d\n", j, Capture_Total_Package);
					//P_Debug(Local.DebugInfo);
					if (j != (Capture_Total_Package - 1))
						CurrSize = PACKDATALEN;
					else
						CurrSize = Capture_Pic_Center[CurrNo].jpegsize
								% PACKDATALEN;
					//sprintf(Local.DebugInfo, "SendCapturePicData CurrSize = %d\n", CurrSize);
					//P_Debug(Local.DebugInfo);
					memcpy(Multi_Udp_Buff[i].buf + 52, &CurrSize,
							sizeof(CurrSize)); //数据长度

					CurrPack = j;
					memcpy(Multi_Udp_Buff[i].buf + 56, &CurrPack,
							sizeof(CurrPack)); //数据包编号

					//sprintf(Local.DebugInfo, "SendCapturePicData CurrPack = %d\n", CurrPack);
					//P_Debug(Local.DebugInfo);

					memcpy(
							Multi_Udp_Buff[i].buf + 58,
							Capture_Pic_Center[CurrNo].jpeg_pic
									+ CurrPack * PACKDATALEN, CurrSize);

					//P_Debug("SendCapturePicData 3\n");
					Multi_Udp_Buff[i].nlength = 58 + CurrSize;
					Multi_Udp_Buff[i].DelayTime = 100;
					Multi_Udp_Buff[i].SendDelayTime = 0; //发送等待时间计数  20101112
					Multi_Udp_Buff[i].isValid = 1;
					sem_post(&multi_send_sem);
					break;
				}
			//打开互斥锁
			pthread_mutex_unlock(&Local.udp_lock);
		}
}
//---------------------------------------------------------------------------
void SendCapturePicFinish(int CurrNo, int Result) {
	int i;
	if (Result == 1)
		P_Debug("发送呼叫照片->发送结束(成功) 命令\n");
	else
		P_Debug("发送呼叫照片->发送结束(失败) 命令\n");
	//锁定互斥锁
	pthread_mutex_lock(&Local.udp_lock);
	//查找可用发送缓冲并填空
	for (i = 0; i < UDPSENDMAX; i++)
		if (Multi_Udp_Buff[i].isValid == 0) {
			sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0",
					LocalCfg.IP_Server[0], LocalCfg.IP_Server[1],
					LocalCfg.IP_Server[2], LocalCfg.IP_Server[3]);
			Multi_Udp_Buff[i].SendNum = 0; //最多发6次
			Multi_Udp_Buff[i].m_Socket = m_DataSocket;
			Multi_Udp_Buff[i].RemotePort = RemoteDataPort;
			Multi_Udp_Buff[i].CurrOrder = 0;
			//头部
			memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
			//命令
			if (Result == 1)
				Multi_Udp_Buff[i].buf[6] = CAPTUREPIC_SEND_SUCC;
			else
				Multi_Udp_Buff[i].buf[6] = CAPTUREPIC_SEND_FAIL;
			Multi_Udp_Buff[i].buf[7] = ASK; //主叫

			memcpy(Multi_Udp_Buff[i].buf + 8, LocalCfg.Addr, 20);
			memcpy(Multi_Udp_Buff[i].buf + 28,
					Capture_Pic_Center[CurrNo].RemoteAddr, 20); //目标地址
			memcpy(Multi_Udp_Buff[i].buf + 48, &CurrNo, sizeof(CurrNo)); //序列号

			Multi_Udp_Buff[i].nlength = 52;
			Multi_Udp_Buff[i].DelayTime = 100;
			Multi_Udp_Buff[i].SendDelayTime = 0; //发送等待时间计数  20101112
			Multi_Udp_Buff[i].isValid = 1;
			sem_post(&multi_send_sem);
			break;
		}
	//打开互斥锁
	pthread_mutex_unlock(&Local.udp_lock);
}
#endif
//---------------------------------------------------------------------------
void OnlineCheckFunc(void) //在线确认检测函数
{
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;
	if (Local.Timer1Num > INTRPERSEC * 6) {
		if (Local.CallConfirmFlag == 0) {
			if ((Local.Status == 1) || (Local.Status == 2)
					|| (Local.Status == 5) || (Local.Status == 6)
					|| (Local.Status == 7) || (Local.Status == 8)
					|| (Local.Status == 9) || (Local.Status == 10)) //对讲
				TalkEnd_Func();
			if ((Local.Status == 3) || (Local.Status == 4)) //监视
				WatchEnd_Func();

			Local.OnlineFlag = 0;
			//通讯中断
			if (DebugMode == 1)
				P_Debug("没有收到在线确认，强制结束\n");

			//********************add by xuqd****************
			printf("******************no online\n");
			g_callIDUFlag = 0;
			g_softSipClearFlag = 1;
			TalkEnd_ClearStatus();
			//***********************************************

		} else
			Local.CallConfirmFlag = 0;
		Local.Timer1Num = 0;
	} else if ((Local.Timer1Num % INTRPERSEC) == 0) {
		//对讲时被叫方发送在线确认包，每秒一个
		//监控时主控方发送在线确认包，每秒一个
		if ((Local.Status == 2) || (Local.Status == 6) || (Local.Status == 8)
				|| (Local.Status == 10) || (Local.Status == 3)) {
			//头部
			memcpy(send_b, UdpPackageHead, 6);
			//命令
			if ((Local.Status == 2) || (Local.Status == 6)
					|| (Local.Status == 8) || (Local.Status == 10)) //对讲
					{
				if (Remote.isDirect == 0) //直通呼叫
					send_b[6] = VIDEOTALK;
				else
					//中转呼叫
					send_b[6] = VIDEOTALKTRANS;
			}
			if (Local.Status == 3) //监视
					{
				if (Remote.isDirect == 0) //直通呼叫
					send_b[6] = VIDEOWATCH;
				else
					//中转呼叫
					send_b[6] = VIDEOWATCHTRANS;
			}

			send_b[7] = ASK; //主叫
			send_b[8] = CALLCONFIRM; //通话在线确认
			//子命令
			if ((Local.Status == 1) || (Local.Status == 3)
					|| (Local.Status == 5) || (Local.Status == 7)
					|| (Local.Status == 9)) //本机为主叫方
					{
				memcpy(send_b + 9, LocalCfg.Addr, 20);
				memcpy(send_b + 29, LocalCfg.IP, 4);
				memcpy(send_b + 33, Remote.Addr[0], 20);
				memcpy(send_b + 53, Remote.IP[0], 4);
			}
			if ((Local.Status == 2) || (Local.Status == 4)
					|| (Local.Status == 6) || (Local.Status == 8)
					|| (Local.Status == 10)) //本机为被叫方
					{
				memcpy(send_b + 9, Remote.Addr[0], 20);
				memcpy(send_b + 29, Remote.IP[0], 4);
				memcpy(send_b + 33, LocalCfg.Addr, 20);
				memcpy(send_b + 53, LocalCfg.IP, 4);
			}
			//确认序号
			send_b[57] = (Local.OnlineNum & 0xFF000000) >> 24;
			send_b[58] = (Local.OnlineNum & 0x00FF0000) >> 16;
			send_b[59] = (Local.OnlineNum & 0x0000FF00) >> 8;
			send_b[60] = Local.OnlineNum & 0x000000FF;
			Local.OnlineNum++;
			sendlength = 61;
			sprintf(RemoteHost, "%d.%d.%d.%d\0", Remote.DenIP[0],
					Remote.DenIP[1], Remote.DenIP[2], Remote.DenIP[3]);
			if (Remote.isDirect == 0) //直通呼叫
				RemotePort = RemoteVideoPort;
			else
				//中转呼叫
				RemotePort = RemoteVideoServerPort;
			UdpSendBuff(m_VideoSocket, RemoteHost, RemotePort, send_b,
					sendlength);
		}
	}
	Local.Timer1Num++;

	//对讲控制，显示通话时间和判断超时
	TalkCtrlFunc();
}
//---------------------------------------------------------------------------
void TimeReportStatusFunc(void) //设备定时报告状态函数
{
	int i, j, k;
	//锁定互斥锁
	pthread_mutex_lock(&Local.udp_lock);
	//查找可用发送缓冲并填空
	for (i = 0; i < UDPSENDMAX; i++)
		if (Multi_Udp_Buff[i].isValid == 0) {
			sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0",
					LocalCfg.IP_Server[0], LocalCfg.IP_Server[1],
					LocalCfg.IP_Server[2], LocalCfg.IP_Server[3]);
			Multi_Udp_Buff[i].SendNum = 0; //最多发6次
			Multi_Udp_Buff[i].m_Socket = m_DataSocket;
			Multi_Udp_Buff[i].RemotePort = RemoteDataPort;
			Multi_Udp_Buff[i].CurrOrder = 0;
			//头部
			memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
			//命令
			Multi_Udp_Buff[i].buf[6] = REPORTSTATUS;
			Multi_Udp_Buff[i].buf[7] = ASK; //主叫

			memcpy(Multi_Udp_Buff[i].buf + 8, LocalCfg.Addr, 20);
			memcpy(Multi_Udp_Buff[i].buf + 28, LocalCfg.Mac_Addr, 6);

			Multi_Udp_Buff[i].nlength = 34;
			Multi_Udp_Buff[i].DelayTime = 100;
			Multi_Udp_Buff[i].SendDelayTime = 0; //发送等待时间计数  20101112
			Multi_Udp_Buff[i].isValid = 1;
			sem_post(&multi_send_sem);
			break;
		}
	//打开互斥锁
	pthread_mutex_unlock(&Local.udp_lock);
}



//updateSipStatusFunc()
//{
//	if(Local.cRegFlg == 0)
//	{
//		sip_error.xLeft = 630;
//		sip_error.yTop = 400;
//        DisplayImage(&sip_error, 0);
//	}
//	else if(Local.cRegFlg == 1)
//	{
//        sip_ok.xLeft = 630;
//        sip_ok.yTop = 400;
//        DisplayImage(&sip_ok, 0);
//	}
//}

//void SipStatusChangeFunc()
//{
////	printf("*****enter:SipStatusChangeFunc*****\n");
//	//add by chensq 20120401 SIP注册成功与失败提示
//	if(cRegOldFlg != Local.cRegFlg)//SIP状态反转
//	{
//		printf("****cRegOldFlg(%d) != Local.cRegFlg (%d)****\n", cRegOldFlg, Local.cRegFlg);
//		if (Local.CurrentWindow == 0) {
//				printf("*****display sip status image:%d*****\n", Local.cRegFlg);
//				updateSipStatusFunc();
//				cRegOldFlg = Local.cRegFlg;
//		}
//	}
//}



//---------------------------------------------------------------------------
void ScreenOperFunc(void) //无操作退回主界面或关屏
{
	int CloseLcd;
#ifdef _DOWNLOAD_PHOTO  //下载图片并播放支持
	if(Local.CurrentWindow == 53)
	{
		Local.PhotoPlayTotalTime ++; //相框播放时间
		if(Local.PhotoPlayTotalTime > 600*INTRPERSEC)//300
		{
			//关闭LCD
			CloseLCD();
			Local.LcdLightFlag = 0;//LCD背光标志
			Local.LcdLightTime = 0;//时间
			Local.CurrentWindow = 0;
			DisplayMainWindow(Local.CurrentWindow);
			return;
		}
	}
#endif
	//如不在开机界面，则计时，30S无操作自动返回
	//不在呼叫和操作界面    不在用户监视界面  不在相框播放界面
	if ((Local.CurrentWindow != 0) && (Local.CurrentWindow != 53)
			&& (Local.Status == 0)) {
		PicStatBuf.KeyPressTime++;
		if (PicStatBuf.KeyPressTime > 15 * INTRPERSEC) //10*30
				{
			printf(
					"******************** return to main window( not in main window 30s )\n");
			PicStatBuf.KeyPressTime = 0;
			Local.CurrentWindow = 0;
			DisplayMainWindow(Local.CurrentWindow);
		}
	}
	//如在开机界面，60S无操作关LCD
	if (Local.CurrentWindow == 0) {
		if (Local.LcdLightFlag == 1) {
			Local.LcdLightTime++;
			if (Local.LcdLightTime > 60 * INTRPERSEC) //60*20*(50-10)
					{
				CloseLcd = 1;
#ifdef _DOWNLOAD_PHOTO  //下载图片并播放支持
				ListPhotoFunc(doorpic_dir); //显示相框目录下文件，并添加到 PlayNode_h
				if(PlayListStrc.TotalNum > 0)
				{
					CloseLcd = 0;
					DisplayPlayPhotoWindow();
					Local.PhotoPlayTotalTime = 0; //相框播放时间
					Local.LcdLightFlag = 1;//LCD背光标志
					Local.LcdLightTime = 0;//时间
				}
#endif

				if (CloseLcd == 1) {
					if (Local.LcdLightFlag == 1) {
						//关闭LCD
						CloseLCD();
						printf("*****close lcd 3: close lcd*****\n");
						if (Local.CurrentWindow != 0) {
							Local.CurrentWindow = 0;
							DisplayMainWindow(Local.CurrentWindow);
						} else {
							ClearTalkEdit(); //清 文本框
						}
					}
					Local.LcdLightFlag = 0; //LCD背光标志
					Local.LcdLightTime = 0; //时间
				}
			}
		}
	} else {
		if (Local.LcdLightFlag == 0) {
			//点亮LCD背光
			OpenLCD();
			printf("*****close lcd 4: open lcd*****\n");
		}
		Local.LcdLightFlag = 1; //LCD背光标志
		Local.LcdLightTime = 0; //时间
	}
}
//--------------------------------------------------------------------------
//清状态提示信息函数
void ClearTipFunc(void) {
	//延时清提示信息标志
	if (PicStatBuf.Flag == 1) {
		PicStatBuf.Time++;
		//1S
		if (PicStatBuf.Time >= (INTRPERSEC * 1)) {
			PicStatBuf.Flag = 0;
			PicStatBuf.Time = 0;
			switch (PicStatBuf.Type) {
			case 5: //设置窗口（一级）
				break;
			case 4: //监视
					//查看是否还是当前窗口
				if (Local.CurrentWindow == PicStatBuf.Type) {
					printf("*****ClearTipFunc()***watch***DisplayMainWindow(0)***Local.Status = %d**Local.CurrentWindow = %d***\n", Local.Status, Local.CurrentWindow);
					DisplayMainWindow(0);
					Local.Status = 0; //状态为空闲
				}
				break;
			case 2: //正在连接中(住户、中心)
			case 3: //正在通话中
					//查看是否还是当前窗口
				if (Local.CurrentWindow == PicStatBuf.Type) {
					printf("*****ClearTipFunc()***talk&&connect***DisplayMainWindow(0)***Local.Status = %d**Local.CurrentWindow = %d**g_sipCallID is %d**\n", Local.Status, Local.CurrentWindow, g_sipCallID);
					DisplayTalkOverWindow();
					sleep(1);
					DisplayMainWindow(0);
					Local.Status = 0; //状态为空闲
				}
				break;
			case 156: //修改工程密码（三级）
			case 192: //修改使用者密码（三级）
				break;
			case 159: //房号设置（三级）
				break;
			case 161: //IP地址设置窗口（三级）
				break;
			}
			PicStatBuf.Type = 0;
		}
	}
}
//---------------------------------------------------------------------------
//写光标函数   t_Flag 0--清除  1--写
void WriteCursor(struct TEdit *t_edit, int t_Flag, int t_show, int PageNo) {
#if 0
	ShowCursor = t_show;
	if(t_Flag == 1)
	{
		fb_line(t_edit->xLeft + t_edit->CursorX + t_edit->BoxLen*(t_edit->fWidth) + 3,
				t_edit->yTop +t_edit->CursorY,
				t_edit->xLeft + t_edit->CursorX + t_edit->BoxLen*(t_edit->fWidth)+ 3,
				t_edit->yTop + t_edit->CursorY + t_edit->CursorHeight - 1, t_edit->CursorCorlor, 0);
	}
	else
	{
		RestorePicBuf_Func(t_edit->xLeft + t_edit->CursorX + t_edit->BoxLen*(t_edit->fWidth) + 3,
				t_edit->yTop +t_edit->CursorY,
				1, t_edit->CursorHeight, t_edit->Cursor_H, 0); //恢复一块屏幕
	}
#endif
}
//---------------------------------------------------------------------------
//显示输入光标函数
void ShowCursorFunc(void) {
	if (ShowCursor == 1) {
		switch (Local.CurrentWindow) {
		case 0: //对讲窗口操作（一级）
			WriteCursor(&r2r_edit[CurrBox], CursorFlag, ShowCursor, 0);
			break;
		case 4: //密码开锁窗口操作（一级）
			WriteCursor(&r2r_edit[CurrBox], CursorFlag, ShowCursor, 0);
			break;
		case 5: //设置窗口操作（一级）
			WriteCursor(&setup_pass_edit, CursorFlag, ShowCursor, 0);
			break;
		case 156: //修改工程密码操作（三级）
		case 192: //修改使用者密码操作（四级）
			WriteCursor(&modi_engi_edit[CurrBox], CursorFlag, ShowCursor, 0);
			break;
		case 159: //房号设置操作（三级）
			WriteCursor(&addr_edit[CurrBox], CursorFlag, ShowCursor, 0);
			break;
		case 161: //IP地址设置操作（三级）
			WriteCursor(&ip_edit[CurrBox], CursorFlag, ShowCursor, 0);
			break;
		}
		if (CursorFlag == 1)
			CursorFlag = 0;
		else
			CursorFlag = 1;
	}
}
//---------------------------------------------------------------------------
void TalkCtrlFunc(void) //对讲控制，显示通话时间和判断超时
{
	char strtime[30];
	int PrevStep = 0;
	int CurrStep = 0;
	//1S
	if ((Local.TimeOut % INTRPERSEC) == 0)
		switch (Local.Status) {
		case 1: //主叫对讲
		case 2: //被叫对讲
			if (Local.TimeOut > CALLTIMEOUT)
			{
				//查看是否在其它组播组内
				DropMultiGroup(m_VideoSocket, NULL);
				//呼叫超时
				CallTimeOut_Func();
			}
			break;
		case 5: //主叫通话
		case 6: //被叫通话
				//计时
			sprintf(strtime, "%02d:%02d\0", Local.TimeOut / INTRPERSEC / 60,
					(Local.TimeOut / INTRPERSEC) % 60);
			switch (Local.CurrentWindow) {
			case 0: //呼叫中心
				//              strcpy(Label_CC_Time.Text, strtime);

				break;
			case 12: //户户通话
				//                  strcpy(Label_R2R_Time.Text, strtime);
				break;
			case 16: //对讲
				//                   strcpy(Label_C_Time.Text, strtime);
				break;
			}
			if (Local.TimeOut > TALKTIMEOUT)
			{
				TalkEnd_Func();
				Local.OnlineFlag = 0;
				if (DebugMode == 1)
					P_Debug("通话超时\n");
			}
			break;
		case 7: //本机主叫留影留言预备
		case 8: //本机被叫留影留言预备
			if (Local.TimeOut > PREPARETIMEOUT)
			{
				TalkEnd_Func();
				Local.OnlineFlag = 0;
				if (DebugMode == 1)
					P_Debug("留影留言预备超时\n");
			}
			break;
		case 9: //本机主叫留影留言
		case 10: //本机被叫留影留言
			if (Local.TimeOut > RECORDTIMEOUT)
			{
				TalkEnd_Func();
				Local.OnlineFlag = 0;
				if (DebugMode == 1)
					P_Debug("留影留言超时\n");
			}
			break;
		case 3: //监视    计时
		case 4: //被监视
			if (Local.TimeOut > WATCHTIMEOUT)
			{
				WatchEnd_Func();
				Local.OnlineFlag = 0;
				if (DebugMode == 1)
					P_Debug("监视超时\n");
			}
			break;
		}
	Local.TimeOut++; //监视超时,  通话超时,  呼叫超时，无人接听
}
//---------------------------------------------------------------------------
//检测IP地址冲突
void CheckIPConflict(void) {
	int result;
	int i;
	unsigned char to_user[7];
	result = 0;
	result = ioctl(m_EthSocket/*m_DataSocket*/, CONFLICTARP, to_user);
	if (result == CONFLICTARP)
	{
		if (to_user[0] == 1) //请求
				{
			sprintf(Local.DebugInfo,
					"IP地址冲突,对方硬件地址%02X:%02X:%02X:%02X:%02X:%02X\n", to_user[1],
					to_user[2], to_user[3], to_user[4], to_user[5], to_user[6]);
			P_Debug(Local.DebugInfo);
		}
		if (to_user[0] == 2) //应答
				{
			//锁定互斥锁
			pthread_mutex_lock(&Local.udp_lock);
			for (i = 0; i < UDPSENDMAX; i++)
				if (Multi_Udp_Buff[i].isValid == 1)
					if (Multi_Udp_Buff[i].m_Socket == ARP_Socket) {
						Multi_Udp_Buff[i].isValid = 0;
						sprintf(Local.DebugInfo,
								"IP地址冲突,对方硬件地址%02X:%02X:%02X:%02X:%02X:%02X\n",
								to_user[1], to_user[2], to_user[3], to_user[4],
								to_user[5], to_user[6]);
						P_Debug(Local.DebugInfo);
						break;
					}
			//打开互斥锁
			pthread_mutex_unlock(&Local.udp_lock);
		}
	}
}
//---------------------------------------------------------------------------
static int oldNet = 0;
//检测网络信号
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

	if (retval != Local.NetStatus) //网络状态 0 断开  1 接通
			{
		if (retval == 0) {
			Local.NetStatus = retval;
			Local.cRegFlg = 0;//add by chensq 20120401
			LPSetSipRegStatusForBV(REG_STATUS_OFFLINE);
			P_Debug("Link down\n");
		}

		if (retval == 1) {
			Local.NetStatus = retval;
			P_Debug("Link up\n");
			//发送免费ARP
			SendFreeArp();

			NewNetSpeed = get_ethtool_speed(m_EthSocket, "eth0");
			if (NewNetSpeed != Local.OldNetSpeed) {
				Local.OldNetSpeed = NewNetSpeed;
				system("ethtool -s eth0 autoneg off");
				usleep(1000 * 1000);
				system("ethtool -s eth0 autoneg on");
				//set_ethtool_speed(m_EthSocket, "eth0", 100, 0, 0); //关闭自适应
				//set_ethtool_speed(m_EthSocket, "eth0", 100, 0, 1); //打开自适应
				P_Debug("重新设置网卡速率\n");
			}
		}
	}
}
//---------------------------------------------------------------------------
int detect_ethtool(int skfd, char *ifname) {
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
int get_ethtool_speed(int skfd, char *ifname) //获取网卡速率
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
int set_ethtool_speed(int skfd, char *ifname, int speed, int duplex,
		int autoneg) //设置网卡速率
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
int detect_mii(int skfd, char *ifname) {
	struct ifreq ifr;
	u16 *data, mii_val;
	unsigned phy_id;

	/* Get the vitals from the interface. */
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	if (ioctl(skfd, SIOCGMIIPHY, &ifr) < 0) {
		fprintf(stderr, "SIOCGMIIPHY on %s failed\n", ifname);
		// (void) close(skfd);
		return 2;
	}

	data = (u16 *) (&ifr.ifr_data);
	phy_id = data[0];
	data[1] = 1;

	if (ioctl(skfd, SIOCGMIIREG, &ifr) < 0) {
		fprintf(stderr, "SIOCGMIIREG on %s failed\n", ifr.ifr_name);
		return 2;
	}

	mii_val = data[3];

	return (((mii_val & 0x0016) == 0x0004) ? 0 : 1);}
//---------------------------------------------------------------------------
void OpenDoorFunc(void) //开锁函数
{
	int OpenLockTime[2][2] = { { 100, 500 }, { 5000, 10000 } };
	Local.OpenDoorTime++;
	switch (Local.OpenDoorFlag) {
	case 2: // 开锁中
		if (Local.OpenDoorTime > 500 / INTRTIME)
		{
			Local.OpenDoorFlag = 0; //开锁标志 0 未开锁  1 开锁延时中  2 开锁中
			Local.OpenDoorTime = 0; //时间
			//关锁函数
			CloseLock_Func();
			printf("开锁完成\n");
		}
		break;
	}
}
//---------------------------------------------------------------------------
void CheckMainFbPage(void) //检查是否在framebuffer 的第0页
{
	if (Local.CurrFbPage != 0) {
		Local.CurrFbPage = 0;
		fb_setpage(fbdev, Local.CurrFbPage);
	}
	Local.LcdLightTime = 0;
	if (Local.LcdLightFlag == 0) {
		//点亮LCD背光
		OpenLCD();
		Local.LcdLightFlag = 1; //LCD背光标志
		Local.LcdLightTime = 0; //时间
	}
}
//---------------------------------------------------------------------------
