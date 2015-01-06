#ifndef XTINTFLAYER_H
#define XTINTFLAYER_H

typedef int CallID;
#define INVALID_SIP_CALL_ID 0
#define VALID_SIP_CALL_ID	1

#define XTINTF_MSG_QUE_LEN 1024

extern CallID g_sipCallID;
extern int g_callIDUFlag;
extern int g_softSipClearFlag;//add by chensq 0表示由室内机清理标志，1表示由软终端清理标志
extern int SipRecAudioOpen;
extern int SipPlayAudioOpen;
extern int SndRecAudioOpen;
extern int SndPlayAudioOpen;

extern int XTTask(int argc, char *argv[]);
extern int TerminateLpCall(CallID id);
extern int InvokeLpCall(const char *pcHouseNum);
extern int PickupIDU(void);
extern void *XTIntfThread(void *arg);



#endif
