package com.fsti.ladder.common;

import com.fsti.ladder.engine.IDBTEngine;

import android.content.Intent;
import android.util.Log;

public class MyJavaCallback {
	public final static int STATE_IDLE = 0;// 0表示空闲
	public final static int CALL_AUDIO_INCOMING_RECEIVED = 1; 	//收到语音来电
	public final static int CALL_VIDEO_INCOMING_RECEIVED = 2; 	//收到视频来电
	public final static int OUTGOING_INIT = 3;					//呼出初始化
	public final static int OUTGOING_PROGRESS = 4;				//呼出进行中
	public final static int OUTGOING_RINGING = 5;				//呼出后对方振铃
	public final static int CALL_ERROR = 7;						//呼叫错误
	public final static int CALL_OUTGOING_EARLYMEDIA = 8;		//呼出后收到对方的提示音
	public final static int CALL_CONNECTED = 9;					//呼出后对方接通
	public final static int CALL_END = 10;						//对方挂断结束
	public final static int SIP_REG_STATUS_OFFLINE = 11;		//SIP注册状态为离线
	public final static int SIP_REG_STATUS_ONLINE = 12;			//SIP注册状态为在线
	public final static int JNI_DEV_TM_STATUS_OFFLINE = 13;//
	public final static int JNI_DEV_TM_STATUS_ONLINE = 14;//
	public final static int JNI_DEV_NETWORK_STATUS_OFFLINE = 15;//
	public final static int JNI_DEV_NETWORK_STATUS_ONLINE = 16;//
	
	public final static int JNI_DEV_RFIDCARD_VALIDATE_STATUS_INVALID = 17;//刷卡提示无效
    public final static int JNI_DEV_RFIDCARD_VALIDATA_STATUS_VALID = 18;//卡有效
    
    public final static int JNI_DEV_DTMF_OPENDOOR = 19;//对端按#开门
    public final static int JNI_DEV_ADVERT_UPDATE = 20;//	

    public final static int CALL_ERROR_ROOM_INVALID = 21;//	
    public final static int CALL_ERROR_ROOM_VALID = 22;//	
    
    public final static int JNI_DEV_REBOOT = 23;//	
    public final static int CONFIG_UPDATE_DEV_REBOOT = 24;//	
    public final static int JNI_ADV_DOWNLOAD = 25;//
    public final static int JNI_ANN_DOWNLOAD = 26;
    public final static int CALL_RING_STOP = 27;						
	/**
	 * 
	 * 函数名称 : callbackByC 功能描述 : 参数说明：
	 * 
	 * @param nSig
	 *            0表示空闲，1表示通话状态，2表示呼叫被取消，3表示通话被挂断，4表示正接受视频监控，8表示SIP已注册，9表示SIP未注册
	 *            。
	 * @return 返回值： int 修改记录： 日期：2013-1-22 上午11:09:38 修改人：林利澍 描述 ：
	 * 
	 */
	public static int callbackByC(int nSig) {
		Log.i("WINNY", "callbackByC in MyJavaCallback");
		Intent intent = new Intent(BVAction.ACTION_IDBT_STATE);
		intent.putExtra("state", nSig);
		Log.i("WINNY", "nSig:"+nSig);
		AndroidBVApp.getInstance().sendBroadcast(intent);
		return 0;
	}
	
	public static void callbackByCMethodChar(String str) 
	{
	    String[] infoValue = null;
		Log.i("WINNY", "callbackByCMethodChar in MyJavaCallback");
		Intent intent = new Intent(BVAction.ACTION_IDBT_STATE);
		infoValue = str.split("\\&");
		intent.putExtra("state",Integer.parseInt(infoValue[0]));
		Log.i("WINNY", "nSig:"+infoValue[0]);
		intent.putExtra("id", infoValue[1]);
		Log.i("WINNY", "nId:"+infoValue[1]);
		intent.putExtra("url", infoValue[2]);
		Log.i("WINNY", "nUrl:"+infoValue[2]);
		intent.putExtra("end", infoValue[3]);
		Log.i("WINNY", "nEnd:"+infoValue[3]);
		AndroidBVApp.getInstance().sendBroadcast(intent);
		return;
	}
}
