package com.fsti.ladder.engine;

import com.fsti.ladder.tools.LogUtils;

/**
 * 
 *	��Ȩ����  (c)2012,   �ʿ�<p>	
 *  �ļ�����	��IDBTEngine.java<p>
 *
 *  ����ժҪ	��<p>
 *
 *  ����	��������
 *  ����ʱ��	��2013-1-4 ����8:49:03 
 *  ��ǰ�汾�ţ�v1.0
 *  ��ʷ��¼	:
 *  ����	: 2013-1-4 ����8:49:03 	�޸��ˣ�
 *  ����	:
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
	 *  �������� : getRegStatus
	 *  �������� :  get the sip register status
	 *  ����˵����
	 *  ����ֵ��
	 *  	int
	 *  �޸ļ�¼��
	 *  ���ڣ�2013-1-4 ����8:50:47	�޸��ˣ�������
	 *  ����	��
	 *
	 */
	public native static int getSipRegStatus();
	
	/**
	 * 
	 *  �������� : getNetwrokStatus
	 *  �������� :  get the network status
	 *  ����˵����
	 *  ����ֵ��
	 *  	int
	 *  �޸ļ�¼��
	 *  ���ڣ�2013-1-4 ����8:50:47	�޸��ˣ�������
	 *  ����	��
	 *
	 */
	public native static int getNetwrokStatus();
	
	/**
	 * 
	 *  �������� : getTmLinkStatus
	 *  �������� :  get the TM link status
	 *  ����˵����
	 *  ����ֵ��
	 *  	int
	 *  �޸ļ�¼��
	 *  ���ڣ�2013-1-4 ����8:50:47	�޸��ˣ�������
	 *  ����	��
	 *
	 */
	public native static int getTmLinkStatus();
	
	/**
	 * 
	 *  �������� : sendHouseCode
	 *  �������� :  input roomnum to call
	 *  ����˵����
	 *  	@param roomNum   room number		+"#"
	 *  	@return	0: room exist		| -1:room	inexistence
	 *  ����ֵ��
	 *  	int
	 *  �޸ļ�¼��
	 *  ���ڣ�2013-1-4 ����8:50:47	�޸��ˣ�������
	 *  ����	��
	 *
	 */
	public native static int sendHouseCode(String roomNum) ; 

	
	public native static int checkImsCallEnable() ; 
	public native static int getImsCallTimeout() ; 
	
	
	
	
	/**
	 * 
	 *  �������� : cancelCall
	 *  �������� :  cancel user call
	 *  ����˵����
	 *  ����ֵ��
	 *  	void
	 *  �޸ļ�¼��
	 *  ���ڣ�2013-1-4 ����8:51:43	�޸��ˣ�������
	 *  ����	��
	 *
	 */
	public static native void cancelCall();
	
	/**
	 * 
	 *  �������� : hangupCall
	 *  �������� :  hang up the call on call
	 *  ����˵����
	 *  ����ֵ��
	 *  	void
	 *  �޸ļ�¼��
	 *  ���ڣ�2013-1-4 ����8:55:06	�޸��ˣ�������
	 *  ����	��
	 *
	 */
	public static native void hangupCall();
	
	/**
	 * 
	 *  �������� : sendOpenDoorPassword
	 *  �������� :  input right password to open door 
	 *  ����˵����
	 *  	@param password
	 *  	@return
	 *  ����ֵ��
	 *  	int	0: send message to open door success	|	-1:password wrong
	 *  �޸ļ�¼��
	 *  ���ڣ�2013-1-4 ����8:58:49	�޸��ˣ�������
	 *  ����	��
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
	 *  �������� : createIDBTQue
	 *  �������� :  create message queue to receive message from IDBT
	 *  ����˵����
	 *  	@param arg the maximum message count 
	 *  	@return
	 *  ����ֵ��
	 *  	int	0:create success	|	-1:create fail
	 *  �޸ļ�¼��
	 *  ���ڣ�2013-1-4 ����9:02:23	�޸��ˣ�������
	 *  ����	��
	 *
	 */
	public static native int createIDBTQue(int arg);
	
	
	/**
	 * 
	 *  �������� : IDBTInit
	 *  �������� :  start IDBT module
	 *  ����˵����
	 *  	@return
	 *  ����ֵ��
	 *  	int	0:start success	|	-1:start fail
	 *  �޸ļ�¼��
	 *  ���ڣ�2013-1-4 ����9:09:39	�޸��ˣ�������
	 *  ����	��
	 *
	 */
	public static native int IDBTInit();
	
	
	/**
	 * 
	 *  �������� : deleteIDBTQue
	 *  �������� :  delete IDBT message queue when the service stop
	 *  ����˵����
	 *  	@return
	 *  ����ֵ��
	 *  	int	0:delete success	|	-1:delete fail
	 *  �޸ļ�¼��
	 *  ���ڣ�2013-1-4 ����9:12:04	�޸��ˣ�������
	 *  ����	��
	 *
	 */
	public static native int deleteIDBTQue();
	
	
	/**
	 * 
	 *  �������� : listenIDBTState
	 *  �������� :  
	 *  ����˵����
	 *  	@param interval
	 *  	@return
	 *  ����ֵ��
	 *  	int	0:free	|	1:
	 *  �޸ļ�¼��
	 *  ���ڣ�2013-1-4 ����9:13:50	�޸��ˣ�������
	 *  ����	��
	 *
	 */
	public static native int listenIDBTState(int interval);
	
	
	/**
	 * 
	 *  �������� : listenIDBTInit
	 *  �������� :  ��ʼ��
	 *  ����˵����
	 *  	@return
	 *  ����ֵ��
	 *  	int
	 *  �޸ļ�¼��
	 *  ���ڣ�2013-1-8 ����9:52:21	�޸��ˣ�������
	 *  ����	��
	 *
	 */
	public static native int listenIDBTInit();
}
