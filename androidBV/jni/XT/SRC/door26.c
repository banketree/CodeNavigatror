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

#include "common.h"

//**************************add by xuqd************************
//#include "idbt.h"
//#include "idbt_notify_delegate.h"
#include <YKSystem.h>
#include <XTIntfLayer.h>
#include <IDBT.h>
int fdNull = -1;

pthread_t idbt_notify_delegate_pthread;
pthread_t cc_pthread;

pthread_mutex_t audio_open_lock;
pthread_mutex_t audio_close_lock;
pthread_mutex_t audio_lock;

extern void XTTaskUninit(void);
extern void DisplayMainWindow(short wintype);
extern int InitArpSocket(void);
extern void AddMultiGroup(int m_Socket, char *McastAddr);
extern int Init_Timer(void);
extern int Init_Gpio(void);

//******************************end****************************

#ifdef  _MESSAGE_SUPPORT  //消息机制
#include "type.h"
#endif

#define idcard_name "/mnt/mtd/config/idcardcfg"

//framebuffer 函数
int fb_open(char *fb_device);
int fb_close(int fd);
int fb_display_open(int fd);
int fb_display_close(int fd);
int fb_stat(int fd, int *width, int *height, int *depth);
//设置显存页
int fb_setpage(int fd, int fbn);
void *fb_mmap(int fd, unsigned int screensize);
int fb_munmap(void *start, size_t length);
int fb_pixel(/*unsigned char *fbmem, */int width, int height, int x, int y,
		unsigned short color, int PageNo);
void fb_line(int start_x, int start_y, int end_x, int end_y,
		unsigned short color, int PageNo);
FILE *hzk16fp;
void openhzk16(void);
void closehzk16(void);
void outxy16(int x, int y, int wd, int clr, int mx, int my, char s[128],
		int pass, int PageNo); //写16点阵汉字
FILE *hzk24fp, *hzk24t, *ascfp;
void openhzk24(void);
void closehzk24(void);
void outxy24(int x, int y, int wd, int clr, int mx, int my, char s[128],
		int pass, int PageNo); //写24点阵汉字

char sZkPath[80] = "/usr/zk/";
void ReadCfgFile(void);
//写本地设置文件
void WriteCfgFile(void);

#ifdef _BRUSHIDCARD_SUPPORT   //刷卡功能支持
//读ID卡文件

void ReadIDCardCfgFile(void);

//写ID卡文件

void WriteIDCardFile(void);

//读ID卡应答

void ReadIDCardReply(char *cFromIP);

#endif

extern struct timeval ref_time; //基准时间,音频或视频第一帧

void OpenLCD(void);
void CloseLCD(void);
//PMU 关闭时钟
void SetPMU(void);

//与守护进程采用共享内存方式通信
extern int shmid;
extern char *c_addr;
//进程扫描
int proc_scan(char *proc_name);

void InitVideoParam(void); //初始化视频参数
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
	//printf("P_Debug1111\n");
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
		UdpSendBuff(m_DataSocket, Local.DebugIP, RemoteDataPort, send_b,
				sendlength);
		//printf("P_Debug1112, Local.DebugIP = %s, send_b = %s\n", Local.DebugIP, send_b);
	}
}
//---------------------------------------------------------------------------
void InitVideoParam(void) //初始化视频参数
{
	//system("insmod /mnt/mtd/panel_a025dn01.ko");
	//亮度
	system("echo w brightness 50 > /proc/isp0/command");
	//对比度
	system("echo w contrast 50 > /proc/isp0/command");
	//色度
	system("echo w hue 50 > /proc/isp0/command");
	//饱和度
	system("echo w saturation 50 > /proc/isp0/command");
	//锐度
	system("echo w sharpness 30 > /proc/isp0/command");

	system("echo 1 > /proc/isp0/ae/pwr_freq"); //0  -- 60HZ   1 -- 50HZ
}
//---------------------------------------------------------------------------
int XTTask(int argc, char *argv[])
{

//	system("cp /mnt/mtd/config-idbt/10grc_bak /mnt/mtd/config-idbt/10grc -rf");
//	printf(
//			"cp /mnt/mtd/config-idbt/10grc_bak /mnt/mtd/config-idbt/10grc -rf\n");
//	system(]


//			"cp /mnt/mtd/config-idbt/yk069_configure.ini_bak /mnt/mtd/config-idbt/yk069_configure.ini -rf");
//	printf(
//			"cp /mnt/mtd/config-idbt/yk069_configure.ini_bak /mnt/mtd/config-idbt/yk069_configure.ini -rf\n");
//
	fdNull = open("/dev/null", O_RDWR);

	/*idbt_app_run();
	 while(1){
	 usleep(100);
	 }*/

	pthread_attr_t attr;
	int ch;
	int programrun; //程序运行标志
	int dwSize;
	int i, j;
	uint32_t Ip_Int;
	unsigned short color;
	//declaration for framebuffer device
	char *fb_device;
	unsigned int x;
	unsigned int y;
	DIR *dirp;
	char wavFile[80];
	unsigned char buff[50];
	unsigned int screensize;
	int tmp_pos;
	//时钟
	time_t t;

//	DebugMode = 0;
//	if (argv[1] != NULL)
//	{
//		//printf("i am here1!\n");
//		if (strcmp(argv[1], "-debug") == 0)
//		{
//			DebugMode = 1; //调试模式
//		}
//	}
//
	DebugMode = 1;

	Init_Gpio(); ////初始化Gpio

	system("echo 18 > /proc/flcd200/pip/fb0_input"); //显示为 RGB565

	//open framebuffer device
	fbdev = fb_open(FB_DEV);

	//get status of framebuffer device
	fb_stat(fbdev, &fb_width, &fb_height, &fb_depth);

	//map framebuffer device to shared memory
	screensize = fb_width * fb_height * fb_depth / 8;
	printf("screensize =%d\n", screensize);
	fbmem = fb_mmap(fbdev, screensize);

	printf("fb_width =%d\n", fb_width);
	printf("fb_height =%d\n", fb_height);

	//初始化libimage
	InitLibImage(fbmem, fb_width, fb_height, &f_data);

	ref_time.tv_sec = 0; //初始时间戳
	ref_time.tv_usec = 0;
	TimeStamp.OldCurrVideo = 0; //上一次当前视频时间
	TimeStamp.CurrVideo = 0;
	TimeStamp.OldCurrAudio = 0; //上一次当前音频时间
	TimeStamp.CurrAudio = 0;

	//初始化链表头结点
	if (TempAudioNode_h == NULL)
		TempAudioNode_h = (TempAudioNode1 *) init_audionode();
	//语音播放缓冲
	for (i = 0; i < WAVFILEMAX; i++)
		WavFileBuf[i].isValid = 0;

	strcpy(sPath, "/usr/pic/720x480");
	strcpy(wavPath, "/usr/wav/");
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
	Local.OpenDoorFlag = 0; //开锁标志 0 未开锁  1 开锁延时中  2 开锁中
	Local.OpenDoorTime = 0; //时间

#ifdef _IPCAMERA_SUPPORT     //模拟成 IP Camera
	Local.IP_Watch_Num = 0;
	for(i=0; i<IP_WATCH_MAX; i++)
	Local.IP_Watch_AddrTable[i].isValid = 0;
#endif

	Local.cRegFlg = 0; //add by chensq 20120401 是否注册成功
	Local.cPlayFlg = 0; //add by chensq 20120401 是否播放彩铃

	Local.cSipCallPassive =  1;//被动

	Local.LoginServer = 0; //是否登录服务器

	Local.LcdLightFlag = 1; //LCD背光标志
	Local.LcdLightTime = 0; //时间

	Local.PlayLMovieTipFlag = 0; //播放留言提示标志

	//读配置文件
	ReadCfgFile();
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

	//电锁类型  0 －－ 电控锁  1 －－ 电磁锁
	LocalCfg.LockType = 0;
	//开锁时间  电控锁 0 ――100ms  1――500ms
	//          电磁锁 0 ――5s  1――10s
	LocalCfg.OpenLockTime = 1;

#ifdef _BRUSHIDCARD_SUPPORT   //刷卡功能支持
	//读ID卡文件
	ReadIDCardCfgFile();
	sprintf(Local.DebugInfo, "IDCardNo.Num = %d\n", IDCardNo.Num);
	P_Debug(Local.DebugInfo);

	//写ID卡信息
	RecvIDCardNo.isNewWriteFlag = 0;
	RecvIDCardNo.Num = 0;
	RecvIDCardNo.serialnum = IDCardNo.serialnum;
#endif

	Local.ReportSend = 1; //设备定时报告状态已发送
	Local.RandReportTime = 1;
	Local.ReportTimeNum = 0;

	Local.nowvideoframeno = 1; //当前视频帧编号
	Local.nowaudioframeno = 1; //当前音频帧编号

	Local._TESTNSSERVER = 0; //测试服务器解析模式
	Local._TESTTRANS = 0; //测试视频中转模式

	Local.SearchAllNo = 0; //中心机搜索所有设备序号

	Local.DownProgramOK = 0; //下载应用程序完成
	Local.download_image_flag = 0; //下载系统映像

	Local.CallKeyFlag = 0; //呼叫键标志     20110901  xu
	Local.CallKeyTime = 0; //呼叫键计数

	Local.TalkFinishFlag = 0; //通话结束标志     20101029  xu
	Local.TalkFinishTime = 0; //通话结束计数

	Local.CaptureVideoStartFlag = 0; //捕获视频开始标志     20101109  xu
	Local.CaptureVideoStartTime = 0; //捕获视频开始计数

	//刷ID卡记录
#ifdef _BRUSHIDCARD_SUPPORT   //刷卡功能支持
	BrushIDCard.Num = 0;
#endif

	//开锁时间     ms
	OpenLockTime[0] = 200;
	OpenLockTime[1] = 500;
	OpenLockTime[2] = 5000;
	OpenLockTime[3] = 10000;
	strcpy(OpenLockTimeText[0], "200ms");
	strcpy(OpenLockTimeText[1], "500ms");
	strcpy(OpenLockTimeText[2], "5 s  ");
	strcpy(OpenLockTimeText[3], "10 s ");
	//延时开锁时间  s
	DelayLockTime[0] = 0;
	DelayLockTime[1] = 3;
	DelayLockTime[2] = 5;
	DelayLockTime[3] = 10;
	strcpy(DelayLockTimeText[0], "0 s  ");
	strcpy(DelayLockTimeText[1], "3 s  ");
	strcpy(DelayLockTimeText[2], "5 s  ");
	strcpy(DelayLockTimeText[3], "10 s ");

	//呼叫回铃
	strcpy(CallWaitRingText[0], "普通型");
	strcpy(CallWaitRingText[1], "卡通型");
	strcpy(CallWaitRingText[2], "摇滚型");
	strcpy(CallWaitRingText[3], "抒情型");

	//防区状态
	DefenceCfg.DefenceStatus = 0;
	DefenceCfg.DefenceNum = 32;
	for (i = 0; i < 32; i++)
		for (j = 0; j < 10; j++)
			DefenceCfg.DefenceInfo[i][j] = 0;

	DeltaLen = 9 + sizeof(struct talkdata1); //数据包有效数据偏移量

	strcpy(UdpPackageHead, "XXXCID");
	for (i = 0; i < 20; i++)
		NullAddr[i] = '0';
	NullAddr[20] = '\0';

	Local.Status = 0; //状态为空闲
	Local.NSReplyFlag = 0; //NS应答处理标志
	Local.RecPicSize = 1; //默认视频大小为352*288
	Local.PlayPicSize = 1; //默认视频大小为352*288

	Local.DefenceDelayFlag = 0; //布防延时标志
	Local.DefenceDelayTime = 0; //计数
	Local.SingleFortifyFlag = 0; //单防区布防延时标志
	Local.SingleFortifyTime = 0; //单防区布防计数
	Local.Weather[0] = 0; //天气预报
	Local.Weather[1] = 0;
	Local.Weather[2] = 0;

	Ip_Int = inet_addr("192.168.68.88");
	memcpy(Remote.IP, &Ip_Int, 4);
	memcpy(Remote.Addr[0], NullAddr, 20);
	memcpy(Remote.Addr[0], "S00010101010", 12); //

	//InitVideoParam();   //初始化视频参数

	InitAudioParam(); //初始化音频参数
	//打开声卡播放
	/*  if(!OpenSnd(AUDIODSP1))
	 {
	 P_Debug("Open play sound device error!\n");
	 return;
	 }

	 //打开声卡录制
	 if(!OpenSnd(AUDIODSP))
	 {
	 printf("Open record sound device error!\n");
	 return;
	 }               */

	//线程运行标志
	Local.Save_File_Run_Flag = 1;
#ifdef _BRUSHIDCARD_SUPPORT   //刷卡功能支持
	Local.Dispart_Send_Run_Flag = 1;
#endif
	Local.Multi_Send_Run_Flag = 1;
	Local.Multi_Comm_Send_Run_Flag = 1;
	//系统初始化标志
	//InitSuccFlag = 0;

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
	InterfaceInit(); //界面初始化
	//打开16点阵汉字库
	//openhzk16();
	//打开24点阵汉字库
	//openhzk24();

	//COMM
	/*  Comm2fd = OpenComm(2,9600,8,1,'N');
	 if(Comm2fd <= 0)
	 P_Debug("Comm2 Open error!");  */

	Comm3fd = OpenComm(3, 9600, 8, 1, 'N');
	if (Comm3fd <= 0)
		P_Debug("Comm3 Open error!\n");

	/*  Comm4fd = OpenComm(4,9600,8,1,'N');
	 if(Comm4fd <= 0)
	 P_Debug("Comm4 Open error!\n");  */

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

#ifdef _BRUSHIDCARD_SUPPORT   //刷卡功能支持
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
	pthread_create(&dispart_send_thread, &attr,
			(void *) dispart_send_thread_func, NULL);
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
	pthread_create(&multi_send_thread, &attr, (void *) multi_send_thread_func,
			NULL);
	pthread_attr_destroy(&attr);
	if (multi_send_thread == 0)
	{
		P_Debug("无法创建UDP主动命令数据发送线程\n");
		return FALSE;
	}

	//将COMM发送缓冲置为无效

	for (i = 0; i < COMMSENDMAX; i++)
	{
		Multi_Comm_Buff[i].isValid = 0;
		Multi_Comm_Buff[i].SendNum = 0;
	}

	//主动命令数据发送线程：终端主动发送命令，如延时一段没收到回应，则多次发送

	//用于UDP和Comm通信
	sem_init(&multi_comm_send_sem, 0, 0);
	multi_comm_send_flag = 1;
	//创建互斥锁
	pthread_mutex_init(&Local.comm_lock, NULL);
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&multi_comm_send_thread, &attr,
			(void *) multi_comm_send_thread_func, NULL);
	pthread_attr_destroy(&attr);
	if (multi_comm_send_thread == 0)
	{
		P_Debug("无法创建串口主动命令数据发送线程\n");
		return FALSE;
	}

	//将FLASH存储缓冲置为无效

	for (i = 0; i < SAVEMAX; i++)
		Save_File_Buff[i].isValid = 0;
//FLASH存储线程
	sem_init(&save_file_sem, 0, 0);
	save_file_flag = 1;
	//创建互斥锁
	pthread_mutex_init(&Local.save_lock, NULL);
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&save_file_thread, &attr, (void *) save_file_thread_func,
			NULL);
	pthread_attr_destroy(&attr);
	if (save_file_thread == 0)
	{
		P_Debug("无法创建FLASH存储线程\n");
		//return;
	}

	//创建互斥锁

#ifdef _BRUSHIDCARD_SUPPORT   //刷卡功能支持
	pthread_mutex_init(&Local.idcard_lock, NULL);

#endif

	//SetPMU();  //PMU 关闭时钟

	//当前界面为"开机界面"
	Local.PrevWindow = 0;
	Local.CurrentWindow = 0;
	Local.isDisplayWindowFlag = 0; //窗口正在显示中

	//显示信息提示窗口
	DisplayMainWindow(Local.CurrentWindow);

	//Init_PWM();
	//初始化ARP Socket
	InitArpSocket();
	//加入NS组播组
	AddMultiGroup(m_VideoSocket, NSMULTIADDR);
	AddMultiGroup(m_DataSocket, NSMULTIADDR);

	sprintf(Local.DebugInfo, "LocalCfg.IP=%d.%d.%d.%d, LocalCfg.Addr = %s\n",
			LocalCfg.IP[0], LocalCfg.IP[1], LocalCfg.IP[2], LocalCfg.IP[3],
			LocalCfg.Addr);
	P_Debug(Local.DebugInfo);

	//发送免费ARP
//  SendFreeArp();

	Init_Timer(); //初始化定时器

	//Init_Gpio(); ////初始化Gpio

	SendOneOrder(_M8_WATCHDOG_START, 0, NULL); //启动 M64 看门狗

	//延时清提示信息标志
	PicStatBuf.Flag = 0;
	PicStatBuf.Type = 0;
	PicStatBuf.Time = 0;

	programrun = 1;
	ch = ' ';
	P_Debug("Please select: 'q' is exit!\n");
	time(&t);
	curr_tm_t = localtime(&t);
	//默认视频显示为窗口方式
	TimeReportStatusFunc(); //设备定时报告状态

	//add by chensq
//	system("/usr/sbin/telnetd");
//	printf("-----------------telnetd started\n");

	//-----------------------------add by xuqd------------------------------------------
	// 启动idbt
//	if (idbt_app_run() == RST_ERR)
//	{
//		programrun = 0;
//	}
//
//	pthread_create(&idbt_notify_delegate_pthread, &attr,
//			(void *) run_idbt_notify_delegate, NULL);
//
//	pthread_create(&cc_pthread, &attr,
//				(void *) run_cc, NULL);
//
//	pthread_mutex_init(&audio_open_lock, NULL);
//	pthread_mutex_init(&audio_close_lock, NULL);
//	pthread_mutex_init(&audio_lock, NULL);
	//-------------------------------end------------------------------------------------

//	do
//	{
//		// 以无限回圈不断等待键盘操作
//		ch = getchar();
//
//		// 等待自键盘输入字元
//		switch (ch)
//		{ // 判断输入字元为何
//		case 'Y':
//			break;
//		case 'Z':
//			//gm_spi_write(mtdimage_name, "/dev/mtd0", "192.168.68.98");
//			break;
//		case 'C':
//			system("cat /proc/meminfo");
//			break;
//		case 'D':
//			system("ps");
//			break;
//		case 'T':
//			//system("top");
//			break;
//		case 'E':
//			//printf("OpenFillLight_Func\n");
//			//OpenFillLight_Func();
//			//printf("OpenLcdLight_Func\n");
//			//OpenLcdLight_Func();
//			//printf("OpenLock\n");
//			//OpenLock_Func();
//			break;
//		case 'F':
//			//printf("CloseFillLight_Func\n");
//			//CloseFillLight_Func();
//			//printf("CloseLcdLight_Func\n");
//			//CloseLcdLight_Func();
//			//printf("CloseLock\n");
//			//CloseLock_Func();
//			break;
//		case 'P': // 播放声音
//			//StartPlayWav("/usr/wav/cartoon.wav", 0);
//			break;
//		case 'S': // 停止播放声音
//			break;
//		case 'Q': // 判断是否[q]键被按下
//			programrun = 0; //退出程序
//			break;
//		case '0':
//		case '1': // 判断是否menu键被按下
//		case '2':
//		case '3':
//		case '4':
//		case '5':
//		case '6':
//		case '7':
//		case '8':
//		case '9':
//			break;
//		case 'a': //0
//		case 'b': //1
//		case 'c': //2
//		case 'd': //3
//		case 'e': //4
//		case 'f':
//		case 'g':
//		case 'h':
//		case 'i':
//		case 'j':
//		case 'k':
//		case 'l':
//		case 'm':
//		case 'n':
//		case 'o':
//		case 'p': //14
//		case 'q':
//		case 'r':
//		case 's':
//		case 't':
//		case 'u':
//		case 'v':
//		case 'w':
//		case 'x':
//		case 'y':
//		case 'z':
//			//KeyAndTouchFunc(ch);
//			break;
//		default:
//			break;
//		}
//	} while (programrun);

	g_iRunXTTaskFlag = TRUE;
	while(g_iRunXTTaskFlag)
	{
//		sleep(20);//rectify by xuqd 前面改成休眠20秒的兄弟注意了，这个时间设置为30秒，这个模块退出的时候最长等待时间也是30秒
//		printf("XT thread is alive!\n");
		usleep(10000);
	}
	//*******************************add by xuqd****************************
//	Uninit_idbt_app();

	//*********************************end**********************************

	printf("begin to exit XT thread.....\n");

	Uninit_Gpio(); //释放Gpio

	Uninit_Timer(); //释放定时器

	//退出NS组播组
	DropNsMultiGroup(m_VideoSocket, NSMULTIADDR);
	DropNsMultiGroup(m_DataSocket, NSMULTIADDR);

	SendOneOrder(_M8_WATCHDOG_END, 0, NULL); //启动 M64 看门狗

	usleep(100 * 1000);

	//关闭声卡录制
	//CloseSnd(AUDIODSP);
	//关闭声卡播放
	//CloseSnd(AUDIODSP1);

	//关闭Comm及接收线程
	CloseComm();

	//FLASH存储线程
	save_file_flag = 0;
	usleep(40 * 1000);
	pthread_cancel(save_file_thread);
	sem_destroy(&save_file_sem);
	pthread_mutex_destroy(&Local.save_lock); //删除互斥锁

#ifdef _BRUSHIDCARD_SUPPORT   //刷卡功能支持
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
	pthread_cancel(multi_send_thread);
	sem_destroy(&multi_send_sem);
	pthread_mutex_destroy(&Local.udp_lock); //删除互斥锁

	//COMM主动命令数据发送线程
	multi_comm_send_flag = 0;
	usleep(40 * 1000);
	pthread_cancel(multi_comm_send_thread);
	sem_destroy(&multi_comm_send_sem);
	pthread_mutex_destroy(&Local.comm_lock); //删除互斥锁

#ifdef _BRUSHIDCARD_SUPPORT   //刷卡功能支持
	pthread_mutex_destroy(&Local.idcard_lock); //删除互斥锁
#endif

#ifdef _MESSAGE_SUPPORT  //消息机制
	DestroyMsgQueue();
#endif

	pthread_mutex_destroy(&audio_open_lock);
	pthread_mutex_destroy(&audio_close_lock);
	pthread_mutex_destroy(&audio_lock);

	//UnInit_PWM();
	//关闭ARP
	CloseArpSocket();
	//关闭UDP及接收线程
	CloseUdpSocket();

	fb_munmap(fbmem, screensize);
	//close framebuffer device
	fb_close(fbdev);

	//关闭16点阵汉字库
	//closehzk16();
	//关闭24点阵汉字库
	//closehzk24();

	close(fdNull);

	printf("2222\n");
	//Edit释放
	EditUnInit();
	printf("4444\n");
	//Image释放
	ImageUnInit();

	return (0);

}
//---------------------------------------------------------------------------
void save_file_thread_func(void)
{
#if 1
	int i;
	if (DebugMode == 1)
		P_Debug("创建FLASH存储线程\n");
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
						WriteIDCardFile();
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
		sprintf(Local.DebugInfo, "捕获图片传到中心 Capture_Pic_Num = %d\n",
				Capture_Pic_Num);
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
#ifdef _BRUSHIDCARD_SUPPORT   //刷卡功能支持
void dispart_send_thread_func(void)
{
	int i;
#ifdef _DEBUG
	P_Debug("创建拆分包发送线程\n");
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
		P_Debug("创建主动命令数据发送线程：\n");
		printf("multi_send_flag=%d\n", multi_send_flag);
	}
	while (multi_send_flag == 1)
	{
		//等待有按键按下的信号
		sem_wait(&multi_send_sem);
		if (Local.Multi_Send_Run_Flag == 0)
			Local.Multi_Send_Run_Flag = 1;
		else
		{
			HaveDataSend = 1;
			while (HaveDataSend)
			{
				//锁定互斥锁
				//  pthread_mutex_lock (&Local.udp_lock);
				for (i = 0; i < UDPSENDMAX; i++)
					if (Multi_Udp_Buff[i].isValid == 1)
					{
						//if(DebugMode == 1)
						//     printf("send Multi_Udp_Buff[i].RemoteHost = %s,Multi_Udp_Buff[i].buf[6]=%d [8] = %d\n",
						//       Multi_Udp_Buff[i].RemoteHost,Multi_Udp_Buff[i].buf[6], Multi_Udp_Buff[i].buf[8]);
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
										UdpSendBuff(Multi_Udp_Buff[i].m_Socket,
												Multi_Udp_Buff[i].RemoteHost,
												Multi_Udp_Buff[i].RemotePort,
												Multi_Udp_Buff[i].buf,
												Multi_Udp_Buff[i].nlength);
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
							if (Multi_Udp_Buff[i].SendDelayTime
									>= Multi_Udp_Buff[i].DelayTime)
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
									P_Debug("免费ARP发送完成\n");
							}
							else
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
											P_Debug("查找副机失败\n");
									}
									else
									{

										if (g_sipCallID == INVALID_SIP_CALL_ID)
										{
											printf("**********************net mask error g_sipCallID == INVALID_SIP_CALL_ID\n");
											StopPlayWavFile();
											WaitAudioUnuse(1000); //等待声卡空闲
											sprintf(wavFile, "%s/callfail.wav\0", wavPath);
											StartPlayWav(wavFile, 0);
											DisplayMainWindow(0);
											//ClearTalkEdit();
										}
										else
										{
											printf("**********************net mask error g_sipCallID != INVALID_SIP_CALL_ID\n");
											//g_callIDUFlag = 0;
											Local.Status = 1;
											DisplayTalkConnectWindow();
											g_softSipClearFlag = 1;
//											printf("***********************i am here************!\n");
//											printf("***********************i am here************!\n");
//											printf("***********************i am here************!\n");
//											printf("***********************i am here************!\n");

											WaitAudioUnuse(1000);
											sprintf(wavFile,"%s/cartoon.wav\0", wavPath);
											StartPlayWav(wavFile, 1);

										}
										Multi_Udp_Buff[i].isValid = 0;
										g_callIDUFlag = 0;

//										//若该命令为子网解析，则下一个命令为向服务器解析
//										Multi_Udp_Buff[i].SendNum = 0;
//										//更改UDP端口
//										Multi_Udp_Buff[i].m_Socket =
//												m_DataSocket;
//										Multi_Udp_Buff[i].RemotePort =
//												RemoteDataPort;
//										sprintf(Multi_Udp_Buff[i].RemoteHost,
//												"%d.%d.%d.%d\0",
//												LocalCfg.IP_NS[0],
//												LocalCfg.IP_NS[1],
//												LocalCfg.IP_NS[2],
//												LocalCfg.IP_NS[3]);
//										printf("%d.%d.%d.%d\0",
//												LocalCfg.IP_NS[0],
//												LocalCfg.IP_NS[1],
//												LocalCfg.IP_NS[2],
//												LocalCfg.IP_NS[3]);
//										//命令, 服务器解析
//										Multi_Udp_Buff[i].buf[6] = NSSERVERORDER;
//										if (DebugMode == 1)
//											P_Debug("正在向NS服务器解析地址\n");
									}
									break;
								case NSSERVERORDER: //服务器解析
									Multi_Udp_Buff[i].isValid = 0;
#ifdef _DEBUG
									printf("服务器解析失败\n");
#endif

									//呼叫窗口
									if (Local.CurrentWindow == 0)
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
											if (g_sipCallID == INVALID_SIP_CALL_ID)
											{
												WaitAudioUnuse(200); //等待声卡空闲
												sprintf(wavFile,"%s/callfail.wav\0",wavPath);
												StartPlayWav(wavFile, 0);
											}
											else
											{
												g_callIDUFlag = 0;
												g_softSipClearFlag = 1;
											}
										}
										ClearTalkEdit(); //清 文本框
									}

									if (g_sipCallID == INVALID_SIP_CALL_ID)
									{
										//延时清提示信息标志
										printf("****multi_send_thread_func()********Local.CurrentWindow is %d****\n",
												Local.CurrentWindow);
										PicStatBuf.Type = Local.CurrentWindow;
										PicStatBuf.Time = 0;
										PicStatBuf.Flag = 1;
									}
									g_callIDUFlag = 0;
									break;
								case VIDEOTALK: //局域网可视对讲
								case VIDEOTALKTRANS: //局域网可视对讲中转服务
									switch (Multi_Udp_Buff[i].buf[8])
									{
									case JOINGROUP: //加入组播组（主叫方->被叫方，被叫方应答）
										Multi_Udp_Buff[i].isValid = 0;
#ifdef _DEBUG
										printf("加入组播组失败, %d\n",
												Multi_Udp_Buff[i].buf[6]);
#endif
										break;
									case CALL:
										if (Multi_Udp_Buff[i].buf[6] == VIDEOTALK)
										{
											//有主副机，清其它的呼叫
											for (k = 0; k < UDPSENDMAX; k++)
												if (Multi_Udp_Buff[k].isValid == 1)
													if (Multi_Udp_Buff[k].buf[8] == CALL)
														if (k != i)
															Multi_Udp_Buff[k].isValid = 0;
											Multi_Udp_Buff[i].isValid = 0;
											if (g_sipCallID == INVALID_SIP_CALL_ID)
											{
												//延时清提示信息标志
												PicStatBuf.Type = Local.CurrentWindow;
												PicStatBuf.Time = 0;
												PicStatBuf.Flag = 1;

												WaitAudioUnuse(200); //等待声卡空闲
												sprintf(wavFile, "%s/callfail.wav\0", wavPath);
												StartPlayWav(wavFile, 0);
#ifdef _DEBUG
												printf("呼叫失败, %d\n", Multi_Udp_Buff[i].buf[6]);
#endif
											}
											else
											{
												g_callIDUFlag = 0;

												Local.Status = 1; //状态为主叫对讲
												DisplayTalkConnectWindow(); //显示对讲 正在连接窗口
											}
										}
										break;
									case CALLEND: //通话结束
										Multi_Udp_Buff[i].isValid = 0;
										Local.OnlineFlag = 0;
										Local.CallConfirmFlag = 0; //设置在线标志
										printf(
												"multi_send_thread_func :: TalkEnd_ClearStatus()\n");
										//对讲结束，清状态和关闭音视频
										TalkEnd_ClearStatus();
										break;
									default: //为其它命令，本次通信结束
										Multi_Udp_Buff[i].isValid = 0;
#ifdef _DEBUG
										printf("通信失败1, %d\n",
												Multi_Udp_Buff[i].buf[6]);
#endif
										break;
									}
									break;
								case VIDEOWATCH: //局域网监控
								case VIDEOWATCHTRANS: //局域网监控中转服务
									switch (Multi_Udp_Buff[i].buf[8])
									{
									case CALL:
										if (Multi_Udp_Buff[i].buf[6]
												== VIDEOWATCH)
										{
											Multi_Udp_Buff[i].isValid = 0;
											//  Local.Status = 0;
											//延时清提示信息标志
											PicStatBuf.Type = Local.CurrentWindow;
											PicStatBuf.Time = 0;
											PicStatBuf.Flag = 1;
#ifdef _DEBUG
											printf("监视失败, %d\n",
													Multi_Udp_Buff[i].buf[6]);
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
												//延时清提示信息标志
											PicStatBuf.Type = 4;
											PicStatBuf.Time = 0;
											PicStatBuf.Flag = 1;
											break;
										case 4: //本机被监视
											Local.Status = 0; //状态为空闲
											//    StopRecVideo();
											//延时清提示信息标志
											PicStatBuf.Type = 4;
											PicStatBuf.Time = 0;
											PicStatBuf.Flag = 1;
											break;
										}
										break;
									default: //为其它命令，本次通信结束
										Multi_Udp_Buff[i].isValid = 0;
#ifdef _DEBUG
										printf("通信失败2, %d\n",
												Multi_Udp_Buff[i].buf[6]);
#endif
										break;
									}
									break;
								default: //为其它命令，本次通信结束
									Multi_Udp_Buff[i].isValid = 0;
									//     Local.Status = 0;
#ifdef _DEBUG
									printf("通信失败3, %d\n",
											Multi_Udp_Buff[i].buf[6]);
#endif
									break;
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
}
//---------------------------------------------------------------------------
void multi_comm_send_thread_func(void)
{
	int i;
	int HaveDataSend;
	char ubuff[20];
	if (DebugMode == 1)
	{
		P_Debug("创建COMM主动命令数据发送线程：\n");
		printf("multi_comm_send_flag=%d\n", multi_comm_send_flag);
	}
	while (multi_comm_send_flag == 1)
	{
		sem_wait(&multi_comm_send_sem);
		if (Local.Multi_Comm_Send_Run_Flag == 0)
			Local.Multi_Comm_Send_Run_Flag = 1;
		else
		{
			HaveDataSend = 1;
			while (HaveDataSend)
			{

				for (i = 0; i < COMMSENDMAX; i++)
					if (Multi_Comm_Buff[i].isValid == 1)
					{
						if (Multi_Comm_Buff[i].SendNum < MAXCOMMSENDNUM)
						{
							//COMM发送
							CommSendBuff(Multi_Comm_Buff[i].m_Comm,
									Multi_Comm_Buff[i].buf,
									Multi_Comm_Buff[i].nlength);
							//     printf("send buff %d\n", Multi_Comm_Buff[i].buf[3] - 0x31);
							memcpy(ubuff, Multi_Comm_Buff[i].buf + 5, 16);
							ubuff[16] = '\0';
							Multi_Comm_Buff[i].SendNum++;
							printf("comm thread i=%d ubuff=%s\n", i, ubuff);
							break;
						}
					}

				usleep(100 * 1000); //  (20*1000)

				if ((Multi_Comm_Buff[i].isValid == 1)
						&& (Multi_Comm_Buff[i].SendNum >= MAXCOMMSENDNUM))
				{
					//锁定互斥锁
					pthread_mutex_lock(&Local.comm_lock);
					switch (Multi_Comm_Buff[i].buf[6])
					{
					default: //为其它命令，本次通信结束
						Multi_Comm_Buff[i].isValid = 0;
						//if(DebugMode == 1)
						//   printf("通信失败, %d\n", Multi_Comm_Buff[i].buf[3]);
						break;
					}
					//打开互斥锁
					pthread_mutex_unlock(&Local.comm_lock);
				}
				//判断数据是否全部发送完，若是，线程终止
				HaveDataSend = 0;
				for (i = 0; i < COMMSENDMAX; i++)
					if (Multi_Comm_Buff[i].isValid == 1)
					{
						HaveDataSend = 1;
						break;
					}
				if (HaveDataSend == 0)
					LCD_Bak.isFinished = 1;

			}
		}
	}
}
//---------------------------------------------------------------------------
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
		if ((cfg_fd = fopen(cfg_name, "rb")) == NULL)
		{
			P_Debug("配置文件不存在，创建新文件\n");
			// if((cfg_fd = open(cfg_name, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) == -1)
			if ((cfg_fd = fopen(cfg_name, "wb")) == NULL
				)
				P_Debug("无法创建配置文件\n");
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
					P_Debug("读取配置文件失败,以默认方式重建配置文件\n");
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
							LocalCfg.IP_Broadcast[i] = LocalCfg.IP_Mask[i]
									& LocalCfg.IP[i];
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
	if ((cfg_fd = fopen(cfg_name, "wb")) == NULL
		)
		P_Debug("无法创建配置文件\n");
	else
	{
		//重写本地设置文件
		fwrite(&LocalCfg, sizeof(LocalCfg), 1, cfg_fd);
		fclose(cfg_fd);
		P_Debug("存储本地设置成功\n");
	}
}
//---------------------------------------------------------------------------
#ifdef _BRUSHIDCARD_SUPPORT   //刷卡功能支持
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

				bytes_write = fwrite(&IDCardNo.Num, sizeof(IDCardNo.Num), 1,
						idcardcfg_fd);
				bytes_write = fwrite(&IDCardNo.serialnum,
						sizeof(IDCardNo.serialnum), 1, idcardcfg_fd);
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
	printf("iddata.TotalPackage = %d, cFromIP = %s\n", iddata.TotalPackage,
			cFromIP);
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
			iddata.CurrNum = IDCardNo.Num
					- CARDPERPACK * (iddata.CurrPackage - 1);
		else
			iddata.CurrNum = CARDPERPACK;

		memcpy(send_b + 8, &iddata, sizeof(iddata));
		memcpy(
				send_b + 48,
				IDCardNo.SN + CARDPERPACK * (iddata.CurrPackage - 1) * BYTEPERSN
				, BYTEPERSN * iddata.CurrNum);

		sendlength = 48 + BYTEPERSN * iddata.CurrNum;

		UdpSendBuff(m_DataSocket, cFromIP, RemoteDataPort, send_b, sendlength);

		if (((i % 3) == 0) && (i != 0))
			usleep(10);
	}
	//打开互斥锁
}
#endif
//---------------------------------------------------------------------------
//PMU 关闭时钟
void SetPMU(void)
{
#if 0
	unsigned int PMU1, PMU2;
	PMU1 = 0;
	PMU2 = 0;
	PMU1 |= 0x8000; //15	HS18OFF	R/W	Turns off the clock of the hs18_hclk(for MPCA)
	PMU1 |= 0x4000;//14	HS17OFF	R/W	Turns off the clock of the hs17_hclk(for MPCA)
	PMU1 |= 0x2000;//13	HS14OFF	R/W	Turns off the clock of the hs14_hclk(for MPCA)
	PMU1 |= 0x1000;//12	HS13OFF	R/W	Turns off the clock of the hs13_hclk(for MPCA)
	PMU1 |= 0x0800;//11	HS11OFF	R/W	Turns off the clock of the hs11_hclk(for MPCA)
	PMU1 |= 0x0400;//10	IDEOFF	R/W	Turns off the clock of the IDE controller
	PMU1 |= 0x0200;//9	PCIOFF	R/W	Turns off the clock of the PCI controller
	//0x0100 //8	MACOFF	R/W	Turns off the clock of the MAC controller
	//0x80   //7	DMAOFF	R/W	Turns off the clock of the DMA controller
	//6	-	-	Reserved	-	-
	//0x20   //5	MCPOFF	R/W	Turns off the clock of the MCP controller
	PMU1 |= 0x10;//4	LCDOFF	R/W	Turns off the clock of the LCD controller
	//0x08   //3	SDRAMOFF	R/W	Turns off the clock of the SDRAM controller
	//0x04   //2	CAPOFF	R/W	Turns off the clock of the Capture controller
	//PMU1 |= 0x02;   //1	OTGOFF	R/W	Turns off the clock of the USB 2.0 OTG controller
	//0x01   // MEMOFF	R/W	Turns off the clock of the SRAM controller

	PMU2 |= 0x040000;//18	UART4OFF	R/W	Turns off the clock of the UART 4 module
	//0x020000; //17	UART3OFF	R/W	Turns off the clock of the UART 3 module
	//PMU2 |= 0x010000; //16	UART2OFF	R/W	Turns off the clock of the UART 2 module
	//0x8000 //15	PS28OFF	R/W	Turns off the clock of the ps28_pclk
	//0x4000 //14	PS27OFF	R/W	Turns off the clock of the ps27_pclk
	//0x2000 //13	PS26OFF	R/W	Turns off the clock of the ps26_pclk
	//0x1000 //12	PS25OFF	R/W	Turns off the clock of the ps25_pclk
	//0x0800 //11	PS24OFF	R/W	Turns off the clock of the ps24_pclk
	//0x0400 //10	PS23OFF	R/W	Turns off the clock of the ps23_pclk
	//0x0200 //9	MCLKOFF	R/W	Turns off the main clock of audio CODEC
	//0x0100 //8	RTCOFF	R/W	Turns off the clock of the RTC module
	//0x80   //7	SSP1OFF	R/W	Turns off the clock of the SSP 1 module
	PMU2 |= 0x40;//6	SDCOFF	R/W	Turns off the clock of the SDC controller
	//0x20   //5	I2COFF	R/W	Turns off the clock of the I2C controller
	//0x10   //4	TIMEROFF	R/W	Turns off the clock of the TIMER module
	//0x08   //3	UART1OFF	R/W	Turns off the clock of the UART 1 module
	//0x04   //2	INTCOFF	R/W	Turns off the clock of the INTC controller
	//0x02   //1	WDTOFF	R/W	Turns off the clock of the WDT controller
	//0x01   //0	GPIOOFF	R/W	Turns off the clock of the GPIO controller
	//PMU 关闭时钟   关闭IDE  关闭PCI
	ioctl(gpio_fd, CLOSE_PMU1, PMU1);
	ioctl(gpio_fd, CLOSE_PMU2, PMU2);
#endif  
}
//---------------------------------------------------------------------------
int proc_scan(char *proc_name)
{
	static DIR *dir;
	struct dirent *ptr;
	char *name, status[32], buf[1024];
	int pid;
	struct stat sb;
	FILE *fp;

	if (!dir)
	{
		dir = opendir("/proc");
		if (!dir)
			printf("FStarter: Can't open /proc \n");
	}
	for (;;)
	{
		if ((ptr = readdir(dir)) == NULL)
		{
			closedir(dir);
			dir = 0;
			return 0;
		}
		name = ptr->d_name;
		if (!(*name >= '0' && *name <= '9'))
			continue;
		pid = atoi(name);
		sprintf(status, "/proc/%d", pid);
		if (stat(status, &sb))
			continue;
		sprintf(status, "/proc/%d/stat", pid);
		if ((fp = fopen(status, "r")) == NULL
			)
			continue;
		fgets(buf, sizeof(buf), fp);
		fclose(fp);
		name = strstr(buf, proc_name);
		if (name != NULL)
		{
			closedir(dir);
			dir = 0;
			return 1;
		}

	}
	closedir(dir);
	dir = 0;
	return -1;
}
//---------------------------------------------------------------------------
/*
 * open framebuffer device.
 * return positive file descriptor if success,
 * else return -1.
 */
int fb_open(char *fb_device)
{
	int fd;
	if ((fd = open(fb_device, O_RDWR)) < 0)
	{
		perror(__func__);
		return (-1);
	}
	return (fd);
}
//---------------------------------------------------------------------------
/*
 * get framebuffer's width,height,and depth.
 * return 0 if success, else return -1.
 */
int fb_stat(int fd, int *width, int *height, int *depth)
{
	struct fb_fix_screeninfo fb_finfo;
	struct fb_var_screeninfo fb_vinfo;
	int fbn = 0;

	if (ioctl(fd, FBIOGET_FSCREENINFO, &fb_finfo))
	{
		perror(__func__);
		return (-1);
	}

	if (ioctl(fd, FBIOGET_VSCREENINFO, &fb_vinfo))
	{
		perror(__func__);
		return (-1);
	}

	//init to 0
	if (ioctl(fd, FLCD_SET_FB_NUM, &fbn) < 0)
	{
		printf("Fail to set fb num\n");
		return 0;
	}

	if (ioctl(fd, FLCD_GET_DATA_SEP, &f_data) < 0)
	{
		printf("LCD Error: can not operate 0x%x\n", FLCD_GET_DATA_SEP);
		return 0;
	}
	fb_uvoffset = f_data.uv_offset;
#ifdef _DEBUG
	printf("f_data.uv_offset = %d\n", fb_uvoffset);
#endif
	*width = fb_vinfo.xres;
	*height = fb_vinfo.yres;
	*depth = fb_vinfo.bits_per_pixel;
	return (0);
}
//---------------------------------------------------------------------------
//设置显存页
int fb_setpage(int fd, int fbn)
{
	//init to 0
	if (ioctl(fd, FLCD_SET_FB_NUM, &fbn) < 0)
	{
		printf("Fail to set fb num\n");
		return 0;
	}
}
//---------------------------------------------------------------------------
//打开显示
int fb_display_open(int fd)
{
	if (ioctl(fd, FLCD_OPEN, NULL))
	{
		perror(__func__);
		return (-1);
	}
	return (0);
}
//---------------------------------------------------------------------------
//关闭显示
int fb_display_close(int fd)
{
	if (ioctl(fd, FLCD_CLOSE, NULL))
	{
		perror(__func__);
		return (-1);
	}
	return (0);
}
//---------------------------------------------------------------------------
/*
 * map shared memory to framebuffer device.
 * return maped memory if success,
 * else return -1, as mmap dose.
 */
void *fb_mmap(int fd, unsigned int screensize)
{
	caddr_t fbmem;
	if ((fbmem = mmap(0, screensize, PROT_READ | PROT_WRITE,
	MAP_SHARED, fd, 0)) == MAP_FAILED)
	{
		perror(__func__);
		return (void *) (-1);
	}
	return (fbmem);
}
//---------------------------------------------------------------------------
/*
 * unmap map memory for framebuffer device.
 */
int fb_munmap(void *start, size_t length)
{
	return (munmap(start, length));
}
//---------------------------------------------------------------------------
/*
 * close framebuffer device
 */
int fb_close(int fd)
{
	return (close(fd));
}
//---------------------------------------------------------------------------
/*
 * display a pixel on the framebuffer device.
 * fbmem is the starting memory of framebuffer,
 * width and height are dimension of framebuffer,
 * x and y are the coordinates to display,
 * color is the pixel's color value.
 * return 0 if success, otherwise return -1.
 */
/*int
 fb_pixel(void *fbmem, int width, int height,
 int x, int y, unsigned short color)
 {
 unsigned short *dst;
 if ((x > width) || (y > height))
 return (-1);

 *dst = ((unsigned short *) fbmem + y * width + x);

 *dst = color;
 return (0);
 }*/
int fb_pixel(/*unsigned char *fbmem, */int width, int height, int x, int y,
		unsigned short color, int PageNo)
{
	uint8_t isU;
	uint8_t isChr;
	/*  uint8_t B;
	 uint8_t G;
	 uint8_t R;*/
	uint8_t Y;
	uint8_t U;
	uint8_t V;
//  printf("x=%d,y=%d",x,y);
//  printf("Y=%d,U=%d,V=%d",Y,U,V);
	if ((x > width) || (y > height))
		return (-1);

	isU = 0;
	if (y % 2 == 0)
		isU = 1; // this is a U line

	/*  B = 0;
	 G = 0;
	 R = 255;
	 Y = (uint8_t)(0.257*R + 0.504*G + 0.098*B + 16);
	 U = (uint8_t)(-0.148*R - 0.291*G + 0.439*B + 128);
	 V = (uint8_t)(0.439*R - 0.368*G - 0.071*B + 128);  */
	switch (color)
	{
	case cWhite: //  1
		Y = 235;
		U = 128;
		V = 128;
		break;
	case cYellow: // 2
		Y = 210;
		U = 16;
		V = 146;
		break;
	case cCyan: //   3
		Y = 170;
		U = 166;
		V = 16;
		break;
	case cGreen: //  4
		Y = 145;
		U = 54;
		V = 34;
		break;
	case cMagenta: //  5
		Y = 106;
		U = 202;
		V = 222;
		break;
	case cRed: //  6
		Y = 81;
		U = 90;
		V = 240;
		break;
	case cBlue: //  7
		Y = 41;
		U = 240;
		V = 110;
		break;
	case cBlack: //  8
		Y = 16;
		U = 128;
		V = 128;
		break;
	default:
		Y = 16;
		U = 128;
		V = 128;
		break;
	}
	*(fbmem + f_data.buf_len * PageNo + y * width + x) = Y;

	return (0);
}
//---------------------------------------------------------------------------
void fb_line(int start_x, int start_y, int end_x, int end_y,
		unsigned short color, int PageNo)
{
	int i, j;
	for (j = start_y; j <= end_y; j++)
		for (i = start_x; i <= end_x; i++)
		{
			//     printf("seg 100");
			fb_pixel(fb_width, fb_height, i, j, color, PageNo);
		}

}
//---------------------------------------------------------------------------
void openhzk16(void)
{
	char hzk16name[80];
	strcpy(hzk16name, sZkPath);
	strcat(hzk16name, "hzk16");
	if ((hzk16fp = fopen(hzk16name, "rb")) == NULL)
	{
		printf("Can't open file \" hzk16 \".");
		return;
	}
}
//---------------------------------------------------------------------------
void closehzk16(void)
{
	fclose(hzk16fp);
}
//---------------------------------------------------------------------------
void outxy16(int x, int y, int wd, int clr, int mx, int my, char s[128],
		int pass, int PageNo)
{
//  FILE *ccfp,*ascfp;
	unsigned char ccdot[16][2];
	unsigned char ascdot[16];
	int col, byte, bit, mask;
	int mxc, myc;
	int len, cx, cy;
	int ascm, oldclr;
	char exasc[2];
	unsigned long offset;
	unsigned long ascoff;
	/* char hzk16name[80];
	 char asc16name[80];
	 strcpy(hzk16name, sZkPath);
	 strcat(hzk16name,"hzk16");
	 if((ccfp=fopen(hzk16name,"rb"))==NULL){
	 printf("Can't open file 'HZK16'.");
	 return;
	 }
	 strcpy(asc16name, sZkPath);
	 strcat(asc16name,"asc16");
	 if((ascfp=fopen(asc16name,"rb"))==NULL){
	 printf("Can't open file 'ASC16'.");
	 return;
	 }     */
	for (len = 0; len < strlen(s); len += 2)
	{
		if ((s[len] & 0x80) && (s[len + 1] & 0x80))
		{
			offset = (((unsigned char) s[len] - 0xa1) * 94
					+ (unsigned char) s[len + 1] - 0xa1) * 32L;
			if (fseek(hzk16fp, offset, SEEK_SET) != 0)
			{
				printf("Seek File \" HZK16\" Error.");
			}
			fread(ccdot, 2, 16, hzk16fp);

			for (col = 0; col < 16; col++)
			{
				cx = x + col;
				for (byte = 0; byte < 2; byte++)
				{
					cy = y + 8 * byte;
					mask = 0x80;
					for (bit = 0; bit < 8; bit++)
					{
						if (ccdot[col][byte] & mask)
							for (myc = 0; myc < my; myc++)
								for (mxc = 0; mxc < mx; mxc++)
									//         putpixel(x+8*byte*mx+bit*mx+mxc,y+col*my+myc,clr);
									fb_pixel(fb_width, fb_height,
											x + 8 * byte * mx + bit * mx + mxc,
											y + col * my + myc, clr, PageNo);
						mask = mask >> 1;
					}
				}
			}
			if (pass == 0)
				x = x + 16 * mx + wd;
			else
				y = y + 16 * my + wd;
		}
		else
		{
			/*   if(s[len]&0x80)
			 {
			 exasc[0]=s[len];
			 exasc[1]='\0';
			 oldclr=getcolor();
			 setcolor(clr);
			 outtextxy(x,y,exasc);
			 setcolor(oldclr);
			 x +=8;
			 }
			 else    */
			{
				ascoff = (long) s[len] * 16;
				if (fseek(ascfp, ascoff, SEEK_SET) != 0)
				{
					printf("Seek File \" ASC16\" Error.");
				}
				fread(ascdot, 1, 16, ascfp);
				for (byte = 0; byte < 16; byte++)
				{
					mask = 0x80;
					cy = y + byte + 2; //*2;
					for (bit = 0; bit < 8; bit++)
					{
						if (ascdot[byte] & mask)
						{
							cx = x + bit; //*2;
							fb_pixel(fb_width, fb_height, cx/*+ascm*/, cy, clr,
									PageNo);
//                fb_pixel(fb_width, fb_height,
//                           cx/*+ascm*/,cy+1, clr, PageNo);
							//  }
						}
						mask = mask >> 1;
					}
					/*   for(bit=0;bit < 8;bit++){
					 if(ascdot[byte]&mask){
					 cx=x+bit*2;
					 for(ascm=0;ascm < 2;ascm++)
					 //     putpixel(cx+ascm,cy,clr);
					 fb_pixel(fb_width, fb_height,
					 cx+ascm,cy, clr, PageNo);
					 for(ascm=0;ascm < 2;ascm++)
					 //    putpixel(cx+ascm,cy+1,clr);
					 fb_pixel(fb_width, fb_height,
					 cx+ascm,cy, clr, PageNo);
					 }
					 mask=mask>>1;
					 }*/
				}
				if (pass == 0)
					x = x + 8 + wd;
				else
					y = y + 8 + wd;
			}
			len--;
		}
	}
//  fclose(ascfp);
//  fclose(ccfp);
}
//---------------------------------------------------------------------------
void openhzk24(void)
{
	char hzk24name[80];
	char hzk24tname[80];
	char asc16name[80];
	strcpy(hzk24name, sZkPath);
	strcat(hzk24name, "hzk24s");
	if ((hzk24fp = fopen(hzk24name, "rb")) == NULL)
	{
		printf("Can't open file \" hzk24s \".");
		return;
	}

	strcpy(hzk24tname, sZkPath);
	strcat(hzk24tname, "hzk24t");
	if ((hzk24t = fopen(hzk24tname, "rb")) == NULL)
	{
		printf("Can't open file \" hzk24t \".");
		return;
	}

	strcpy(asc16name, sZkPath);
	strcat(asc16name, "asc16");
	if ((ascfp = fopen(asc16name, "rb")) == NULL)
	{
		printf("Can't open file \" ASC16 \".");
		return;
	}
}
//---------------------------------------------------------------------------
void closehzk24(void)
{
	fclose(ascfp);
	fclose(hzk24fp);
	fclose(hzk24t);
}
//---------------------------------------------------------------------------
void outxy24(int x, int y, int wd, int clr, int mx, int my, char s[128],
		int pass, int PageNo)
{
//  FILE *ccfp,*ascfp;
	unsigned char ccdot[24][3];
	unsigned char ascdot[16];
	int col, byte, bit, mask, mxc, myc;
	int len, cx, cy;
	int ascm, oldclr;
	char exasc[2];
	unsigned long offset;
	unsigned long ascoff;

	for (len = 0; len < strlen(s); len += 2)
	{
		if ((s[len] & 0x80) && (s[len + 1] & 0x80))
		{
			if (s[len] <= 0xA3)
			{
				//24点阵字符库
				offset = (((unsigned char) s[len] - 0xa1) * 94
						+ (unsigned char) s[len + 1] - 0xa1) * 72L;
				if (fseek(hzk24t, offset, SEEK_SET) != 0)
				{
					printf("Seek File \" HZK24T\" Error.");
					return;
				}
				fread(ccdot, 3, 24, hzk24t);
			}
			else
			{
				//24点阵字库中没有制表符等,故需减一偏移量
				offset = (((unsigned char) s[len] - 0xa1 - 15) * 94
						+ (unsigned char) s[len + 1] - 0xa1) * 72L;
				if (fseek(hzk24fp, offset, SEEK_SET) != 0)
				{
					printf("Seek File \" HZK24S\" Error.");
					return;
				}
				fread(ccdot, 3, 24, hzk24fp);
			}
			for (col = 0; col < 24; col++)
			{
				cx = x + col * mx;
				for (byte = 0; byte < 3; byte++)
				{
					cy = y + 8 * byte * my;
					mask = 0x80;
					for (bit = 0; bit < 8; bit++)
					{
						if (ccdot[col][byte] & mask)
							for (myc = 0; myc < my; myc++)
								for (mxc = 0; mxc < mx; mxc++)
									//          putpixel(cx+mxc,cy+bit*my+myc,clr);
									fb_pixel(fb_width, fb_height, cx + mxc,
											cy + bit * my + myc, clr, PageNo);
						mask = mask >> 1;
					}
				}
			}
			if (pass == 0)
				x = x + 24 * mx + wd;
			else
				y = y + 24 * my + wd;
		}
		else
		{
			/*	if(s[len]&0x80){
			 exasc[0]=s[len];
			 exasc[1]='\0';
			 oldclr=getcolor();
			 setcolor(clr);
			 outtextxy(x,y,exasc);
			 setcolor(oldclr);
			 x +=8;
			 }
			 else   */
			ascoff = (unsigned char) s[len] * 16L;
			if (fseek(ascfp, ascoff, SEEK_SET) != 0)
			{
				printf("Seek File \" ASC16\" Error.");
				return;
			}
			fread(ascdot, 1, 16, ascfp);
			for (byte = 0; byte < 16; byte++)
			{
				mask = 0x80;
				// cy=y+byte*2-2;
				cy = y + byte * 2 - 2;
				for (bit = 0; bit < 8; bit++)
				{
					if (ascdot[byte] & mask)
					{
						cx = x + bit; //*2;
						//  for(ascm=0;ascm < 2;ascm++){
						//     fb_pixel(fb_width, fb_height,
						//               cx+ascm, cy, clr);
						//     putpixel(cx+ascm,cy,clr);
						fb_pixel(fb_width, fb_height, cx/*+ascm*/, cy, clr,
								PageNo);
						fb_pixel(fb_width, fb_height, cx/*+ascm*/, cy + 1, clr,
								PageNo);
						//  }
					}
					mask = mask >> 1;
				}
			}
			if (pass == 0)
				x = x + 8 + wd;
			else
				y = y + 8 + wd;

			/*				x +=8;*/
			//    }
			len--;
		}
	}
//   fclose(ascfp);
//   fclose(ccfp);
}
//---------------------------------------------------------------------------
void OpenLCD(void)
{
	//开启LCD 背光函数
	OpenLcdLight_Func();
}
//---------------------------------------------------------------------------
void CloseLCD(void)
{
	//关闭LCD 背光函数
	CloseLcdLight_Func();
}
//---------------------------------------------------------------------------

