package com.fsti.ladder.activity;

import java.io.File;
import java.util.Timer;
import java.util.TimerTask;
import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.InputFilter;
import android.text.InputFilter.LengthFilter;
import android.text.method.HideReturnsTransformationMethod;
import android.text.method.PasswordTransformationMethod;
import android.view.KeyEvent;
import android.view.View;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.widget.ImageView;
import android.widget.TextSwitcher;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.ViewSwitcher.ViewFactory;
import com.fsti.ladder.R;
import com.fsti.ladder.common.AlertSound;
import com.fsti.ladder.common.BVAction;
import com.fsti.ladder.common.MyJavaCallback;
import com.fsti.ladder.common.SoundManager;
import com.fsti.ladder.engine.IDBTEngine;
import com.fsti.ladder.service.IDBTService;
import com.fsti.ladder.tools.CopyUtils;
import com.fsti.ladder.tools.FileUtils;
import com.fsti.ladder.tools.LogUtils;
import com.fsti.ladder.widget.InputEditText;

/**
 * 版权所有 (c)2012, 邮科
 * <p>
 * 文件名称 ：InputNumberActivity.java
 * <p>
 * 内容摘要 ：activity for input number to call room 作者 ：林利澍 创建时间 ：2012-12-31
 * 下午2:00:22 当前版本号：v1.0 历史记录 : 日期 : 2012-12-31 下午2:00:22 修改人： 描述 :
 */
public class InputNumberActivity extends Activity implements ViewFactory{
    private TextSwitcher tvHintInputForWhat;
    private TextSwitcher tvOperationTips;
    private TextSwitcher tvDelTips;
    private InputEditText etRoomNum;// 输入房间号
    private ImageView ivSipNone;// sip注册状态
    private ImageView ivLinkNone;// 是否连接
    private ImageView ivNetworkNet;// 网络是否连接
    private BroadcastReceiver mReceiver;
    private Timer closeTimer;
    private BackToAdvertTask backToAdvertTask;
    private SoundManager soundMgr;
    private AlertSound alertImsCallUnable= null;
    private AlertSound alertHouseCodeInvail= null;
    private AlertSound alertReboot= null;
    private AlertSound alertCenterInvail= null;
    private AlertSound alertTouch= null;
    private boolean isOpenDoorByPwd = false;// 是否是密码开门
    private boolean isStartCall = false;
    private InputFilter[] pwdInputFilter = new InputFilter[]{new LengthFilter(10)};
    private InputFilter[] roomInputFilter = new InputFilter[]{new LengthFilter(4)};
    private String sRoomNum;
    private Handler mHandler = null;
    private boolean iSoundInit = false; 
    private static int iPressTimes = 0;
    private final static int INIT_ALERT_SOUND = 100;
    /**
     * RunType CallRoom:call resident by room number | OpenByPwd: open door by
     * password
     */
    enum RunType {
        CallRoom, OpenByPwd
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // TODO Auto-generated method stub
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_input_number);
        mHandler = new Handler(){
   			@Override
			public void handleMessage(Message msg) {
				// TODO Auto-generated method stub
				super.handleMessage(msg);
				switch (msg.what) {
				case INIT_ALERT_SOUND:
					 alertTouch = new AlertSound(getBaseContext());
					 alertImsCallUnable = new AlertSound(getBaseContext());
					 alertHouseCodeInvail = new AlertSound(getBaseContext());
					 alertCenterInvail = new AlertSound(getBaseContext());
					 alertReboot = new AlertSound(getBaseContext());
					 soundMgr = new SoundManager();
					 soundMgr.initSounds(getBaseContext());
					 soundMgr.addSound(1, R.raw.card_fail);
					 soundMgr.addSound(2, R.raw.card_succ);
					 soundMgr.addSound(3, R.raw.door_opened);
					break;
				default:
					break;
				}
			}
   		 };
        sRoomNum = new String(); 
        mHandler.post(mInitView); 
    }
    
    final Runnable mInitView = new Runnable() {
        public void run() {
            initView();
        }
    };
    
    final Runnable mUpdateResults = new Runnable() {
        public void run() {
            if (IDBTEngine.getSipRegStatus() == MyJavaCallback.SIP_REG_STATUS_ONLINE) {
//            	ivSipNone.setImageResource(R.drawable.img_sip_success);
            	ivSipNone.setVisibility(View.INVISIBLE);
			}
			 if (IDBTEngine.getTmLinkStatus() == MyJavaCallback.JNI_DEV_TM_STATUS_ONLINE) {
//				 ivLinkNone.setImageResource(R.drawable.img_link_sucess);
				 ivLinkNone.setVisibility(View.INVISIBLE);
				}
			 if (IDBTEngine.getNetwrokStatus() == MyJavaCallback.JNI_DEV_NETWORK_STATUS_ONLINE) {
//				 ivNetworkNet.setImageResource(R.drawable.img_network_sucess);
				 ivNetworkNet.setVisibility(View.INVISIBLE);
				}
        }
    };

    @Override
    protected void onStart() {
        // TODO Auto-generated method stub
    	mHandler.sendEmptyMessage(INIT_ALERT_SOUND);
 	 	 mHandler.post(mUpdateResults); 
       initBroadReceiver();
       super.onStart();
    }

    @Override
    protected void onResume() {
        // TODO Auto-generated method stub
        LogUtils.printLog("onResume startCloseTask");
        startCloseTask();
        isStartCall = false;
        iPressTimes = 0;
        super.onResume();
    }

    @Override
    protected void onPause() {
        // TODO Auto-generated method stub
        LogUtils.printLog("onPause stopCloseTask");
        stopCloseTask();
        super.onPause();
    }

    @Override
    protected void onStop() {
        // TODO Auto-generated method stub
        uninitBroadcastReceiver();
    	uninitAlertSound();
        soundMgr = null;
        super.onStop();
    }
    
    @Override
    protected void onDestroy() {
        // TODO Auto-generated method stub
//    	mHandler = null;
        sRoomNum = null; 
        super.onDestroy();
    }
//	private void initHandler() {
//		mHandler = new Handler() {
//			@Override
//			public void handleMessage(Message msg) {
//				// TODO Auto-generated method stub
//				super.handleMessage(msg);
//				switch (msg.what) {
//				case HANDLE_SIP_SUCCESS:
//					ivSipNone.setImageResource(R.drawable.img_sip_success);
//					break;
//				case HANDLE_LINK_SUCCESS:
//				    ivLinkNone.setImageResource(R.drawable.img_link_sucess);
//					break;
//				case HANDLE_NET_SUCCESS:
//				    ivNetworkNet.setImageResource(R.drawable.img_network_sucess);
//					break;
//				default:
//					break;
//				}
//			}
//		};
//	}
	
    private void uninitAlertSound() {
        if (alertImsCallUnable != null) {
        	alertImsCallUnable.releaseSound();
        	alertImsCallUnable = null;
        }
        if (alertHouseCodeInvail != null) {
        	alertHouseCodeInvail.releaseSound();
        	alertHouseCodeInvail = null;
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
        if(alertTouch != null)
        {
        	alertTouch.releaseSound();
        	alertTouch = null;
        }
    }

    /**
     * 函数名称 : initView 功能描述 : initial UI view 参数说明： 返回值： void 修改记录： 日期：2013-1-8
     * 下午3:11:43 修改人：林利澍 描述 ：
     */
    private void initView() {
        Animation in = AnimationUtils.loadAnimation(this,android.R.anim.fade_in);
        Animation out = AnimationUtils.loadAnimation(this,android.R.anim.fade_out);
        tvHintInputForWhat = (TextSwitcher) findViewById(R.id.tvHintInputForWhat);
        tvHintInputForWhat.setFactory(this);
        tvHintInputForWhat.setInAnimation(in);
        tvHintInputForWhat.setOutAnimation(out);
        tvOperationTips = (TextSwitcher) findViewById(R.id.tvOperationTips);
        tvOperationTips.setFactory(this);
        tvOperationTips.setInAnimation(in);
        tvOperationTips.setOutAnimation(out);
        tvDelTips = (TextSwitcher) findViewById(R.id.tvDelTips);
        tvDelTips.setFactory(this);
        tvDelTips.setInAnimation(in);
        tvDelTips.setOutAnimation(out);
        ivSipNone = (ImageView) findViewById(R.id.ivSipNone);
        ivLinkNone = (ImageView) findViewById(R.id.ivLinkNone);
        ivNetworkNet = (ImageView) findViewById(R.id.ivNetworkNet);
        etRoomNum = (InputEditText) findViewById(R.id.etRoomNum);
        int keyPassCode = getIntent().getIntExtra(BVAction.KEY_FIGURE, -1);
        isOpenDoorByPwd = getIntent().getBooleanExtra(BVAction.KEY_OPEN_PWD, false);
        LogUtils.printLog("isOpenDoorByPwd:" + isOpenDoorByPwd);
        setRunType(isOpenDoorByPwd);
        if (keyPassCode != -1) {
            etRoomNum.getEditableText().append(String.valueOf(keyPassCode));
        }
        etRoomNum.setOnKeyListener(new View.OnKeyListener() {
            @Override
            public boolean onKey(View v, int keyCode, KeyEvent event) {
                // TODO Auto-generated method stub
            	if(keyCode == KeyEvent.KEYCODE_0 && event.getRepeatCount() != 0){
            		return true;
            	}
                if (event.getAction() == KeyEvent.ACTION_UP) {
                	if(alertTouch != null)
                	{
                		if(alertTouch.isHaveResource() == false)
                    	{
           				 	alertTouch.setResource(R.raw.touch);
                    	}
                    	alertTouch.starSound();
                	}
                    LogUtils.printLog("onKey stopCloseTask	startCloseTask");
                    stopCloseTask(); // stop close task before onkey
                    startCloseTask();// start new close task onkey
                    if (keyCode == KeyEvent.KEYCODE_ENTER && event.getRepeatCount() == 0) {
                        String inputContent = getInputContent();
                        LogUtils.printLog("inputContent:" + inputContent);
                        if (isOpenDoorByPwd) {
                            String pwd = IDBTEngine.getManagementPassword();
                            LogUtils.printLog("password:" + pwd);
                            if (inputContent.equals(pwd)) {
                            	etRoomNum.getEditableText().clear();
                                Intent intent = new Intent(InputNumberActivity.this, ConfigSettingsActivity.class);
                                startActivity(intent);
                                isOpenDoorByPwd = false;
                                setRunType(isOpenDoorByPwd);
//                                finish();
                            } else {
                                runByInputMsg(RunType.OpenByPwd, inputContent);
                            }
                        } else {                            
                                runByInputMsg(RunType.CallRoom, inputContent);
                        }
                    } else if (keyCode == KeyEvent.KEYCODE_V && event.getRepeatCount() == 0) {
                        LogUtils.printLog("KEYCODE_V  click");
                        isOpenDoorByPwd = !isOpenDoorByPwd;
                        setRunType(isOpenDoorByPwd);
                    } else if (keyCode == KeyEvent.KEYCODE_M && event.getRepeatCount() == 0) {
                        runByInputMsg(RunType.CallRoom, IDBTEngine.PROPERTY_MANAGEMENT_NUM);
                    }
                }
                return false;
            }
        });
    }

    private void setRunType(boolean isOpenByPwd) {        
        if (isOpenByPwd) {
            isOpenDoorByPwd = true; 
            String please_input_password = getResources().getString(R.string.please_input_password);
            String hint_pwd_to_open_door = getResources().getString(R.string.hint_pwd_to_open_door);
            String hint_del_figure_click_star = getResources().getString(R.string.hint_del_figure_click_star);
            tvHintInputForWhat.setText(please_input_password);
            tvOperationTips.setText(hint_pwd_to_open_door);
            tvDelTips.setText(hint_del_figure_click_star);
            etRoomNum.setTransformationMethod(PasswordTransformationMethod.getInstance());
            etRoomNum.setFilters(pwdInputFilter);
            etRoomNum.getEditableText().clear();
        } else {
            isOpenDoorByPwd = false;
            String hint_room_num_to_call = getResources().getString(R.string.hint_room_num_to_call);
            String hint_call_room_host = getResources().getString(R.string.hint_call_room_host);
            String hint_del_figure_or_cancle_call = getResources().getString(R.string.hint_del_figure_or_cancle_call);
            LogUtils.printLog("tvHintInputForWhat:"+tvHintInputForWhat);
            LogUtils.printLog("hint_room_num_to_call:"+hint_room_num_to_call);
            tvHintInputForWhat.setText(hint_room_num_to_call);
            tvOperationTips.setText(hint_call_room_host);
            tvDelTips.setText(hint_del_figure_or_cancle_call);
            etRoomNum.setTransformationMethod(HideReturnsTransformationMethod.getInstance());
            etRoomNum.setFilters(roomInputFilter);
            etRoomNum.getEditableText().clear();
        }
    }

    private void initBroadReceiver() {
        mReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                // TODO Auto-generated method stub
                final String action = intent.getAction();
                LogUtils.printLog("action:" + action);
                if (action.equals(BVAction.ACTION_IDBT_STATE)) {
                    int state = intent.getIntExtra(BVAction.KEY_STATE, Context.MODE_PRIVATE);
                    switch (state) {
                        case MyJavaCallback.SIP_REG_STATUS_ONLINE :
//                            ivSipNone.setImageResource(R.drawable.img_sip_success);
                        	ivSipNone.setVisibility(View.INVISIBLE);
                            break;
                        case MyJavaCallback.SIP_REG_STATUS_OFFLINE :
                        	ivSipNone.setVisibility(View.VISIBLE);
                            ivSipNone.setImageResource(R.drawable.img_sip_none);
                            break;
                        case MyJavaCallback.JNI_DEV_NETWORK_STATUS_ONLINE :
//                            ivNetworkNet.setImageResource(R.drawable.img_network_sucess);
                        	ivNetworkNet.setVisibility(View.INVISIBLE);
                            break;
                        case MyJavaCallback.JNI_DEV_NETWORK_STATUS_OFFLINE :
                        	ivNetworkNet.setVisibility(View.VISIBLE);
                            ivNetworkNet.setImageResource(R.drawable.img_network_none);
                            break;
                        case MyJavaCallback.JNI_DEV_TM_STATUS_ONLINE :
//                          ivNetworkNet.setImageResource(R.drawable.img_network_sucess);
                        	ivLinkNone.setVisibility(View.INVISIBLE);
                            break;
	                    case MyJavaCallback.JNI_DEV_TM_STATUS_OFFLINE :
	                    	ivLinkNone.setVisibility(View.VISIBLE);
	                    	ivLinkNone.setImageResource(R.drawable.img_link_none);
	                        break;
                        case MyJavaCallback.CALL_ERROR_ROOM_INVALID:
                        	if(sRoomNum.equals("9999"))
                        	{
                        		showToast("物业中心号码未开通,请联系管理员");
                        		if(alertCenterInvail != null)
                        		{
                            		if(alertCenterInvail.isHaveResource() == false)
                            		{
                                		alertCenterInvail.setResource(R.raw.management_center);
                            		}
                            		alertCenterInvail.starSound();
                        		}
                        	}
                        	else
                        	{
                                showToast("对不起，该住户暂未开通此业务");
                                if(alertHouseCodeInvail != null)
                                {
                                    if(alertHouseCodeInvail.isHaveResource() == false)
                                    {
                                    	alertHouseCodeInvail.setResource(R.raw.call_unauthorized);
                                    }
                                    alertHouseCodeInvail.starSound();
                                }
                        	}
                        	InputNumberActivity.this.etRoomNum.setText("");
                        	isStartCall = false;
                        	break;
                        case MyJavaCallback.CALL_ERROR_ROOM_VALID:
                        	Intent intentCallChat = new Intent(InputNumberActivity.this, CallChatActivity.class);
                        	intentCallChat.putExtra("roomnum", sRoomNum);
                            startActivity(intentCallChat);
                            finish();
                        	break;
                        case MyJavaCallback.JNI_DEV_REBOOT:
                        	alertReboot.setResource(R.raw.reboot);   
                        	alertReboot.starSound();
                        	showToast("系统即将重启");
                        	showToast("系统即将重启");
                        	showToast("系统即将重启");
                        	break;     	
                        case MyJavaCallback.CONFIG_UPDATE_DEV_REBOOT:
                        	alertReboot.setResource(R.raw.config_reboot);   
                        	alertReboot.starSound();
                        	showToast("配置文件已更新,即将重启");
                        	showToast("配置文件已更新,即将重启");
                        	showToast("配置文件已更新,即将重启");
                        	break;
                        default :
                            break;
                    }
                }
            }
        };
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(BVAction.ACTION_IDBT_STATE);
        registerReceiver(mReceiver, intentFilter);
    }

    private void uninitBroadcastReceiver() {
        if (mReceiver != null) {
            unregisterReceiver(mReceiver);
            mReceiver = null;
        }
    }

    /**
     * 函数名称 : runByInputMsg 功能描述 : run by what is expected as the input 参数说明：
     * 
     * @param type
     * @param inputContent
     *            返回值： void 修改记录： 日期：2013-1-9 上午9:18:17 修改人：林利澍 描述 ：
     */
    private void runByInputMsg(RunType type, String inputContent) {
        if (inputContent == null) {
            return;
        }
        if(inputContent.length() == 0)
    	{
        	return;
    	}
        switch (type) {
            case CallRoom :// call room number
//            	//***************************************************************************
//                if (IDBTEngine.getSipRegStatus() == MyJavaCallback.SIP_REG_STATUS_ONLINE) {
//                    try {
//                    	//判断呼叫使能
//                    	int imsCallState = IDBTEngine.checkImsCallEnable();
//                    	if(imsCallState == 0)
//                    	{
//            	//***************************************************************************
//            				showToast("开始拨打房间:" + inputContent);
                    		sRoomNum = inputContent;
                    		if(isStartCall)
                    		{
                    			LogUtils.printLog("isStartCall is true");
                    			if(iPressTimes++ >= 3)
                    			{
                    				iPressTimes = 0;
                    				isStartCall = false;
                    			}
                    			break;
                    		}
                    		isStartCall = true;
                    		int rst = IDBTEngine.sendHouseCode(inputContent);
                    		LogUtils.printLog("isStartCall is true ,but rst = " + rst);
                    		if(rst != 0)
                    		{
                    			LogUtils.printLog("isStartCall is true ,but rst != 0");
                    			isStartCall = false; 
                    		}
	                        LogUtils.printLog("try to sendHouseCode：" + rst);
//                  //***************************************************************************
//                    	}
//                    	else
//                    	{
//                    		showToast("呼叫功能已被限制使用，请联系管理员");
//                    		if(alertImsCallUnable != null)
//                    		{
//                        		if(alertImsCallUnable.isHaveResource() == false)
//                        		{
//                        			alertImsCallUnable.setResource(R.raw.call_unable);
//                        		}
//                        		alertImsCallUnable.starSound();
//                    		}
//                    	}
//                    }
//                    catch (Exception e) {
//                        LogUtils.printLog("sendHouseCode error");
//                    }
//                }
//    			else {
//                    showToast(R.string.sip_offline);
//                }
//              //***************************************************************************
                break;
            case OpenByPwd :// open door by password
                try {
                    LogUtils.printLog("password:" + inputContent);
                    int result = IDBTEngine.sendOpenDoorPassword(inputContent);
                    if (result == IDBTEngine.IDBT_OK) {
                        showToast(R.string.right_password_door_opened);
                        soundMgr.playSound(2);
                        soundMgr.playSound(3);
                    } else {
                        soundMgr.playSound(1);
                        showToast(R.string.wrong_password);
                    }
                    LogUtils.printLog("sendOpenDoorPassword result:" + result);
                }
                catch (Exception e) {
                    LogUtils.printLog("sendOpenDoorPassword error");
                }
                etRoomNum.getEditableText().clear();
                break;
            default :
                break;
        }
    }

    private String getInputContent() {
        return etRoomNum.getText().toString().trim();
    }

    private void showToast(String text) {
        Toast.makeText(this, text, Toast.LENGTH_SHORT).show();
    }

    private void showToast(int stringId) {
        Toast.makeText(this, stringId, Toast.LENGTH_SHORT).show();
    }

    /**
     * 函数名称 : startCloseTask
     * 功能描述 : 开启关闭操作界面的任务
     * 参数说明：
     * 返回值：
     * void
     * 修改记录：
     * 日期：2013-2-1 下午1:52:05 修改人：林利澍
     * 描述 ：
     */
    private void startCloseTask() {
        LogUtils.printLog("startCloseTask");
        if (closeTimer == null) {
            closeTimer = new Timer();
            backToAdvertTask = new BackToAdvertTask();
            closeTimer.schedule(backToAdvertTask, 10000);
        }
    }

    /**
     * 函数名称 : stopCloseTask
     * 功能描述 : 停止关闭操作页面的任务
     * 参数说明：
     * 返回值：
     * void
     * 修改记录：
     * 日期：2013-2-1 下午1:52:30 修改人：林利澍
     * 描述 ：
     */
    private void stopCloseTask() {
        LogUtils.printLog("stopCloseTask");
        if (closeTimer != null) {
            closeTimer.cancel();
            closeTimer = null;
            backToAdvertTask = null;
        }
    }

    private class BackToAdvertTask extends TimerTask {
        @Override
        public void run() {
            // TODO Auto-generated method stub
            LogUtils.printLog("10 s  no touch");
            finish();
        }
    }

    @Override
    public View makeView() {
        // TODO Auto-generated method stub
        TextView t = new TextView(this);        
        t.setTextColor(Color.WHITE);
        t.setTextSize(25);
        return t;
    }
}
