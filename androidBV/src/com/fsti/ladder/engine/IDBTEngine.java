package com.fsti.ladder.engine;

import com.fsti.ladder.tools.LogUtils;

/**
 * 
 *	版权所有  (c)2012,   邮科<p>	
 *  文件名称	：IDBTEngine.java<p>
 *
 *  内容摘要	：<p>
 *
 *  作者	：林利澍
 *  创建时间	：2013-1-4 上午8:49:03 
 *  当前版本号：v1.0
 *  历史记录	:
 *  日期	: 2013-1-4 上午8:49:03 	修改人：
 *  描述	:
 */
public class IDBTEngine {
	private final static String LIB_SO_PATH = "IDBT";
	public final static int IDBT_OK = 0;//return value : 0 mean success
	public final static int IDBT_FAIL = -1;//return value : -1 mean fail	
	public final static String PROPERTY_MANAGEMENT_NUM = "9999";
	static{
		try{
			System.loadLibrary(LIB_SO_PATH);
//		    System.load("/data/data/com.fsti.ladder/lib/libIDBT.so");
		}catch (UnsatisfiedLinkError e) {
			LogUtils.printLog("could not load libray");
		}
	}
	
	/**
	 * 
	 *  函数名称 : getRegStatus
	 *  功能描述 :  get the sip register status
	 *  参数说明：
	 *  返回值：
	 *  	int
	 *  修改记录：
	 *  日期：2013-1-4 上午8:50:47	修改人：林利澍
	 *  描述	：
	 *
	 */
	public native static int getSipRegStatus();
	
	/**
	 * 
	 *  函数名称 : getNetwrokStatus
	 *  功能描述 :  get the network status
	 *  参数说明：
	 *  返回值：
	 *  	int
	 *  修改记录：
	 *  日期：2013-1-4 上午8:50:47	修改人：林利澍
	 *  描述	：
	 *
	 */
	public native static int getNetwrokStatus();
	
	/**
	 * 
	 *  函数名称 : getTmLinkStatus
	 *  功能描述 :  get the TM link status
	 *  参数说明：
	 *  返回值：
	 *  	int
	 *  修改记录：
	 *  日期：2013-1-4 上午8:50:47	修改人：林利澍
	 *  描述	：
	 *
	 */
	public native static int getTmLinkStatus();
	
	/**
	 * 
	 *  函数名称 : sendHouseCode
	 *  功能描述 :  input roomnum to call
	 *  参数说明：
	 *  	@param roomNum   room number		+"#"
	 *  	@return	0: room exist		| -1:room	inexistence
	 *  返回值：
	 *  	int
	 *  修改记录：
	 *  日期：2013-1-4 上午8:50:47	修改人：林利澍
	 *  描述	：
	 *
	 */
	public native static int sendHouseCode(String roomNum) ; 

	
	public native static int checkImsCallEnable() ; 
	public native static int getImsCallTimeout() ; 
	
	
	
	
	/**
	 * 
	 *  函数名称 : cancelCall
	 *  功能描述 :  cancel user call
	 *  参数说明：
	 *  返回值：
	 *  	void
	 *  修改记录：
	 *  日期：2013-1-4 上午8:51:43	修改人：林利澍
	 *  描述	：
	 *
	 */
	public static native void cancelCall();
	
	/**
	 * 
	 *  函数名称 : hangupCall
	 *  功能描述 :  hang up the call on call
	 *  参数说明：
	 *  返回值：
	 *  	void
	 *  修改记录：
	 *  日期：2013-1-4 上午8:55:06	修改人：林利澍
	 *  描述	：
	 *
	 */
	public static native void hangupCall();
	
	/**
	 * 
	 *  函数名称 : sendOpenDoorPassword
	 *  功能描述 :  input right password to open door 
	 *  参数说明：
	 *  	@param password
	 *  	@return
	 *  返回值：
	 *  	int	0: send message to open door success	|	-1:password wrong
	 *  修改记录：
	 *  日期：2013-1-4 上午8:58:49	修改人：林利澍
	 *  描述	：
	 *
	 */
	public static native int sendOpenDoorPassword(String pwd);
	
	public static native String getDevInfo();
	
	public static native String getVersionInfo();
	
	public static native String getNetworkInfo();
	
	public static native String getNetworkJoinMethod();
	
	public static native String getDevicePosition();
	
	public static native String getNetMonitorInfo();
	
	public static native int getVedioQuality();
	
	public static native int getVideoFormat();
	public static native int getNegotiateMode();
	
	public static native int getVedioRotation();
	
	public static native int setBlockNum(String blockNum);
	
	public static native int setUnitNum(String unitNum);
	
	public static native int setNetworkJoinMethod(int selectMethod);
	
	public static native int setIPAddress(String address);
	
	public static native int setNetMask(String netmask);
	
	public static native int setGateway(String gateway);
	
	public static native int setDNS(String dns);
	
	public static native int setDebugMode(String type);
	
	public static native int setNetMode(String type);
	
	public static native int setVideoFormat(String type);
	
	public static native int setNegotiateMode(String type);
	
	public static native int setPPPOEUsername(String username);

	public static native int setPPPOEPassword(String password);
	
	public static native String getPlatformInfo();
	
	public static native int setDevNum(String devNum);
	
	public static native int setRotationValue(String value);
	
	public static native int setPlatformDomainName(String domainName);
	
	public static native int setI5PlatformDomainName(String domainName);
	
	public static native int setPlatformIP(String ip);
	
	public static native int setPlatformPort(String port);
	
	public static native int setPlatformUserName(String userName);
	
	public static native int setPlatformPassword(String password);
	
	public static native int setMonitorIP(String IP);
	
	public static native int setMonitorPort(String port);
	
	public static native String getSIPInfo();
	
	public static native String getManagementPassword();
	
	public static native int setManagementPassword(String managementPassword);
	
	public static native int setVedioQuality(String strVedioQuality);
	
	public static native int setOpendoorPassword(String opendoorPassword);
	
	public static native int setResetDev();
	
	/**
	 * 
	 *  函数名称 : createIDBTQue
	 *  功能描述 :  create message queue to receive message from IDBT
	 *  参数说明：
	 *  	@param arg the maximum message count 
	 *  	@return
	 *  返回值：
	 *  	int	0:create success	|	-1:create fail
	 *  修改记录：
	 *  日期：2013-1-4 上午9:02:23	修改人：林利澍
	 *  描述	：
	 *
	 */
	public static native int createIDBTQue(int arg);
	
	
	/**
	 * 
	 *  函数名称 : IDBTInit
	 *  功能描述 :  start IDBT module
	 *  参数说明：
	 *  	@return
	 *  返回值：
	 *  	int	0:start success	|	-1:start fail
	 *  修改记录：
	 *  日期：2013-1-4 上午9:09:39	修改人：林利澍
	 *  描述	：
	 *
	 */
	public static native int IDBTInit();
	
	
	/**
	 * 
	 *  函数名称 : deleteIDBTQue
	 *  功能描述 :  delete IDBT message queue when the service stop
	 *  参数说明：
	 *  	@return
	 *  返回值：
	 *  	int	0:delete success	|	-1:delete fail
	 *  修改记录：
	 *  日期：2013-1-4 上午9:12:04	修改人：林利澍
	 *  描述	：
	 *
	 */
	public static native int deleteIDBTQue();
	
	
	/**
	 * 
	 *  函数名称 : listenIDBTState
	 *  功能描述 :  
	 *  参数说明：
	 *  	@param interval
	 *  	@return
	 *  返回值：
	 *  	int	0:free	|	1:
	 *  修改记录：
	 *  日期：2013-1-4 上午9:13:50	修改人：林利澍
	 *  描述	：
	 *
	 */
	public static native int listenIDBTState(int interval);
	
	
	/**
	 * 
	 *  函数名称 : listenIDBTInit
	 *  功能描述 :  初始化
	 *  参数说明：
	 *  	@return
	 *  返回值：
	 *  	int
	 *  修改记录：
	 *  日期：2013-1-8 上午9:52:21	修改人：林利澍
	 *  描述	：
	 *
	 */
	public static native int listenIDBTInit();
}
