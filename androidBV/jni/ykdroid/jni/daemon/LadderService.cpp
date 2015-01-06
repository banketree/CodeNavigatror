#include <stdio.h>
#include <binder/IServiceManager.h>
#include <binder/IBinder.h>
#include <binder/Parcel.h>
#include <binder/ProcessState.h>
#include <binder/IPCThreadState.h>
#include "Ladder.h"
#include "DMNM.h"
#include "DMPM.h"
#include "DMNMUtils.h"
#include "IDBTCfg.h"
#include "dirent.h"


using namespace android;

#ifdef LOG_TAG
	#undef LOG_TAG
#endif

#define LOG_TAG "LadderService"

extern int g_iUpdateMac ;

class LadderService : public BBinder
{
public:

	 
	LadderService()
	{
		mydescriptor = String16("media.hello");
		n=0;
		cb = 0;
	}

	virtual ~LadderService() {}

	//This function is used when call Parcel::checkInterface(IBinder*)
	virtual const String16& getInterfaceDescriptor() const
	{
		LOGE("this is enter ==========getInterfaceDescriptor");
		return mydescriptor;
	}

	void Callback(uint32_t code)
	{
		if (cb != NULL)
		{
			LOGD("MyService interface 4 (%d)", code);
			Parcel in, out;
//			in.writeInterfaceToken(String16("ghq.callback"));
//			in.writeInt32(n++);
//			in.writeString16(String16("This is a string."));
			cb->transact(code, in, &out, 0);
		}
	}
	
protected:
	void reboot(){
		LOGD("LadderService will reboot");
		system("reboot");
	}
	void setStaticIp(const char *ip){
		char acBuff[64] = {0x00};
		strcpy(g_stIdbtCfg.stNetWorkInfo.acStaticIP, ip);
		sprintf(acBuff, "busybox ifconfig eth0 %s", ip);
		LOGD(acBuff);
		system(acBuff);
	}

	void setNetMask(const char *netmask){
		char acBuff[64] = {0x00};
		strcpy(g_stIdbtCfg.stNetWorkInfo.acStaticMask, netmask);
		sprintf(acBuff, "busybox ifconfig eth0 netmask %s", netmask);
		LOGD(acBuff);
		system(acBuff);
	}

	void setGateWay(const char *gateway){
		char acBuff[64] = {0x00};
		strcpy(g_stIdbtCfg.stNetWorkInfo.acStaticGateWay, gateway);
		sprintf(acBuff, "busybox route add default gw %s", gateway);
		LOGD(acBuff);
		system(acBuff);
	}

	void setDns(const char *dns){
		char acBuff[64] = {0x00};
		strcpy(g_stIdbtCfg.stNetWorkInfo.acStaticDNS, dns);
		sprintf(acBuff, "setprop net.dns1 %s", dns);
		LOGD(acBuff);
		system(acBuff);
	}


	void setUsbDebug(){
		LOGD("LadderService setUsbDebug");
		system("setprop service.adb.tcp.port -1");
		system("stop adbd");
		system("start adbd");
	}

	void setNetDebug(){
		LOGD("LadderService setNetDebug port:5555");
		system("setprop service.adb.tcp.port 5555");
		system("stop adbd");
		system("start adbd");
	}

	void updateApp(){
		LOGD("LadderService install apk");
		system("pm install -r /data/fsti_tools/AndroidLadder.apk");
		sleep(5);
		LOGD("LadderService restart apk");
		system("am start -n com.fsti.ladder/com.fsti.ladder.activity.SplashActivity");
		system("am start -n com.fsti.ladder/com.fsti.ladder.activity.SplashActivity");
	}

	void updateMac(){
		LOGD("LadderService update Mac");
		g_iUpdateMac = 1;
	}

	void startApp(){
		system("am start -n com.fsti.ladder/com.fsti.ladder.activity.SplashActivity");
		system("am start -n com.fsti.ladder/com.fsti.ladder.activity.SplashActivity");
	}

	void setSyncTime(const char *datetime){
//		char acDatatime[128];
//		sprintf(acDatatime,"busybox date -s %s",datetime);
		setenv("TZ", "CST", 1);
	//	system("date");
		LOGD("i6_set_datetime cmd : %s",datetime);
		system(datetime);
		system("busybox hwclock -w");
		setenv("TZ", "CST-8", 1);
//		setenv("TZ", "Asia/Shanghai", 1);
	}

	virtual status_t onTransact( uint32_t code,const Parcel& data,Parcel* reply,uint32_t flags = 0)
	{
		LOGD("enter LadderService onTransact and the code is [%d]",code);
		switch (code)
		{
		case LS_REBOOT:
			LOGD("LadderService recive reboot common");
			reboot();
			break;
		case LS_SET_STATIC_IP:
			LOGD("LadderService recive set static ip common");
			setStaticIp(data.readCString());
			break;
		case LS_SET_STATIC_NETMASK:
			LOGD("LadderService recive set static netmask common");
			setNetMask(data.readCString());
			break;
		case LS_SET_STATIC_GATEWAY:
			LOGD("LadderService recive set static gateway common");
			setGateWay(data.readCString());
			break;
		case LS_SET_STATIC_DNS:
			LOGD("LadderService recive set static dns common");
			setDns(data.readCString());
			break;
		case LS_UPDATE_APP:
			LOGD("LadderService recive update app common");
			updateApp();
			break;
		case LS_START_APP:
			LOGD("LadderService recive start app common");
			startApp();
			break;
		case LS_SET_SYNCTIME:
			LOGD("LadderService recive set synctime");
			setSyncTime(data.readCString());
			break;
		case LS_SET_CALLBACK:
		{//call cb
			LOGD("LadderService recive LS_SET_CALLBACK common");
			cb = data.readStrongBinder();
			break;
		}
		case LS_USB_DEBUG:
			LOGD("LadderService receive usb debug");
			setUsbDebug();
			break;
		case LS_NET_DEBUG:
			LOGD("LadderService receive net debug");
			setNetDebug();
			break;
		case LS_UPDATE_MAC:
			LOGD("LadderService receive update mac");
			updateMac();
			break;
		default:
			return BBinder::onTransact(code, data, reply, flags);
		}

		return 0;
}

private:
	String16 mydescriptor;
	sp<IBinder> cb;
	int n;
};




LadderService* srv = NULL;

static void indoor_mac_init()
{
	LOGD("CSQ indoor mac setting......");
	char acDefaultMac[] = "00:09:C0:FF:EC:50";


	char acMac[20] = {0x00};
	FILE *pCfgFd = NULL;
	//打开IDBT配置文件
	pCfgFd=fopen("/data/fsti_tools/idbt.cfg","r");
	if(pCfgFd != NULL)
	{
		char buf[300];
		fread(buf,300,1,pCfgFd);
		fseek(pCfgFd,0,SEEK_SET);
		fscanf(pCfgFd,"mac_addr=%s\n", acMac);
		LOGD("CSQ mac_addr=%s", acMac);
		//关闭配置文件
		fclose(pCfgFd);
	}

	if(strlen(acMac) == strlen(acDefaultMac))
	{
		LOGD("CSQ check mac_addr=%s", acMac);
	}
	else
	{
		 srand(time(NULL));
		 acDefaultMac[strlen(acDefaultMac)-8] = rand()%9+1 + '0';
		 LOGD("CSQ check rand mac_addr1=%c", acDefaultMac[strlen(acDefaultMac)-8]);
		 acDefaultMac[strlen(acDefaultMac)-7] = rand()%9+1 + '0';
		 LOGD("CSQ check rand mac_addr2=%c", acDefaultMac[strlen(acDefaultMac)-7]);
		 acDefaultMac[strlen(acDefaultMac)-5] = rand()%9+1 + '0';
		 LOGD("CSQ check rand mac_addr3=%c", acDefaultMac[strlen(acDefaultMac)-5]);
		 acDefaultMac[strlen(acDefaultMac)-4] = rand()%9+1 + '0';
		 LOGD("CSQ check rand mac_addr4=%c", acDefaultMac[strlen(acDefaultMac)-4]);
		 acDefaultMac[strlen(acDefaultMac)-2] = rand()%9+1 + '0';
		 LOGD("CSQ check rand mac_addr5=%c", acDefaultMac[strlen(acDefaultMac)-2]);
		 acDefaultMac[strlen(acDefaultMac)-1] = rand()%9+1 + '0';
		 LOGD("CSQ check rand mac_addr6=%c", acDefaultMac[strlen(acDefaultMac)-1]);
		memcpy(acMac, acDefaultMac, strlen(acDefaultMac));
		LOGD("CSQ check rand mac_addr=%s", acMac);
	}
	char acCMD[128] = {0x0};
	//修改mac地址
	system("busybox ifconfig eth0 down");
	sprintf(acCMD,"busybox ifconfig eth0 hw ether %s", acMac);
	if(system(acCMD) == 0)
	{
		LOGD("CSQ set mac: %s", acMac);
	}
	system("busybox ifconfig eth0 up");
}


int main()
{



	if( opendir("/data/data/com.fsti.ladder") == NULL )
	{//表示是室内机
		LOGD("CSQ check device: indoor device");
		indoor_mac_init();
		while(1)
		{
			sleep(5);
		}
	}
	else
	{//表示是梯口机
		LOGD("CSQ check device: outdoor device");

//		system("setprop service.adb.tcp.port 5555");
//		system("stop adbd");
//		system("start adbd");

//		system("ps | busybox grep 'adbd'| busybox grep -v 'grep'| busybox cut -c 5-16| busybox xargs kill -9");
	}

	system("chmod 777 /data/fsti_tools");
	system("chmod 777 /data/data/com.fsti.ladder");
	system("am start -n com.fsti.ladder/com.fsti.ladder.activity.SplashActivity");
	system("am start -n com.fsti.ladder/com.fsti.ladder.activity.SplashActivity");

	sleep(5);
	sp<IServiceManager> sm = defaultServiceManager();
	status_t ret;
	//register MyService to ServiceManager

	LOGD("addservice media.ladder return");

	srv = new LadderService();
	ret = sm->addService(String16("media.ladder"), srv);
	LOGD("addservice media.hello return %d", ret);
//	printf("addservice media.hello return %d", ret);

	if(DMNMInit() != 0)
	{
		DMNMFini();
	}

	if(DMPMInit() != 0)
	{
		DMPMFini();
	}

	//call binder thread pool to start
	ProcessState::self()->startThreadPool();
	IPCThreadState::self()->joinThreadPool(true);
	return 0;
}



void notifyServiceNetOn()
{
	LOGD("addservice NotifyServiceNetOn");
	srv->Callback(LS_CB_NETWORK_ON);
}

void notifyServiceNetOff()
{
	LOGD("addservice NotifyServiceNetOff");
	srv->Callback(LS_CB_NETWORK_OFF);
}


