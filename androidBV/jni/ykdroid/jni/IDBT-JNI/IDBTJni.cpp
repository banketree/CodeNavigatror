extern "C" {
#include "IDBT.h"
#include "IDBTIntfLayer.h"
#include "SMRfCard.h"
#include "TMInterface.h"
#include "NetworkMonitor.h"
#include "LOGIntfLayer.h"
#include <stdlib.h>
#include <malloc.h>
#include "IDBTCfg.h"
}

#include <string.h>
#include <jni.h>
#include <android/log.h>
#include <pthread.h>
#include "Ladder.h"
#include "CFimc.h"

#define JNI_BUF_SIZE 256

extern CFIMC g_cam;


using namespace android;
class IDBTListner:public LadderListener
  
{
public:
	virtual void notify(int msgType)
	{
		LOGE("callback code=%d",msgType);
		switch(msgType)
		{
			case LS_CB_NETWORK_ON:
				LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_NET_CONNECT, 0, "");
				break;
			case LS_CB_NETWORK_OFF:
				LOG_EVENTLOG_INFO(LOG_EVENTLOG_FORMAT, LOG_EVENTLOG_NO_ROOM, LOG_EVT_NET_CONNECT, 1, "");
				break;
			default :
				break;
		}
	}

//	virtual void postData(int msgType, const sp<IMemory>& dataPtr){;}
	IDBTListner(){}
	~IDBTListner(){}
	
};


#ifdef __cplusplus
extern "C" {
#endif

YK_MSG_QUE_ST  *g_pstJNIMsgQ;
LadderDaemon g_daemon;


pthread_key_t jnienv_key;
JavaVM *g_vm = NULL;

JNIEnv *g_env = NULL;

static jint g_sipRegStatus;
static jint g_networkStatus;
static jint g_tmLinkStatus;

static int JNICbInit(JNIEnv *env, jclass obj);
static void IDBTJniRegStatusChanged(REG_STATUS_EN enRegStatus);

void jni_android_key_cleanup(void *data){
	__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "Thread end, detaching jvm from current thread");
	JNIEnv* env=(JNIEnv*)pthread_getspecific(jnienv_key);
	if (env != NULL) {
		g_vm->DetachCurrentThread();
		pthread_setspecific(jnienv_key,NULL);
	}
}
extern void ms_set_jvm(JavaVM *vm);
void jni_set_jvm(JavaVM *vm)
{
	g_vm = vm;

	ms_set_jvm(vm);
	pthread_key_create(&jnienv_key, jni_android_key_cleanup);
}

JNIEXPORT jint JNICALL  JNI_OnLoad(JavaVM *ajvm, void *reserved)
{

	jni_set_jvm(ajvm);

	return JNI_VERSION_1_6;
}

JavaVM *jni_get_jvm(void){
	return g_vm;
}

JNIEnv *jni_get_jni_env(void)
{
	JNIEnv *env=NULL;

	if (g_vm==NULL)
	{
		__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "wangle file =%s,line=%d\n",__FILE__,__LINE__);
		__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "Calling ms_get_jni_env() while no jvm has been set using ms_set_jvm().");
	}
	else
	{
		env=(JNIEnv*)pthread_getspecific(jnienv_key);
		if (env==NULL)
		{
			int nRet = g_vm->AttachCurrentThread(&env,NULL);
			if (nRet!=0)
			{
				__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "AttachCurrentThread() failed !:"+nRet);
				return NULL;
			}
			pthread_setspecific(jnienv_key,env);
		}
		__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "ms_get_jni_env() not implemented on windows.");
	}
	return env;
}

/*
 * Class:     com_fsti_ladder_engine_IDBTEngine
 * Method:    getSipRegStatus
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_getSipRegStatus(JNIEnv *env, jclass obj)
{
	return g_sipRegStatus;
}

/*
 * Class:     com_fsti_ladder_engine_IDBTEngine
 * Method:    getNetwrokStatus
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_getNetwrokStatus(JNIEnv *env, jclass obj)
{
	return g_networkStatus;
}

/*
 * Class:     com_fsti_ladder_engine_IDBTEngine
 * Method:    getTmStatus
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_getTmLinkStatus
  (JNIEnv *, jclass)
{
	return g_tmLinkStatus;
}

/*
 * Class:     com_fsti_ladder_engine_IDBTEngine
 * Method:    sendHouseCode
 * Signature: (I)I
 *	
 */
JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_sendHouseCode(JNIEnv *env, jclass obj, jstring arg)
{


	int ret = 0;
	const char *houseCode = env->GetStringUTFChars(arg, 0);

	ret = SendHouseCode(houseCode);
	env->ReleaseStringUTFChars(arg, houseCode);

	return ret;
}


/*
 * Class:     com_fsti_ladder_engine_IDBTEngine
 * Method:    checkImsCallEnable
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_checkImsCallEnable
  (JNIEnv *, jclass)
{
	return CheckImsCallEnable();
}

/*
 * Class:     com_fsti_ladder_engine_IDBTEngine
 * Method:    getImsCallTimeout
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_getImsCallTimeout
  (JNIEnv *, jclass)
{
	return GetImsCallTimeout();
}

/*
 * Class:     com_fsti_ladder_engine_IDBTEngine
 * Method:    cancelCall
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_fsti_ladder_engine_IDBTEngine_cancelCall(JNIEnv *env, jclass obj)
{
	 HangupCall();
}

/*
 * Class:     com_fsti_ladder_engine_IDBTEngine
 * Method:    hangupCall
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_fsti_ladder_engine_IDBTEngine_hangupCall(JNIEnv *env, jclass obj)
{
	HangupCall();
}

/*
 * Class:     com_fsti_ladder_engine_IDBTEngine
 * Method:    sendOpenDoorPassword
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_sendOpenDoorPassword(JNIEnv *env, jclass obj, jstring arg)
{
	 int ret;
	 const char *password = env->GetStringUTFChars(arg, 0);

	 ret = ValidatePassword(password);

	 env->ReleaseStringUTFChars(arg, password);

	 if(ret == 1)
	 {
		 return 0;
	 }
	 else
	 {
		 return -1;
	 }
}

 /*
  * Class:     com_fsti_ladder_engine_IDBTEngine
  * Method:    getDevInfo
  * Signature: ()Ljava/lang/String;
  <DeviceInfo>
  		<SerialNumber><Manufacturer><OUI><ProductClass>
  </DeviceInfo>
  		
  */

 JNIEXPORT jstring JNICALL Java_com_fsti_ladder_engine_IDBTEngine_getDevInfo(JNIEnv *env, jclass obj)
{
	 char * devinfo=TMGetDevInfo();
	
//	 jclass objClass = (*env)->FindClass(env, "java/lang/String");

	 jstring jstr = env->NewStringUTF(devinfo);

	 return(jstr);
}

 /*
  * Class:     com_fsti_ladder_engine_IDBTEngine
  * Method:    getVersionInfo
  * Signature: ()Ljava/lang/String;
  <DeviceInfo>
  		<FirmwareVersion><HardwareVersion><KernelVersion>
  </DeviceInfo>
  
  */
 JNIEXPORT jstring JNICALL Java_com_fsti_ladder_engine_IDBTEngine_getVersionInfo
   (JNIEnv *env, jclass)
 {
		int ret;
		char devinfo[128]= {0x0};

		ret = TMGetVersionInfo(devinfo);

		__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "devinfo = %s",devinfo);

		jstring jstr = NULL;

		if (ret == 0)
		{
			jstr = env->NewStringUTF(devinfo);
		}

//	 free(devinfo);

	 return jstr;
 }

 /*
  * Class:     com_fsti_ladder_engine_IDBTEngine
  * Method:    getNetworkInfo
  * Signature: ()Ljava/lang/String;
  <network_type><IP><mask><gateway><dns><pppoeusername>
  */
 JNIEXPORT jstring JNICALL Java_com_fsti_ladder_engine_IDBTEngine_getNetworkInfo
   (JNIEnv *env, jclass)
 {
	 jstring jstr;
	char *p;
	char localIP[64];
	char localMac[64];
	char pstr[JNI_BUF_SIZE];
	int rst = 0;
	if(!pstr) return NULL;
	memset(pstr,0,JNI_BUF_SIZE);
	p = pstr;
	
	memset(localIP,0,64);
	memset(localMac,0,64);

	sprintf(p,"%s|",g_stIdbtCfg.stNetWorkInfo.acNetFlag);
	p+=strlen(g_stIdbtCfg.stNetWorkInfo.acNetFlag);
	p+=1;
	if(g_stIdbtCfg.stNetWorkInfo.acNetFlag[0] == '2')
	{
		NMGetPPPoeIpAddress(localIP);
		NMGetLocalMacByName(localMac,"ppp");
	}
	else
	{
		NMGetIpAddress(localIP);
		NMGetLocalMacByName(localMac,"eth");
	}
	sprintf(p,"%s|",localMac);
	p+=strlen(localMac);
	p+=1;
	sprintf(p,"%s|",localIP);
	p+=strlen(localIP);
	p+=1;
	sprintf(p,"%s|",g_stIdbtCfg.stNetWorkInfo.acStaticIP);
	p+=strlen(g_stIdbtCfg.stNetWorkInfo.acStaticIP);
	p+=1;
	sprintf(p,"%s|",g_stIdbtCfg.stNetWorkInfo.acStaticMask);
	p+=strlen(g_stIdbtCfg.stNetWorkInfo.acStaticMask);
	p+=1;
	sprintf(p,"%s|",g_stIdbtCfg.stNetWorkInfo.acStaticGateWay);
	p+=strlen(g_stIdbtCfg.stNetWorkInfo.acStaticGateWay);
	p+=1;
	sprintf(p,"%s|",g_stIdbtCfg.stNetWorkInfo.acStaticDNS);
	p+=strlen(g_stIdbtCfg.stNetWorkInfo.acStaticDNS);
	p+=1;
	sprintf(p,"%s|",g_stIdbtCfg.stNetWorkInfo.acWanUserName);
	p+=strlen(g_stIdbtCfg.stNetWorkInfo.acWanUserName);
	p+=1;
	__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "NetworkInfo g_stIdbtCfg.iNetMode= %d",g_stIdbtCfg.iNetMode);
	sprintf(p,"%d|",g_stIdbtCfg.iNetMode);
	p+=2;

	__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "NetworkInfo = %s",pstr);

	 jstr = env->NewStringUTF(pstr);
	 return jstr;
 }

 /*
  * Class:     com_fsti_ladder_engine_IDBTEngine
  * Method:    getNetworkJoinMethod
  * Signature: ()Ljava/lang/String;
  */
 JNIEXPORT jstring JNICALL Java_com_fsti_ladder_engine_IDBTEngine_getNetworkJoinMethod
   (JNIEnv *env, jclass)
 {
	 char pstr[2] = {0};

	 memcpy(pstr, &g_stIdbtCfg.stNetWorkInfo.acNetFlag[0], 1);
	 pstr[1] = 0x0;
	 __android_log_print(ANDROID_LOG_INFO, "JNIMsg", "IDBTEngine_getNetworkJoinMethod = %s",pstr);

	 jstring jstr = env->NewStringUTF(pstr);

	 return(jstr);
 }

 /*
  * Class:     com_fsti_ladder_engine_IDBTEngine
  * Method:    getNetworkJoinMethod
  * Signature: ()Ljava/lang/String;
  */
 JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_getVedioQuality
   (JNIEnv *env, jclass)
 {
	 jint jValue;

	 __android_log_print(ANDROID_LOG_INFO, "JNIMsg", "IDBTEngine_getVedioQuality = %d",g_stIdbtCfg.iVedioQuality);

	 jValue = g_stIdbtCfg.iVedioQuality;

	 return(jValue);
 }

 /*
  * Class:     com_fsti_ladder_engine_IDBTEngine
  * Method:    getVedioRotation
  * Signature: ()Ljava/lang/String;
  */
 JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_getVedioRotation
   (JNIEnv *env, jclass)
 {
	 jint jValue;

	 __android_log_print(ANDROID_LOG_INFO, "JNIMsg", "IDBTEngine_getVedioRotation = %d",g_stIdbtCfg.iRotation);

	 jValue = g_stIdbtCfg.iRotation;

	 return(jValue);
 }


 /*
  * Class:     com_fsti_ladder_engine_IDBTEngine
  * Method:    getVideoFormat
  * Signature: ()I
  */
 JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_getVideoFormat
 (JNIEnv *env, jclass)
{
	 jint jValue;

	 __android_log_print(ANDROID_LOG_INFO, "JNIMsg", "IDBTEngine_getVideoFormat = %d",g_stIdbtCfg.iVedioFmt);

	 jValue = g_stIdbtCfg.iVedioFmt;

	 return(jValue);
}


 /*
  * Class:     com_fsti_ladder_engine_IDBTEngine
  * Method:    getVideoFormat
  * Signature: ()I
  */
 JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_getNegotiateMode
 (JNIEnv *env, jclass)
{
	 jint jValue;

	 __android_log_print(ANDROID_LOG_INFO, "JNIMsg", "IDBTEngine_getNegotiateMode = %d",g_stIdbtCfg.iNegotiateMode);

	 jValue = g_stIdbtCfg.iNegotiateMode;

	 return(jValue);
}


 /*
  * Class:     com_fsti_ladder_engine_IDBTEngine
  * Method:    getNetMonitorInfo
  * Signature: ()Ljava/lang/String;
  */
 JNIEXPORT jstring JNICALL Java_com_fsti_ladder_engine_IDBTEngine_getNetMonitorInfo
   (JNIEnv *env, jclass)
 {
	 char pstr[32] = {0};

 	 __android_log_print(ANDROID_LOG_INFO, "JNIMsg", "getNetMonitorInfo = %s : %d",g_stIdbtCfg.stDestIpInfo.acDestIP1,g_stIdbtCfg.stDestIpInfo.port1);

 	 sprintf(pstr,"%s|%d|",g_stIdbtCfg.stDestIpInfo.acDestIP1,g_stIdbtCfg.stDestIpInfo.port1);

 	__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "getNetMonitorInfo = %s",pstr);

	 jstring jstr = env->NewStringUTF(pstr);

	 return(jstr);
 }

 /*
   * Class:     com_fsti_ladder_engine_IDBTEngine
   * Method:    getDevicePosition
   * Signature: ()Ljava/lang/String;
   */
  JNIEXPORT jstring JNICALL Java_com_fsti_ladder_engine_IDBTEngine_getDevicePosition
    (JNIEnv *env, jclass)
  {
  	char pstr[32] = {0};

 	 __android_log_print(ANDROID_LOG_INFO, "JNIMsg", "IDBTEngine_getDeviceBlock = %s",g_stIdbtCfg.acPositionCode);

 	 strncpy(pstr, g_stIdbtCfg.acPositionCode+1, 4);
 	 strcat(pstr,"|");
 	 strncpy(pstr+strlen(pstr),g_stIdbtCfg.acPositionCode+5,2);
 	 strcat(pstr,"|");

 	__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "IDBTEngine_getDeviceBlock = %s",pstr);

	 jstring jstr = env->NewStringUTF(pstr);

 	 return(jstr);
  }

 /*
  * Class:     com_fsti_ladder_engine_IDBTEngine
  * Method:    setNetworkJoinMethod
  * Signature: (I)I
  <network_type>
  */
 JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_setNetworkJoinMethod
   (JNIEnv *env, jclass obj, jint type)
 {
	memset(g_stIdbtCfg.stNetWorkInfo.acNetFlag, 0, NM_NET_FLAG_LEN);
	g_stIdbtCfg.stNetWorkInfo.acNetFlag[0] = (char)type + 0x30;
	int ret = SaveIdbtConfig();
//	__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "setNetworkJoinMethod = %d",g_stIdbtCfg.stNetWorkInfo.acNetFlag[0]);

	//sleep(3);
	//__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "reboot ");

	//g_daemon.retoot();

	return ret;
 }

 /*
  * Class:     com_fsti_ladder_engine_IDBTEngine
  * Method:    setBlockNum
  * Signature: (Ljava/lang/String;)I
  */
 JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_setBlockNum
   (JNIEnv * env, jclass obj, jstring arg)
 {
	 const char *s= env->GetStringUTFChars(arg, 0);
	 char acBuf[8];
	 sprintf(acBuf,"%04d",atoi(s));
	 memcpy(g_stIdbtCfg.acPositionCode+1,acBuf,4);
	 __android_log_print(ANDROID_LOG_INFO, "JNIMsg", "setBlockNum = %s", g_stIdbtCfg.acPositionCode);
	 int ret = SaveIdbtConfig();

	 env->ReleaseStringUTFChars(arg, s);
	 return ret;
 }

 /*
  * Class:     com_fsti_ladder_engine_IDBTEngine
  * Method:    setUnitNum
  * Signature: (Ljava/lang/String;)I
  */
 JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_setUnitNum
   (JNIEnv * env, jclass obj, jstring arg)
 {
	 const char *s= env->GetStringUTFChars(arg, 0);
	 char acBuf[8];
	 sprintf(acBuf,"%02d",atoi(s));
	 memcpy(g_stIdbtCfg.acPositionCode+5,acBuf,2);
	 __android_log_print(ANDROID_LOG_INFO, "JNIMsg", "setUnitNum = %s", g_stIdbtCfg.acPositionCode);
	 int ret = SaveIdbtConfig();

	 env->ReleaseStringUTFChars(arg, s);
	 return ret;
 }

 /*
  * Class:     com_fsti_ladder_engine_IDBTEngine
  * Method:    setIPAddress
  * Signature: (Ljava/lang/String;)I
  */
 JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_setIPAddress
   (JNIEnv * env, jclass obj, jstring arg)
 {
	const char *s= env->GetStringUTFChars(arg, 0);
	 memset(g_stIdbtCfg.stNetWorkInfo.acStaticIP, 0, NM_ADD_IP_LEN);
	 sprintf(g_stIdbtCfg.stNetWorkInfo.acStaticIP,"%s",s);
	 __android_log_print(ANDROID_LOG_INFO, "JNIMsg", "setIPAddress = %s", g_stIdbtCfg.stNetWorkInfo.acStaticIP);
	 int ret = SaveIdbtConfig();

	 g_daemon.setStaticIp(g_stIdbtCfg.stNetWorkInfo.acStaticIP);

	 env->ReleaseStringUTFChars(arg, s);
	 return ret;
 }

 /*
  * Class:     com_fsti_ladder_engine_IDBTEngine
  * Method:    setNetMask
  * Signature: (Ljava/lang/String;)I
  */
 JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_setNetMask
   (JNIEnv * env, jclass obj, jstring arg)
 {
	const char *s= env->GetStringUTFChars(arg, 0);
	memset(g_stIdbtCfg.stNetWorkInfo.acStaticMask,0,sizeof(g_stIdbtCfg.stNetWorkInfo.acStaticMask) );
	sprintf(g_stIdbtCfg.stNetWorkInfo.acStaticMask,"%s",s);
	__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "setNetMask = %s", g_stIdbtCfg.stNetWorkInfo.acStaticMask);
	int ret = SaveIdbtConfig();

	g_daemon.setNetMask(g_stIdbtCfg.stNetWorkInfo.acStaticMask);


	env->ReleaseStringUTFChars(arg, s);
	return ret;
 }

 /*
  * Class:     com_fsti_ladder_engine_IDBTEngine
  * Method:    setGateway
  * Signature: (Ljava/lang/String;)I
  */
 JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_setGateway
   (JNIEnv * env, jclass obj, jstring arg)
 {
	 const char *s= env->GetStringUTFChars(arg, 0);
	 memset(g_stIdbtCfg.stNetWorkInfo.acStaticGateWay,0,sizeof(g_stIdbtCfg.stNetWorkInfo.acStaticGateWay) );
	 sprintf(g_stIdbtCfg.stNetWorkInfo.acStaticGateWay,"%s",s);
	 __android_log_print(ANDROID_LOG_INFO, "JNIMsg", "setGateway = %s", g_stIdbtCfg.stNetWorkInfo.acStaticGateWay);
	 int ret = SaveIdbtConfig();

	 g_daemon.setGateWay(g_stIdbtCfg.stNetWorkInfo.acStaticGateWay);


	 env->ReleaseStringUTFChars(arg, s);
	 return ret;
 }

 /*
  * Class:     com_fsti_ladder_engine_IDBTEngine
  * Method:    setDNS
  * Signature: (Ljava/lang/String;)I
  */
 JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_setDNS
   (JNIEnv * env, jclass obj, jstring arg)
 {
	 const char *s= env->GetStringUTFChars(arg, 0);
	 memset(g_stIdbtCfg.stNetWorkInfo.acStaticDNS,0,sizeof(g_stIdbtCfg.stNetWorkInfo.acStaticDNS) );
	 sprintf(g_stIdbtCfg.stNetWorkInfo.acStaticDNS,"%s",s);
	 __android_log_print(ANDROID_LOG_INFO, "JNIMsg", "setDNS = %s", g_stIdbtCfg.stNetWorkInfo.acStaticDNS);
	 int ret = SaveIdbtConfig();

	 g_daemon.setDns(g_stIdbtCfg.stNetWorkInfo.acStaticDNS);


	 env->ReleaseStringUTFChars(arg, s);
	 return ret;
 }

 /*
  * Class:     com_fsti_ladder_engine_IDBTEngine
  * Method:    setPPPOEUsername
  * Signature: (Ljava/lang/String;)I
  */
 JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_setPPPOEUsername
   (JNIEnv * env, jclass obj, jstring arg)
 {
	 int ret;

	 const char *s= env->GetStringUTFChars(arg, 0);
	 memset(g_stIdbtCfg.stNetWorkInfo.acWanUserName,0,sizeof(g_stIdbtCfg.stNetWorkInfo.acWanUserName) );
	 sprintf(g_stIdbtCfg.stNetWorkInfo.acWanUserName,"%s",s);
	 __android_log_print(ANDROID_LOG_INFO, "JNIMsg", "setPPPOEUsername = %s", g_stIdbtCfg.stNetWorkInfo.acWanUserName);
	 //更新TMData.xml
	 ret = TMSetPPPOEUername(s);

	 NMUpdateInfo();

	 env->ReleaseStringUTFChars(arg, s);

	 return ret;
 }

 /*
  * Class:     com_fsti_ladder_engine_IDBTEngine
  * Method:    setPPPOEPassword
  * Signature: (Ljava/lang/String;)I
  */
 JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_setPPPOEPassword
   (JNIEnv *env, jclass, jstring arg)
 {
	 int ret;

	 const char *s= env->GetStringUTFChars(arg, 0);
	 memset(g_stIdbtCfg.stNetWorkInfo.acWanPassword,0,sizeof(g_stIdbtCfg.stNetWorkInfo.acWanPassword) );
	 sprintf(g_stIdbtCfg.stNetWorkInfo.acWanPassword,"%s",s);
	 __android_log_print(ANDROID_LOG_INFO, "JNIMsg", "setPPPOEPassword = %s", g_stIdbtCfg.stNetWorkInfo.acWanPassword);
	 ret = TMSetPPPOEPassword(s);

	 NMUpdateInfo();

	 env->ReleaseStringUTFChars(arg, s);

	 return ret;
 }

 /*
  * Class:     com_fsti_ladder_engine_IDBTEngine
  * Method:    getPlatformInfo
  * Signature: ()Ljava/lang/String;
  <DeviceInfo>
  		<DeviceNo>
  </DeviceInfo>
  <ManagementServer>
  	<URL>
  </ManagementServer>

  */
 JNIEXPORT jstring JNICALL Java_com_fsti_ladder_engine_IDBTEngine_getPlatformInfo
   (JNIEnv *env, jclass obj)
 {
	 char * platformInfo=TMGetPlatformInfo();

	 __android_log_print(ANDROID_LOG_INFO, "JNIMsg", "getPlatformInfo = %s", platformInfo);

	 jstring jstr = env->NewStringUTF(platformInfo);

	 return(jstr);
 }

 /*
  * Class:     com_fsti_ladder_engine_IDBTEngine
  * Method:    setDevNum
  * Signature: (Ljava/lang/String;)I
  */
 JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_setDevNum
   (JNIEnv *env, jclass obj, jstring arg)
 {
	 int ret;

	 const char *s= env->GetStringUTFChars(arg, 0);

	 ret = TMSetDevNum(s);

	 env->ReleaseStringUTFChars(arg, s);

	 return ret;
 }

 /*
  * Class:     com_fsti_ladder_engine_IDBTEngine
  * Method:    setPlatformDomainName
  * Signature: (Ljava/lang/String;)I
  */
 JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_setPlatformDomainName
   (JNIEnv *env, jclass obj, jstring arg)
 {
	 int ret;

	 const char *s= env->GetStringUTFChars(arg, 0);

	 ret = TMSetPlatformDomainName(s);

	 env->ReleaseStringUTFChars(arg, s);

	 return ret;
 }

 /*
  * Class:     com_fsti_ladder_engine_IDBTEngine
  * Method:    setPlatformI5DomainName
  * Signature: (Ljava/lang/String;)I
  */
 JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_setI5PlatformDomainName
   (JNIEnv *env, jclass obj, jstring arg)
 {
	 int ret;

	 const char *s= env->GetStringUTFChars(arg, 0);

	 ret = TMSetI5PlatformDomainName(s);

	 env->ReleaseStringUTFChars(arg, s);

	 return ret;
 }

 /*
  * Class:     com_fsti_ladder_engine_IDBTEngine
  * Method:    setPlatformUserName
  * Signature: (Ljava/lang/String;)I
  */
 JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_setPlatformUserName
   (JNIEnv *env, jclass obj, jstring arg)
 {
	 int ret;

	 const char *s= env->GetStringUTFChars(arg, 0);

	 ret = TMSetPlatformUserName(s);

	 env->ReleaseStringUTFChars(arg, s);

	 return ret;
 }

 /*
  * Class:     com_fsti_ladder_engine_IDBTEngine
  * Method:    setPlatformPassword
  * Signature: (Ljava/lang/String;)I
  */
 JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_setPlatformPassword
   (JNIEnv *env, jclass obj, jstring arg)
 {
	 int ret;

	 const char *s= env->GetStringUTFChars(arg, 0);

	 ret = TMSetPlatformPassword(s);

	 env->ReleaseStringUTFChars(arg, s);

	 return ret;
 }

 /*
  * Class:     com_fsti_ladder_engine_IDBTEngine
  * Method:    setMonitorIP
  * Signature: (Ljava/lang/String;)I
  */
 JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_setMonitorIP
   (JNIEnv *env, jclass obj, jstring arg)
 {
	 const char *s= env->GetStringUTFChars(arg, 0);
	 memset(g_stIdbtCfg.stDestIpInfo.acDestIP1,0,sizeof(g_stIdbtCfg.stDestIpInfo.acDestIP1) );
	 sprintf(g_stIdbtCfg.stDestIpInfo.acDestIP1,"%s",s);
	 __android_log_print(ANDROID_LOG_INFO, "JNIMsg", "setMonitorIP = %s", g_stIdbtCfg.stDestIpInfo.acDestIP1);
	 int ret = SaveIdbtConfig();

	 env->ReleaseStringUTFChars(arg, s);
	 return ret;
 }

 /*
  * Class:     com_fsti_ladder_engine_IDBTEngine
  * Method:    setMonitorPort
  * Signature: (Ljava/lang/String;)I
  */
 JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_setMonitorPort
   (JNIEnv *env, jclass obj, jstring arg)
 {
	 const char *s= env->GetStringUTFChars(arg, 0);
	 g_stIdbtCfg.stDestIpInfo.port1 = atoi(s);
	 __android_log_print(ANDROID_LOG_INFO, "JNIMsg", "setMonitorPort = %d", g_stIdbtCfg.stDestIpInfo.port1);
	 int ret = SaveIdbtConfig();

	 env->ReleaseStringUTFChars(arg, s);
	 return ret;
 }

 /*
  * Class:     com_fsti_ladder_engine_IDBTEngine
  * Method:    setPlatformIP
  * Signature: (Ljava/lang/String;)I
  */
 JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_setPlatformIP
   (JNIEnv *env, jclass obj, jstring arg)
 {
	 const char *s= env->GetStringUTFChars(arg, 0);
	 __android_log_print(ANDROID_LOG_INFO, "JNIMsg", "setPlatformIP = %s", s);
	 int ret = TMSetPlatformIP(s);

	 env->ReleaseStringUTFChars(arg, s);
	 return ret;
 }

 /*
  * Class:     com_fsti_ladder_engine_IDBTEngine
  * Method:    setPlatformPort
  * Signature: (Ljava/lang/String;)I
  */
 JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_setPlatformPort
   (JNIEnv *env, jclass obj, jstring arg)
 {
	 const char *s= env->GetStringUTFChars(arg, 0);
	 __android_log_print(ANDROID_LOG_INFO, "JNIMsg", "setPlatformPort = %d", atoi(s));
	 int ret = TMSetPlatformPort(s);

	 env->ReleaseStringUTFChars(arg, s);
	 return ret;
 }

 /*
  * Class:     com_fsti_ladder_engine_IDBTEngine
  * Method:    getSIPInfo
  * Signature: ()Ljava/lang/String;
  <Communication>
  <SipAccount>  <ImsDomainName><ImsProxyIP><ImsProxyPort>
  </Communication>

  */
 JNIEXPORT jstring JNICALL Java_com_fsti_ladder_engine_IDBTEngine_getSIPInfo
   (JNIEnv *env, jclass obj)
 {
	 char * sipInfo = TMGetSIPInfo();

	 jstring jstr = env->NewStringUTF(sipInfo);

	 return(jstr);
 }

 /*
  * Class:     com_fsti_ladder_engine_IDBTEngine
  * Method:    getManagementPassword
  * Signature: (Ljava/lang/String;)I
	NULL
  */
JNIEXPORT jstring JNICALL Java_com_fsti_ladder_engine_IDBTEngine_getManagementPassword
    (JNIEnv *env, jclass obj)
{
	//char *managementPassword;
	int ret;
	char managementPassword[16]= {0x0};

//	ret = TMGetManagementPassword(managementPassword);
	strcpy(managementPassword, g_stIdbtCfg.acManagementPassword);

	__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "managementPassword = %s",managementPassword);

	jstring jstr = NULL;

//	if (ret == 0)
	{
		jstr = env->NewStringUTF(managementPassword);
	}
	//free(managementPassword);
	return(jstr);
}


 /*
  * Class:     com_fsti_ladder_engine_IDBTEngine
  * Method:    setManagementPassword
  * Signature: (Ljava/lang/String;)I
	NULL
  */
JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_setManagementPassword
   (JNIEnv *env, jclass obj, jstring arg)
{
	int ret;
	char strBuf[JNI_BUF_SIZE] = {0};

	const char *s= env->GetStringUTFChars(arg, 0);

//	ret = TMSetManagementPassword(s);
	strcpy(g_stIdbtCfg.acManagementPassword, s);

	__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "setManagementPassword = %s", g_stIdbtCfg.acManagementPassword);

	ret = SaveIdbtConfig();

	env->ReleaseStringUTFChars(arg, s);

	return ret;
}

/*
 * Class:     com_fsti_ladder_engine_IDBTEngine
 * Method:    setVedioQuality
 * Signature: (Ljava/lang/String;)I
	NULL
 */
JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_setVedioQuality
  (JNIEnv *env, jclass obj, jstring arg)
{
	int ret;
	char strBuf[JNI_BUF_SIZE] = {0};

	const char *s= env->GetStringUTFChars(arg, 0);

//	ret = TMSetManagementPassword(s);
	g_stIdbtCfg.iVedioQuality = atoi(s);

	__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "setVedioQuality = %d", g_stIdbtCfg.iVedioQuality);

	ret = SaveIdbtConfig();

	env->ReleaseStringUTFChars(arg, s);

	return ret;
}

/*
 * Class:     com_fsti_ladder_engine_IDBTEngine
 * Method:    setRotationValue
 * Signature: (Ljava/lang/String;)I
	NULL
 */
JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_setRotationValue
  (JNIEnv *env, jclass obj, jstring arg)
{
	int ret;
	char strBuf[JNI_BUF_SIZE] = {0};

	const char *s= env->GetStringUTFChars(arg, 0);

//	ret = TMSetManagementPassword(s);
	g_stIdbtCfg.iRotation = atoi(s);

	__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "setRotationValue = %d", g_stIdbtCfg.iRotation);

	ret = SaveIdbtConfig();

	env->ReleaseStringUTFChars(arg, s);

	return ret;
}

 /*
  * Class:     com_fsti_ladder_engine_IDBTEngine
  * Method:    setOpendoorPassword
  * Signature: (Ljava/lang/String;)I
  <Config>
	  <PasswordOpendoor>
  </Config>
  
  */
JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_setOpendoorPassword
   (JNIEnv *env, jclass obj, jstring arg)
{
	int strLen, ret;

	const char *s= env->GetStringUTFChars(arg, 0);

//	ret = TMSetOpendoorPassword(s);
	strcpy(g_stIdbtCfg.acOpendoorPassword, s);

	__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "setOpendoorPassword = %s", g_stIdbtCfg.acOpendoorPassword);

	ret = SaveIdbtConfig();

	env->ReleaseStringUTFChars(arg, s);

	return ret;
}

/*
 * Class:     com_fsti_ladder_engine_IDBTEngine
 * Method:    setDebugMode
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_setDebugMode
(JNIEnv *env, jclass obj, jstring arg)
{
	char strBuf[JNI_BUF_SIZE] = {0};

	const char *s= env->GetStringUTFChars(arg, 0);

	__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "setDebugMode = %d", atoi(s));

	g_daemon.setDebugMode(atoi(s));

	env->ReleaseStringUTFChars(arg, s);

	return 0;
}

/*
 * Class:     com_fsti_ladder_engine_IDBTEngine
 * Method:    setNetMode
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_setNetMode
(JNIEnv *env, jclass obj, jstring arg)
{
	int ret;
	char strBuf[JNI_BUF_SIZE] = {0};

	const char *s= env->GetStringUTFChars(arg, 0);

	g_stIdbtCfg.iNetMode = atoi(s);

	__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "setNetMode = %d", g_stIdbtCfg.iNetMode);

	if(g_stIdbtCfg.iNetMode == 1)
	{
		TMUseLocalSipInfo();
	}

	ret = SaveIdbtConfig();

	env->ReleaseStringUTFChars(arg, s);

	return ret;
}




/*
 * Class:     com_fsti_ladder_engine_IDBTEngine
 * Method:    setVideoFormat
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_setVideoFormat
(JNIEnv *env, jclass obj, jstring arg)
{
	int ret;
	char strBuf[JNI_BUF_SIZE] = {0};


	if(g_stIdbtCfg.iNegotiateMode == 1)
	{
		LOGE("CFIMC iNegotiateMode == 1 can't change video format\n");
		return -1;
	}

	const char *s= env->GetStringUTFChars(arg, 0);

	g_stIdbtCfg.iVedioFmt = atoi(s);
	g_stIdbtCfg.iNegotiateMode = 0;
	ret = SaveIdbtConfig();
	__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "setVideoFormat = %d", g_stIdbtCfg.iVedioFmt);



	if(g_stIdbtCfg.iVedioFmt == 0)
	{
		g_cam.uninit_video();
		LOGE("CFIMC setVideoFormat CIF\n");
		//cif
		if(g_cam.init_video(0, 352, 288) < 0){
			LOGE("CFIMC camera init faild\n");
			return -1;
		}
	}
	else if(g_stIdbtCfg.iVedioFmt == 1)
	{
		g_cam.uninit_video();
		LOGE("CFIMC setVideoFormat D1\n");
		//d1
		if(g_cam.init_video(0, 704, 576) < 0){
			LOGE("CFIMC camera init faild\n");
			return -1;
		}
	}
	else if(g_stIdbtCfg.iVedioFmt == 2)
	{
		g_cam.uninit_video();
		LOGE("CFIMC setVideoFormat 720P\n");
		//cif
		if(g_cam.init_video(0, 1280, 720) < 0){
			LOGE("CFIMC camera init faild\n");
			return -1;
		}
	}


	env->ReleaseStringUTFChars(arg, s);

	return ret;
}



/*
 * Class:     com_fsti_ladder_engine_IDBTEngine
 * Method:    setNegotiateMode
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_setNegotiateMode
(JNIEnv *env, jclass obj, jstring arg)
{
	int ret;
	char strBuf[JNI_BUF_SIZE] = {0};

	const char *s= env->GetStringUTFChars(arg, 0);

	g_stIdbtCfg.iNegotiateMode = atoi(s);
	ret = SaveIdbtConfig();
	__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "setNegotiateMode = %d", g_stIdbtCfg.iNegotiateMode);



	if(g_stIdbtCfg.iNegotiateMode == 0)
	{
		if(g_stIdbtCfg.iVedioFmt == 0)
			{
				g_cam.uninit_video();
				LOGE("CFIMC setVideoFormat CIF\n");
				//cif
				if(g_cam.init_video(0, 352, 288) < 0){
					LOGE("CFIMC camera init faild\n");
					return -1;
				}
			}
			else if(g_stIdbtCfg.iVedioFmt == 1)
			{
				g_cam.uninit_video();
				LOGE("CFIMC setVideoFormat D1\n");
				//d1
				if(g_cam.init_video(0, 704, 576) < 0){
					LOGE("CFIMC camera init faild\n");
					return -1;
				}
			}
			else if(g_stIdbtCfg.iVedioFmt == 2)
			{
				g_cam.uninit_video();
				LOGE("CFIMC setVideoFormat 720P\n");
				//cif
				if(g_cam.init_video(0, 1280, 720) < 0){
					LOGE("CFIMC camera init faild\n");
					return -1;
				}
			}
	}
	else if(g_stIdbtCfg.iNegotiateMode == 1)
	{
		g_cam.auto_mode_width = 704;
		g_cam.auto_mode_height = 576;

		g_cam.uninit_video();
		LOGE("CFIMC iNegotiateMode == 1 will use D1\n");
		//d1
		if(g_cam.init_video(0, 704, 576) < 0){
			LOGE("CFIMC camera init faild\n");
			return -1;
		}
	}


	env->ReleaseStringUTFChars(arg, s);

	return ret;
}

/*
 * Class:     com_fsti_ladder_engine_IDBTEngine
 * Method:    setResetDev
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_setResetDev
   (JNIEnv *, jclass)
{
	//int ret = system("busybox reboot");
	__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "reboot ");

	g_daemon.reboot();
//	__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "reboot result:%d", ret);
}

jstring stoJstring(JNIEnv* env, const char* pat)
{
	jclass strClass = env->FindClass("java/lang/String");
	jmethodID ctorID = env->GetMethodID(strClass, "<init>", "([BLjava/lang/String;)V");
	jbyteArray bytes = env->NewByteArray(strlen(pat));
	env->SetByteArrayRegion(bytes, 0, strlen(pat), (jbyte*)pat);
	jstring encoding = env->NewStringUTF("utf-8");
	return (jstring)env->NewObject(strClass, ctorID, bytes, encoding);
}

char g_stAdvInfo[260] = {0x0};
jstring jstrAdvInfo;
/*
 * Class:     com_fsti_ladder_engine_IDBTEngine
 * Method:    IDBTInit
 * Signature: ()I
 */
 JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_IDBTInit(
		JNIEnv *env, jclass obj) {

	int ret;
	void *pvMsg = NULL;
	OTAB_REBOOT_ST *pstRebootMsg;
	SIPAB_REG_STATUS_ST *pstRegMsg;
	SIPAB_NETWORK_STATUS_ST *pstNetworkMsg;
	SMAB_RFIDCARD_VALIDATE_ST *pstRfidcardValidateMsg;
	TMAB_TM_LINK_STATUS_ST *pstTmLinkMsg;
	TM_JNI_ADV_INFO_ST *pstAdvInfo;

//	__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "jni cb init begin");
//
//	ret = JNICbInit(env, obj);
//
//	__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "jni cb init end");

//	IDBTJniSetRegStatusChangedCB(IDBTJniRegStatusChanged);

//	return (IDBTMain(0, 0));

	jclass TestProvider = NULL;
	jobject mTestProvider = NULL;
	jmethodID sFunCallback = NULL;
	jmethodID cFunCallback = NULL;

	if (env == NULL) {
		return FAILURE;
	}

	if (TestProvider == NULL) {
		TestProvider = env->FindClass("com/fsti/ladder/common/MyJavaCallback");
		if (TestProvider == NULL) {
			__android_log_print(ANDROID_LOG_INFO, "JNIMsg",
					"find class MyJavaCallback failed");
			return FAILURE;
		}
		__android_log_print(ANDROID_LOG_INFO, "JNIMsg",
				"find class MyJavaCallback ok");
	}

	if (sFunCallback == NULL) {
		sFunCallback = env->GetStaticMethodID(TestProvider, "callbackByC",
				"(I)I");
		if (sFunCallback == NULL) {
			__android_log_print(ANDROID_LOG_INFO, "JNIMsg",
					"find static Method callbackByC failed");
			env->DeleteLocalRef(TestProvider);
			return FAILURE;
		}
		__android_log_print(ANDROID_LOG_INFO, "JNIMsg",
				"find static Method callbackByC ok");
	}

	if (cFunCallback == NULL) {
		cFunCallback = env->GetStaticMethodID(TestProvider, "callbackByCMethodChar",
				"(Ljava/lang/String;)V");
		if (cFunCallback == NULL) {
			__android_log_print(ANDROID_LOG_INFO, "JNIMsg",
					"find static Method callbackByCMethodChar failed");
			env->DeleteLocalRef(TestProvider);
			return FAILURE;
		}
		__android_log_print(ANDROID_LOG_INFO, "JNIMsg",
				"find static Method callbackByCMethodChar ok");
	}

	//初始设备的网络状态
	g_sipRegStatus = (jint)JNI_DEV_SIP_REG_STATUS_OFFLINE;
	g_tmLinkStatus = (jint)JNI_DEV_TM_LINK_STATUS_OFFLINE;
	g_networkStatus = (jint)JNI_DEV_NETWORK_STATUS_OFFLINE;

	IDBTMain(0, 0);

	 jstring jstr;

	 char * devinfo=TMGetDevInfo();
#if 1
	 g_daemon.connect();

	IDBTListner *plistner = new IDBTListner();

	 g_daemon.setListener(plistner);
#endif
//	 jstr = env->NewStringUTF(devinfo);

//	 __android_log_print(ANDROID_LOG_INFO, "JNIMsg",
//	 				"jstring is: %s", devinfo);
//
//	 char * versionInfo = TMGetVersionInfo();
//	 __android_log_print(ANDROID_LOG_INFO, "JNIMsg",
//		 				"jstring is: %s", versionInfo);
//
//	 char * platformInfo = TMGetPlatformInfo();
//	 __android_log_print(ANDROID_LOG_INFO, "JNIMsg",
//	 	 				"jstring is: %s", platformInfo);
//
//	 char * sipInfo = TMGetSIPInfo();
//
//	 __android_log_print(ANDROID_LOG_INFO, "JNIMsg",
//	 	 	 				"jstring is: %s", sipInfo);

//
//	 free(devinfo);





	while(1)
	{
		int iErrCode = YKReadQue(g_pstJNIMsgQ, &pvMsg, 0);//AB_QUE_READ_TIMEOUT
		if ( 0 != iErrCode || NULL == pvMsg )
		{
			continue;
		}

		__android_log_print(ANDROID_LOG_INFO, "CB", "YKReadQue:g_pstJNIMsgQ=0x%04x", *(( unsigned int *)pvMsg));

		switch ( *(( unsigned int *)pvMsg) )
		{
		case OTAB_REBOOT:
			pstRebootMsg = (OTAB_REBOOT_ST *)pvMsg;
			__android_log_print(ANDROID_LOG_INFO, "CB", "IDBTJni OTAB_REBOOT");
			env->CallStaticIntMethod(TestProvider, sFunCallback, (jint) (JNI_DEV_REBOOT));
			sleep(10);
			__android_log_print(ANDROID_LOG_INFO, "CB", "IDBTJni OTAB_REBOOT");
			g_daemon.reboot();
			break;
		case OTAB_CONFIG_REBOOT:
			pstRebootMsg = (OTAB_REBOOT_ST *)pvMsg;
			__android_log_print(ANDROID_LOG_INFO, "CB", "IDBTJni OTAB_REBOOT");
			env->CallStaticIntMethod(TestProvider, sFunCallback, (jint) (CONFIG_UPDATE_DEV_REBOOT));
			sleep(10);
			__android_log_print(ANDROID_LOG_INFO, "CB", "IDBTJni OTAB_REBOOT");
			g_daemon.reboot();
			break;
		case SIPAB_STOP_RING:
			__android_log_print(ANDROID_LOG_INFO, "CB", "IDBTJni SIPAB_STOP_RING");
			env->CallStaticIntMethod(TestProvider, sFunCallback, (jint) (JNI_DEV_SIPAB_STOP_RING));
			break;
		case SIPAB_REG_STATUS:
			pstRegMsg = (SIPAB_REG_STATUS_ST *)pvMsg;
			if(pstRegMsg->uiRegStatus == REG_STATUS_OFFLINE)
			{
				__android_log_print(ANDROID_LOG_INFO, "CB", "IDBTJni SIPAB_REG_STATUS:OFFLINE");
				env->CallStaticIntMethod(TestProvider, sFunCallback, (jint) (JNI_DEV_SIP_REG_STATUS_OFFLINE));
				g_sipRegStatus = (jint) JNI_DEV_SIP_REG_STATUS_OFFLINE;
			}
			else if(pstRegMsg->uiRegStatus == REG_STATUS_ONLINE)
			{
				__android_log_print(ANDROID_LOG_INFO, "CB", "IDBTJni SIPAB_REG_STATUS:ONLINE");
				env->CallStaticIntMethod(TestProvider, sFunCallback, (jint) (JNI_DEV_SIP_REG_STATUS_ONLINE));
				g_sipRegStatus = (jint) JNI_DEV_SIP_REG_STATUS_ONLINE;
			}
			break;
		case SIPAB_NETWORK_STATUS:
			pstNetworkMsg = (SIPAB_NETWORK_STATUS_ST *)pvMsg;
			if(pstNetworkMsg->uiNetworkStatus == NETWORK_STATUS_OFFLINE)
			{
				__android_log_print(ANDROID_LOG_INFO, "CB", "IDBTJni SIPAB_NETWORK_STATUS:OFFLINE");
				env->CallStaticIntMethod(TestProvider, sFunCallback, (jint) (JNI_DEV_NETWORK_STATUS_OFFLINE));
				g_networkStatus = (jint) JNI_DEV_NETWORK_STATUS_OFFLINE;
			}
			else if(pstNetworkMsg->uiNetworkStatus == NETWORK_STATUS_ONLINE)
			{
				__android_log_print(ANDROID_LOG_INFO, "CB", "IDBTJni SIPAB_NETWORK_STATUS:ONLINE");
				env->CallStaticIntMethod(TestProvider, sFunCallback, (jint) (JNI_DEV_NETWORK_STATUS_ONLINE));
				g_networkStatus = (jint) JNI_DEV_NETWORK_STATUS_ONLINE;
			}
			break;
		case TMAB_TM_LINK_STATUS:
			pstTmLinkMsg = (TMAB_TM_LINK_STATUS_ST *)pvMsg;
			if(pstTmLinkMsg->uiLinkStatus == LINK_STATUS_OFFLINE)
			{
				__android_log_print(ANDROID_LOG_INFO, "CB", "IDBTJni TMAB_TM_LINK_STATUS:OFFLINE");
				env->CallStaticIntMethod(TestProvider, sFunCallback, (jint) (JNI_DEV_TM_LINK_STATUS_OFFLINE));
				g_tmLinkStatus = (jint) JNI_DEV_TM_LINK_STATUS_OFFLINE;
			}
			else if(pstTmLinkMsg->uiLinkStatus == LINK_STATUS_ONLINE)
			{
				__android_log_print(ANDROID_LOG_INFO, "CB", "IDBTJni TMAB_TM_LINK_STATUS:ONLINE");
				env->CallStaticIntMethod(TestProvider, sFunCallback, (jint) (JNI_DEV_TM_LINK_STATUS_ONLINE));
				g_tmLinkStatus = (jint) JNI_DEV_TM_LINK_STATUS_ONLINE;
			}


			break;
//		case CCAB_AUDIO_INCOMING:
//			__android_log_print(ANDROID_LOG_INFO, "CB", "CCAB_AUDIO_INCOMING");
//			env->CallStaticIntMethod(TestProvider, sFunCallback, (jint) (JNI_DEV_CALL_AUDIO_INCOMING_RECEIVED));
//			break;
//		case CCAB_VIDEO_INCOMING:
//			__android_log_print(ANDROID_LOG_INFO, "CB", "CCAB_VIDEO_INCOMING");
//			env->CallStaticIntMethod(TestProvider, sFunCallback, (jint) (JNI_DEV_CALL_VIDEO_INCOMING_RECEIVED));
//			break;
//		case CCAB_CALLEE_OUTGOING_INIT:
//			__android_log_print(ANDROID_LOG_INFO, "CB", "SIPAB_CALLEE_OUTGOING_INIT");
//			env->CallStaticIntMethod(TestProvider, sFunCallback, (jint) (JNI_DEV_OUTGOING_INIT));
//		break;
//		case CCAB_CALLEE_OUTGOING_PROGRESS:
//			__android_log_print(ANDROID_LOG_INFO, "CB", "CCAB_CALLEE_OUTGOING_PROGRESS");
//			env->CallStaticIntMethod(TestProvider, sFunCallback, (jint) (JNI_DEV_OUTGOING_PROGRESS));
//			break;
//		case CCAB_CALLEE_OUTGOING_RINGING:
//			__android_log_print(ANDROID_LOG_INFO, "CB", "CCAB_CALLEE_OUTGOING_RINGING");
//			env->CallStaticIntMethod(TestProvider, sFunCallback, (jint) (JNI_DEV_OUTGOING_RINGING));
//			break;
//		case CCAB_CALLEE_OUTGOING_EARLYMEDIA:
//			__android_log_print(ANDROID_LOG_INFO, "CB", "CCAB_CALLEE_OUTGOING_EARLYMEDIA");
//			env->CallStaticIntMethod(TestProvider, sFunCallback, (jint) (JNI_DEV_CALL_OUTGOING_EARLYMEDIA));
//			break;
		case CCAB_CALLEE_PICK_UP:
			__android_log_print(ANDROID_LOG_INFO, "CB", "IDBTJni CCAB_CALLEE_PICK_UP");
			env->CallStaticIntMethod(TestProvider, sFunCallback, (jint) (JNI_DEV_CALL_CONNECTED));
			break;
		case CCAB_CALLEE_HANG_OFF:
			__android_log_print(ANDROID_LOG_INFO, "CB", "IDBTJni CCAB_CALLEE_HANG_OFF");
			env->CallStaticIntMethod(TestProvider, sFunCallback, (jint) (JNI_DEV_CALL_END));
			break;
		case CCAB_CALLEE_ERR:
			__android_log_print(ANDROID_LOG_INFO, "CB", "IDBTJni CCAB_CALLEE_ERR");
			env->CallStaticIntMethod(TestProvider, sFunCallback, (jint) (JNI_DEV_CALL_ERROR));
			break;
		case CCAB_CALLEE_ERR_ROOM_INVALID:
			__android_log_print(ANDROID_LOG_INFO, "CB", "IDBTJni CCAB_CALLEE_ERR_ROOM_INVALID");
			env->CallStaticIntMethod(TestProvider, sFunCallback, (jint) (CALL_ERROR_ROOM_INVALID));
			break;
		case CCAB_CALLEE_ERR_ROOM_VALID:
			__android_log_print(ANDROID_LOG_INFO, "CB", "IDBTJni CCAB_CALLEE_ERR_ROOM_VALID");
			env->CallStaticIntMethod(TestProvider, sFunCallback, (jint) (CALL_ERROR_ROOM_VALID));
			break;
		case CCAB_OPENDOOR:
		case CCAB_CALLEE_SEND_DTMF:
			__android_log_print(ANDROID_LOG_INFO, "CB", "IDBTJni CCAB_OPENDOOR");
			env->CallStaticIntMethod(TestProvider, sFunCallback, (jint) (JNI_DEV_OPENDOOR));
			break;
		case CCAB_VEDIO_MONITOR:
			__android_log_print(ANDROID_LOG_INFO, "CB", "IDBTJni CCAB_VEDIO_MONITOR");
			env->CallStaticIntMethod(TestProvider, sFunCallback, (jint) (JNI_DEV_CALL_VIDEO_INCOMING_RECEIVED));
			break;
		case TMAB_ADVERT_UPDATE:
			__android_log_print(ANDROID_LOG_INFO, "CB", "IDBTJni TMAB_ADVERT_UPDATE");
			env->CallStaticIntMethod(TestProvider, sFunCallback, (jint) (JNI_DEV_ADVERT_UPDATE));
			break;
		case SMAB_RFIDCARD_VALIDATE_STATUS:
			pstRfidcardValidateMsg = (SMAB_RFIDCARD_VALIDATE_ST *)pvMsg;
			if(pstRfidcardValidateMsg->uiValidateStatus == RFIDCARD_VALID)
			{
				__android_log_print(ANDROID_LOG_INFO, "CB", "IDBTJni SMAB_RFIDCARD_VALIDATE_STATUS:VALID");
				env->CallStaticIntMethod(TestProvider, sFunCallback, (jint) (JNI_DEV_RFIDCARD_VALIDATA_STATUS_VALID));
			}
			else if(pstRfidcardValidateMsg->uiValidateStatus == RFIDCARD_INVALID)
			{
				__android_log_print(ANDROID_LOG_INFO, "CB", "IDBTJni SMAB_RFIDCARD_VALIDATE_STATUS:INVALID");
				env->CallStaticIntMethod(TestProvider, sFunCallback, (jint) (JNI_DEV_RFIDCARD_VALIDATE_STATUS_INVALID));
			}
			break;
		case TM_I6_ADV_DOWNLOAD:
			pstAdvInfo = (TM_JNI_ADV_INFO_ST *)pvMsg;
			__android_log_print(ANDROID_LOG_INFO, "JNIMsg",
								"find static Method callbackByCMethodChar 1 failed");
			sprintf(g_stAdvInfo,"%d&%s",JNI_ADV_DOWNLOAD,pstAdvInfo->auAdvInfo);
			__android_log_print(ANDROID_LOG_INFO, "JNIMsg",
								"find static Method callbackByCMethodChar 11 failed");
//			strcpy(g_stAdvInfo,"24&123456&/data/data/com.fsti.ladder/&214213441&");
			jstrAdvInfo = stoJstring(env,g_stAdvInfo);
			__android_log_print(ANDROID_LOG_INFO, "JNIMsg",
								"find static Method callbackByCMethodChar 2 failed");
			env->CallStaticVoidMethod(TestProvider,cFunCallback, jstrAdvInfo);
			__android_log_print(ANDROID_LOG_INFO, "JNIMsg",
								"find static Method callbackByCMethodChar 3 failed");
			break;
		case TM_I6_ANN_DOWNLOAD:
			pstAdvInfo = (TM_JNI_ADV_INFO_ST *)pvMsg;
			__android_log_print(ANDROID_LOG_INFO, "JNIMsg",
								"find static Method callbackByCMethodChar 1 failed");
			sprintf(g_stAdvInfo,"%d&%s",JNI_ANN_DOWNLOAD,pstAdvInfo->auAdvInfo);
			__android_log_print(ANDROID_LOG_INFO, "JNIMsg",
								"find static Method callbackByCMethodChar 11 failed");
//			strcpy(g_stAdvInfo,"24&123456&/data/data/com.fsti.ladder/&214213441&");
			jstrAdvInfo = stoJstring(env,g_stAdvInfo);
			__android_log_print(ANDROID_LOG_INFO, "JNIMsg",
								"find static Method callbackByCMethodChar 2 failed");
			env->CallStaticVoidMethod(TestProvider,cFunCallback, jstrAdvInfo);
			__android_log_print(ANDROID_LOG_INFO, "JNIMsg",
								"find static Method callbackByCMethodChar 3 failed");
			break;
		default:
			__android_log_print(ANDROID_LOG_INFO, "CB", "IDBTJni g_pstJNIMsgQ:default %d",*(( unsigned int *)pvMsg));
			break;
		}

		free(pvMsg);
	}
}


/*
 * Class:     com_fsti_ladder_engine_IDBTEngine
 * Method:    listenIDBTState
 * Signature: (I)I
 */
 JNIEXPORT jint JNICALL Java_com_fsti_ladder_engine_IDBTEngine_listenIDBTState(
		JNIEnv *env, jclass obj, jint arg)
{

}

 jobject g_obj;
 jclass g_clazz;
 jmethodID g_mid;

static int JNICbInit(JNIEnv *env, jclass obj) {
	jclass TestProvider = NULL;
	jobject mTestProvider = NULL;
	jmethodID sFunCallback = NULL;

//	g_obj = env->NewGlobalRef(obj);
//
//	// save refs for callback
//	//g_clazz = env->GetObjectClass(g_obj);
//	g_clazz = env->FindClass("com/fsti/ladder/common/MyJavaCallback");
//	if (g_clazz == NULL)
//	{
//		__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "GetObjectClass failed");
//	}
//
//	g_mid = env->GetStaticMethodID(g_clazz, "callbackByC", "(I)I");
//	if (g_mid == NULL)
//	{
//		__android_log_print(ANDROID_LOG_INFO, "JNIMsg", "GetMethodID failed");
//	}
//
////	env->CallStaticIntMethod(g_clazz, g_mid, (jint) 9);
//
//	return SUCCESS;

//	if (env == NULL) {
//		return FAILURE;
//	}
//
//	if (TestProvider == NULL) {
//		TestProvider = env->FindClass("com/fsti/ladder/common/MyJavaCallback");
//		if (TestProvider == NULL) {
//			__android_log_print(ANDROID_LOG_INFO, "JNIMsg",
//					"find class MyJavaCallback failed");
//			return FAILURE;
//		}
//		__android_log_print(ANDROID_LOG_INFO, "JNIMsg",
//				"find class MyJavaCallback ok");
//	}
//
//	if (sFunCallback == NULL) {
//		sFunCallback = env->GetStaticMethodID(TestProvider, "callbackByC",
//				"(I)I");
//		if (sFunCallback == NULL) {
//			__android_log_print(ANDROID_LOG_INFO, "JNIMsg",
//					"find static Method callbackByC failed");
//			env->DeleteLocalRef(TestProvider);
//			return FAILURE;
//		}
//		__android_log_print(ANDROID_LOG_INFO, "JNIMsg",
//				"find static Method callbackByC ok");
//	}

//	gENV = env;
//	gClass = TestProvider;
//	gFuncCallBack = sFunCallback;

//	env->CallStaticIntMethod(TestProvider, sFunCallback, (jint) 9);
	//env->CallStaticObjectMethod(TestProvider, sFunCallback, 9);
//
//	return SUCCESS;
}

static void IDBTJniRegStatusChanged(REG_STATUS_EN enRegStatus) {
//	gENV->CallStaticIntMethod(gClass, gFuncCallBack, (jint) enRegStatus);

//	JNIEnv * g_env;
	// double check it's all ok
//	int getEnvStat = g_vm->GetEnv((void **)&g_env, JNI_VERSION_1_6);
//	if (getEnvStat == JNI_EDETACHED)
//	{
//		if (g_vm->AttachCurrentThread(&g_env, NULL) != 0)
//		{
//			__android_log_print(ANDROID_LOG_INFO, "CB", "g_vm->AttachCurrentThread");
//		}
//	}
//	else if (getEnvStat == JNI_OK)
//	{
//		__android_log_print(ANDROID_LOG_INFO, "CB", "getEnvStat == JNI_OK");
//	}
//	else if (getEnvStat == JNI_EVERSION)
//	{
//		__android_log_print(ANDROID_LOG_INFO, "CB", "getEnvStat == JNI_EVERSION");
//	}
//
//	__android_log_print(ANDROID_LOG_INFO, "CB", "CallStaticIntMethod begin");
//
//	g_env->CallStaticIntMethod(g_clazz, g_mid, (jint) enRegStatus);
//
//	__android_log_print(ANDROID_LOG_INFO, "CB", "CallStaticIntMethod end");
//
//	if (g_env->ExceptionCheck()) {
//	 g_env->ExceptionDescribe();
//	}
//
//	g_vm->DetachCurrentThread();

	JNIEnv *jenv=NULL;
	jmethodID sFunCallback = NULL;
	jclass wc;

	//jenv = jni_get_jni_env();

	 jint res = g_vm->AttachCurrentThread(&jenv, NULL);
	 if(jenv == NULL)
	 {
		 __android_log_print(ANDROID_LOG_INFO, "CB", "find JENV failed:%d"+res);
		 return;
	 }



	wc = jenv->FindClass("com/fsti/ladder/common/MyJavaCallback");
	//wc = jenv->FindClass("java/lang/String");

	jthrowable exc;
	exc = jenv->ExceptionOccurred();
	     if (exc) {
	         jclass newExcCls;
	         jenv->ExceptionDescribe();
	         jenv->ExceptionClear();

	     }

	 if (wc==0)
	{
		__android_log_print(ANDROID_LOG_INFO, "CB", "find class MyJavaCallback failed");
		return;
	}

	__android_log_print(ANDROID_LOG_INFO, "CB", "find class MyJavaCallback ok");

	if (sFunCallback == NULL)
	{
		sFunCallback = jenv->GetStaticMethodID(wc, "callbackByC", "(I)I");
		if (sFunCallback == NULL)
		{
			__android_log_print(ANDROID_LOG_INFO, "CB", "find static Method callbackByC failed");
			jenv->DeleteLocalRef(wc);
			return;
		}
		__android_log_print(ANDROID_LOG_INFO, "CB", "find static Method callbackByC ok");
	}

	jenv->CallStaticIntMethod(wc, sFunCallback, (jint) enRegStatus);

	__android_log_print(ANDROID_LOG_INFO, "CB", "IDBTJniRegStatusChanged");
}


void notifyUpdateApp()
{
	g_daemon.updateApp();
}

void notifySetSyncTime(const char *datetime)
{
	g_daemon.setSyncTime(datetime);
}

void notifySetDebugMode(int iType)
{
	g_daemon.setDebugMode(iType);
}

void notifyUpdateMac()
{
	g_daemon.updateMac();
}

#ifdef __cplusplus
}
#endif

