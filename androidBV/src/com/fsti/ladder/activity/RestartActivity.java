package com.fsti.ladder.activity;

import android.content.Intent;
import android.view.KeyEvent;
import android.widget.RemoteViews.ActionException;

import com.fsti.ladder.R;
import com.fsti.ladder.engine.IDBTEngine;
import com.fsti.ladder.tools.LogUtils;

/**
 * ��Ȩ���� (c)2012, �ʿ�
 * <p>
 * File Name ��RestartActivity.java
 * <p>
 * Synopsis ��
 * <p>
 * 
 * @author ��������
 *         Creation Time ��2013-3-5 ����8:47:18
 *         Current Version��v1.0
 *         Historical Records :
 *         Date : 2013-3-5 ����8:47:18 �޸��ˣ�
 *         Description :
 */
public class RestartActivity extends BaseBackActivity {

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        // TODO Auto-generated method stub
        if (keyCode == KeyEvent.KEYCODE_ENTER && event.getRepeatCount() == 0) {
            try{
                LogUtils.printLog("reboot");
//                Intent reboot = new Intent(Intent.ACTION_REBOOT);
//                reboot.putExtra("nowait", 1);
//                reboot.putExtra("interval", 1);
//                reboot.putExtra("window", 0);
//                sendBroadcast(reboot);
//                String cmd = "su -c \"reboot\"";
//                Runtime.getRuntime().exec(cmd);
                IDBTEngine.setResetDev();
            }catch(Exception ex){
                LogUtils.printLog("reboot fail");
                ex.printStackTrace();
            }
//        } else if (keyCode == KeyEvent.KEYCODE_DEL && event.getRepeatCount() == 0) {
//            finish();
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    protected void initView() {
        // TODO Auto-generated method stub
        setContentView(R.layout.activity_restart);
    }
}
