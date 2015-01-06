#ifndef __I_IDBT_SERVER__
#define __I_IDBT_SERVER__ 1
#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>
#include <binder/IMemory.h>
#include <utils/String8.h>
#include <binder/IServiceManager.h>
#include <binder/IBinder.h>
#include <binder/Parcel.h>
#include <binder/ProcessState.h>
#include <binder/IPCThreadState.h>
#include <private/binder/binder_module.h>


//code for ladder daemon clent-->service
#define LS_REBOOT   1
#define LS_SET_CALLBACK 2
#define LS_SET_STATIC_IP   3
#define LS_SET_STATIC_NETMASK   4
#define LS_SET_STATIC_GATEWAY   5
#define LS_SET_STATIC_DNS   6
#define LS_UPDATE_APP   7
#define LS_START_APP   8
#define LS_SET_SYNCTIME   9
#define LS_USB_DEBUG   10
#define LS_NET_DEBUG   11
#define LS_UPDATE_MAC   12

//code for ladder daemon callback service-->client
#define LS_CB_NETWORK_ON	1
#define LS_CB_NETWORK_OFF	2

extern "C" void notifyServiceNetOn();
extern "C" void notifyServiceNetOff();


namespace android {

class LadderListener: virtual public RefBase
{
public:
    virtual void notify(int msgType){}
    virtual void postData(int msgType, const sp<IMemory>& dataPtr){}
};

class LadderCallback : public BBinder
{
public:
	LadderCallback()
	{
		mListener = NULL;
		mydescriptor = String16("android.os.ISetupCallback");
	}
	virtual ~LadderCallback() {}
	virtual const String16& getInterfaceDescriptor() const
	{
		return mydescriptor;
	}
	void setlinster(sp<LadderListener> listener)
	{
		mListener = listener;
	}
protected:
	virtual status_t onTransact( uint32_t code,const Parcel& data,Parcel* reply,uint32_t flags = 0)
	{
		LOGD("enter MySetupCallback onTransact, code=%u", code);
//		if (data.checkInterface(this))
//			LOGD("checkInterface OK");
//		else
//		{
//			LOGW("checkInterface failed");
//			return -1;
//		}
		if(mListener!=NULL)
			mListener->notify(code);
		return 0;
	}
private:
	sp<LadderListener> mListener;
	String16 mydescriptor;
	
};


class LadderDaemon
{
	public:
		LadderDaemon()
		{
			status = 0;
		}
		int connect();
		void reboot();
		void setStaticIp(const char *ip);
		void setNetMask(const char *netmask);
		void setGateWay(const char *gateway);
		void setDns(const char *dns);
		void setSyncTime(const char *datetime);
		void updateApp();
		void updateMac();
		void setDebugMode(int iType);
		void startApp();
		void setListener(sp < LadderListener > listener)	{
			cb.setlinster(listener);
		}
	private:
		sp<IBinder> mLadder;
		LadderCallback cb;
		int status;
};

};
#endif
