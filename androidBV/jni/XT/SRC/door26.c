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

#ifdef  _MESSAGE_SUPPORT  //��Ϣ����
#include "type.h"
#endif

#define idcard_name "/mnt/mtd/config/idcardcfg"

//framebuffer ����
int fb_open(char *fb_device);
int fb_close(int fd);
int fb_display_open(int fd);
int fb_display_close(int fd);
int fb_stat(int fd, int *width, int *height, int *depth);
//�����Դ�ҳ
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
		int pass, int PageNo); //д16������
FILE *hzk24fp, *hzk24t, *ascfp;
void openhzk24(void);
void closehzk24(void);
void outxy24(int x, int y, int wd, int clr, int mx, int my, char s[128],
		int pass, int PageNo); //д24������

char sZkPath[80] = "/usr/zk/";
void ReadCfgFile(void);
//д���������ļ�
void WriteCfgFile(void);

#ifdef _BRUSHIDCARD_SUPPORT   //ˢ������֧��
//��ID���ļ�

void ReadIDCardCfgFile(void);

//дID���ļ�

void WriteIDCardFile(void);

//��ID��Ӧ��

void ReadIDCardReply(char *cFromIP);

#endif

extern struct timeval ref_time; //��׼ʱ��,��Ƶ����Ƶ��һ֡

void OpenLCD(void);
void CloseLCD(void);
//PMU �ر�ʱ��
void SetPMU(void);

//���ػ����̲��ù����ڴ淽ʽͨ��
extern int shmid;
extern char *c_addr;
//����ɨ��
int proc_scan(char *proc_name);

void InitVideoParam(void); //��ʼ����Ƶ����
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
	//printf("P_Debug1111\n");
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
		UdpSendBuff(m_DataSocket, Local.DebugIP, RemoteDataPort, send_b,
				sendlength);
		//printf("P_Debug1112, Local.DebugIP = %s, send_b = %s\n", Local.DebugIP, send_b);
	}
}
//---------------------------------------------------------------------------
void InitVideoParam(void) //��ʼ����Ƶ����
{
	//system("insmod /mnt/mtd/panel_a025dn01.ko");
	//����
	system("echo w brightness 50 > /proc/isp0/command");
	//�Աȶ�
	system("echo w contrast 50 > /proc/isp0/command");
	//ɫ��
	system("echo w hue 50 > /proc/isp0/command");
	//���Ͷ�
	system("echo w saturation 50 > /proc/isp0/command");
	//���
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
	int programrun; //�������б�־
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
	//ʱ��
	time_t t;

//	DebugMode = 0;
//	if (argv[1] != NULL)
//	{
//		//printf("i am here1!\n");
//		if (strcmp(argv[1], "-debug") == 0)
//		{
//			DebugMode = 1; //����ģʽ
//		}
//	}
//
	DebugMode = 1;

	Init_Gpio(); ////��ʼ��Gpio

	system("echo 18 > /proc/flcd200/pip/fb0_input"); //��ʾΪ RGB565

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

	//��ʼ��libimage
	InitLibImage(fbmem, fb_width, fb_height, &f_data);

	ref_time.tv_sec = 0; //��ʼʱ���
	ref_time.tv_usec = 0;
	TimeStamp.OldCurrVideo = 0; //��һ�ε�ǰ��Ƶʱ��
	TimeStamp.CurrVideo = 0;
	TimeStamp.OldCurrAudio = 0; //��һ�ε�ǰ��Ƶʱ��
	TimeStamp.CurrAudio = 0;

	//��ʼ������ͷ���
	if (TempAudioNode_h == NULL)
		TempAudioNode_h = (TempAudioNode1 *) init_audionode();
	//�������Ż���
	for (i = 0; i < WAVFILEMAX; i++)
		WavFileBuf[i].isValid = 0;

	strcpy(sPath, "/usr/pic/720x480");
	strcpy(wavPath, "/usr/wav/");
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
	Local.OpenDoorFlag = 0; //������־ 0 δ����  1 ������ʱ��  2 ������
	Local.OpenDoorTime = 0; //ʱ��

#ifdef _IPCAMERA_SUPPORT     //ģ��� IP Camera
	Local.IP_Watch_Num = 0;
	for(i=0; i<IP_WATCH_MAX; i++)
	Local.IP_Watch_AddrTable[i].isValid = 0;
#endif

	Local.cRegFlg = 0; //add by chensq 20120401 �Ƿ�ע��ɹ�
	Local.cPlayFlg = 0; //add by chensq 20120401 �Ƿ񲥷Ų���

	Local.cSipCallPassive =  1;//����

	Local.LoginServer = 0; //�Ƿ��¼������

	Local.LcdLightFlag = 1; //LCD�����־
	Local.LcdLightTime = 0; //ʱ��

	Local.PlayLMovieTipFlag = 0; //����������ʾ��־

	//�������ļ�
	ReadCfgFile();
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

	//��������  0 ���� �����  1 ���� �����
	LocalCfg.LockType = 0;
	//����ʱ��  ����� 0 �D�D100ms  1�D�D500ms
	//          ����� 0 �D�D5s  1�D�D10s
	LocalCfg.OpenLockTime = 1;

#ifdef _BRUSHIDCARD_SUPPORT   //ˢ������֧��
	//��ID���ļ�
	ReadIDCardCfgFile();
	sprintf(Local.DebugInfo, "IDCardNo.Num = %d\n", IDCardNo.Num);
	P_Debug(Local.DebugInfo);

	//дID����Ϣ
	RecvIDCardNo.isNewWriteFlag = 0;
	RecvIDCardNo.Num = 0;
	RecvIDCardNo.serialnum = IDCardNo.serialnum;
#endif

	Local.ReportSend = 1; //�豸��ʱ����״̬�ѷ���
	Local.RandReportTime = 1;
	Local.ReportTimeNum = 0;

	Local.nowvideoframeno = 1; //��ǰ��Ƶ֡���
	Local.nowaudioframeno = 1; //��ǰ��Ƶ֡���

	Local._TESTNSSERVER = 0; //���Է���������ģʽ
	Local._TESTTRANS = 0; //������Ƶ��תģʽ

	Local.SearchAllNo = 0; //���Ļ����������豸���

	Local.DownProgramOK = 0; //����Ӧ�ó������
	Local.download_image_flag = 0; //����ϵͳӳ��

	Local.CallKeyFlag = 0; //���м���־     20110901  xu
	Local.CallKeyTime = 0; //���м�����

	Local.TalkFinishFlag = 0; //ͨ��������־     20101029  xu
	Local.TalkFinishTime = 0; //ͨ����������

	Local.CaptureVideoStartFlag = 0; //������Ƶ��ʼ��־     20101109  xu
	Local.CaptureVideoStartTime = 0; //������Ƶ��ʼ����

	//ˢID����¼
#ifdef _BRUSHIDCARD_SUPPORT   //ˢ������֧��
	BrushIDCard.Num = 0;
#endif

	//����ʱ��     ms
	OpenLockTime[0] = 200;
	OpenLockTime[1] = 500;
	OpenLockTime[2] = 5000;
	OpenLockTime[3] = 10000;
	strcpy(OpenLockTimeText[0], "200ms");
	strcpy(OpenLockTimeText[1], "500ms");
	strcpy(OpenLockTimeText[2], "5 s  ");
	strcpy(OpenLockTimeText[3], "10 s ");
	//��ʱ����ʱ��  s
	DelayLockTime[0] = 0;
	DelayLockTime[1] = 3;
	DelayLockTime[2] = 5;
	DelayLockTime[3] = 10;
	strcpy(DelayLockTimeText[0], "0 s  ");
	strcpy(DelayLockTimeText[1], "3 s  ");
	strcpy(DelayLockTimeText[2], "5 s  ");
	strcpy(DelayLockTimeText[3], "10 s ");

	//���л���
	strcpy(CallWaitRingText[0], "��ͨ��");
	strcpy(CallWaitRingText[1], "��ͨ��");
	strcpy(CallWaitRingText[2], "ҡ����");
	strcpy(CallWaitRingText[3], "������");

	//����״̬
	DefenceCfg.DefenceStatus = 0;
	DefenceCfg.DefenceNum = 32;
	for (i = 0; i < 32; i++)
		for (j = 0; j < 10; j++)
			DefenceCfg.DefenceInfo[i][j] = 0;

	DeltaLen = 9 + sizeof(struct talkdata1); //���ݰ���Ч����ƫ����

	strcpy(UdpPackageHead, "XXXCID");
	for (i = 0; i < 20; i++)
		NullAddr[i] = '0';
	NullAddr[20] = '\0';

	Local.Status = 0; //״̬Ϊ����
	Local.NSReplyFlag = 0; //NSӦ�����־
	Local.RecPicSize = 1; //Ĭ����Ƶ��СΪ352*288
	Local.PlayPicSize = 1; //Ĭ����Ƶ��СΪ352*288

	Local.DefenceDelayFlag = 0; //������ʱ��־
	Local.DefenceDelayTime = 0; //����
	Local.SingleFortifyFlag = 0; //������������ʱ��־
	Local.SingleFortifyTime = 0; //��������������
	Local.Weather[0] = 0; //����Ԥ��
	Local.Weather[1] = 0;
	Local.Weather[2] = 0;

	Ip_Int = inet_addr("192.168.68.88");
	memcpy(Remote.IP, &Ip_Int, 4);
	memcpy(Remote.Addr[0], NullAddr, 20);
	memcpy(Remote.Addr[0], "S00010101010", 12); //

	//InitVideoParam();   //��ʼ����Ƶ����

	InitAudioParam(); //��ʼ����Ƶ����
	//����������
	/*  if(!OpenSnd(AUDIODSP1))
	 {
	 P_Debug("Open play sound device error!\n");
	 return;
	 }

	 //������¼��
	 if(!OpenSnd(AUDIODSP))
	 {
	 printf("Open record sound device error!\n");
	 return;
	 }               */

	//�߳����б�־
	Local.Save_File_Run_Flag = 1;
#ifdef _BRUSHIDCARD_SUPPORT   //ˢ������֧��
	Local.Dispart_Send_Run_Flag = 1;
#endif
	Local.Multi_Send_Run_Flag = 1;
	Local.Multi_Comm_Send_Run_Flag = 1;
	//ϵͳ��ʼ����־
	//InitSuccFlag = 0;

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
	InterfaceInit(); //�����ʼ��
	//��16�����ֿ�
	//openhzk16();
	//��24�����ֿ�
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

#ifdef _BRUSHIDCARD_SUPPORT   //ˢ������֧��
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
	pthread_create(&dispart_send_thread, &attr,
			(void *) dispart_send_thread_func, NULL);
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
	pthread_create(&multi_send_thread, &attr, (void *) multi_send_thread_func,
			NULL);
	pthread_attr_destroy(&attr);
	if (multi_send_thread == 0)
	{
		P_Debug("�޷�����UDP�����������ݷ����߳�\n");
		return FALSE;
	}

	//��COMM���ͻ�����Ϊ��Ч

	for (i = 0; i < COMMSENDMAX; i++)
	{
		Multi_Comm_Buff[i].isValid = 0;
		Multi_Comm_Buff[i].SendNum = 0;
	}

	//�����������ݷ����̣߳��ն����������������ʱһ��û�յ���Ӧ�����η���

	//����UDP��Commͨ��
	sem_init(&multi_comm_send_sem, 0, 0);
	multi_comm_send_flag = 1;
	//����������
	pthread_mutex_init(&Local.comm_lock, NULL);
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&multi_comm_send_thread, &attr,
			(void *) multi_comm_send_thread_func, NULL);
	pthread_attr_destroy(&attr);
	if (multi_comm_send_thread == 0)
	{
		P_Debug("�޷��������������������ݷ����߳�\n");
		return FALSE;
	}

	//��FLASH�洢������Ϊ��Ч

	for (i = 0; i < SAVEMAX; i++)
		Save_File_Buff[i].isValid = 0;
//FLASH�洢�߳�
	sem_init(&save_file_sem, 0, 0);
	save_file_flag = 1;
	//����������
	pthread_mutex_init(&Local.save_lock, NULL);
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&save_file_thread, &attr, (void *) save_file_thread_func,
			NULL);
	pthread_attr_destroy(&attr);
	if (save_file_thread == 0)
	{
		P_Debug("�޷�����FLASH�洢�߳�\n");
		//return;
	}

	//����������

#ifdef _BRUSHIDCARD_SUPPORT   //ˢ������֧��
	pthread_mutex_init(&Local.idcard_lock, NULL);

#endif

	//SetPMU();  //PMU �ر�ʱ��

	//��ǰ����Ϊ"��������"
	Local.PrevWindow = 0;
	Local.CurrentWindow = 0;
	Local.isDisplayWindowFlag = 0; //����������ʾ��

	//��ʾ��Ϣ��ʾ����
	DisplayMainWindow(Local.CurrentWindow);

	//Init_PWM();
	//��ʼ��ARP Socket
	InitArpSocket();
	//����NS�鲥��
	AddMultiGroup(m_VideoSocket, NSMULTIADDR);
	AddMultiGroup(m_DataSocket, NSMULTIADDR);

	sprintf(Local.DebugInfo, "LocalCfg.IP=%d.%d.%d.%d, LocalCfg.Addr = %s\n",
			LocalCfg.IP[0], LocalCfg.IP[1], LocalCfg.IP[2], LocalCfg.IP[3],
			LocalCfg.Addr);
	P_Debug(Local.DebugInfo);

	//�������ARP
//  SendFreeArp();

	Init_Timer(); //��ʼ����ʱ��

	//Init_Gpio(); ////��ʼ��Gpio

	SendOneOrder(_M8_WATCHDOG_START, 0, NULL); //���� M64 ���Ź�

	//��ʱ����ʾ��Ϣ��־
	PicStatBuf.Flag = 0;
	PicStatBuf.Type = 0;
	PicStatBuf.Time = 0;

	programrun = 1;
	ch = ' ';
	P_Debug("Please select: 'q' is exit!\n");
	time(&t);
	curr_tm_t = localtime(&t);
	//Ĭ����Ƶ��ʾΪ���ڷ�ʽ
	TimeReportStatusFunc(); //�豸��ʱ����״̬

	//add by chensq
//	system("/usr/sbin/telnetd");
//	printf("-----------------telnetd started\n");

	//-----------------------------add by xuqd------------------------------------------
	// ����idbt
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
//		// �����޻�Ȧ���ϵȴ����̲���
//		ch = getchar();
//
//		// �ȴ��Լ���������Ԫ
//		switch (ch)
//		{ // �ж�������ԪΪ��
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
//		case 'P': // ��������
//			//StartPlayWav("/usr/wav/cartoon.wav", 0);
//			break;
//		case 'S': // ֹͣ��������
//			break;
//		case 'Q': // �ж��Ƿ�[q]��������
//			programrun = 0; //�˳�����
//			break;
//		case '0':
//		case '1': // �ж��Ƿ�menu��������
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
//		sleep(20);//rectify by xuqd ǰ��ĳ�����20����ֵ�ע���ˣ����ʱ������Ϊ30�룬���ģ���˳���ʱ����ȴ�ʱ��Ҳ��30��
//		printf("XT thread is alive!\n");
		usleep(10000);
	}
	//*******************************add by xuqd****************************
//	Uninit_idbt_app();

	//*********************************end**********************************

	printf("begin to exit XT thread.....\n");

	Uninit_Gpio(); //�ͷ�Gpio

	Uninit_Timer(); //�ͷŶ�ʱ��

	//�˳�NS�鲥��
	DropNsMultiGroup(m_VideoSocket, NSMULTIADDR);
	DropNsMultiGroup(m_DataSocket, NSMULTIADDR);

	SendOneOrder(_M8_WATCHDOG_END, 0, NULL); //���� M64 ���Ź�

	usleep(100 * 1000);

	//�ر�����¼��
	//CloseSnd(AUDIODSP);
	//�ر���������
	//CloseSnd(AUDIODSP1);

	//�ر�Comm�������߳�
	CloseComm();

	//FLASH�洢�߳�
	save_file_flag = 0;
	usleep(40 * 1000);
	pthread_cancel(save_file_thread);
	sem_destroy(&save_file_sem);
	pthread_mutex_destroy(&Local.save_lock); //ɾ��������

#ifdef _BRUSHIDCARD_SUPPORT   //ˢ������֧��
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
	pthread_cancel(multi_send_thread);
	sem_destroy(&multi_send_sem);
	pthread_mutex_destroy(&Local.udp_lock); //ɾ��������

	//COMM�����������ݷ����߳�
	multi_comm_send_flag = 0;
	usleep(40 * 1000);
	pthread_cancel(multi_comm_send_thread);
	sem_destroy(&multi_comm_send_sem);
	pthread_mutex_destroy(&Local.comm_lock); //ɾ��������

#ifdef _BRUSHIDCARD_SUPPORT   //ˢ������֧��
	pthread_mutex_destroy(&Local.idcard_lock); //ɾ��������
#endif

#ifdef _MESSAGE_SUPPORT  //��Ϣ����
	DestroyMsgQueue();
#endif

	pthread_mutex_destroy(&audio_open_lock);
	pthread_mutex_destroy(&audio_close_lock);
	pthread_mutex_destroy(&audio_lock);

	//UnInit_PWM();
	//�ر�ARP
	CloseArpSocket();
	//�ر�UDP�������߳�
	CloseUdpSocket();

	fb_munmap(fbmem, screensize);
	//close framebuffer device
	fb_close(fbdev);

	//�ر�16�����ֿ�
	//closehzk16();
	//�ر�24�����ֿ�
	//closehzk24();

	close(fdNull);

	printf("2222\n");
	//Edit�ͷ�
	EditUnInit();
	printf("4444\n");
	//Image�ͷ�
	ImageUnInit();

	return (0);

}
//---------------------------------------------------------------------------
void save_file_thread_func(void)
{
#if 1
	int i;
	if (DebugMode == 1)
		P_Debug("����FLASH�洢�߳�\n");
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
						WriteIDCardFile();
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
		sprintf(Local.DebugInfo, "����ͼƬ�������� Capture_Pic_Num = %d\n",
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
#ifdef _BRUSHIDCARD_SUPPORT   //ˢ������֧��
void dispart_send_thread_func(void)
{
	int i;
#ifdef _DEBUG
	P_Debug("������ְ������߳�\n");
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
		P_Debug("���������������ݷ����̣߳�\n");
		printf("multi_send_flag=%d\n", multi_send_flag);
	}
	while (multi_send_flag == 1)
	{
		//�ȴ��а������µ��ź�
		sem_wait(&multi_send_sem);
		if (Local.Multi_Send_Run_Flag == 0)
			Local.Multi_Send_Run_Flag = 1;
		else
		{
			HaveDataSend = 1;
			while (HaveDataSend)
			{
				//����������
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
								ArpSendBuff(); //���ARP����
							else
							{
								//UDP����
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
							//20101112 ���͵ȴ�ʱ�����, ���ⷢ��ʱ��̫�����������
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
							//����������
							//    pthread_mutex_lock (&Local.udp_lock);
							if (Multi_Udp_Buff[i].m_Socket == ARP_Socket)
							{
								Multi_Udp_Buff[i].isValid = 0;
								if (DebugMode == 1)
									P_Debug("���ARP�������\n");
							}
							else
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
											P_Debug("���Ҹ���ʧ��\n");
									}
									else
									{

										if (g_sipCallID == INVALID_SIP_CALL_ID)
										{
											printf("**********************net mask error g_sipCallID == INVALID_SIP_CALL_ID\n");
											StopPlayWavFile();
											WaitAudioUnuse(1000); //�ȴ���������
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

//										//��������Ϊ��������������һ������Ϊ�����������
//										Multi_Udp_Buff[i].SendNum = 0;
//										//����UDP�˿�
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
//										//����, ����������
//										Multi_Udp_Buff[i].buf[6] = NSSERVERORDER;
//										if (DebugMode == 1)
//											P_Debug("������NS������������ַ\n");
									}
									break;
								case NSSERVERORDER: //����������
									Multi_Udp_Buff[i].isValid = 0;
#ifdef _DEBUG
									printf("����������ʧ��\n");
#endif

									//���д���
									if (Local.CurrentWindow == 0)
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
											if (g_sipCallID == INVALID_SIP_CALL_ID)
											{
												WaitAudioUnuse(200); //�ȴ���������
												sprintf(wavFile,"%s/callfail.wav\0",wavPath);
												StartPlayWav(wavFile, 0);
											}
											else
											{
												g_callIDUFlag = 0;
												g_softSipClearFlag = 1;
											}
										}
										ClearTalkEdit(); //�� �ı���
									}

									if (g_sipCallID == INVALID_SIP_CALL_ID)
									{
										//��ʱ����ʾ��Ϣ��־
										printf("****multi_send_thread_func()********Local.CurrentWindow is %d****\n",
												Local.CurrentWindow);
										PicStatBuf.Type = Local.CurrentWindow;
										PicStatBuf.Time = 0;
										PicStatBuf.Flag = 1;
									}
									g_callIDUFlag = 0;
									break;
								case VIDEOTALK: //���������ӶԽ�
								case VIDEOTALKTRANS: //���������ӶԽ���ת����
									switch (Multi_Udp_Buff[i].buf[8])
									{
									case JOINGROUP: //�����鲥�飨���з�->���з������з�Ӧ��
										Multi_Udp_Buff[i].isValid = 0;
#ifdef _DEBUG
										printf("�����鲥��ʧ��, %d\n",
												Multi_Udp_Buff[i].buf[6]);
#endif
										break;
									case CALL:
										if (Multi_Udp_Buff[i].buf[6] == VIDEOTALK)
										{
											//�����������������ĺ���
											for (k = 0; k < UDPSENDMAX; k++)
												if (Multi_Udp_Buff[k].isValid == 1)
													if (Multi_Udp_Buff[k].buf[8] == CALL)
														if (k != i)
															Multi_Udp_Buff[k].isValid = 0;
											Multi_Udp_Buff[i].isValid = 0;
											if (g_sipCallID == INVALID_SIP_CALL_ID)
											{
												//��ʱ����ʾ��Ϣ��־
												PicStatBuf.Type = Local.CurrentWindow;
												PicStatBuf.Time = 0;
												PicStatBuf.Flag = 1;

												WaitAudioUnuse(200); //�ȴ���������
												sprintf(wavFile, "%s/callfail.wav\0", wavPath);
												StartPlayWav(wavFile, 0);
#ifdef _DEBUG
												printf("����ʧ��, %d\n", Multi_Udp_Buff[i].buf[6]);
#endif
											}
											else
											{
												g_callIDUFlag = 0;

												Local.Status = 1; //״̬Ϊ���жԽ�
												DisplayTalkConnectWindow(); //��ʾ�Խ� �������Ӵ���
											}
										}
										break;
									case CALLEND: //ͨ������
										Multi_Udp_Buff[i].isValid = 0;
										Local.OnlineFlag = 0;
										Local.CallConfirmFlag = 0; //�������߱�־
										printf(
												"multi_send_thread_func :: TalkEnd_ClearStatus()\n");
										//�Խ���������״̬�͹ر�����Ƶ
										TalkEnd_ClearStatus();
										break;
									default: //Ϊ�����������ͨ�Ž���
										Multi_Udp_Buff[i].isValid = 0;
#ifdef _DEBUG
										printf("ͨ��ʧ��1, %d\n",
												Multi_Udp_Buff[i].buf[6]);
#endif
										break;
									}
									break;
								case VIDEOWATCH: //���������
								case VIDEOWATCHTRANS: //�����������ת����
									switch (Multi_Udp_Buff[i].buf[8])
									{
									case CALL:
										if (Multi_Udp_Buff[i].buf[6]
												== VIDEOWATCH)
										{
											Multi_Udp_Buff[i].isValid = 0;
											//  Local.Status = 0;
											//��ʱ����ʾ��Ϣ��־
											PicStatBuf.Type = Local.CurrentWindow;
											PicStatBuf.Time = 0;
											PicStatBuf.Flag = 1;
#ifdef _DEBUG
											printf("����ʧ��, %d\n",
													Multi_Udp_Buff[i].buf[6]);
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
												//��ʱ����ʾ��Ϣ��־
											PicStatBuf.Type = 4;
											PicStatBuf.Time = 0;
											PicStatBuf.Flag = 1;
											break;
										case 4: //����������
											Local.Status = 0; //״̬Ϊ����
											//    StopRecVideo();
											//��ʱ����ʾ��Ϣ��־
											PicStatBuf.Type = 4;
											PicStatBuf.Time = 0;
											PicStatBuf.Flag = 1;
											break;
										}
										break;
									default: //Ϊ�����������ͨ�Ž���
										Multi_Udp_Buff[i].isValid = 0;
#ifdef _DEBUG
										printf("ͨ��ʧ��2, %d\n",
												Multi_Udp_Buff[i].buf[6]);
#endif
										break;
									}
									break;
								default: //Ϊ�����������ͨ�Ž���
									Multi_Udp_Buff[i].isValid = 0;
									//     Local.Status = 0;
#ifdef _DEBUG
									printf("ͨ��ʧ��3, %d\n",
											Multi_Udp_Buff[i].buf[6]);
#endif
									break;
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
}
//---------------------------------------------------------------------------
void multi_comm_send_thread_func(void)
{
	int i;
	int HaveDataSend;
	char ubuff[20];
	if (DebugMode == 1)
	{
		P_Debug("����COMM�����������ݷ����̣߳�\n");
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
							//COMM����
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
					//����������
					pthread_mutex_lock(&Local.comm_lock);
					switch (Multi_Comm_Buff[i].buf[6])
					{
					default: //Ϊ�����������ͨ�Ž���
						Multi_Comm_Buff[i].isValid = 0;
						//if(DebugMode == 1)
						//   printf("ͨ��ʧ��, %d\n", Multi_Comm_Buff[i].buf[3]);
						break;
					}
					//�򿪻�����
					pthread_mutex_unlock(&Local.comm_lock);
				}
				//�ж������Ƿ�ȫ�������꣬���ǣ��߳���ֹ
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
		if ((cfg_fd = fopen(cfg_name, "rb")) == NULL)
		{
			P_Debug("�����ļ������ڣ��������ļ�\n");
			// if((cfg_fd = open(cfg_name, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) == -1)
			if ((cfg_fd = fopen(cfg_name, "wb")) == NULL
				)
				P_Debug("�޷����������ļ�\n");
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
					P_Debug("��ȡ�����ļ�ʧ��,��Ĭ�Ϸ�ʽ�ؽ������ļ�\n");
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
//д���������ļ�
void WriteCfgFile(void)
{
	FILE *cfg_fd;
	int j;
	uint8_t isValid;
	if ((cfg_fd = fopen(cfg_name, "wb")) == NULL
		)
		P_Debug("�޷����������ļ�\n");
	else
	{
		//��д���������ļ�
		fwrite(&LocalCfg, sizeof(LocalCfg), 1, cfg_fd);
		fclose(cfg_fd);
		P_Debug("�洢�������óɹ�\n");
	}
}
//---------------------------------------------------------------------------
#ifdef _BRUSHIDCARD_SUPPORT   //ˢ������֧��
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
	printf("iddata.TotalPackage = %d, cFromIP = %s\n", iddata.TotalPackage,
			cFromIP);
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
	//�򿪻�����
}
#endif
//---------------------------------------------------------------------------
//PMU �ر�ʱ��
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
	//PMU �ر�ʱ��   �ر�IDE  �ر�PCI
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
//�����Դ�ҳ
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
//����ʾ
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
//�ر���ʾ
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
				//24�����ַ���
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
				//24�����ֿ���û���Ʊ����,�����һƫ����
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
	//����LCD ���⺯��
	OpenLcdLight_Func();
}
//---------------------------------------------------------------------------
void CloseLCD(void)
{
	//�ر�LCD ���⺯��
	CloseLcdLight_Func();
}
//---------------------------------------------------------------------------

