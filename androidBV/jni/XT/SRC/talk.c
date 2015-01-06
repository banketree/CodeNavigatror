#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>       //sem_t
#include "LPIntfLayer.h"
#include "TMInterface.h"

#define CommonH
#include "common.h"
extern char sPath[80];
extern char NullAddr[21]; //空字符串

static char acCurRoom[20];

//****************************add by xuqd****************
#include <XTIntfLayer.h>
//*****************************end************************
void DisplayWelcomeWindow(void);

void DisplayTalkWindow(void); //显示对讲窗口（一级）
void OperateTalkWindow(short wintype, int currwindow, int g_sipCallID); //对讲窗口操作（一级）

void ClearTalkEdit(void); //清 文本框

void DisplayTalkConnectWindow(void); //显示对讲 正在连接窗口
void OperateTalkConnectWindow(short wintype, int currwindow); //对讲 正在连接窗口操作

void DisplayTalkStartWindow(void); //显示对讲 通话窗口
void OperateTalkStartWindow(short wintype, int currwindow); //对讲 通话窗口操作

void DisplayWatchWindow(void); //监视窗口操作（二级）
void OperateWatchWindow(short wintype, int currwindow); //监视窗口操作（二级）

void Call_Func(int uFlag, char *call_addr, char *CenterAddr); //呼叫   1  中心  2 住户
//void call_sip_func(char *call_addr); //add by xuqd

void DisplayOpenLockWindow(void); //显示密码开锁窗口（一级）
void OperateOpenLockWindow(short wintype, int currwindow); //密码开锁窗口操作（一级）

void Talk_Func(void); //通话 接听
void TalkEnd_Func(void);
void WatchEnd_Func(void);
void CallTimeOut_Func(void); //呼叫超时


//---------------------------------------------------------------------------

void DisplayWelcomeWindow(void) //显示欢迎界面
{
	int i;

	while (Local.isDisplayWindowFlag == 1) //窗口正在显示中
		usleep(10);
	Local.isDisplayWindowFlag = 1;

	Local.PrevWindow = Local.CurrentWindow;
	printf("*****DisplayTalkWindow()******Local.CurrentWindow is %d*****\n",
			Local.CurrentWindow);
	Local.CurrentWindow = 0;
	printf("*****DisplayTalkWindow()******Local.CurrentWindow set 0*****\n");
	if (Local.CurrFbPage != 0)
	{
		Local.CurrFbPage = 0;
		fb_setpage(fbdev, Local.CurrFbPage);
	}

	DisplayImage(&talk_image, 0);

	Local.isDisplayWindowFlag = 0; //窗口正在显示中
	DisplaySipStatusWindow();
}

void DisplayTalkWindow(void) //显示对讲窗口
{
	int i;

	while (Local.isDisplayWindowFlag == 1) //窗口正在显示中
		usleep(10);
	Local.isDisplayWindowFlag = 1;

	Local.PrevWindow = Local.CurrentWindow;
	printf("*****DisplayTalkWindow()******Local.CurrentWindow is %d*****\n",
			Local.CurrentWindow);
	Local.CurrentWindow = 1;
	printf("*****DisplayTalkWindow()******Local.CurrentWindow set 0*****\n");
	if (Local.CurrFbPage != 0)
	{
		Local.CurrFbPage = 0;
		fb_setpage(fbdev, Local.CurrFbPage);
	}
	if (LocalCfg.Addr[0] == 'M')
		DisplayImage(&talk1_image, 0);
	else
		DisplayImage(&talk2_image, 0);
#ifdef YK_XT8126_BV_FKT
	r2r_edit[0].MaxLen = 11;
#else
	r2r_edit[0].MaxLen = 6;
#endif

	CurrBox = 0;

	ClearTalkEdit(); //清 文本框
	//updateSipStatusFunc();
	Local.isDisplayWindowFlag = 0; //窗口正在显示中
	DisplaySipStatusWindow();
}
//---------------------------------------------------------------------------
#ifdef YK_XT8126_BV_FKT
void OperateTalkWindow(short wintype, int currwindow, int g_sipCallID) //对讲窗口操作（一级）
{
	int i, j;

	char tmp_text[20];
	char str[10];
	int isOK;

	printf("OperateTalkWindow::wintype = %d\n", wintype);
	CurrBox = 0;

	switch (wintype)
	{
	case 0: //a
	case 1: //b
	case 2: //c
	case 3: //d
	case 4: //e
	case 5: //f
	case 6: //g
	case 7: //h
	case 8: //i
	case 9: //j
		if (r2r_edit[CurrBox].BoxLen < r2r_edit[CurrBox].MaxLen)
		{
			str[0] = '0' + wintype;
			str[1] = '\0';
			strcat(r2r_edit[CurrBox].Text, str);
#if 0
			outxy24(r2r_edit[CurrBox].xLeft + r2r_edit[CurrBox].CursorX + r2r_edit[CurrBox].BoxLen*12,
					r2r_edit[CurrBox].yTop +r2r_edit[CurrBox].CursorY, 2, cBlack, 2, 2, str, 0, 0);
#else
			//printf("str = %s\n", str);
			outxy_pic_num(
					r2r_edit[CurrBox].xLeft + r2r_edit[CurrBox].CursorX
							+ r2r_edit[CurrBox].BoxLen * num_yk[0].width,
					r2r_edit[CurrBox].yTop + r2r_edit[CurrBox].CursorY, str, 0);
			//printf("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx r2r_edit[CurrBox].yTop + r2r_edit[CurrBox].CursorY: %d\n", r2r_edit[CurrBox].yTop + r2r_edit[CurrBox].CursorY);
#endif
			r2r_edit[CurrBox].BoxLen++;
		}
		break;
	case 10: //*  k

		if (Local.Status != 0)
		{
			if ((Local.Status == 3) || (Local.Status == 4))
			{
				WatchEnd_Func();
			}
			else
			{
				TalkEnd_Func();
			}
			break;
			//*************************add by xuqd***************
			if (INVALID_SIP_CALL_ID != g_sipCallID)
			{
				printf("**************OperateTalkWindow!****TerminateLpCall(g_sipCallID)\n");
				TerminateLpCall(g_sipCallID);
			}
			//**************************end**********************
		}
		if(r2r_edit[CurrBox].Text[0] == '\0' && Local.CurrentWindow == 1)
		{
			DisplayWelcomeWindow();
		}

		if(r2r_edit[CurrBox].Text[0] != '\0')
		{
			DisplayEdit(&r2r_edit[0], 0); //清 文本框
			            
		    //*键单个删除数字
            r2r_edit[CurrBox].BoxLen--;
            r2r_edit[CurrBox].Text[r2r_edit[CurrBox].BoxLen] = '\0';
			outxy_pic_num(
					r2r_edit[CurrBox].xLeft + r2r_edit[CurrBox].CursorX
							+ 0 * num_yk[0].width,
					r2r_edit[CurrBox].yTop + r2r_edit[CurrBox].CursorY, 
					r2r_edit[CurrBox].Text, 0);
			
		}
		break;
	case 11: //#  l
//		isOK = 0;
		if (r2r_edit[0].BoxLen == 0)
		{
			DisplayOpenLockWindow(); //显示密码开锁窗口（一级）
			break;
		}
		r2r_edit[0].Text[r2r_edit[0].BoxLen] = '\0';
		if ((r2r_edit[0].BoxLen == 6)
				&& (strcmp(r2r_edit[0].Text, "999999") == 0))
		{
			DisplaySetupWindow(); //显示设置窗口（一级）
			break;
		}

		Call_Func(2, r2r_edit[0].Text, ""); //呼叫   1  中心  2 住户

		// 初始化sip呼叫的为被动状态
		Local.cSipCallPassive = 1;
		if(Local.cRegFlg == 1)
		{
			Local.cSipCallPassive = 0;
			if(r2r_edit[0].BoxLen <= 11)
			{
				InvokeLpCall(r2r_edit[0].Text);
				strcpy(acCurRoom, r2r_edit[0].Text);
			}
			else
			{
				printf("input is error!\n");
			}
		}

		break;
	}
}
#else
void OperateTalkWindow(short wintype, int currwindow, int g_sipCallID) //对讲窗口操作（一级）
{
	int i, j;

	char tmp_text[20];
	char str[10];
	int isOK;

	printf("OperateTalkWindow::wintype = %d\n", wintype);
	CurrBox = 0;

	switch (wintype)
	{
	case 0: //a
	case 1: //b
	case 2: //c
	case 3: //d
	case 4: //e
	case 5: //f
	case 6: //g
	case 7: //h
	case 8: //i
	case 9: //j
		if (r2r_edit[CurrBox].BoxLen < r2r_edit[CurrBox].MaxLen)
		{
			str[0] = '0' + wintype;
			str[1] = '\0';
			strcat(r2r_edit[CurrBox].Text, str);
#if 0
			outxy24(r2r_edit[CurrBox].xLeft + r2r_edit[CurrBox].CursorX + r2r_edit[CurrBox].BoxLen*12,
					r2r_edit[CurrBox].yTop +r2r_edit[CurrBox].CursorY, 2, cBlack, 2, 2, str, 0, 0);
#else
			//printf("str = %s\n", str);
			outxy_pic_num(
					r2r_edit[CurrBox].xLeft + r2r_edit[CurrBox].CursorX
							+ r2r_edit[CurrBox].BoxLen * num_yk[0].width,
					r2r_edit[CurrBox].yTop + r2r_edit[CurrBox].CursorY, str, 0);
			//printf("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx r2r_edit[CurrBox].yTop + r2r_edit[CurrBox].CursorY: %d\n", r2r_edit[CurrBox].yTop + r2r_edit[CurrBox].CursorY);
#endif
			r2r_edit[CurrBox].BoxLen++;
		}
		break;
	case 10: //*  k

		if (Local.Status != 0)
		{
			if ((Local.Status == 3) || (Local.Status == 4))
			{
				WatchEnd_Func();
			}
			else
			{
				TalkEnd_Func();
			}
			break;
			//*************************add by xuqd***************
			if (INVALID_SIP_CALL_ID != g_sipCallID)
			{
				printf("**************OperateTalkWindow!****TerminateLpCall(g_sipCallID)\n");
				TerminateLpCall(g_sipCallID);
			}
			//**************************end**********************
		}
		if(r2r_edit[CurrBox].Text[0] == '\0' && Local.CurrentWindow == 1)
		{
			DisplayWelcomeWindow();
		}

		if(r2r_edit[CurrBox].Text[0] != '\0')
		{
			ClearTalkEdit(); //清 文本框
		}
		break;
	case 11: //#  l
		isOK = 0;
		if (r2r_edit[0].BoxLen == 0)
		{
			DisplayOpenLockWindow(); //显示密码开锁窗口（一级）
			break;
		}
		r2r_edit[0].Text[r2r_edit[0].BoxLen] = '\0';
		if ((r2r_edit[0].BoxLen == 6)
				&& (strcmp(r2r_edit[0].Text, "999999") == 0))
		{
			DisplaySetupWindow(); //显示设置窗口（一级）
			break;
		}

//		if ((r2r_edit[0].BoxLen == 1) && (r2r_edit[0].Text[0] == '0'))
//		{
//			isOK = 1;
//		}
		if ((LocalCfg.Addr[0] == 'M')
				&& ((r2r_edit[0].BoxLen == 3) || (r2r_edit[0].BoxLen == 4)))
		{
			isOK = 1;
			if (r2r_edit[0].BoxLen == 3)
			{
				tmp_text[0] = '0';
				tmp_text[1] = r2r_edit[0].Text[0];
				tmp_text[2] = r2r_edit[0].Text[1];
				tmp_text[3] = r2r_edit[0].Text[2];
				tmp_text[4] = '\0';
				strcpy(r2r_edit[0].Text, tmp_text);
				r2r_edit[0].BoxLen = 4;

				DisplayEdit(&r2r_edit[0], 0);
#if 0
				outxy24(r2r_edit[0].xLeft + r2r_edit[0].CursorX,
						r2r_edit[0].yTop + r2r_edit[0].CursorY, 4, cBlack, 1, 1, r2r_edit[0].Text, 0, 0);
#else
				outxy_pic_num(r2r_edit[0].xLeft + r2r_edit[0].CursorX,
						r2r_edit[0].yTop + r2r_edit[0].CursorY,
						r2r_edit[0].Text, 0);
#endif
			}
			strncpy(acCurRoom, r2r_edit[0].Text, 5);
			acCurRoom[4] = '\0';
		}

		if ((LocalCfg.Addr[0] == 'W')
				&& ((r2r_edit[0].BoxLen == 4) || (r2r_edit[0].BoxLen == 10)))
		{
			isOK = 1;
		}

		if (isOK == 1)
		{
			if ((strcmp(r2r_edit[0].Text, "0000") == 0) || (strcmp(r2r_edit[0].Text, "0") == 0))
			{
				Call_Func(1, r2r_edit[0].Text, "Z00010000000"); //呼叫   1  中心  2 住户
			}
			else
			{
				Call_Func(2, r2r_edit[0].Text, ""); //呼叫   1  中心  2 住户
				//sleep(1);

				// 初始化sip呼叫的为被动状态
				Local.cSipCallPassive = 1;
				if(Local.cRegFlg == 1)
				{
					Local.cSipCallPassive = 0;
					if(r2r_edit[0].BoxLen == 4)
					{
						printf("***************r2r_edit[0].Text= %s\n", r2r_edit[0].Text);
						InvokeLpCall(r2r_edit[0].Text); // 梯口机呼叫室内机
					}
					else if(r2r_edit[0].BoxLen == 10)
					{
						InvokeLpCall(r2r_edit[0].Text + 6); // 区口机呼叫室内机
					}
					else
					{
						printf("*****OperateTalkWindow()***call num len is error***\n");
					}

					//printf("***************StartPlayWav(cartoon.wav)\n");
					//播放回铃音
					//WaitAudioUnuse(1000);
					//sprintf(wavFile, "%s/cartoon.wav\0", wavPath);
					//StartPlayWav(wavFile, 1);
				}
				else
				{
//					sprintf(wavFile, "%s/callfail.wav\0", wavPath);
//					StartPlayWav(wavFile, 0);
//					DisplayMainWindow(0);
				}


			}
		}
		else
		{
			printf("%s, %d\n", r2r_edit[0].Text, r2r_edit[0].BoxLen);
			WaitAudioUnuse(2000); //等待声卡空闲
			sprintf(wavFile, "%s/inputerror.wav\0", wavPath);
			StartPlayWav(wavFile, 0);
			DisplayMainWindow(0);
		}
		break;
	}
}
#endif
//---------------------------------------------------------------------------
void ClearTalkEdit(void) //清 文本框
{
	if (Local.CurrentWindow == 1)
	{
		r2r_edit[0].BoxLen = 0;
		r2r_edit[0].Text[r2r_edit[0].BoxLen] = '\0';
		DisplayEdit(&r2r_edit[0], 0);
	}
	if (Local.CurrentWindow == 5)
	{
		openlock_edit.BoxLen = 0;
		openlock_edit.Text[openlock_edit.BoxLen] = '\0';
		DisplayEdit(&openlock_edit, 0);
	}
}
//---------------------------------------------------------------------------
void DisplayTalkConnectWindow(void) //显示对讲 正在连接窗口
{
	int i;

	while (Local.isDisplayWindowFlag == 1) //窗口正在显示中
		usleep(10);
	if (Local.CurrentWindow == 2)
		return;
	Local.isDisplayWindowFlag = 1;

	Local.PrevWindow = Local.CurrentWindow;
	Local.CurrentWindow = 2;
	CheckMainFbPage(); //检查是否在framebuffer 的第0页

	DisplayImage(&talk_connect_image, 0);
	
#ifdef YK_XT8126_BV_FKT
	outxy_pic_num(80,260,acCurRoom, 0);
#else
	outxy_pic_num(310,165,acCurRoom, 0);
#endif

	Local.isDisplayWindowFlag = 0; //窗口正在显示中
}

//---------------------------------------------------------------------------
void OperateTalkOverWindow(short wintype, int currwindow)
{
	switch (wintype)
	{
	case 10:
		printf("***OperateTalkOverWindow()***wintype = %d, Local.Status = %d\n",
				wintype, Local.Status);
		Local.CurrentWindow = 0;
		DisplayMainWindow(0);
		break;
	}
}

void DisplayTalkOverWindow(void)
{
	while (Local.isDisplayWindowFlag == 1) //窗口正在显示中
		usleep(10);

	Local.isDisplayWindowFlag = 1;
	Local.PrevWindow = Local.CurrentWindow;
	Local.CurrentWindow = 7;

	DisplayImage(&talkover_image, 0);
	Local.isDisplayWindowFlag = 0; //窗口正在显示中
}

//---------------------------------------------------------------------------
void OperateTalkConnectWindow(short wintype, int currwindow) //对讲 正在连接窗口操作
{
	int i, j;

	switch (wintype)
	{
	case 10: //清除
		printf("wintype = %d, Local.Status = %d\n", wintype, Local.Status);
		if (Local.Status != 0)
		{

			if ((Local.Status == 1) || (Local.Status == 2)
					|| (Local.Status == 5) || (Local.Status == 6)
					|| (Local.Status == 7) || (Local.Status == 8)
					|| (Local.Status == 9) || (Local.Status == 10)) //状态为对讲
				//*************************add by xuqd***************
				printf("OperateTalkConnectWindow,press '*' to stop sip call or indoor call*************\n");

			//（1）在连接过程中，如果按下*，此时sip很快就回复error，从而使g_sipCallID=0，没有
			//调用TerminateLpCall()，而只是调用TalkEnd_Func()，就可能会出现界面无法恢复
			//的情况，因为在没有室内机的情况下，TalkEnd_Func()是没有作用的，界面就可能一直停在
			//连接状态，只能等到30秒后才能恢复。
			//（2）在连接过程中，如果按下*，此时很快出现NS解析失败，则g_callIDUFlag=0，但可能
			//g_sipCallID！=0，此时还需要关闭sip
			if (INVALID_SIP_CALL_ID != g_sipCallID)
			{
				TerminateLpCall(g_sipCallID);

				if(g_callIDUFlag != 0)
				{
					TalkEnd_Func();
				}
			}
			else
			{
				if(g_callIDUFlag != 0)
				{
					TalkEnd_Func();
				}
				else
				{
					StopPlayWavFile(); //关闭回铃音
					WaitAudioUnuse(1000); //等待声卡空闲    //延时是为了等待声音播放完成，否则会有杂音
					DisplayTalkOverWindow();
					Local.Status = 0; //状态恢复为空闲状态
					sleep(1);
					DisplayMainWindow(0);
				}
			}


			//**************************end**********************
		}
		else
		{
			g_callIDUFlag = 0;

			if (INVALID_SIP_CALL_ID != g_sipCallID)
			{
				g_softSipClearFlag = 1;
				TerminateLpCall(g_sipCallID);
			}
			else
			{
				DisplayTalkOverWindow();
				Local.Status = 0; //状态恢复为空闲状态
				sleep(1);
				DisplayMainWindow(0);
			}
		}

		//正在链接增加关闭回铃音 2012 04 13
		StopPlayWavFile(); //关闭回铃音
		WaitAudioUnuse(1000); //等待声卡空闲    //延时是为了等待声音播放完成，否则会有杂音

		break;
	}
}
//---------------------------------------------------------------------------

void DisplayTalkStartWindow(void) //显示对讲 通话窗口
{
	int i;

	while (Local.isDisplayWindowFlag == 1) //窗口正在显示中
		usleep(10);
	if (Local.CurrentWindow == 3)
		return;
	Local.isDisplayWindowFlag = 1;

	Local.PrevWindow = Local.CurrentWindow;
	Local.CurrentWindow = 3;
	CheckMainFbPage(); //检查是否在framebuffer 的第0页

	DisplayImage(&talk_start_image, 0);
	
#ifdef YK_XT8126_BV_FKT
	outxy_pic_num(80,260,acCurRoom, 0);
#else
	outxy_pic_num(72,178,acCurRoom, 0);
#endif

	//可视对讲 通话背景 开锁Label

	if (label_talk_start.image == NULL)
	{
		label_talk_start.width = 130;
		label_talk_start.height = 24;
		label_talk_start.xLeft = 296;
		label_talk_start.yTop = 338;

		label_talk_start.image = (unsigned char *) malloc(
				label_talk_start.width * label_talk_start.height * 3 / 2); //保存一块屏幕缓冲
		SavePicBuf_Func(label_talk_start.xLeft, label_talk_start.yTop,
				label_talk_start.width, label_talk_start.height,
				label_talk_start.image, 0);
	}

	Local.isDisplayWindowFlag = 0; //窗口正在显示中
}
//---------------------------------------------------------------------------
void OperateTalkStartWindow(short wintype, int currwindow) //对讲 通话窗口操作
{
	int i, j;

	switch (wintype)
	{
	case 10: //清除
		printf(
				"*****press the '*' in the operateTalkStartWindow,wintype = %d, Local.Status = %d\n",
				wintype, Local.Status);
		if (Local.Status != 0)
		{

			if ((Local.Status == 1) || (Local.Status == 2)
					|| (Local.Status == 5) || (Local.Status == 6)
					|| (Local.Status == 7) || (Local.Status == 8)
					|| (Local.Status == 9) || (Local.Status == 10)) //状态为对讲
			//*************************add by xuqd***************
			{
				printf("Local.Status = %d,stop idu and sip**************\n", Local.Status);

				if (INVALID_SIP_CALL_ID != g_sipCallID)
				{
					printf("****OperateTalkStartWindow,press '*' to stop sip call***g_sipCallID = %d********\n", g_sipCallID);
					g_callIDUFlag = 0;
					g_softSipClearFlag = 1;
					TerminateLpCall(g_sipCallID);
				}

				TalkEnd_Func();
			}
			//**************************end**********************
		}
		else
		{
//			g_callIDUFlag = 0;
//			clear_exec_flag = 1;
//			TerminateLpCall(g_sipCallID);
		}



		break;
	}
}

//-----------------------------------add by xuqd-------------------------------
//void call_sip_func(char *call_addr)
//{
//	const char *tel_num = NULL;
//
//	if (INVALID_SIP_CALL_ID != g_sipCallID)
//	{
//		// 如果存在sip呼叫
//		printf("**************************sip called already so will close sip first\n");
//		TerminateLpCall(g_sipCallID);
//		sleep(3);
//	}
//
//	//g_sipCallID = sip_call_invoke(call_addr);
//	//printf("****call_sip_func()****g_sipCallID is %d*****\n", g_sipCallID);
//
//	sip_call_invoke(call_addr);
//	printf("****call_sip_func() is executed!*********\n");
//	DisplayTalkConnectWindow();
//}

//--------------------------------------end------------------------------------

//---------------------------------------------------------------------------

void Call_Func(int uFlag, char *call_addr, char *CenterAddr) //呼叫   1  中心  2 住户
{
	int i;
	if ((Local.Status == 0) && ((uFlag == 1) || (uFlag == 2)))
	{
		Local.NSReplyFlag = 0; //NS应答处理标志
		//锁定互斥锁
		pthread_mutex_lock(&Local.udp_lock);
		//查找可用发送缓冲并填空
		for (i = 0; i < UDPSENDMAX; i++)
			if (Multi_Udp_Buff[i].isValid == 0)
			{
				Multi_Udp_Buff[i].SendNum = 0;
				Multi_Udp_Buff[i].m_Socket = m_VideoSocket;
				Multi_Udp_Buff[i].RemotePort = RemoteVideoPort;
				Multi_Udp_Buff[i].CurrOrder = VIDEOTALK;
				//   sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0",LocalCfg.IP_Broadcast[0],
				//           LocalCfg.IP_Broadcast[1],LocalCfg.IP_Broadcast[2],LocalCfg.IP_Broadcast[3]);

				strcpy(Multi_Udp_Buff[i].RemoteHost, NSMULTIADDR);
				//头部
				memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
				//命令  ,子网广播解析
				Multi_Udp_Buff[i].buf[6] = NSORDER;
				Multi_Udp_Buff[i].buf[7] = ASK; //主叫

				memcpy(Multi_Udp_Buff[i].buf + 8, LocalCfg.Addr, 20);
				memcpy(Multi_Udp_Buff[i].buf + 28, LocalCfg.IP, 4);
				memcpy(Remote.Addr[0], NullAddr, 20);
				switch (uFlag)
				{
				case 1: //呼叫中心
					memcpy(Remote.Addr[0], CenterAddr, 12);
					break;
				case 2: //呼叫住户
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
						if (strlen(call_addr) == 4) //别墅
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
				Multi_Udp_Buff[i].SendDelayTime = 0; //发送等待时间计数  20101112
				Multi_Udp_Buff[i].isValid = 1;
#ifdef _DEBUG
				printf("正在解析地址\n");
#endif
				sem_post(&multi_send_sem);
				break;
			}

		//**********add by xuqd************
//#ifdef _CODEC_MPEG4_
		g_callIDUFlag = 1;
//#endif
		g_softSipClearFlag = 0;
		//*************end*****************

		//打开互斥锁
		pthread_mutex_unlock(&Local.udp_lock);
	}
	else
	{
#ifdef _DEBUG
		printf("本机正忙,无法呼叫\n");
#endif
	}
}
//---------------------------------------------------------------------------
void DisplayOpenLockWindow(void) //显示密码开锁窗口（一级）
{
	int i;

	while (Local.isDisplayWindowFlag == 1) //窗口正在显示中
		usleep(10);
	if (Local.CurrentWindow == 5)
		return;
	Local.isDisplayWindowFlag = 1;
	Local.PrevWindow = Local.CurrentWindow;
	Local.CurrentWindow = 5;
	CheckMainFbPage(); //检查是否在framebuffer 的第0页

	DisplayImage(&openlock_image, 0);

	openlock_edit.BoxLen = 0;
	openlock_edit.MaxLen = 6;
	openlock_edit.Text[0] = '\0';
	CurrBox = 0;

	DisplayEdit(&openlock_edit, 0);

	//20110523 xu
	//WaitAudioUnuse(200); //等待声卡空闲
	//sprintf(wavFile, "%s/ring1.wav\0", wavPath);
	//StartPlayWav(wavFile, 1);

	Local.isDisplayWindowFlag = 0; //窗口正在显示中
}

//add by chensq
void DisplaySipStatusWindow(void)
{
//	printf("********************DisplaySipStatusWindow sip status:%d\n",  Local.cRegFlg);
//	printf("********************DisplaySipStatusWindow CurrentWindow:%d\n",  Local.CurrentWindow);
	if(Local.CurrentWindow == 0 || Local.CurrentWindow == 1)
	{
		while (Local.isDisplayWindowFlag == 1)
			usleep(10);
		Local.isDisplayWindowFlag = 1;

//		if (Local.cRegFlg == 0)
		if (LPGetSipRegStatus() == REG_STATUS_OFFLINE)
		{
			Local.cRegFlg = 0;
			sip_error.xLeft = 630;
			sip_error.yTop = 400;
			DisplayImage(&sip_error, 0);
		}
//		else if (Local.cRegFlg == 1)
		else if (LPGetSipRegStatus() == REG_STATUS_ONLINE)
		{
			Local.cRegFlg = 1;
			sip_ok.xLeft = 630;
			sip_ok.yTop = 400;
			DisplayImage(&sip_ok, 0);
		}
		Local.isDisplayWindowFlag = 0;
	}
}

//---------------------------------------------------------------------------
void OperateOpenLockWindow(short wintype, int currwindow) //密码开锁窗口操作（一级）
{
	int i, j;


	char str[3];

	printf("wintype = %d\n", wintype);
	CurrBox = 0;

	switch (wintype)
	{
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
		if (openlock_edit.BoxLen < openlock_edit.MaxLen)
		{
			str[0] = '0' + wintype;
			str[1] = '\0';
			strcat(openlock_edit.Text, str);
			str[0] = '*';
			str[1] = '\0';
#if 0
			outxy24(openlock_editxLeft + openlock_edit.CursorX + openlock_edit.BoxLen*12,
					openlock_edit.yTop + openlock_edit.CursorY, 2, cBlack, 1, 1, str, 0, 0);
#else
			outxy_pic_num(
					openlock_edit.xLeft + openlock_edit.CursorX
							+ openlock_edit.BoxLen * num_yk[0].width,
					openlock_edit.yTop + openlock_edit.CursorY, str, 0);
#endif
			openlock_edit.BoxLen++;
		}
		break;
	case 10: //清除
		if (openlock_edit.BoxLen != 0)
		{
			openlock_edit.Text[0] = 0;
			openlock_edit.BoxLen = 0;
			DisplayEdit(&openlock_edit, 0);
		}
		else
		{
			//20110523 xu
			//StopPlayWavFile();  //关闭铃音
			//WaitAudioUnuse(200); //等待声卡空闲    //延时是为了等待声音播放完成，否则会有杂音
			printf("***OperateOpenLockWindow()******DisplayMainWindow(0)***\n");
			DisplayMainWindow(0);
		}
		break;
	case 11: //开锁
		printf("LocalCfg.OpenLockPass = %s, Text = %s\n", LocalCfg.OpenLockPass,
				openlock_edit.Text);
		openlock_edit.Text[openlock_edit.BoxLen] = '\0';
		printf("LocalCfg.OpenLockPass = %s, Text = %s\n", LocalCfg.OpenLockPass,
				openlock_edit.Text);

		//modified by lcx
		//if (strcmp(openlock_edit.Text, LocalCfg.OpenLockPass) == 0)
		if(1 == TMXTIsValidDoorPassword(openlock_edit.Text))
		{
			Local.OpenDoorFlag = 2; //开锁标志 0 未开锁  1 开锁延时中  2 开锁中
			Local.OpenDoorTime = 0; //时间
			//打开门锁
			OpenLock_Func(); //开锁函数

			WaitAudioUnuse(2000); //等待声卡空闲
			sprintf(wavFile, "%s/dooropen.wav\0", wavPath);
			StartPlayWav(wavFile, 0);

			door_openlock.xLeft = 258;
			door_openlock.yTop = 300;
			DisplayImage(&door_openlock, 0);
		}
		else
		{
			WaitAudioUnuse(2000); //等待声卡空闲
			sprintf(wavFile, "%s/passerror.wav\0", wavPath);
			StartPlayWav(wavFile, 0);
		}
		ClearTalkEdit(); //清 文本框
		break;
	}
}
//---------------------------------------------------------------------------
void DisplayWatchWindow(void) //监视窗口操作（二级）
{
	int i;

	while (Local.isDisplayWindowFlag == 1) //窗口正在显示中
		usleep(10);
	if (Local.CurrentWindow == 4)
		return;
	Local.isDisplayWindowFlag = 1;

	Local.PrevWindow = Local.CurrentWindow;
	Local.CurrentWindow = 4;
	CheckMainFbPage(); //检查是否在framebuffer 的第0页

	DisplayImage(&watch_image, 0);

	Local.isDisplayWindowFlag = 0; //窗口正在显示中
}
//---------------------------------------------------------------------------
void OperateWatchWindow(short wintype, int currwindow) //监视窗口操作（二级）
{
	int i, j;

	switch (wintype)
	{
	case 10: //清除
		printf("wintype = %d, Local.Status = %d\n", wintype, Local.Status);
		if (Local.Status != 0)
		{
			if ((Local.Status == 3) || (Local.Status == 4))
				WatchEnd_Func();
			PicStatBuf.Type = Local.CurrentWindow;
			PicStatBuf.Time = 0;
			PicStatBuf.Flag = 1;
		}
		break;
	}
}
//---------------------------------------------------------------------------
void Talk_Func(void) //通话 接听
{
	int i;
	if (Local.Status == 2) //状态为被对讲
	{
		StopPlayWavFile(); //关闭铃音
		WaitAudioUnuse(1000);
		for (i = 0; i < UDPSENDMAX; i++)
			if (Multi_Udp_Buff[i].isValid == 0)
			{
				//锁定互斥锁
				pthread_mutex_lock(&Local.udp_lock);
				Multi_Udp_Buff[i].SendNum = 0;
				Multi_Udp_Buff[i].m_Socket = m_VideoSocket;
				Multi_Udp_Buff[i].RemotePort = RemoteVideoPort;
				sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0",
						Remote.DenIP[0], Remote.DenIP[1], Remote.DenIP[2],
						Remote.DenIP[3]);
				//头部
				memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
				//命令  ,子网广播解析
				if (Remote.isDirect == 1)
					Multi_Udp_Buff[i].buf[6] = VIDEOTALKTRANS;
				else
					Multi_Udp_Buff[i].buf[6] = VIDEOTALK;
				Multi_Udp_Buff[i].CurrOrder = Multi_Udp_Buff[i].buf[6];
				Multi_Udp_Buff[i].buf[7] = ASK; //主叫
				Multi_Udp_Buff[i].buf[8] = CALLSTART;

				//本机为主叫方
				if ((Local.Status == 1) || (Local.Status == 3)
						|| (Local.Status == 5) || (Local.Status == 7)
						|| (Local.Status == 9))
				{
					memcpy(Multi_Udp_Buff[i].buf + 9, LocalCfg.Addr, 20);
					memcpy(Multi_Udp_Buff[i].buf + 29, LocalCfg.IP, 4);
					memcpy(Multi_Udp_Buff[i].buf + 33, Remote.Addr[0], 20);
					memcpy(Multi_Udp_Buff[i].buf + 53, Remote.IP[0], 4);
				}
				//本机为被叫方
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
				Multi_Udp_Buff[i].SendDelayTime = 0; //发送等待时间计数  20101112
				Multi_Udp_Buff[i].isValid = 1;

//                Local.Status = 0;  //状态为空闲
				//打开互斥锁
				pthread_mutex_unlock(&Local.udp_lock);
				sem_post(&multi_send_sem);
				break;
			}
	}
}
//---------------------------------------------------------------------------
void TalkEnd_Func(void)
{
	int i, j;
//  StopPlayWavFile();  //关闭回铃音
	sprintf(Local.DebugInfo, "****TalkEnd_Func()***Local.Status = %d****\n",
			Local.Status);
	P_Debug(Local.DebugInfo);
	if ((Local.Status == 1) || (Local.Status == 2) || (Local.Status == 5)
			|| (Local.Status == 6) || (Local.Status == 7) || (Local.Status == 8)
			|| (Local.Status == 9) || (Local.Status == 10)) //状态为对讲
	{
		sprintf(Local.DebugInfo, "Remote.DenNum=%d\n", Remote.DenNum);
		P_Debug(Local.DebugInfo);
		//锁定互斥锁
		pthread_mutex_lock(&Local.udp_lock);
		for (j = 0; j < Remote.DenNum; j++)
		{
			for (i = 0; i < UDPSENDMAX; i++)
				if (Multi_Udp_Buff[i].isValid == 0)
				{
					//头部
					memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
					//命令  ,子网广播解析
					Multi_Udp_Buff[i].buf[6] = VIDEOTALK;
					Multi_Udp_Buff[i].buf[7] = ASK; //主叫
					Multi_Udp_Buff[i].buf[8] = CALLEND; //CALLEND
					Multi_Udp_Buff[i].SendNum = 0;
					Multi_Udp_Buff[i].m_Socket = m_VideoSocket;
					Multi_Udp_Buff[i].RemotePort = RemoteVideoPort;
					Multi_Udp_Buff[i].CurrOrder = VIDEOTALK;
					sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0",
							Remote.IP[j][0], Remote.IP[j][1], Remote.IP[j][2],
							Remote.IP[j][3]);

					sprintf(Local.DebugInfo, "%d.%d.%d.%d\n", Remote.IP[j][0],
							Remote.IP[j][1], Remote.IP[j][2], Remote.IP[j][3]);
					P_Debug(Local.DebugInfo);

					//本机为主叫方
					if ((Local.Status == 1) || (Local.Status == 3)
							|| (Local.Status == 5) || (Local.Status == 7)
							|| (Local.Status == 9))
					{
						memcpy(Multi_Udp_Buff[i].buf + 9, LocalCfg.Addr, 20);
						memcpy(Multi_Udp_Buff[i].buf + 29, LocalCfg.IP, 4);
						memcpy(Multi_Udp_Buff[i].buf + 33, Remote.Addr[j], 20);
						memcpy(Multi_Udp_Buff[i].buf + 53, Remote.IP[j], 4);
					}
					//本机为被叫方
					if ((Local.Status == 2) || (Local.Status == 4)
							|| (Local.Status == 6) || (Local.Status == 8)
							|| (Local.Status == 10))
					{
						memcpy(Multi_Udp_Buff[i].buf + 9, Remote.Addr[j], 20);
						memcpy(Multi_Udp_Buff[i].buf + 29, Remote.IP[j], 4);
						memcpy(Multi_Udp_Buff[i].buf + 33, LocalCfg.Addr, 20);
						memcpy(Multi_Udp_Buff[i].buf + 53, LocalCfg.IP, 4);
					}

					Multi_Udp_Buff[i].nlength = 57;
					Multi_Udp_Buff[i].DelayTime = DIRECTCALLTIME;
					Multi_Udp_Buff[i].SendDelayTime = 0; //发送等待时间计数  20101112
					Multi_Udp_Buff[i].isValid = 1;
					sem_post(&multi_send_sem);
//                Local.Status = 0;  //状态为空闲

					break;
				}
		}
		//打开互斥锁
		pthread_mutex_unlock(&Local.udp_lock);

	}
	//**************add by xuqd**************
	g_callIDUFlag = 0;
	g_softSipClearFlag = 1;
	//******************end******************
}
//---------------------------------------------------------------------------
void WatchEnd_Func(void)
{
	int i;
	if ((Local.Status == 3) || (Local.Status == 4)) //状态为监视
	{
		//锁定互斥锁
		pthread_mutex_lock(&Local.udp_lock);
		for (i = 0; i < UDPSENDMAX; i++)
			if (Multi_Udp_Buff[i].isValid == 0)
			{
				Multi_Udp_Buff[i].SendNum = 0;
				Multi_Udp_Buff[i].m_Socket = m_VideoSocket;
				Multi_Udp_Buff[i].RemotePort = RemoteVideoPort;
				Multi_Udp_Buff[i].CurrOrder = VIDEOWATCH;
				sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0",
						Remote.DenIP[0], Remote.DenIP[1], Remote.DenIP[2],
						Remote.DenIP[3]);
				//头部
				memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
				//命令  ,子网广播解析
				if (Remote.isDirect == 1)
					Multi_Udp_Buff[i].buf[6] = VIDEOWATCHTRANS;
				else
					Multi_Udp_Buff[i].buf[6] = VIDEOWATCH;
				Multi_Udp_Buff[i].buf[7] = ASK; //主叫
				Multi_Udp_Buff[i].buf[8] = CALLEND; //CALLEND

				//本机为主叫方
				if ((Local.Status == 1) || (Local.Status == 3)
						|| (Local.Status == 5) || (Local.Status == 7)
						|| (Local.Status == 9))
				{
					memcpy(Multi_Udp_Buff[i].buf + 9, LocalCfg.Addr, 20);
					memcpy(Multi_Udp_Buff[i].buf + 29, LocalCfg.IP, 4);
					memcpy(Multi_Udp_Buff[i].buf + 33, Remote.Addr[0], 20);
					memcpy(Multi_Udp_Buff[i].buf + 53, Remote.IP[0], 4);
				}
				//本机为被叫方
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
				Multi_Udp_Buff[i].SendDelayTime = 0; //发送等待时间计数  20101112
				Multi_Udp_Buff[i].isValid = 1;

//                Local.Status = 0;  //状态为空闲
				sem_post(&multi_send_sem);
				break;
			}
		//打开互斥锁
		pthread_mutex_unlock(&Local.udp_lock);
		StopRecVideo();
	}
}
//---------------------------------------------------------------------------
//呼叫超时
void CallTimeOut_Func(void)
{
	int i;
	int isHost;
	//如本机为被叫方
	//查看SD是否存在和留影标志 和 主叫方地址为围墙机和门口机和别墅门口机
	if ((Local.SD_Status == 1) && (Local.Status == 2)
			&& ((Remote.Addr[0][0] == 'W') || (Remote.Addr[0][0] == 'M')
					|| (Remote.Addr[0][0] == 'H')))
	{
		isHost = 0;
		if (LocalCfg.Addr[0] == 'S')
			if (LocalCfg.Addr[11] == '0')
				isHost = 1;
		if (LocalCfg.Addr[0] == 'B')
			if (LocalCfg.Addr[5] == '0')
				isHost = 1;
		if (isHost == 1)
		{
			//发送开始留影预备命令
			for (i = 0; i < UDPSENDMAX; i++)
				if (Multi_Udp_Buff[i].isValid == 0)
				{
					//锁定互斥锁
					pthread_mutex_lock(&Local.udp_lock);
					Multi_Udp_Buff[i].SendNum = 0;
					Multi_Udp_Buff[i].m_Socket = m_VideoSocket;
					Multi_Udp_Buff[i].RemotePort = RemoteVideoPort;

					sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0",
							Remote.DenIP[0], Remote.DenIP[1], Remote.DenIP[2],
							Remote.DenIP[3]);
					//头部
					memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
					//命令
					if (Remote.isDirect == 1)
						Multi_Udp_Buff[i].buf[6] = VIDEOTALKTRANS;
					else
						Multi_Udp_Buff[i].buf[6] = VIDEOTALK;
					Multi_Udp_Buff[i].buf[7] = ASK; //主叫
					Multi_Udp_Buff[i].buf[8] = PREPARERECORD;

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
		else
		{
			if ((Local.Status == 1) || (Local.Status == 2)
					|| (Local.Status == 5) || (Local.Status == 6)
					|| (Local.Status == 7) || (Local.Status == 8)
					|| (Local.Status == 9) || (Local.Status == 10))

				TalkEnd_Func();

			if (INVALID_SIP_CALL_ID != g_sipCallID)
			{
				// 如果存在sip呼叫，就关闭sip呼叫
				//TerminateLpCall(g_sipCallID);
			}

			Local.OnlineFlag = 0;
#ifdef _DEBUG
			printf("呼叫超时\n");
#endif
		}
	}
	else
	{
		if ((Local.Status == 1) || (Local.Status == 2) || (Local.Status == 5)
				|| (Local.Status == 6) || (Local.Status == 7)
				|| (Local.Status == 8) || (Local.Status == 9)
				|| (Local.Status == 10))

			printf("*******CallTimeOut_Func()******g_sipCallID is %d****\n",
					g_sipCallID);

		TalkEnd_Func();

		if (INVALID_SIP_CALL_ID != g_sipCallID)
		{
			// 如果存在sip呼叫
			while (g_callIDUFlag == 1)
			{
				// 休眠20ms
				usleep(20000);
			}

			TerminateLpCall(g_sipCallID);
		}

		// 等待sip那边先挂断
		//while(INVALID_SIP_CALL_ID != g_sipCallID);

		Local.OnlineFlag = 0;

#ifdef _DEBUG
		printf("呼叫超时\n");
#endif
	}
}

void xintel_sth_open_door(const char* type)
{

	printf("LocalCfg.OpenLockPass = %s, Text = %s\n", LocalCfg.OpenLockPass,
			openlock_edit.Text);
	openlock_edit.Text[openlock_edit.BoxLen] = '\0';
	printf("LocalCfg.OpenLockPass = %s, Text = %s\n", LocalCfg.OpenLockPass,
			openlock_edit.Text);
	// if(strcmp(openlock_edit.Text, LocalCfg.OpenLockPass) == 0)
	{
		Local.OpenDoorFlag = 2; //开锁标志 0 未开锁  1 开锁延时中  2 开锁中
		Local.OpenDoorTime = 0; //时间
		//打开门锁
		OpenLock_Func(); //开锁函数

		WaitAudioUnuse(1000); //等待声卡空闲
		sprintf(wavFile, "%s/dooropen.wav\0", wavPath);
		StartPlayWav(wavFile, 0);

		door_openlock.xLeft = 258;
		door_openlock.yTop = 300;
		DisplayImage(&door_openlock, 0);
	}
	/* else
	 {
	 WaitAudioUnuse(1000); //等待声卡空闲
	 sprintf(wavFile, "%s/passerror.wav\0", wavPath);
	 StartPlayWav(wavFile, 0);
	 }*/
	ClearTalkEdit(); //清 文本框
}

//---------------------------------------------------------------------------
