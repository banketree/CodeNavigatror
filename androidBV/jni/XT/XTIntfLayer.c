#include <IDBT.h>
#include <YKSystem.h>
#include <XTIntfLayer.h>
#include <SMEventHndl.h>
#include <common.h>

CallID g_sipCallID 		= INVALID_SIP_CALL_ID;
int g_callIDUFlag 		= FALSE;
int g_softSipClearFlag 	= FALSE;
int SipRecAudioOpen 	= 0;
int SipPlayAudioOpen 	= 0;
int SndRecAudioOpen 	= 0;
int SndPlayAudioOpen 	= 0;
YK_MSG_QUE_ST*  g_pstXTIntfMsgQ = NULL;          //XT接口消息队列指针

extern int g_ossPlayFlag;
extern pthread_mutex_t audio_lock;

int TerminateXTCall(void)
{
	printf("*********************g_callIDUFlag = %d\n", g_callIDUFlag);
	if(g_callIDUFlag == 0)
	{
		StopPlayWavFile(); //关闭回铃音
		WaitAudioUnuse(1000); //等待声卡空闲    //延时是为了等待声音播放完成，否则会有杂音
		if(Local.cSipCallPassive == 0)//监控不显示
		{
			////printf("i am here!!!!!!  Local.Status = %d \n", Local.Status);
			if((Local.Status != 0) && (Local.Status != 1))
			{
				DisplayTalkOverWindow();
			}
		}
		Local.Status = 0; //状态恢复为空闲状态l
		sleep(1);
		if(Local.cSipCallPassive == 0)//监控不显示
		{
			DisplayMainWindow(0);
		}
		//soft_sip_clear_flag = 0;
	}
}

int TerminateLpCall(CallID id)
{
	int iErrCode;

	XTSM_TERMINATECALL_DATA_ST *pstXTSMTerminateCallData = YKNew0(XTSM_TERMINATECALL_DATA_ST, 1);

	if(pstXTSMTerminateCallData == NULL)
	{
		return FALSE;
	}

	pstXTSMTerminateCallData->uiPrmvType = XTSM_TERMINATE_LP_CALL;

	pstXTSMTerminateCallData->uiCallID = id;

	iErrCode = YKWriteQue( g_pstSMMsgQ , pstXTSMTerminateCallData,  0);
	if(iErrCode == 0)
	{
		g_sipCallID = INVALID_SIP_CALL_ID;
		TerminateXTCall();
		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}

	return SUCCESS;
}

int InvokeLpCall(const char *pcCallAddr)
{
	int iErrCode;

	XTSM_INVOKECALL_DATA_ST *pstXTSMInvokeCallData = YKNew0(XTSM_INVOKECALL_DATA_ST, 1);

	if(pstXTSMInvokeCallData == NULL)
	{
		return FALSE;
	}

	pstXTSMInvokeCallData->uiPrmvType = XTSM_INVOKE_LP_CALL;

	strcpy(pstXTSMInvokeCallData->aucCallAddr, pcCallAddr);

	iErrCode = YKWriteQue( g_pstSMMsgQ , pstXTSMInvokeCallData,  0);


	if(iErrCode == 0)
	{
		printf("**********************InvokeLpCall\n");
		g_sipCallID = VALID_SIP_CALL_ID;

		// 起呼的时候默认关闭oss开关
		g_ossPlayFlag = 0;
		CloseFillLight_Func();
		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}
}

int PickupIDU(void)
{
	int iErrCode;

	XTSM_PICKUPIDU_ST *pstXTSMPickupIDU = YKNew0(XTSM_PICKUPIDU_ST, 1);

	if(pstXTSMPickupIDU == NULL)
	{
		return FALSE;
	}

	pstXTSMPickupIDU->uiPrmvType = XTSM_PICKUP_IDU;

	iErrCode = YKWriteQue( g_pstSMMsgQ , pstXTSMPickupIDU,  0);

	if(iErrCode == 0)
	{
		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}

}


static void SMXTCalleeCallError(unsigned int uiErrorCause)
{
	usleep(1000*1000);	//等待g_callIDUFlag置0
	if (g_callIDUFlag == 0)
	{
		// 设立此标志的目的是当同时支持室内机和sip终端的时候
		// 如果sip拨号出现错误，如果执行以下代码，则马上会使界面恢复到主界面
		// 而如果没有室内机的情况下，当sip拨号出现错误，执行以下代码可以恢复到主界面
		////printf("i am here00000!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		sleep(2);
		if(Local.Status != 0)
		{
		//sleep(2);
		Local.Status = 0; //状态恢复为空闲状态
		//WaitAudioUnuse(2000);
		StopPlayWavFile();
		WaitAudioUnuse(2000);
		//DisplayTalkOverWindow();
		//sleep(1);

		//WaitAudioUnuse(2000);

		if(Local.CurrentWindow != 0)
		{
			if(uiErrorCause == CALLEE_ERROR_PHONENUM_NOT_EXIST)
			{
				sprintf(wavFile, "%s/room_inexistence.wav\0", wavPath);
				StartPlayWav(wavFile, 0);
			}
			else
			{
				sprintf(wavFile, "%s/callfail.wav\0", wavPath);
				StartPlayWav(wavFile, 0);
			}
		}
		WaitAudioUnuse(2000);
		sleep(1);
//		WaitAudioUnuse(2000);
//		StopPlayWavFile();
		DisplayMainWindow(0);
		}
	}

	g_sipCallID = INVALID_SIP_CALL_ID;

}

static void SMXTCalleePickup(void)
{

	//IDBT_DEBUG("----------SM SM:callee_pick_up[callid :%d]\n", id);
	//WriteDayLog(1, "%02d %s", LOG_EVT_USERS_PHONE_PICK_UP, USERS_PHONE_PICK_UP);
	pthread_mutex_lock(&audio_lock);
	printf("--------------------SMXTCalleePickup[cSipCallPassive=%d,g_ossPlayFlag=>1,call_idu_flag=%d]\n",Local.cSipCallPassive, g_callIDUFlag);
	//2012 04 13
	StopPlayWavFile(); //关闭回铃音
	WaitAudioUnuse(2000); //等待声卡空闲    //延时是为了等待声音播放完成，否则会有杂音

	pthread_mutex_unlock(&audio_lock);

	g_softSipClearFlag = 1;
	//g_ossPlayFlag = 1; //oss播放使能
	//if (INVALID_SIP_CALL_ID != id)
	//{
		if(Local.cSipCallPassive == 0)//0主动 1被动
		{
			//主动
			g_ossPlayFlag = 1; //oss播放使能
			printf("--------------------SM callee_pick_up[cSipCallPassive=0,g_ossPlayFlag=>1,call_idu_flag=%d]\n", g_callIDUFlag);

			// 此id是有效的，则挂断室内机
			// 关闭梯口机与室内机之间的通话
			if (g_callIDUFlag == 1)
			{
				TalkEnd_Func();
			}

			printf("--------------------SM callee_pick_up[wait:call_idu_flag == 1]\n");
			while (g_callIDUFlag == 1)
			{
				usleep(20000);
			}
//
//			StopPlayWavFile();  //关闭回铃音
//			WaitAudioUnuse(2000); //等待声卡空闲    //延时是为了等待声音播放完成，否则会有杂音
			Local.Status = 5; //状态修改为主叫通话
			Local.TimeOut = 0; //监视超时,  通话超时,  呼叫超时，无人接听
			DisplayTalkStartWindow(); //显示对讲 正在通话的窗口
			printf("--------------------SM callee_pick_up[start talking in sip]\n");
		}
		else
		{
			//被动 视频监控
			printf("--------------------SM callee_pick_up[g_ossPlayFlag => 0]\n");
			g_ossPlayFlag = 0; //oss播放禁能
		}
//	}
//	else
//	{
//		g_ossPlayFlag = 0; //oss播放禁能，表示处于视频监控状态
//		printf("--------------------SM callee_pick_up[g_ossPlayFlag = 0, g_sipId = %d, call_idu_flag = %d]\n",
//						id, g_callIDUFlag);
//	}

}

static void SMXTCalleeHangoff(void)
{
	//2012 04 13
	printf("********************SMXTCalleeHangoff\n");
	StopPlayWavFile(); //关闭回铃音
	WaitAudioUnuse(1000); //等待声卡空闲    //延时是为了等待声音播放完成，否则会有杂音

	if (g_softSipClearFlag == 1)//0表示由室内机清理标志，1表示由软终端清理标志
	{
		if(Local.Status != 0)
		{
			if(Local.cSipCallPassive == 0)
			{
				DisplayTalkOverWindow(); //监控不要显示通话结束界面
			}
			Local.Status = 0; //状态恢复为空闲状态
			sleep(1);

			if(Local.cSipCallPassive == 0)
			{
				DisplayMainWindow(0);
			}
		}
		g_softSipClearFlag = 0;
	}

	g_sipCallID = INVALID_SIP_CALL_ID;
	g_ossPlayFlag = 0;
}

static void SMXTCalleeReject(void)
{
	printf("********************SMXTCalleeReject\n");
	g_ossPlayFlag = 0;
	WaitAudioUnuse(2000); //等待声卡空闲
	sprintf(wavFile, "%s/reject.wav\0", wavPath);
	StartPlayWav(wavFile, 0);

	sleep(3);

	StopPlayWavFile(); //关闭回铃音
	WaitAudioUnuse(1000); //等待声卡空闲    //延时是为了等待声音播放完成，否则会有杂音

	if (INVALID_SIP_CALL_ID != g_sipCallID)
	{
		printf("callee reject, end this talking!\n");
		g_callIDUFlag = 0;
		g_softSipClearFlag = 1;
		TerminateLpCall(g_sipCallID);
	}

	if (g_softSipClearFlag == 1)//0表示由室内机清理标志，1表示由软终端清理标志
	{
		if(Local.Status != 0)
		{
			if(Local.cSipCallPassive == 0)
			{
				DisplayTalkOverWindow(); //监控不要显示通话结束界面
			}
			Local.Status = 0; //状态恢复为空闲状态
			sleep(1);

			if(Local.cSipCallPassive == 0)
			{
				DisplayMainWindow(0);
			}
		}
		g_softSipClearFlag = 0;
	}

	g_sipCallID = INVALID_SIP_CALL_ID;
}

static void xintel_sth_open_door(const char* type)
{

	printf("LocalCfg.OpenLockPass = %s, Text = %s\n", LocalCfg.OpenLockPass,
			openlock_edit.Text);
	openlock_edit.Text[openlock_edit.BoxLen] = '\0';
	printf("LocalCfg.OpenLockPass = %s, Text = %s\n", LocalCfg.OpenLockPass,
			openlock_edit.Text);
	// if(strcmp(openlock_edit.Text, LocalCfg.OpenLockPass) == 0)
	if(Local.cLockFlg == 0)
	{
		Local.OpenDoorFlag = 2; //开锁标志 0 未开锁  1 开锁延时中  2 开锁中
		Local.OpenDoorTime = 0; //时间
		//打开门锁
		OpenLock_Func(); //开锁函数
		if(g_ossPlayFlag == 1)
		{
			g_ossPlayFlag = 0;
		}
		WaitAudioUnuse(2000); //等待声卡空闲
        sprintf(wavFile, "%s/dooropen.wav\0", wavPath);
		StartPlayWav(wavFile, 0);
		g_ossPlayFlag = 1;
		if(Local.CurrentWindow != 0)
		{
//			door_openlock.xLeft = 258;
//			door_openlock.yTop = 300;
			DisplayImage(&door_openlock, 0);
		}
		Local.cLockFlg = 1;
	}
	/* else
	 {
	 WaitAudioUnuse(1000); //等待声卡空闲
	 sprintf(wavFile, "%s/passerror.wav\0", wavPath);
	 StartPlayWav(wavFile, 0);
	 }*/
	ClearTalkEdit(); //清 文本框
}

static void SMXTCalleeDtmf(void)
{
	//if (INVALID_SIP_CALL_ID != id && key == OPEN_LOCK_FLAG)
	//{ //回话ID有效且key为"#"情况下
	  //梯口机开锁
		xintel_sth_open_door("cmd 0x19");
		//IDBT_DEBUG("-----------SM virtual in door open");

//		Local.Status = 0; //状态恢复为空闲状态l
//		sleep(1);
//		DisplayMainWindow(0);
		//WriteDayLog(1, "%s %02d %s", buffRoom10Temp, LOG_EVT_USERS_PHONE_OPEN_DOOR, USERS_PHONE_OPEN_DOOR);
	//}
}

static void SMXTVideoMonitor(void)
{
	g_ossPlayFlag = 0; //oss播放禁能
}

void* XTIntfThread(void *arg)
{
	void *pvMsg = NULL;

	//创建XT接口层消息队列
	g_pstXTIntfMsgQ  = YKCreateQue(XTINTF_MSG_QUE_LEN);
	if(NULL  == g_pstXTIntfMsgQ)
	{
		return NULL;
	}

	while(g_iRunXTTaskFlag == FALSE)
	{
		sleep(1);
	}

	while(g_iRunXTTaskFlag == TRUE)
	{

		YKReadQue(g_pstXTIntfMsgQ, &pvMsg, 0);			//阻塞读取
		if(NULL == pvMsg)
		{
			//printf("**********************i am XTIntfThread!\n");
			continue;
		}
		else
		{
			//printf("***************XTIntfThread\n");
			switch(*(( unsigned int *)pvMsg))
			{
			case SMXT_CALLEE_PICK_UP:
				//printf("***************SMXTCalleePickup()\n");
				SMXTCalleePickup();
				break;
			case SMXT_CALLEE_HANG_OFF:
				SMXTCalleeHangoff();
				CloseFillLight_Func();
				break;
			case SMXT_CALLEE_ERR:
				SMXTCalleeCallError(((SMXT_CALLEE_ERROR_MSG_ST *)pvMsg)->uiErrorCause);
				CloseFillLight_Func();
				break;
			case SMXT_CALLEE_MUSIC_END:
				break;
			//modified by lcx
			//------------------------
			case SMXT_OPEN_LED:
				OpenFillLight_Func();
				break;
			case SMXT_CLOSE_LED:
				CloseFillLight_Func();
				break;
//			case SMXT_CALLEE_SEND_DTMF:
//				SMXTCalleeDtmf();
			case SMXT_CALLEE_REJECT:
				SMXTCalleeReject();
				CloseFillLight_Func();
				break;
			//------------------------
			case SMXT_OPENDOOR:
				xintel_sth_open_door("cmd 0x19");
				break;
			case SMXT_VIDEO_MONITOR:
				SMXTVideoMonitor();
				break;
			case XT_EXIT:
				g_iRunXTTaskFlag = FALSE;
				break;
			default:
				break;
			}

			free(pvMsg);
		}
	}

	return NULL;
}



