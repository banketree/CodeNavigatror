package com.fsti.ladder.common;

import com.fsti.ladder.engine.IDBTEngine;

import android.content.Intent;
import android.util.Log;

public class MyJavaCallback {
	public final static int STATE_IDLE = 0;// 0��ʾ����
	public final static int CALL_AUDIO_INCOMING_RECEIVED = 1; 	//�յ���������
	public final static int CALL_VIDEO_INCOMING_RECEIVED = 2; 	//�յ���Ƶ����
	public final static int OUTGOING_INIT = 3;					//������ʼ��
	public final static int OUTGOING_PROGRESS = 4;				//����������
	public final static int OUTGOING_RINGING = 5;				//������Է�����
	public final static int CALL_ERROR = 7;						//���д���
	public final static int CALL_OUTGOING_EARLYMEDIA = 8;		//�������յ��Է�����ʾ��
	public final static int CALL_CONNECTED = 9;					//������Է���ͨ
	public final static int CALL_END = 10;						//�Է��ҶϽ���
	public final static int SIP_REG_STATUS_OFFLINE = 11;		//SIPע��״̬Ϊ����
	public final static int SIP_REG_STATUS_ONLINE = 12;			//SIPע��״̬Ϊ����
	public final static int JNI_DEV_TM_STATUS_OFFLINE = 13;//
	public final static int JNI_DEV_TM_STATUS_ONLINE = 14;//
	public final static int JNI_DEV_NETWORK_STATUS_OFFLINE = 15;//
	public final static int JNI_DEV_NETWORK_STATUS_ONLINE = 16;//
	
	public final static int JNI_DEV_RFIDCARD_VALIDATE_STATUS_INVALID = 17;//ˢ����ʾ��Ч
    public final static int JNI_DEV_RFIDCARD_VALIDATA_STATUS_VALID = 18;//����Ч
    
    public final static int JNI_DEV_DTMF_OPENDOOR = 19;//�Զ˰�#����
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
	 * �������� : callbackByC �������� : ����˵����
	 * 
	 * @param nSig
	 *            0��ʾ���У�1��ʾͨ��״̬��2��ʾ���б�ȡ����3��ʾͨ�����Ҷϣ�4��ʾ��������Ƶ��أ�8��ʾSIP��ע�ᣬ9��ʾSIPδע��
	 *            ��
	 * @return ����ֵ�� int �޸ļ�¼�� ���ڣ�2013-1-22 ����11:09:38 �޸��ˣ������� ���� ��
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
