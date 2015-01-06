#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>       //sem_t
#include <dirent.h>

#define CommonH
#include "common.h"

struct TImage talkover_image; //呼叫结束

//键盘和触摸屏处理函数
void KeyAndTouchFunc(int pressed);

extern int ShowCursor; //是否显示光标
//界面显示部分
void DisplayMainWindow(short wintype); //窗口

//当前窗口
//          0 -- 开机界面
//          1 -- 呼叫界面
//          2 -- 正在连接中(住户、中心)
//          3 -- 正在通话中
//          4 -- 正在监视中
//          5 -- 密码开锁界面
//          6 -- 进入设置界面
//          7--呼叫结束

//          53 -- 相片播放窗口
//          60 -- 设置主界面

//          61 -- 本机信息界面
//          62 -- 设备地址变更界面
//          63 -- 管理员密码变更界面
//          64 -- 开锁密码变更界面

//          71 -- MAC地址变更界面
//          72 -- IP地址变更界面
//          73 -- 子网掩码变更界面
//          74 -- 网关地址变更界面
//          75 -- 服务器地址变更界面

void InterfaceInit(void); //界面初始化
void MainPageInit(void); //首页初始化
void EditInit(void); //文本框初始化
void EditUnInit(void); //文本框释放
void ImageInit(void); //Image初始化
void ImageUnInit(void); //Image释放
void LabelInit(void); //Label初始化
void LabelUnInit(void); //Label释放

//void ShowLabel(struct TLabel *t_label, int refreshflag);
//显示Clock
void ShowClock(struct TLabel *t_label, int refreshflag);

void PlaySoundTip(void); //播放提示音

//显示图形数字
void outxy_pic_num(int xLeft, int yTop, char *str, int PageNo);
//---------------------------------------------------------------------------
//显示图形数字
void outxy_pic_num(int xLeft, int yTop, char *str, int PageNo)
{
	int i, j;
	int len;
	char cText[11] = "0123456789*";
	len = strlen(str);
	for (i = 0; i < len; i++)
	{
		for (j = 0; j < 11; j++)
			if (cText[j] == str[i])
			{
				num_yk[j].xLeft = xLeft + num_yk[j].width * i;
				num_yk[j].yTop = yTop;
				DisplayImage(&num_yk[j], PageNo);
				//str[len] = '\0';
				//printf("j = %d, str = %s\n", j, str);
				break;
			}
	}
}

//显示图形数字(小)
void outxy_pic_num1(int xLeft, int yTop, char *str, int PageNo)
{
	int i, j;
	int len;
	char cText[14] = "0123456789.:*";
	len = strlen(str);
	for (i = 0; i < len; i++)
	{
		for (j = 0; j < 13; j++)
			if (cText[j] == str[i])
			{
				num_image[j].xLeft = xLeft + num_image[j].width * i;
				num_image[j].yTop = yTop;
				DisplayImage(&num_image[j], PageNo);
				//str[len] = '\0';
				//printf("j = %d, str = %s\n", j, str);
				break;
			}
	}
}
//---------------------------------------------------------------------------
void DisplayMainWindow(short wintype)
{
	int i, j;
	//先关闭光标
	if (ShowCursor == 1)
		ShowCursor = 0;

	switch (wintype)
	{
	case 0: //主窗口
		DisplayWelcomeWindow(); //显示对讲窗口
		break;
	case 1: //对讲窗口（一级）
		if (Local.CurrentWindow != wintype)
			DisplayTalkWindow();
		break;
	case 5: //设置窗口（一级）
		if (Local.CurrentWindow != wintype)
			DisplaySetupWindow();
		break;
	case 12: //户户通话窗口（二级）
		break;
	case 13: //监视窗口（二级）
		if (Local.CurrentWindow != wintype)
			DisplayWatchWindow();
		break;
	case 16: //显示图像操作窗口（二级）,有呼叫时自动显示
		break;
	default:
		break;
	}

}
//---------------------------------------------------------------------------
/*void key_press_thread_func(void)
 {
 int i,j;

 #ifdef _DEBUG
 printf("创建按键处理线程：\n" );
 printf("key_press_flag=%d\n",key_press_flag);
 #endif
 while(key_press_flag == 1)
 {
 //等待有按键按下的信号
 sem_wait(&key_press_sem);
 //系统初始化标志,如没有初始化完成则等待
 while(InitSuccFlag == 0)
 usleep(10000);
 //    printf("Local.CurrentWindow = %d", Local.CurrentWindow);
 if(Local.Key_Press_Run_Flag == 0)
 Local.Key_Press_Run_Flag = 1;
 else
 {
 //键盘和触摸屏处理函数
 KeyAndTouchFunc(key_press);
 }
 }
 }                                     */
//---------------------------------------------------------------------------
void PlaySoundTip(void) //播放提示音
{
	printf("Local.Status:%d\n", Local.Status);
	if (Local.Status == 0)
	{
		WaitAudioUnuse(200); //等待声卡空闲
		sprintf(wavFile, "%s/sound1.wav\0", wavPath);
		StartPlayWav(wavFile, 0);
	}
	else
		printf("Local.Status = %d, cannot playsoundtip\n", Local.Status);
}
//---------------------------------------------------------------------------
//键盘和触摸屏处理函数
void KeyAndTouchFunc(int pressed)
{
	printf("pressed = %d\n", pressed);

	if (CheckTouchDelayTime() == 0) //触摸屏处理时检查延迟
	{
		printf("touch is too quick XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
		return;
	}
	PlaySoundTip(); //播放提示音

	PicStatBuf.KeyPressTime = 0;
	Local.LcdLightTime = 0;

	if(Local.LcdLightFlag == 0)
	{
		DisplayWelcomeWindow();
		return ;
	}

	if(Local.CurrentWindow == 0 && pressed != 107)
	{
		DisplayTalkWindow();
	}


//永远开背光
#if 0
	if(Local.LcdLightFlag == 0)
	{
		//点亮LCD背光
		OpenLCD();
		Local.LcdLightFlag = 1;//LCD背光标志
		Local.LcdLightTime = 0;//时间
		return;
	}
#endif   

	switch (pressed)
	{
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
	case 'i':
	case 'j':
	case 'k':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
	case 'p': //14
	case 'q':
	case 'r':
	case 's':
	case 't':
	case 'u':
	case 'v':
	case 'w':
	case 'x':
	case 'y':
	case 'z': //其它窗口下的按钮
	case 'a' + 30:
	case 'a' + 31:
	case 'a' + 32:
	case 'a' + 33:
	case 'a' + 34:
	case 'a' + 35:
		printf("*****KeyAndTouchFunc()***Local.CurrentWindow=%d***press key is %c***\n",
				Local.CurrentWindow, pressed);
		if ((Local.CurrentWindow >= 0) && (Local.CurrentWindow <= 300))
			switch (Local.CurrentWindow)
			{
			case 1: //对讲界面操作（一级）
				OperateTalkWindow(pressed - 'a', Local.CurrentWindow);
				break;
			case 2: //对讲 正在连接窗口操作
				OperateTalkConnectWindow(pressed - 'a', Local.CurrentWindow);
				break;
			case 3: //对讲 通话窗口操作
				OperateTalkStartWindow(pressed - 'a', Local.CurrentWindow);
				break;
			case 4: //监视界面操作(二级）
				OperateWatchWindow(pressed - 'a', Local.CurrentWindow);
				break;
			case 5: //密码开锁界面操作（一级）
				OperateOpenLockWindow(pressed - 'a', Local.CurrentWindow);
				break;
			case 6: //进入设置窗口操作（一级）
				OperateSetupWindow(pressed - 'a', Local.CurrentWindow);
				break;
				//*******************add by xuqd*******************************
			case 7: // 呼叫结束的窗口操作
				OperateTalkOverWindow(pressed - 'a', Local.CurrentWindow);
				break;

#ifdef _DOWNLOAD_PHOTO  //下载图片并播放支持
				case 53: //相片播放窗口操作（一级）
				OperatePlayPhotoWindow(pressed - 'a', Local.CurrentWindow);
				break;
#endif
			case 60: ////设置主界面操作（二级）
				OperateSetupMainWindow(pressed - 'a', Local.CurrentWindow);
				break;
			case 61: //查询本机信息操作
				OperateSetupInfoWindow(pressed - 'a', Local.CurrentWindow);
				break;
			case 62: //设备地址变更界面
				OperateAddressWindow(pressed - 'a', Local.CurrentWindow);
				break;
			case 63: //管理员密码变更界面
			case 64: //开锁密码变更界面
				OperateModiPassWindow(pressed - 'a', Local.CurrentWindow);
				break;
			case 71: //MAC地址变更界面
				OperateMacWindow(pressed - 'a', Local.CurrentWindow);
				break;
			case 72: //IP地址设置操作（三级）
				OperateIPWindow(pressed - 'a', Local.CurrentWindow);
				break;
			case 73: //子网掩码变更界面
				OperateIPMaskWindow(pressed - 'a', Local.CurrentWindow);
				break;
			case 74: //网关地址变更界面
				OperateIPGateWindow(pressed - 'a', Local.CurrentWindow);
				break;
			case 75: //服务器地址变更界面
				OperateIPServerWindow(pressed - 'a', Local.CurrentWindow);
				break;
			}
		break;
	}
}
//---------------------------------------------------------------------------
void InterfaceInit(void) //界面初始化
{
	MainPageInit(); //首页初始化
	//初始化Image
	ImageInit();

	//初始化Edit
	EditInit();

	printf("InterfaceInit 11\n");
}
//---------------------------------------------------------------------------
void MainPageInit(void) //首页初始化
{
	strcpy(currpath, sPath);
}
//---------------------------------------------------------------------------
#ifdef YK_XT8126_BV_FKT
void EditInit(void) //文本框初始化
{
	int i;
	char jpgfilename[80];

	/*户户通话窗口***************************************/
	//文本框1
	FreeEdit(&r2r_edit[0]);
	//光标
	if (r2r_edit[0].Cursor_H != NULL)
	{
		free(r2r_edit[0].Cursor_H);
		r2r_edit[0].Cursor_H = NULL;
	}

	sprintf(jpgfilename, "%s/talk_edit.jpg", currpath);
	EditLoadFromFile(jpgfilename, &r2r_edit[0]);

	r2r_edit[0].CursorX = (r2r_edit[0].width - 11 * num_yk[0].width) / 2;
	r2r_edit[0].CursorY = (r2r_edit[0].height - num_yk[0].height) / 2;
	r2r_edit[0].CursorHeight = 24;
	r2r_edit[0].CursorCorlor = cBlack;
	r2r_edit[0].fWidth = num_yk[0].width;

	r2r_edit[0].xLeft = 46;
	r2r_edit[0].yTop = 118;

	r2r_edit[0].Text[0] = '\0';
	r2r_edit[0].BoxLen = 0; //文本框当前输入长度
	r2r_edit[0].MaxLen = 11; //文本框最大输入长度
	r2r_edit[0].Focus = 0; //显示输入光标
	r2r_edit[0].Visible = 0;


	//密码开锁窗口
	openlock_edit = r2r_edit[0];

	openlock_edit.xLeft = 46;
	openlock_edit.yTop = 118;
	openlock_edit.BoxLen = 0; //文本框当前输入长度
	openlock_edit.MaxLen = 11; //文本框最大输入长度
	openlock_edit.Focus = 0; //显示输入光标
	openlock_edit.Visible = 0;


	//设置窗口
	setup_pass_edit = r2r_edit[0];

	setup_pass_edit.xLeft = 46;
	setup_pass_edit.yTop = 118;
	setup_pass_edit.BoxLen = 0; //文本框当前输入长度
	setup_pass_edit.MaxLen = 11; //文本框最大输入长度
	setup_pass_edit.Focus = 0; //显示输入光标
	setup_pass_edit.Visible = 0;

	//房号设置窗口文本框
	if (addr_edit[0].image != NULL)
		FreeImage(&addr_edit[0]);
	sprintf(jpgfilename, "%s/talk_edit1.jpg", currpath);
	EditLoadFromFile(jpgfilename, &addr_edit[0]);

	addr_edit[0].xLeft = 132;
	addr_edit[0].yTop = 166;
	addr_edit[0].BoxLen = 0; //文本框当前输入长度
	addr_edit[0].MaxLen = 7; //文本框最大输入长度
	addr_edit[0].Focus = 0; //显示输入光标
	addr_edit[0].Visible = 0;

	//MAC地址设置窗口文本框
	mac_edit[0] = addr_edit[0];

	mac_edit[0].xLeft = 132;
	mac_edit[0].yTop = 166;
	mac_edit[0].BoxLen = 0; //文本框当前输入长度
	mac_edit[0].MaxLen = 8; //文本框最大输入长度
	mac_edit[0].Focus = 0; //显示输入光标
	mac_edit[0].Visible = 0;

	//IP地址设置窗口文本框
	ip_edit[0] = addr_edit[0];

	ip_edit[0].CursorX = (ip_edit[0].width - 15 * num_image[0].width) / 2;
	ip_edit[0].CursorY = (ip_edit[0].height - num_image[0].height) / 2;
	ip_edit[0].fWidth = num_image[0].width;

	ip_edit[0].xLeft = 132;
	ip_edit[0].yTop = 166;
	ip_edit[0].BoxLen = 0; //文本框当前输入长度
	ip_edit[0].MaxLen = 15; //文本框最大输入长度
	ip_edit[0].Focus = 0; //显示输入光标
	ip_edit[0].Visible = 0;

	/*子网掩码设置窗口文本框***************************************/
	ipmask_edit[0] = addr_edit[0];

	ipmask_edit[0].xLeft = 132;
	ipmask_edit[0].yTop = 166;
	ipmask_edit[0].BoxLen = 0; //文本框当前输入长度
	ipmask_edit[0].MaxLen = 15; //文本框最大输入长度
	ipmask_edit[0].Focus = 0; //显示输入光标
	ipmask_edit[0].Visible = 0;

	/*网关地址设置窗口文本框***************************************/
	ipgate_edit[0] = addr_edit[0];

	ipgate_edit[0].xLeft = 132;
	ipgate_edit[0].yTop = 166;
	ipgate_edit[0].BoxLen = 0; //文本框当前输入长度
	ipgate_edit[0].MaxLen = 15; //文本框最大输入长度
	ipgate_edit[0].Focus = 0; //显示输入光标
	ipgate_edit[0].Visible = 0;

	/*服务器地址设置窗口文本框***************************************/
	ipserver_edit[0] = addr_edit[0];

	ipserver_edit[0].xLeft = 132;
	ipserver_edit[0].yTop = 166;
	ipserver_edit[0].BoxLen = 0; //文本框当前输入长度
	ipserver_edit[0].MaxLen = 15; //文本框最大输入长度
	ipserver_edit[0].Focus = 0; //显示输入光标
	ipserver_edit[0].Visible = 0;

	/*修改工程密码窗口文本框***************************************/
	modi_engi_edit[0] = addr_edit[0];

	modi_engi_edit[0].xLeft = 132;
	modi_engi_edit[0].yTop = 166;
	modi_engi_edit[0].BoxLen = 0; //文本框当前输入长度
	modi_engi_edit[0].MaxLen = 6; //文本框最大输入长度
	modi_engi_edit[0].Focus = 0; //显示输入光标
	modi_engi_edit[0].Visible = 0;

	modi_engi_edit[1] = addr_edit[0];

	modi_engi_edit[1].xLeft = 132;
	modi_engi_edit[1].yTop = 166;
	modi_engi_edit[1].BoxLen = 0; //文本框当前输入长度
	modi_engi_edit[1].MaxLen = 6; //文本框最大输入长度
	modi_engi_edit[1].Focus = 0; //显示输入光标
	modi_engi_edit[1].Visible = 0;
}
#else
void EditInit(void) //文本框初始化
{
	int i;
	char jpgfilename[80];

	/*户户通话窗口***************************************/
	//文本框1
	FreeEdit(&r2r_edit[0]);
	//光标
	if (r2r_edit[0].Cursor_H != NULL)
	{
		free(r2r_edit[0].Cursor_H);
		r2r_edit[0].Cursor_H = NULL;
	}

	sprintf(jpgfilename, "%s/talk_edit.jpg", currpath);
	EditLoadFromFile(jpgfilename, &r2r_edit[0]);

	r2r_edit[0].CursorX = (r2r_edit[0].width - 6 * num_yk[0].width) / 2;
	r2r_edit[0].CursorY = (r2r_edit[0].height - num_yk[0].height) / 2;
	r2r_edit[0].CursorHeight = 24;
	r2r_edit[0].CursorCorlor = cBlack;
	r2r_edit[0].fWidth = num_yk[0].width;

	r2r_edit[0].xLeft = 120;
	r2r_edit[0].yTop = 108;

	r2r_edit[0].Text[0] = '\0';
	r2r_edit[0].BoxLen = 0; //文本框当前输入长度
	r2r_edit[0].MaxLen = 6; //文本框最大输入长度
	r2r_edit[0].Focus = 0; //显示输入光标
	r2r_edit[0].Visible = 0;


	//密码开锁窗口
	openlock_edit = r2r_edit[0];

	openlock_edit.xLeft = 120;
	openlock_edit.yTop = 108;
	openlock_edit.BoxLen = 0; //文本框当前输入长度
	openlock_edit.MaxLen = 6; //文本框最大输入长度
	openlock_edit.Focus = 0; //显示输入光标
	openlock_edit.Visible = 0;


	//设置窗口
	setup_pass_edit = r2r_edit[0];

	setup_pass_edit.xLeft = 120;
	setup_pass_edit.yTop = 108;
	setup_pass_edit.BoxLen = 0; //文本框当前输入长度
	setup_pass_edit.MaxLen = 6; //文本框最大输入长度
	setup_pass_edit.Focus = 0; //显示输入光标
	setup_pass_edit.Visible = 0;

	//房号设置窗口文本框
	if (addr_edit[0].image != NULL)
		FreeImage(&addr_edit[0]);
	sprintf(jpgfilename, "%s/talk_edit1.jpg", currpath);
	EditLoadFromFile(jpgfilename, &addr_edit[0]);

	addr_edit[0].xLeft = 132;
	addr_edit[0].yTop = 166;
	addr_edit[0].BoxLen = 0; //文本框当前输入长度
	addr_edit[0].MaxLen = 7; //文本框最大输入长度
	addr_edit[0].Focus = 0; //显示输入光标
	addr_edit[0].Visible = 0;

	//MAC地址设置窗口文本框
	mac_edit[0] = addr_edit[0];

	mac_edit[0].xLeft = 132;
	mac_edit[0].yTop = 166;
	mac_edit[0].BoxLen = 0; //文本框当前输入长度
	mac_edit[0].MaxLen = 8; //文本框最大输入长度
	mac_edit[0].Focus = 0; //显示输入光标
	mac_edit[0].Visible = 0;

	//IP地址设置窗口文本框
	ip_edit[0] = addr_edit[0];

	ip_edit[0].CursorX = (ip_edit[0].width - 15 * num_image[0].width) / 2;
	ip_edit[0].CursorY = (ip_edit[0].height - num_image[0].height) / 2;
	ip_edit[0].fWidth = num_image[0].width;

	ip_edit[0].xLeft = 132;
	ip_edit[0].yTop = 166;
	ip_edit[0].BoxLen = 0; //文本框当前输入长度
	ip_edit[0].MaxLen = 15; //文本框最大输入长度
	ip_edit[0].Focus = 0; //显示输入光标
	ip_edit[0].Visible = 0;

	/*子网掩码设置窗口文本框***************************************/
	ipmask_edit[0] = addr_edit[0];

	ipmask_edit[0].xLeft = 132;
	ipmask_edit[0].yTop = 166;
	ipmask_edit[0].BoxLen = 0; //文本框当前输入长度
	ipmask_edit[0].MaxLen = 15; //文本框最大输入长度
	ipmask_edit[0].Focus = 0; //显示输入光标
	ipmask_edit[0].Visible = 0;

	/*网关地址设置窗口文本框***************************************/
	ipgate_edit[0] = addr_edit[0];

	ipgate_edit[0].xLeft = 132;
	ipgate_edit[0].yTop = 166;
	ipgate_edit[0].BoxLen = 0; //文本框当前输入长度
	ipgate_edit[0].MaxLen = 15; //文本框最大输入长度
	ipgate_edit[0].Focus = 0; //显示输入光标
	ipgate_edit[0].Visible = 0;

	/*服务器地址设置窗口文本框***************************************/
	ipserver_edit[0] = addr_edit[0];

	ipserver_edit[0].xLeft = 132;
	ipserver_edit[0].yTop = 166;
	ipserver_edit[0].BoxLen = 0; //文本框当前输入长度
	ipserver_edit[0].MaxLen = 15; //文本框最大输入长度
	ipserver_edit[0].Focus = 0; //显示输入光标
	ipserver_edit[0].Visible = 0;

	/*修改工程密码窗口文本框***************************************/
	modi_engi_edit[0] = addr_edit[0];

	modi_engi_edit[0].xLeft = 132;
	modi_engi_edit[0].yTop = 166;
	modi_engi_edit[0].BoxLen = 0; //文本框当前输入长度
	modi_engi_edit[0].MaxLen = 6; //文本框最大输入长度
	modi_engi_edit[0].Focus = 0; //显示输入光标
	modi_engi_edit[0].Visible = 0;

	modi_engi_edit[1] = addr_edit[0];

	modi_engi_edit[1].xLeft = 132;
	modi_engi_edit[1].yTop = 166;
	modi_engi_edit[1].BoxLen = 0; //文本框当前输入长度
	modi_engi_edit[1].MaxLen = 6; //文本框最大输入长度
	modi_engi_edit[1].Focus = 0; //显示输入光标
	modi_engi_edit[1].Visible = 0;
}
#endif
//---------------------------------------------------------------------------
void EditUnInit(void) //文本框释放
{
	int i;

	/*户户通话窗口***************************************/
	FreeEdit(&r2r_edit[0]);
	//光标
	if (r2r_edit[0].Cursor_H != NULL)
	{
		free(r2r_edit[0].Cursor_H);
		r2r_edit[0].Cursor_H = NULL;
	}
}
//---------------------------------------------------------------------------
#ifdef YK_XT8126_BV_FKT
void ImageInit(void) //Image初始化
{
	char jpgfilename[80];
	int i;
	int addr_xLeft, addr_yTop;

	addr_xLeft = 132;
	addr_yTop = 81;
	printf("image 1\n");

	if (num_back_image.image != NULL
		)
		FreeImage(&num_back_image);
	sprintf(jpgfilename, "%s/num.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &num_back_image);

	for (i = 0; i < 13; i++)
	{
		num_image[i].width = 28;
		num_image[i].height = 48;

		if (num_image[i].image == NULL)
			num_image[i].image = (unsigned char *) malloc(num_image[i].width * num_image[i].height * 2);
		SavePicS_D_Func(i * num_image[i].width, 0, num_image[i].width,
				num_image[i].height, num_image[i].image, num_back_image.width,
				num_back_image.height, num_back_image.image); //光标
	}

	if (num_back_image.image != NULL)
		FreeImage(&num_back_image);

	for(i=0; i<10; i++)
	{
		if(num_yk[i].image != NULL)
		{
			FreeImage(&num_yk[i]);
		}
		sprintf(jpgfilename, "%s/num_0%d.jpg\0", currpath, i);
		ImageLoadFromFile(jpgfilename, &num_yk[i]);
		num_yk[i].width = 50;
		num_yk[i].height = 96;
	}

	if(num_yk[10].image != NULL)
	{
		FreeImage(&num_yk[10]);
	}
	sprintf(jpgfilename, "%s/num_10.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &num_yk[10]);
	num_yk[10].width = 50;
	num_yk[10].height = 96;

	//显示欢迎界面
	if (talk_image.image != NULL)
		FreeImage(&talk_image);
	sprintf(jpgfilename, "%s/welcome.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &talk_image);
	talk_image.xLeft = 0;
	talk_image.yTop = 0;

	//可视对讲背景图像  单元
	if (talk1_image.image != NULL)
		FreeImage(&talk1_image);
	sprintf(jpgfilename, "%s/talk1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &talk1_image);
	talk1_image.xLeft = 0;
	talk1_image.yTop = 0;

	//可视对讲背景图像  围墙
	if (talk2_image.image != NULL)
		FreeImage(&talk2_image);
	sprintf(jpgfilename, "%s/talk2.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &talk2_image);
	talk2_image.xLeft = 0;
	talk2_image.yTop = 0;

	//可视对讲 正在连接背景图像
	if (talk_connect_image.image != NULL)
		FreeImage(&talk_connect_image);
	sprintf(jpgfilename, "%s/talk_connect.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &talk_connect_image);
	talk_connect_image.xLeft = 0;
	talk_connect_image.yTop = 0;

	//可视对讲 通话背景图像
	if (talk_start_image.image != NULL)
		FreeImage(&talk_start_image);
	sprintf(jpgfilename, "%s/talk_start.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &talk_start_image);
	talk_start_image.xLeft = 0;
	talk_start_image.yTop = 0;

	//密码开锁背景图像
	if (openlock_image.image != NULL)
		FreeImage(&openlock_image);
	sprintf(jpgfilename, "%s/openlock.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &openlock_image);
	openlock_image.xLeft = 0;
	openlock_image.yTop = 0;

	//门锁已打开图像
	if (door_openlock.image != NULL)
		FreeImage(&door_openlock);
	sprintf(jpgfilename, "%s/dooropen.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &door_openlock);
	door_openlock.xLeft = 286;
	door_openlock.yTop = 358;

	if (sip_ok.image != NULL)
		FreeImage(&sip_ok);
	sprintf(jpgfilename, "%s/sip_ok.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &sip_ok);
	sip_ok.xLeft = 630;
	sip_ok.yTop = 400;

	if (sip_error.image != NULL)
		FreeImage(&sip_error);
	sprintf(jpgfilename, "%s/sip_error.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &sip_error);
	sip_error.xLeft = 630;
	sip_error.yTop = 400;

	//监视背景图像
	if (watch_image.image != NULL)
		FreeImage(&watch_image);
	sprintf(jpgfilename, "%s/watch.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &watch_image);
	watch_image.xLeft = 0;
	watch_image.yTop = 0;

	//设置窗口背景图像
	if (setupback_image.image != NULL)
		FreeImage(&setupback_image);
	sprintf(jpgfilename, "%s/setup.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &setupback_image);
	setupback_image.xLeft = 0;
	setupback_image.yTop = 0;

	//设置主窗口背景图像
	if (setupmain_image.image != NULL)
		FreeImage(&setupmain_image);
	sprintf(jpgfilename, "%s/setupmain.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &setupmain_image);
	setupmain_image.xLeft = 0;
	setupmain_image.yTop = 0;

	//查询本机信息窗口 背景
	if (setup_info_image.image != NULL)
		FreeImage(&setup_info_image);
	sprintf(jpgfilename, "%s/setup_info.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &setup_info_image);
	setup_info_image.xLeft = 0;
	setup_info_image.yTop = 0;

	//修改大窗口 背景
	if (setup_modify_image.image != NULL)
		FreeImage(&setup_modify_image);
	sprintf(jpgfilename, "%s/setup_modify_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &setup_modify_image);
	setup_modify_image.xLeft = 0;
	setup_modify_image.yTop = 0;

	//本机地址设置窗口Image
	if (addr_image[0].image != NULL)
		FreeImage(&addr_image[0]);
	sprintf(jpgfilename, "%s/addr_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &addr_image[0]);
	addr_image[0].xLeft = addr_xLeft;
	addr_image[0].yTop = addr_yTop;

	//MAC地址设置窗口Image
	if (mac_image[0].image != NULL)
		FreeImage(&mac_image[0]);
	sprintf(jpgfilename, "%s/mac_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &mac_image[0]);
	mac_image[0].xLeft = addr_xLeft;
	mac_image[0].yTop = addr_yTop;

	//IP设置窗口Image
	if (ip_image[0].image != NULL)
		FreeImage(&ip_image[0]);
	sprintf(jpgfilename, "%s/ip_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &ip_image[0]);
	ip_image[0].xLeft = addr_xLeft;
	ip_image[0].yTop = addr_yTop;

	//子网掩码设置窗口Image
	if (ipmask_image[0].image != NULL)
		FreeImage(&ipmask_image[0]);
	sprintf(jpgfilename, "%s/ipmask_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &ipmask_image[0]);
	ipmask_image[0].xLeft = addr_xLeft;
	ipmask_image[0].yTop = addr_yTop;

	//网关设置窗口Image
	if (ipgate_image[0].image != NULL)
		FreeImage(&ipgate_image[0]);
	sprintf(jpgfilename, "%s/ipgate_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &ipgate_image[0]);
	ipgate_image[0].xLeft = addr_xLeft;
	ipgate_image[0].yTop = addr_yTop;

	//服务器地址设置窗口Image
	if (ipserver_image[0].image != NULL
		)
		FreeImage(&ipserver_image[0]);
	sprintf(jpgfilename, "%s/ipserver_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &ipserver_image[0]);
	ipserver_image[0].xLeft = addr_xLeft;
	ipserver_image[0].yTop = addr_yTop;

	//工程密码设置窗口Image
	if (engineer_pass_image[0].image != NULL)
		FreeImage(&engineer_pass_image[0]);
	sprintf(jpgfilename, "%s/engineer_pass_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &engineer_pass_image[0]);
	engineer_pass_image[0].xLeft = addr_xLeft;
	engineer_pass_image[0].yTop = addr_yTop;

	if (engineer_pass_image[1].image != NULL
		)
		FreeImage(&engineer_pass_image[1]);
	sprintf(jpgfilename, "%s/engineer_pass_image2.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &engineer_pass_image[1]);
	engineer_pass_image[1].xLeft = addr_xLeft;
	engineer_pass_image[1].yTop = addr_yTop;

	//开锁密码设置窗口Image
	if (lock_pass_image[0].image != NULL)
		FreeImage(&lock_pass_image[0]);
	sprintf(jpgfilename, "%s/lock_pass_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &lock_pass_image[0]);
	lock_pass_image[0].xLeft = addr_xLeft;
	lock_pass_image[0].yTop = addr_yTop;

	if (lock_pass_image[1].image != NULL
		)
		FreeImage(&lock_pass_image[1]);
	sprintf(jpgfilename, "%s/lock_pass_image2.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &lock_pass_image[1]);
	lock_pass_image[1].xLeft = addr_xLeft;
	lock_pass_image[1].yTop = addr_yTop;

	//呼叫结束
	if (talkover_image.image != NULL)
		FreeImage(&talkover_image);
	sprintf(jpgfilename, "%s/talk_over.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &talkover_image);
	talkover_image.xLeft = 0;
	talkover_image.yTop = 0;

}
#else
void ImageInit(void) //Image初始化
{
	char jpgfilename[80];
	int i;
	int addr_xLeft, addr_yTop;

	addr_xLeft = 132;
	addr_yTop = 81;
	printf("image 1\n");

	if (num_back_image.image != NULL
		)
		FreeImage(&num_back_image);
	sprintf(jpgfilename, "%s/num.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &num_back_image);

	for (i = 0; i < 13; i++)
	{
		num_image[i].width = 28;
		num_image[i].height = 48;

		if (num_image[i].image == NULL)
			num_image[i].image = (unsigned char *) malloc(num_image[i].width * num_image[i].height * 2);
		SavePicS_D_Func(i * num_image[i].width, 0, num_image[i].width,
				num_image[i].height, num_image[i].image, num_back_image.width,
				num_back_image.height, num_back_image.image); //光标
	}

	if (num_back_image.image != NULL)
		FreeImage(&num_back_image);

	for(i=0; i<10; i++)
	{
		if(num_yk[i].image != NULL)
		{
			FreeImage(&num_yk[i]);
		}
		sprintf(jpgfilename, "%s/num_0%d.jpg\0", currpath, i);
		ImageLoadFromFile(jpgfilename, &num_yk[i]);
		num_yk[i].width = 66;
		num_yk[i].height = 96;
	}

	if(num_yk[10].image != NULL)
	{
		FreeImage(&num_yk[10]);
	}
	sprintf(jpgfilename, "%s/num_10.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &num_yk[10]);
	num_yk[10].width = 66;
	num_yk[10].height = 96;

	//显示欢迎界面
	if (talk_image.image != NULL)
		FreeImage(&talk_image);
	sprintf(jpgfilename, "%s/welcome.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &talk_image);
	talk_image.xLeft = 0;
	talk_image.yTop = 0;

	//可视对讲背景图像  单元
	if (talk1_image.image != NULL)
		FreeImage(&talk1_image);
	sprintf(jpgfilename, "%s/talk1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &talk1_image);
	talk1_image.xLeft = 0;
	talk1_image.yTop = 0;

	//可视对讲背景图像  围墙
	if (talk2_image.image != NULL)
		FreeImage(&talk2_image);
	sprintf(jpgfilename, "%s/talk2.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &talk2_image);
	talk2_image.xLeft = 0;
	talk2_image.yTop = 0;

	//可视对讲 正在连接背景图像
	if (talk_connect_image.image != NULL)
		FreeImage(&talk_connect_image);
	sprintf(jpgfilename, "%s/talk_connect.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &talk_connect_image);
	talk_connect_image.xLeft = 0;
	talk_connect_image.yTop = 0;

	//可视对讲 通话背景图像
	if (talk_start_image.image != NULL)
		FreeImage(&talk_start_image);
	sprintf(jpgfilename, "%s/talk_start.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &talk_start_image);
	talk_start_image.xLeft = 0;
	talk_start_image.yTop = 0;

	//密码开锁背景图像
	if (openlock_image.image != NULL)
		FreeImage(&openlock_image);
	sprintf(jpgfilename, "%s/openlock.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &openlock_image);
	openlock_image.xLeft = 0;
	openlock_image.yTop = 0;

	//门锁已打开图像
	if (door_openlock.image != NULL)
		FreeImage(&door_openlock);
	sprintf(jpgfilename, "%s/dooropen.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &door_openlock);
	door_openlock.xLeft = 258;
	door_openlock.yTop = 300;

	if (sip_ok.image != NULL)
		FreeImage(&sip_ok);
	sprintf(jpgfilename, "%s/sip_ok.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &sip_ok);
	sip_ok.xLeft = 630;
	sip_ok.yTop = 400;

	if (sip_error.image != NULL)
		FreeImage(&sip_error);
	sprintf(jpgfilename, "%s/sip_error.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &sip_error);
	sip_error.xLeft = 630;
	sip_error.yTop = 400;

	//监视背景图像
	if (watch_image.image != NULL)
		FreeImage(&watch_image);
	sprintf(jpgfilename, "%s/watch.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &watch_image);
	watch_image.xLeft = 0;
	watch_image.yTop = 0;

	//设置窗口背景图像
	if (setupback_image.image != NULL)
		FreeImage(&setupback_image);
	sprintf(jpgfilename, "%s/setup.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &setupback_image);
	setupback_image.xLeft = 0;
	setupback_image.yTop = 0;

	//设置主窗口背景图像
	if (setupmain_image.image != NULL)
		FreeImage(&setupmain_image);
	sprintf(jpgfilename, "%s/setupmain.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &setupmain_image);
	setupmain_image.xLeft = 0;
	setupmain_image.yTop = 0;

	//查询本机信息窗口 背景
	if (setup_info_image.image != NULL)
		FreeImage(&setup_info_image);
	sprintf(jpgfilename, "%s/setup_info.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &setup_info_image);
	setup_info_image.xLeft = 0;
	setup_info_image.yTop = 0;

	//修改大窗口 背景
	if (setup_modify_image.image != NULL)
		FreeImage(&setup_modify_image);
	sprintf(jpgfilename, "%s/setup_modify_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &setup_modify_image);
	setup_modify_image.xLeft = 0;
	setup_modify_image.yTop = 0;

	//本机地址设置窗口Image
	if (addr_image[0].image != NULL)
		FreeImage(&addr_image[0]);
	sprintf(jpgfilename, "%s/addr_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &addr_image[0]);
	addr_image[0].xLeft = addr_xLeft;
	addr_image[0].yTop = addr_yTop;

	//MAC地址设置窗口Image
	if (mac_image[0].image != NULL)
		FreeImage(&mac_image[0]);
	sprintf(jpgfilename, "%s/mac_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &mac_image[0]);
	mac_image[0].xLeft = addr_xLeft;
	mac_image[0].yTop = addr_yTop;

	//IP设置窗口Image
	if (ip_image[0].image != NULL)
		FreeImage(&ip_image[0]);
	sprintf(jpgfilename, "%s/ip_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &ip_image[0]);
	ip_image[0].xLeft = addr_xLeft;
	ip_image[0].yTop = addr_yTop;

	//子网掩码设置窗口Image
	if (ipmask_image[0].image != NULL)
		FreeImage(&ipmask_image[0]);
	sprintf(jpgfilename, "%s/ipmask_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &ipmask_image[0]);
	ipmask_image[0].xLeft = addr_xLeft;
	ipmask_image[0].yTop = addr_yTop;

	//网关设置窗口Image
	if (ipgate_image[0].image != NULL)
		FreeImage(&ipgate_image[0]);
	sprintf(jpgfilename, "%s/ipgate_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &ipgate_image[0]);
	ipgate_image[0].xLeft = addr_xLeft;
	ipgate_image[0].yTop = addr_yTop;

	//服务器地址设置窗口Image
	if (ipserver_image[0].image != NULL
		)
		FreeImage(&ipserver_image[0]);
	sprintf(jpgfilename, "%s/ipserver_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &ipserver_image[0]);
	ipserver_image[0].xLeft = addr_xLeft;
	ipserver_image[0].yTop = addr_yTop;

	//工程密码设置窗口Image
	if (engineer_pass_image[0].image != NULL)
		FreeImage(&engineer_pass_image[0]);
	sprintf(jpgfilename, "%s/engineer_pass_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &engineer_pass_image[0]);
	engineer_pass_image[0].xLeft = addr_xLeft;
	engineer_pass_image[0].yTop = addr_yTop;

	if (engineer_pass_image[1].image != NULL
		)
		FreeImage(&engineer_pass_image[1]);
	sprintf(jpgfilename, "%s/engineer_pass_image2.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &engineer_pass_image[1]);
	engineer_pass_image[1].xLeft = addr_xLeft;
	engineer_pass_image[1].yTop = addr_yTop;

	//开锁密码设置窗口Image
	if (lock_pass_image[0].image != NULL)
		FreeImage(&lock_pass_image[0]);
	sprintf(jpgfilename, "%s/lock_pass_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &lock_pass_image[0]);
	lock_pass_image[0].xLeft = addr_xLeft;
	lock_pass_image[0].yTop = addr_yTop;

	if (lock_pass_image[1].image != NULL
		)
		FreeImage(&lock_pass_image[1]);
	sprintf(jpgfilename, "%s/lock_pass_image2.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &lock_pass_image[1]);
	lock_pass_image[1].xLeft = addr_xLeft;
	lock_pass_image[1].yTop = addr_yTop;

	//呼叫结束
	if (talkover_image.image != NULL)
		FreeImage(&talkover_image);
	sprintf(jpgfilename, "%s/talk_over.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &talkover_image);
	talkover_image.xLeft = 0;
	talkover_image.yTop = 0;

}
#endif
//---------------------------------------------------------------------------
void ImageUnInit(void) //Image释放
{
	int i;
	//首页背景图像
	printf("image 0\n");
	for (i = 0; i < 13; i++)
		if (num_image[i].image != NULL)
			FreeImage(&num_image[i]);

	printf("image 1\n");
	//可视对讲背景图像  单元
	if (talk1_image.image != NULL)
		FreeImage(&talk1_image);

	//可视对讲背景图像  围墙
	if (talk2_image.image != NULL)
		FreeImage(&talk2_image);

	//可视对讲 正在连接背景图像
	if (talk_connect_image.image != NULL)
		FreeImage(&talk_connect_image);

	//可视对讲 通话背景图像
	if (talk_start_image.image != NULL)
		FreeImage(&talk_start_image);

	//密码开锁背景图像
	if (openlock_image.image != NULL)
		FreeImage(&openlock_image);

	//门锁已打开图像
	if (door_openlock.image != NULL)
		FreeImage(&door_openlock);

	printf("image 2\n");

	if (sip_ok.image != NULL)
		FreeImage(&sip_ok);

	if (sip_error.image != NULL)
		FreeImage(&sip_error);

	//监视背景图像
	if (watch_image.image != NULL)
		FreeImage(&watch_image);
	printf("image 3\n");
	//设置窗口背景图像
	if (setupback_image.image != NULL)
		FreeImage(&setupback_image);
	printf("image 4\n");
	//网络设置窗口Image
	for (i = 0; i < 1; i++)
		FreeImage(&ip_image[i]);
	printf("image 5\n");
	//房号设置窗口Image
	for (i = 0; i < 1; i++)
		FreeImage(&addr_image[i]);
	printf("image 6\n");
	//工程密码设置窗口Image
	for (i = 0; i < 2; i++)
		FreeImage(&engineer_pass_image[i]);

	//开锁密码设置窗口Image
	for (i = 0; i < 2; i++)
		FreeImage(&lock_pass_image[i]);

	//可视对讲 通话背景 开锁Label
	if (label_talk_start.image != NULL)
		free(label_talk_start.image);

	// sip呼叫结束
	if (talkover_image.image != NULL)
		FreeImage(&talkover_image);
}
//---------------------------------------------------------------------------
void LabelInit(void) //Label初始化
{
}
//---------------------------------------------------------------------------
void LabelUnInit(void) //Label释放
{
}
//---------------------------------------------------------------------------

//显示Label
/*void ShowLabel(struct TLabel *t_label, int refreshflag)
 {
 if(refreshflag == REFRESH)
 RestorePicBuf_Func(t_label->xLeft - 2, t_label->yTop - 2, t_label->width, t_label->height,
 t_label->image, 0);
 if(refreshflag == HILIGHT)
 {
 printf("t_label->xLeft=%d,t_label->yTop=%d,t_label->width=%d,t_label->height=%d \n" ,
 t_label->xLeft, t_label->yTop, t_label->width, t_label->height);
 RestorePicBuf_Func(t_label->xLeft - 2, t_label->yTop -2, t_label->width, t_label->height,
 t_label->image_h, 0);
 }
 outxy24(t_label->xLeft, t_label->yTop, 3, cWhite, 1, 1, t_label->Text, 0, 0);
 }                  */
//---------------------------------------------------------------------------
//显示Clock
char OldClock[20] = "####################"; //2004-33-33 33:33:33
void ShowClock(struct TLabel *t_label, int refreshflag)
{
	if (refreshflag == REFRESH)
		RestorePicBuf_Func(t_label->xLeft, t_label->yTop, t_label->width,
				t_label->height, t_label->image, 0);
	outxy16(t_label->xLeft + 15, t_label->yTop + 2, 1, cBlack, 1, 1,
			t_label->Text, 0, 0);
}
//---------------------------------------------------------------------------
void BuffUninit(void)
{
}
//---------------------------------------------------------------------------
