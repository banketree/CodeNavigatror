///* 
// ============================================================================
// Name        : LP.c
// Author      : chensq
// Version     :
// Copyright   :
// Description : Hello World in C, Ansi-style
//
// export LD_LIBRARY_PATH=/media/sf_D_DRIVE/chensq/YouKe/50_Src/linphone_restruction/LP/src/LpLib
// register sip:100@192.168.1.222 sip:192.168.1.222 100
// call 15960100519 --audio-only --hint_music_start/home/workspace/YK-IDBT/LP/LPConfig/dooropen.wav--hint_music_end
// call 18950381900 --audio-only --hint_music_start/home/workspace/YK-IDBT/LP/LPConfig/dooropen.wav--hint_music_end
// call 101 --audio-only --hint_music_start/home/workspace/YK-IDBT/LP/LPConfig/dooropen.wav--hint_music_end
// LPC call 109 --audio-only
// ============================================================================
// */
//#ifdef LP_ONLY
//
//
//#include <stdio.h>
//#include <stdlib.h>
//#include  "IDBT.h"
//#include  "LPIntfLayer.h"
//#include  "CCTask.h"
//#include  "LPMsgOp.h"
//
//extern YK_MSG_QUE_ST  *g_pstCCMsgQ;
//
//
//int TestMsgSendCcsipCall(char *pcPhone, char mediaType, char hintMusicEn);
//int TestMsgSendCcsipAnswer();
//int TestMsgSendCcsipTerminate(long lCallId);
//int TestMsgSendCcsipTerminateAll();
//int TestMsgSendCcsipTerminateAll();
//
//
//
//
//int main(int argc, char *argv[])
//{
//	puts("test app begin"); /* prints  */
//
//	g_pstCCMsgQ = YKCreateQue(LP_QUE_LEN);
//    if( NULL == g_pstCCMsgQ )
//    {
//        printf("error:YKCreateQue failed [LPInit]\n");
//        return -1;
//    }
//
//
//	if( 0 != LOGIntfInit() )
//	{
//	    printf("error:LOGIntfInit failed\n");
//	    goto exit;
//	}
//
//	if( 0 != LPInit() )
//	{
//	    printf("error:LPInit failed\n");
//	    goto exit;
//	}
//	else
//	{
//	    printf("debug:LPInit success\n");
//	}
//
//
//	LPMsgSendTmSipRegister("8002@192.168.1.5", "123456", "192.168.1.5", "192.168.1.5");               // IBPX
//	//LPMsgSendTmSipRegister("+8659187513258@fj.ctcims.cn", "ykims", "fj.ctcims.cn", "61.131.4.71");  // IMS
//	//LPMsgSendTmSipRegister("6008@61.154.9.84", "123", "61.154.9.84", "61.154.9.84");                // ²šÌØ
//    //LPMsgSendTmSipRegister("105@196.196.196.196", "105", "196.196.196.196", "196.196.196.196");     // my  sip
//    //LPMsgSendTmSipRegister("105@196.196.197.214", "105", "196.196.197.214", "196.196.197.214");     // csq sip
//
//
//	//LPMsgSendTmSipRegister("100@196.196.197.214", "100", "196.196.197.214", "196.196.197.214");     // csq sip
//	//LPMsgSendTmSipRegister("+8659188244165@fj.ctcims.cn", "ykims123", "fj.ctcims.cn", "61.131.4.71");
//
//	//	sleep(5);
////	TestMsgSendCcsipCall("15960100519", MEDIA_TYPE_AUDIO, HINT_MUSIC_DISABLE);
////	sleep(15);400-836-5365
//	while(1)
//	{
//	    sleep(10);
////	    if(linphonec_running == 0)
////	    {
////	    	printf("will exit while\n");
////	    	break;
////	    }
//	}
//exit:
//    LPFini();
//    LOGIntfFini();
//
//	puts("test app end"); /* prints  */
//	return EXIT_SUCCESS;
//}
//
//
//
//#endif
