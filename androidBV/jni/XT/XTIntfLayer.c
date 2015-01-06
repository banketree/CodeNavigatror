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
YK_MSG_QUE_ST*  g_pstXTIntfMsgQ = NULL;          //XT�ӿ���Ϣ����ָ��

extern int g_ossPlayFlag;
extern pthread_mutex_t audio_lock;

int TerminateXTCall(void)
{
	printf("*********************g_callIDUFlag = %d\n", g_callIDUFlag);
	if(g_callIDUFlag == 0)
	{
		StopPlayWavFile(); //�رջ�����
		WaitAudioUnuse(1000); //�ȴ���������    //��ʱ��Ϊ�˵ȴ�����������ɣ������������
		if(Local.cSipCallPassive == 0)//��ز���ʾ
		{
			////printf("i am here!!!!!!  Local.Status = %d \n", Local.Status);
			if((Local.Status != 0) && (Local.Status != 1))
			{
				DisplayTalkOverWindow();
			}
		}
		Local.Status = 0; //״̬�ָ�Ϊ����״̬l
		sleep(1);
		if(Local.cSipCallPassive == 0)//��ز���ʾ
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

		// �����ʱ��Ĭ�Ϲر�oss����
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
	usleep(1000*1000);	//�ȴ�g_callIDUFlag��0
	if (g_callIDUFlag == 0)
	{
		// �����˱�־��Ŀ���ǵ�ͬʱ֧�����ڻ���sip�ն˵�ʱ��
		// ���sip���ų��ִ������ִ�����´��룬�����ϻ�ʹ����ָ���������
		// �����û�����ڻ�������£���sip���ų��ִ���ִ�����´�����Իָ���������
		////printf("i am here00000!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		sleep(2);
		if(Local.Status != 0)
		{
		//sleep(2);
		Local.Status = 0; //״̬�ָ�Ϊ����״̬
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
	StopPlayWavFile(); //�رջ�����
	WaitAudioUnuse(2000); //�ȴ���������    //��ʱ��Ϊ�˵ȴ�����������ɣ������������

	pthread_mutex_unlock(&audio_lock);

	g_softSipClearFlag = 1;
	//g_ossPlayFlag = 1; //oss����ʹ��
	//if (INVALID_SIP_CALL_ID != id)
	//{
		if(Local.cSipCallPassive == 0)//0���� 1����
		{
			//����
			g_ossPlayFlag = 1; //oss����ʹ��
			printf("--------------------SM callee_pick_up[cSipCallPassive=0,g_ossPlayFlag=>1,call_idu_flag=%d]\n", g_callIDUFlag);

			// ��id����Ч�ģ���Ҷ����ڻ�
			// �ر��ݿڻ������ڻ�֮���ͨ��
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
//			StopPlayWavFile();  //�رջ�����
//			WaitAudioUnuse(2000); //�ȴ���������    //��ʱ��Ϊ�˵ȴ�����������ɣ������������
			Local.Status = 5; //״̬�޸�Ϊ����ͨ��
			Local.TimeOut = 0; //���ӳ�ʱ,  ͨ����ʱ,  ���г�ʱ�����˽���
			DisplayTalkStartWindow(); //��ʾ�Խ� ����ͨ���Ĵ���
			printf("--------------------SM callee_pick_up[start talking in sip]\n");
		}
		else
		{
			//���� ��Ƶ���
			printf("--------------------SM callee_pick_up[g_ossPlayFlag => 0]\n");
			g_ossPlayFlag = 0; //oss���Ž���
		}
//	}
//	else
//	{
//		g_ossPlayFlag = 0; //oss���Ž��ܣ���ʾ������Ƶ���״̬
//		printf("--------------------SM callee_pick_up[g_ossPlayFlag = 0, g_sipId = %d, call_idu_flag = %d]\n",
//						id, g_callIDUFlag);
//	}

}

static void SMXTCalleeHangoff(void)
{
	//2012 04 13
	printf("********************SMXTCalleeHangoff\n");
	StopPlayWavFile(); //�رջ�����
	WaitAudioUnuse(1000); //�ȴ���������    //��ʱ��Ϊ�˵ȴ�����������ɣ������������

	if (g_softSipClearFlag == 1)//0��ʾ�����ڻ������־��1��ʾ�����ն������־
	{
		if(Local.Status != 0)
		{
			if(Local.cSipCallPassive == 0)
			{
				DisplayTalkOverWindow(); //��ز�Ҫ��ʾͨ����������
			}
			Local.Status = 0; //״̬�ָ�Ϊ����״̬
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
	WaitAudioUnuse(2000); //�ȴ���������
	sprintf(wavFile, "%s/reject.wav\0", wavPath);
	StartPlayWav(wavFile, 0);

	sleep(3);

	StopPlayWavFile(); //�رջ�����
	WaitAudioUnuse(1000); //�ȴ���������    //��ʱ��Ϊ�˵ȴ�����������ɣ������������

	if (INVALID_SIP_CALL_ID != g_sipCallID)
	{
		printf("callee reject, end this talking!\n");
		g_callIDUFlag = 0;
		g_softSipClearFlag = 1;
		TerminateLpCall(g_sipCallID);
	}

	if (g_softSipClearFlag == 1)//0��ʾ�����ڻ������־��1��ʾ�����ն������־
	{
		if(Local.Status != 0)
		{
			if(Local.cSipCallPassive == 0)
			{
				DisplayTalkOverWindow(); //��ز�Ҫ��ʾͨ����������
			}
			Local.Status = 0; //״̬�ָ�Ϊ����״̬
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
		Local.OpenDoorFlag = 2; //������־ 0 δ����  1 ������ʱ��  2 ������
		Local.OpenDoorTime = 0; //ʱ��
		//������
		OpenLock_Func(); //��������
		if(g_ossPlayFlag == 1)
		{
			g_ossPlayFlag = 0;
		}
		WaitAudioUnuse(2000); //�ȴ���������
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
	 WaitAudioUnuse(1000); //�ȴ���������
	 sprintf(wavFile, "%s/passerror.wav\0", wavPath);
	 StartPlayWav(wavFile, 0);
	 }*/
	ClearTalkEdit(); //�� �ı���
}

static void SMXTCalleeDtmf(void)
{
	//if (INVALID_SIP_CALL_ID != id && key == OPEN_LOCK_FLAG)
	//{ //�ػ�ID��Ч��keyΪ"#"�����
	  //�ݿڻ�����
		xintel_sth_open_door("cmd 0x19");
		//IDBT_DEBUG("-----------SM virtual in door open");

//		Local.Status = 0; //״̬�ָ�Ϊ����״̬l
//		sleep(1);
//		DisplayMainWindow(0);
		//WriteDayLog(1, "%s %02d %s", buffRoom10Temp, LOG_EVT_USERS_PHONE_OPEN_DOOR, USERS_PHONE_OPEN_DOOR);
	//}
}

static void SMXTVideoMonitor(void)
{
	g_ossPlayFlag = 0; //oss���Ž���
}

void* XTIntfThread(void *arg)
{
	void *pvMsg = NULL;

	//����XT�ӿڲ���Ϣ����
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

		YKReadQue(g_pstXTIntfMsgQ, &pvMsg, 0);			//������ȡ
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



