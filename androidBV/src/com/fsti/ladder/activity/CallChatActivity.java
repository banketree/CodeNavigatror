package com.fsti.ladder.activity;

import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Timer;
import java.util.TimerTask;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.AudioManager;
import android.media.Ringtone;
import android.media.RingtoneManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.KeyEvent;
import android.widget.TextView;
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
 * 版权所有 (c)2012, 邮科
 * <p>
 * 文件名称 ：ConverseActivity.java
 * <p>
 * 内容摘要 ：activity for talk with opposite side 作者 ：林利澍 创建时间 ：2012-12-31 下午1:12:25
 * 当前版本号：v1.0 历史记录 : 日期 : 2012-12-31 下午1:12:25 修改人： 描述 :
 */
public class CallChatActivity extends Activity {
	
	private Ringtone mRingTone = null;
	private Timer mTimerRing;
	private static final SimpleDateFormat sDurationTimerFormat = new SimpleDateFormat(
			"mm:ss");
	private TextView tvRoomNum;// call room num
	private TextView tvCallChatStatus = null;// call chat status
	private TextView tvCallChatTimeConsum;// call chat time consum
	private TextView tvOperationRemind;// operation hint message
	private BroadcastReceiver mBroadcastReceiver;
	private Handler mHandler = null;
	private Timer countTimer = null;
	long startTime = 0;
//	private SoundManager soundMgr;
	private AlertSound alertRinging;
	private final static int CLOSE_CALL_CHAT = 99;
	private AdvertApi advert = null;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		initBroadcastReceiver();
		setContentView(R.layout.activity_call_chat);
		initView();
		alertRinging = new AlertSound(getBaseContext());
		initHandler();
		advert = new AdvertApi(this);
		mHandler.post(mUpdateResults); 
	}
	
    final Runnable mUpdateResults = new Runnable() {
        public void run() {
    		alertRinging.setResource(R.raw.ringing);
    		alertRinging.setLooping(true);
    		alertRinging.starSound();
        	String advertPaths = FileUtils.PACKAGE_PATH + FileUtils.ADVERT_FILE
    				+ File.separator + FileUtils.ADVERT_IN_CALL;
        	advert.setAdvertViewFlow(advertPaths, 1);
        }
    };
	
	@Override
	protected void onStop() {
		// TODO Auto-generated method stub
		if(advert !=null)
    	{
        	advert.stopAutoFlow();
        	advert = null;
    	}
		uninitAlertSound();
		uninitBroadcastReceiver();
		super.onStop();
	}

	private void uninitAlertSound() {
		if (alertRinging != null) {
			alertRinging.setLooping(false);
			//alertRinging.stopSound();
			stopRing();
			alertRinging.releaseSound();
			alertRinging = null;
		}
	}

	private void initView() {
		tvCallChatStatus = (TextView) findViewById(R.id.tvCallChatStatus);
		tvCallChatTimeConsum = (TextView) findViewById(R.id.tvCallChatTimeConsum);
		tvOperationRemind = (TextView) findViewById(R.id.tvOperationRemind);
		String roomnum = getIntent().getStringExtra("roomnum");
		tvRoomNum = (TextView) findViewById(R.id.tvRoomNum);
		tvRoomNum.setText(roomnum);
		// int result = IDBTEngine.sendHouseCode(roomnum);

	}

	private void initBroadcastReceiver() {
		mBroadcastReceiver = new BroadcastReceiver() {
			@Override
			public void onReceive(Context context, Intent intent) {
				// TODO Auto-generated method stub

				String action = intent.getAction();
				LogUtils.printLog("action:" + action);

				if (action.equals(BVAction.ACTION_IDBT_STATE)) {
					int state = intent.getIntExtra("state",
							Context.MODE_PRIVATE);
					System.out.print("Get From Jni ,state:" + state + "\n");
					LogUtils.printLog("Get From Jni ,state:" + state + "\n");
					switch (state) {
					case MyJavaCallback.OUTGOING_INIT:
						break;
					case MyJavaCallback.OUTGOING_PROGRESS:// 呼出进行中
						tvCallChatStatus.setText(R.string.call_in_progress);
						break;
					case MyJavaCallback.OUTGOING_RINGING:
						break;
					case MyJavaCallback.CALL_ERROR_ROOM_INVALID:
						// tvCallChatStatus.setText(R.string.call_error);
						// mHandler.sendEmptyMessageDelayed(CLOSE_CALL_CHAT,
						// 3000);
						// showToast("对不起，该住户暂未开通此业务");
						// alertHouseCodeInvail.starSound();
						break;
					case MyJavaCallback.CALL_ERROR:
						tvCallChatStatus.setText(R.string.call_error);
						if(alertRinging != null)
	                	{
	                		if(alertRinging.isHaveResource() == true)
	                    	{
								alertRinging.setLooping(false);
								//alertRinging.stopSound();
								stopRing();
	                    	}
	                	}
						//stopPlayRing();
						mHandler.sendEmptyMessageDelayed(CLOSE_CALL_CHAT, 3000);
						break;
					case MyJavaCallback.CALL_OUTGOING_EARLYMEDIA:
						break;
					case MyJavaCallback.CALL_CONNECTED:
						if(alertRinging != null)
	                	{
	                		if(alertRinging.isHaveResource() == true)
	                    	{
								alertRinging.setLooping(false);
								//alertRinging.stopSound();
								stopRing();
	                    	}
	                	}
						//stopPlayRing();
						countTimer = new Timer();
						startTime = new Date().getTime();
						// 读取最大通话时间
						System.out.print("\nGet From Jni ,timeout = "
								+ IDBTEngine.getImsCallTimeout() + "\n");
						startTime += IDBTEngine.getImsCallTimeout() * 1000;
						countTimer.schedule(mTimerTaskInCall, 0, 1000);
						tvCallChatStatus.setText(R.string.chat_in_progress);
						break;
					case MyJavaCallback.CALL_RING_STOP:
						System.out.print("stopSound1 try\n");
						if(alertRinging != null)
	                	{
	                		if(alertRinging.isHaveResource() == true)
	                    	{
								//alertRinging.setLooping(false);
								//alertRinging.stopSound();
								//stopRing();
								pauseAndRestartRing();
	                    	}
	                	}
						System.out.print("stopSound1 ok\n");

						break;
					case MyJavaCallback.CALL_END:
						tvCallChatStatus.setText(R.string.end_up_call);
						if(alertRinging != null)
	                	{
	                		if(alertRinging.isHaveResource() == true)
	                    	{
								alertRinging.setLooping(false);
								//alertRinging.stopSound();
								stopRing();
	                    	}
	                	}
						//stopPlayRing();
						mHandler.sendEmptyMessageDelayed(CLOSE_CALL_CHAT, 3000);
						break;
					default:
						break;
					}
				}
			}
		};
		IntentFilter intentFliter = new IntentFilter();
		intentFliter.addAction(BVAction.ACTION_IDBT_STATE);
		registerReceiver(mBroadcastReceiver, intentFliter);
	}

	private void showToast(String text) {
		Toast.makeText(this, text, Toast.LENGTH_SHORT).show();
	}

	private void uninitBroadcastReceiver() {
		if (mBroadcastReceiver != null) {
			unregisterReceiver(mBroadcastReceiver);
			mBroadcastReceiver = null;
		}
	}

	private void initHandler() {
		mHandler = new Handler() {
			@Override
			public void handleMessage(Message msg) {
				// TODO Auto-generated method stub
				super.handleMessage(msg);
				switch (msg.what) {
				case CLOSE_CALL_CHAT:
					if (countTimer != null) {
						countTimer.cancel();
						countTimer = null;
					} 
					finish();
					break;
				default:
					break;
				}
			}
		};
	}

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		// TODO Auto-generated method stub
		if (keyCode == KeyEvent.KEYCODE_BACK && event.getRepeatCount() == 0) {
			LogUtils.printLog("KEYCODE_BACK");
			return true;
		} else if (keyCode == KeyEvent.KEYCODE_DEL
				&& event.getRepeatCount() == 0) {
			LogUtils.printLog("KEYCODE_DEL");
			if(alertRinging != null)
        	{
        		if(alertRinging.isHaveResource() == true)
            	{
					alertRinging.setLooping(false);
					//alertRinging.stopSound();
					stopRing();
            	}
        	}
			//stopPlayRing();
			IDBTEngine.cancelCall();// cancel call
			finish();
			return true;
		}
		return super.onKeyDown(keyCode, event);
	}

	private final TimerTask mTimerTaskInCall = new TimerTask() {
		@Override
		public void run() {
			// TODO Auto-generated method stub
			// final long duration = new Date().getTime() - startTime;
			final long duration = startTime - new Date().getTime();
			if (duration < 0) {
				return;
			}
			CallChatActivity.this.runOnUiThread(new Runnable() {
				@Override
				public void run() {
					// TODO Auto-generated method stub
					Date date = new Date(duration);
					tvCallChatTimeConsum.setText(sDurationTimerFormat
							.format(date));
				}
			});
		}
	};
	

	
	private void pauseAndRestartRing() {

		alertRinging.pause();

		if (mTimerRing == null) {
			mTimerRing = new Timer();
			mTimerRing.schedule(new TimerTaskPauseAndRestartRing(), 250);
		}

	}

	
	private void stopRing() {

		if (mTimerRing != null) {
			mTimerRing.cancel();
			mTimerRing = null;
		}

		alertRinging.stopSound();
	}

	
	
	private class TimerTaskPauseAndRestartRing extends TimerTask {
		@Override
		public void run() {
			alertRinging.starSound();
		}
	};
	
}
