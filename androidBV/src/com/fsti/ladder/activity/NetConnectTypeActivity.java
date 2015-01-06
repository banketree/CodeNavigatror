package com.fsti.ladder.activity;

import android.view.KeyEvent;
import android.widget.TextView;
import com.fsti.ladder.R;
import com.fsti.ladder.engine.IDBTEngine;
import com.fsti.ladder.tools.LogUtils;
/**
 * 
 *	版权所有  (c)2012,   邮科<p>	
 *  File Name	：NetConnectTypeActivity.java<p>
 *
 *  Synopsis	：net type choose activity
 *
 *  @author	：林利澍
 *  Creation Time	：2013-3-6 上午8:31:12 
 *  Current Version：v1.0
 *  Historical Records	:
 *  Date	: 2013-3-6 上午8:31:12 	修改人：
 *  Description	:
 */
public class NetConnectTypeActivity extends BaseBackActivity {
    private TextView tv_regular_ip_net;
    private TextView tv_dhcp_ip_net;
    private TextView tv_pppoe_net;
    @Override
    protected void initView() {
        // TODO Auto-generated method stub
        setContentView(R.layout.activity_net_type);  
        tv_regular_ip_net = (TextView) findViewById(R.id.tv_regular_ip_net);
        tv_dhcp_ip_net = (TextView) findViewById(R.id.tv_dhcp_ip_net);
        tv_pppoe_net = (TextView) findViewById(R.id.tv_pppoe_net);
        int connType = 0;
        try{
            connType =Integer.valueOf(IDBTEngine.getNetworkJoinMethod());
            LogUtils.printLog("NetConnectTypeActivity connType:" + connType);
//            connType = Integer.valueOf(IDBTEngine.getNetworkInfo().split("\\|")[0]);
        }catch(Exception e){
            
        }
        switch(connType){
            case 1:
            	tv_dhcp_ip_net.setTextColor(R.color.blue_black);
                break;
            case 2: 
                tv_pppoe_net.setTextColor(R.color.blue_black);
                break;
            case 3:
                tv_regular_ip_net.setTextColor(R.color.blue_black);
                break;
            default:break;
        }
    }
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        // TODO Auto-generated method stub
        if(keyCode == KeyEvent.KEYCODE_1 && event.getRepeatCount() == 0){
            int result = IDBTEngine.setNetworkJoinMethod(1);
            switch(result){
                case 0:
                    showToast("设置成功，重启后生效");
                    finish();
                    break;
                case -1:
                    showToast("设置失败");
                    break;
            }
        }else if(keyCode == KeyEvent.KEYCODE_2 && event.getRepeatCount() == 0){
            int result = IDBTEngine.setNetworkJoinMethod(2);
            switch(result){
                case 0:
                    showToast("设置成功，重启后生效");
                    finish();
                    break;
                case -1:
                    showToast("设置失败");
                    break;
            }
        }else if(keyCode == KeyEvent.KEYCODE_3 && event.getRepeatCount() == 0){
            int result = IDBTEngine.setNetworkJoinMethod(3);
            switch(result){
                case 0:
                    showToast("设置成功，重启后生效");
                    finish();
                    break;
                case -1:
                    showToast("设置失败");
                    break;
            }
        }
        return super.onKeyDown(keyCode, event);
    }

    
}
