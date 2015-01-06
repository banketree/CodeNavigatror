#ifndef LANINTFLAYER_H
#define LANINTFLAYER_H

#define LANINTF_MSG_QUE_LEN 1024

extern int SipRecAudioOpen;
extern int SipPlayAudioOpen;
extern int SndRecAudioOpen;
extern int SndPlayAudioOpen;

extern int MsgCC2LANCall(const char *houseCode);
extern int MsgCC2LANTerminate(void);
extern int MsgCC2LANWatchLineuse(char *cFromIP);
extern int MsgCC2LANWatchRespond(char *cFromIP);
extern int MsgLAN2CCCallFail(void);
extern int MsgLAN2CCCallRespond(void);
extern int MsgLAN2CCCallPickUp(void);
extern int MsgLAN2CCCallHangOff(void);
extern int MsgLAN2CCCallLineuse(void);
extern int MsgLAN2CCCallOpendoor(void);
extern int MsgLAN2CCTalkEnd(void);
extern int MsgLAN2CCWatch(char *cFromIP);
extern int MsgLAN2CCCallTimeOut(void);
extern int MsgLAN2CCTalkTimeOut(void);
extern int LANInit(void);
extern void LANUninit(void);


#endif
