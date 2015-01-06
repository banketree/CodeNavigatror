package com.fsti.ladder.activity;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.view.KeyEvent;
import android.widget.Toast;
/**
 * 
 *	��Ȩ����  (c)2012,   �ʿ�<p>	
 *  File Name	��BaseActivity.java<p>
 *
 *  Synopsis	���̳и����Activity����*�ż��Կɺ���
 *
 *  @author	��������
 *  Creation Time	��2013-3-6 ����9:30:23 
 *  Current Version��v1.0
 *  Historical Records	:
 *  Date	: 2013-3-6 ����9:30:23 	�޸��ˣ�
 *  Description	:
 */
public abstract class BaseBackActivity extends Activity {
	private Handler mHandler = null;
	
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // TODO Auto-generated method stub
        super.onCreate(savedInstanceState);
        mHandler = new Handler();
    	mHandler.post(mInitView);
    }
    
    @Override
    protected void onDestroy()
    {
    	mHandler = null;
    	super.onDestroy();
    }
    
    final Runnable mInitView = new Runnable() {
        public void run() {
        	initView();
        }
    };
    
    protected abstract void initView();

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        // TODO Auto-generated method stub
        if (keyCode == KeyEvent.KEYCODE_DEL && event.getRepeatCount() == 0) {
            finish();
        }
        return super.onKeyDown(keyCode, event);
    }
    protected void showToast(String text) {
        Toast.makeText(this, text, Toast.LENGTH_SHORT).show();
    }
}
