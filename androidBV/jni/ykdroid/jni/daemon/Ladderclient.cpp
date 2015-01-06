#include <binder/IServiceManager.h>
#include <binder/IBinder.h>
#include <binder/Parcel.h>
#include <binder/ProcessState.h>
#include <binder/IPCThreadState.h>
#include <private/binder/binder_module.h>
#include <string.h>
#include <stdio.h>
#include <memory.h>
#include <Ladder.h>


using namespace android;

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "LadderClient"


int LadderDaemon::connect()
{
	sp<IServiceManager> sm = defaultServiceManager();
    do {
    	mLadder= sm->getService(String16("media.ladder")); //Context.MCU_SERVICE
        if (mLadder!= 0)
            break;
        LOGW("LadderService not published, waiting...");
        usleep(500000); // 0.5 s
    } while(true);

	LOGV("find binder service media.ladder success");

	Parcel in1,out1;
//	LadderCallback *cb = new LadderCallback();
	in1.writeStrongBinder(sp<IBinder>(&cb));
	int ret = mLadder->transact(LS_SET_CALLBACK, in1, &out1, 0); //TRANSACTION_registerSetup = 4
//	LOGD("transact(4) return %d", ret);
//	ProcessState::self()->startThreadPool();
//	IPCThreadState::self()->joinThreadPool();
	status = 1;
	return ret;
}
void LadderDaemon::reboot()
{
	if(status==0)
		return;
	Parcel in1,out1;
//	in1.writeCString();
//	out1.readCString()

	mLadder->transact(LS_REBOOT, in1, &out1, 0); 
}



//000.000.000.000 length=15
static char *delPreZero(char* pcStr) {
	__android_log_print(ANDROID_LOG_INFO, "Ladderclient", "delPreZero src:%s", pcStr);
	char cFlg = 0;
	char acBuff[32] = { 0x00 };
	char *acBak = pcStr;

	if (pcStr == NULL)
		return NULL;

	while (*pcStr != '\0') {
		if (*pcStr == '.') {
			cFlg = 0;
			pcStr++;
		} else if (cFlg == 0 && *pcStr != '0') {
			cFlg = 1;
			pcStr++;
		} else if (cFlg == 0 && *pcStr == '0') {
			if(*(pcStr+1) == '.' || *(pcStr+1) == '\0')
			{
				pcStr++;
				break;
			}
			memset(acBuff, 0x00, sizeof(acBuff));
			memcpy(acBuff, pcStr + 1, strlen(pcStr + 1));
			memcpy(pcStr, acBuff, strlen(pcStr + 1) + 1);

			if (*(pcStr) == '0' && *(pcStr + 1) == '0') {
				memset(acBuff, 0x00, sizeof(acBuff));
				memcpy(acBuff, pcStr + 1, strlen(pcStr + 1));
				memcpy(pcStr, acBuff, strlen(pcStr + 1) + 1);
				pcStr++;
			}

		} else if (cFlg == 1) {
			pcStr++;
		} else if (*pcStr == '\0') {
			break;
		}
	}
	__android_log_print(ANDROID_LOG_INFO, "Ladderclient", "delPreZero dst:%s", acBak);
	return acBak;
}

#define NM_ADD_IP_LEN			16
void LadderDaemon::setStaticIp(const char *ip)
{
	char buff[NM_ADD_IP_LEN] = {0x00};
	memset(buff, 0x00, NM_ADD_IP_LEN);
	strcpy(buff, ip);
	if(status==0)
		return;
	Parcel in1,out1;
	in1.writeCString(delPreZero(buff));
//	out1.readCString()
	mLadder->transact(LS_SET_STATIC_IP, in1, &out1, 0);
}

void LadderDaemon::setNetMask(const char *netmask)
{
	char buff[NM_ADD_IP_LEN] = {0x00};
	memset(buff, 0x00, NM_ADD_IP_LEN);
	strcpy(buff, netmask);
	if(status==0)
		return;
	Parcel in1,out1;
	in1.writeCString(delPreZero(buff));
//	out1.readCString()
	mLadder->transact(LS_SET_STATIC_NETMASK, in1, &out1, 0);
}

void LadderDaemon::setGateWay(const char *gateway)
{
	char buff[NM_ADD_IP_LEN] = {0x00};
	memset(buff, 0x00, NM_ADD_IP_LEN);
	strcpy(buff, gateway);
	if(status==0)
		return;
	Parcel in1,out1;
	in1.writeCString(delPreZero(buff));
//	out1.readCString()
	mLadder->transact(LS_SET_STATIC_GATEWAY, in1, &out1, 0);
}


void LadderDaemon::setDns(const char *dns)
{
	char buff[NM_ADD_IP_LEN] = {0x00};
	memset(buff, 0x00, NM_ADD_IP_LEN);
	strcpy(buff, dns);
	if(status==0)
		return;
	Parcel in1,out1;
	in1.writeCString( delPreZero(buff) );
//	out1.readCString()
	mLadder->transact(LS_SET_STATIC_DNS, in1, &out1, 0);
}

void LadderDaemon::setSyncTime(const char *datetime)
{
	char buff[64] = {0x00};
	memset(buff, 0x00, 64);
	strcpy(buff, datetime);
	if(status==0)
		return;
	Parcel in1,out1;
	in1.writeCString( delPreZero(buff) );
//	out1.readCString()
	mLadder->transact(LS_SET_SYNCTIME, in1, &out1, 0);
}


void LadderDaemon::updateApp()
{

	if(status==0)
		return;
	Parcel in1,out1;
//	in1.writeCString();
//	out1.readCString()
	mLadder->transact(LS_UPDATE_APP, in1, &out1, 0);
}

void LadderDaemon::updateMac()
{

	if(status==0)
		return;
	Parcel in1,out1;
//	in1.writeCString();
//	out1.readCString()
	mLadder->transact(LS_UPDATE_MAC, in1, &out1, 0);
}

void LadderDaemon::setDebugMode(int iType)
{
	if(status==0)
		return;
	Parcel in1,out1;

	if(iType == 0)
	{
		mLadder->transact(LS_USB_DEBUG, in1, &out1, 0);
	}
	else if(iType == 1)
	{
		mLadder->transact(LS_NET_DEBUG, in1, &out1, 0);
	}
}

