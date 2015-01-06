package com.fsti.ladder.activity;

import java.util.Timer;
import java.util.TimerTask;
import android.app.Activity;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnKeyListener;
import android.widget.EditText;
/**
 * 
 *	版权所有  (c)2012,   邮科<p>	
 *  File Name	：BaseSettingsActivity.java<p>
 *
 *  Synopsis	：<p>
 *
 *  @author	：林利澍
 *  Creation Time	：2013-3-6 下午2:55:25 
 *  Current Version：v1.0
 *  Historical Records	:
 *  Date	: 2013-3-6 下午2:55:25 	修改人：
 *  Description	:
 */
public class BaseSettingsActivity extends Activity {
    protected EditText et_input_content;//用户输入
    protected final static int NO_TOUCH_CLOSE_TIME = 15000;
    private GoBackTask goBackTask;
    private Timer closeTimer;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // TODO Auto-generated method stub
        super.onCreate(savedInstanceState);
        initView();
    }
    
    protected void initView(){
        et_input_content.setOnKeyListener(new OnKeyListener() {
            @Override
            public boolean onKey(View v, int keyCode, KeyEvent event) {
                // TODO Auto-generated method stub
                if(event.getAction() == KeyEvent.ACTION_UP){
                    if(keyCode == KeyEvent.KEYCODE_DEL && event.getRepeatCount() == 0){
                        if(et_input_content.getText().toString().trim().length() == 0){
                            finish();
                        }
                    }
                }                
                return false;
            }
        });
    }
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        // TODO Auto-generated method stub
//        if(keyCode == KeyEvent.KEYCODE_DEL && event.getRepeatCount() == 0){
//            if(et_input_content.getText().toString().trim().length() == 0){
//                finish();
//            }
//        }
        return super.onKeyDown(keyCode, event);
    }
    protected void startCloseTask(){

        if(closeTimer == null){
            closeTimer = new Timer();
            goBackTask = new GoBackTask();
            closeTimer.schedule(goBackTask, NO_TOUCH_CLOSE_TIME);
        }
    }
    /**
     * 
     *  函数名称 : stopCloseTask
     *  功能描述 :  停止关闭操作页面的任务
     *  参数说明：
     *  返回值：
     *      void
     *  修改记录：
     *  日期：2013-2-1 下午1:52:30   修改人：林利澍
     *  描述  ：
     *
     */ 
    protected void stopCloseTask(){

        if(closeTimer != null){
            closeTimer.cancel();
            closeTimer = null;
            goBackTask = null;
        }
    }
    private class GoBackTask extends TimerTask{

        @Override
        public void run() {
            // TODO Auto-generated method stub
            finish();
        }        
    }
    @Override
    public boolean onKeyLongPress(int keyCode, KeyEvent event) {
        // TODO Auto-generated method stub
        switch (keyCode) {
            case KeyEvent.KEYCODE_DEL :
                finish();
                break;
            default :
                break;
        }
        return super.onKeyLongPress(keyCode, event);
    }
}
