package com.fsti.ladder.activity;

import java.io.File;

import org.apache.http.entity.ContentProducer;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.ContentProvider;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.KeyEvent;
import android.view.WindowManager;
import android.widget.Toast;
import com.fsti.ladder.R;
import com.fsti.ladder.common.AdvertApi;
import com.fsti.ladder.common.AlertSound;
import com.fsti.ladder.common.BVAction;
import com.fsti.ladder.common.MyJavaCallback;
import com.fsti.ladder.common.SoundManager;
import com.fsti.ladder.engine.IDBTEngine;
import com.fsti.ladder.tools.FileUtils;
import com.fsti.ladder.tools.LogUtils;

/**
 * ��Ȩ���� (c)2012, �ʿ�
 * <p>
 * �ļ����� ��AdvertActivity.java
 * <p>
 * ����ժҪ ��show advertisement to visitor ���� �������� ����ʱ�� ��2012-12-31 ����12:52:15
 * ��ǰ�汾�ţ�v1.0 ��ʷ��¼ : ���� : 2012-12-31 ����12:52:15 �޸��ˣ� ���� :
 */
public class AdvertActivity extends Activity {
    private boolean isKeyDown = true;
    private AdvertApi advert = null;
    private AlertSound alertImsCallUnable = null;
    private AlertSound alertReboot = null;
    private AlertSound alertCenterInvail = null;
    private BroadcastReceiver mBroadcastReceiver;
    private Handler mHandler = null;
    private final static int INIT_ALERT_SOUND = 100;
    private final static int SET_ADVERT_VIEWFLOW = 101;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // TODO Auto-generated method stub
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        setContentView(R.layout.activity_advert);
//        mHandler = new Handler();
        advert = new AdvertApi(this);
    }
   
    @Override
    protected void onStart() {
        // TODO Auto-generated method stub
        super.onStart();
        initBroadcastReceiver();
	   	 if(mHandler == null)
	   	 {
	   		 LogUtils.printLog("AdvertActivity onStart Thread run mHandler == null");
	   		 mHandler = new Handler(){
	   			@Override
				public void handleMessage(Message msg) {
					// TODO Auto-generated method stub
					super.handleMessage(msg);
					switch (msg.what) {
					case INIT_ALERT_SOUND:
		                 alertImsCallUnable = new AlertSound(getBaseContext());
		                 alertReboot = new AlertSound(getBaseContext());
		         		 alertCenterInvail = new AlertSound(getBaseContext());
						break;
					default:
						break;
					}
				}
	   		 };
	   	 }
	   	mHandler.sendEmptyMessage(INIT_ALERT_SOUND);
    	 mHandler.post(mUpdateResults);  
    }
    
    final Runnable mUpdateResults = new Runnable() {
        public void run() {
            String standbyAdvertPath = FileUtils.PACKAGE_PATH + FileUtils.ADVERT_FILE + File.separator + FileUtils.ADVERT_IN_STANDBY;
            advert.setAdvertViewFlow(standbyAdvertPath,0);   
        }
    };

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        // TODO Auto-generated method stub
        if (isKeyDown == true && keyCode != KeyEvent.KEYCODE_BACK && event.getRepeatCount() == 0 ) {
            Intent intent = null;
            if(keyCode == KeyEvent.KEYCODE_M){
//                if(IDBTEngine.getSipRegStatus() == MyJavaCallback.SIP_REG_STATUS_ONLINE){
//                	int imsCallState = IDBTEngine.checkImsCallEnable();
//                	if(imsCallState == 0)
//                	{
                		int result = IDBTEngine.sendHouseCode(IDBTEngine.PROPERTY_MANAGEMENT_NUM);
                        LogUtils.printLog("try to sendHouseCode��" + result);
//                	}
//                	else
//                	{
//                		Toast.makeText(this, "���й����ѱ�����ʹ�ã�����ϵ����Ա", Toast.LENGTH_SHORT).show();
//                		if(alertImsCallUnable != null)
//                		{
//                			if(alertImsCallUnable.isHaveResource() == false)
//                			{
//                    	        alertImsCallUnable.setResource(R.raw.call_unable);][
//                			}
//                    		alertImsCallUnable.starSound();
//                		}
//                	}
//                } else {
//                    Toast.makeText(this, R.string.sip_offline,Toast.LENGTH_SHORT).show();                    
//                }
            }else if(keyCode == KeyEvent.KEYCODE_V && event.getAction() == KeyEvent.ACTION_UP){
                isKeyDown = false;
                intent = new Intent(this, InputNumberActivity.class);
                intent.putExtra(BVAction.KEY_OPEN_PWD, true);
                startActivity(intent);
            }else if (isKeyDown ==true && keyCode > 6 && keyCode < 17 ) {
                isKeyDown = false;
                intent = new Intent(this, InputNumberActivity.class);
                intent.putExtra(BVAction.KEY_FIGURE, keyCode - 7);
                startActivity(intent);
            }else{
                intent = new Intent(this, InputNumberActivity.class);
                isKeyDown = false;
                startActivity(intent);
            }             
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    protected void onResume() {
        // TODO Auto-generated method stub
        isKeyDown = true;
        super.onResume();
    }
    @Override
    protected void onDestroy() {
        // TODO Auto-generated method stub
//    	mHandler = null;
//        advert = null;
        super.onDestroy();
    }
    
    private void uninitAlertSound() {
        if (alertImsCallUnable != null) {
        	alertImsCallUnable.releaseSound();
        	alertImsCallUnable = null;
        }
        if (alertReboot != null) {
        	alertReboot.releaseSound();
        	alertReboot = null;
        }
        if(alertCenterInvail != null)
        {
        	alertCenterInvail.releaseSound();
        	alertCenterInvail = null;
        }
    }

    @Override
    protected void onStop() {
    	// TODO �Զ����ɵķ������
    	unitBroadcastReceiver();
    	if(advert !=null)
    	{
        	advert.stopAutoFlow();
    	}
    	uninitAlertSound();
    	super.onStop();
    }
    /**
     * 
     *  @function : initBroadcastReceiver ע��㲥���չ�����¼�
     *  @param��
     *  @return��
     *  	void
     *  @history��
     *  @time��2013-3-18 ����10:19:43	modifier��
     *  @describe	��
     *
     */
    private void initBroadcastReceiver(){
        mBroadcastReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                // TODO Auto-generated method stub
                final String action = intent.getAction();
                if(action.equals(BVAction.ACTION_ADVERT_UPDATE)){
                    String standbyAdvertPath = FileUtils.PACKAGE_PATH + FileUtils.ADVERT_FILE + File.separator + FileUtils.ADVERT_IN_STANDBY;
                    advert.setAdvertViewFlow(standbyAdvertPath,0);  
                }
                else if (action.equals(BVAction.ACTION_IDBT_STATE)) {
                    int state = intent.getIntExtra(BVAction.KEY_STATE, Context.MODE_PRIVATE);
                    switch (state) {
                        case MyJavaCallback.CALL_ERROR_ROOM_INVALID:
                        	LogUtils.printLog("AdvertActivity initBroadcastReceiver MyJavaCallback.CALL_ERROR_ROOM_INVALID");
                        	if(isKeyDown == true)
                        	{
	                            Toast.makeText(AdvertActivity.this, "��ҵ���ĺ���δ��ͨ,����ϵ����Ա", Toast.LENGTH_SHORT).show();
	                            if(alertCenterInvail != null)
	                            {
	                                if(alertCenterInvail.isHaveResource() == false)
	                                {
	                                    alertCenterInvail.setResource(R.raw.management_center);
	                                }
	                                alertCenterInvail.starSound();
	                            }
                        	}
                        	break;
                        case MyJavaCallback.CALL_ERROR_ROOM_VALID:
                        	if(isKeyDown == true)
                        	{
                        		Intent intentCallChat = new Intent(AdvertActivity.this, CallChatActivity.class);
                            	intentCallChat.putExtra("roomnum",IDBTEngine.PROPERTY_MANAGEMENT_NUM);
                                startActivity(intentCallChat);
                        	}
                        	break;
                        case MyJavaCallback.JNI_DEV_REBOOT:
                        	if(alertReboot != null)
                        	{
                                alertReboot.setResource(R.raw.reboot);
                            	alertReboot.starSound();
                        	}
                        	Toast.makeText(AdvertActivity.this, "ϵͳ��������", Toast.LENGTH_LONG).show();
                        	Toast.makeText(AdvertActivity.this, "ϵͳ��������", Toast.LENGTH_LONG).show();
                        	Toast.makeText(AdvertActivity.this, "ϵͳ��������", Toast.LENGTH_LONG).show();
                        	break;     	
                        case MyJavaCallback.CONFIG_UPDATE_DEV_REBOOT:
                        	if(alertReboot != null)
                        	{
                                alertReboot.setResource(R.raw.config_reboot);
                            	alertReboot.starSound();
                        	}
                        	Toast.makeText(AdvertActivity.this, "�����ļ��Ѹ���,��������", Toast.LENGTH_LONG).show();
                        	Toast.makeText(AdvertActivity.this, "�����ļ��Ѹ���,��������", Toast.LENGTH_LONG).show();
                        	Toast.makeText(AdvertActivity.this, "�����ļ��Ѹ���,��������", Toast.LENGTH_LONG).show();
                        	break;
                        default :
                            break;
                    }
                }
            }
        };
        IntentFilter mFilter = new IntentFilter();
        mFilter.addAction(BVAction.ACTION_ADVERT_UPDATE);
        mFilter.addAction(BVAction.ACTION_IDBT_STATE);
        registerReceiver(mBroadcastReceiver, mFilter);
    }
    /**
     * 
     *  �������� : unitBroadcastReceiver
     *  �������� :  ע���㲥������
     *  @param��
     *  @return��
     *  	void
     *  �޸ļ�¼��
     *  ���ڣ�2013-3-18 ����10:06:52	�޸��ˣ�������
     *  ����	��
     *
     */
    private void unitBroadcastReceiver(){
        if(mBroadcastReceiver != null){
            unregisterReceiver(mBroadcastReceiver);
            mBroadcastReceiver = null;
        }
    }
}
