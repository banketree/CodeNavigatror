package com.fsti.ladder.receiver;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import com.fsti.ladder.activity.SplashActivity;
import com.fsti.ladder.tools.LogUtils;
/**
 * 
 *	版权所有  (c)2012,   邮科<p>	
 *  File Name	：BootBroadcastReceiver.java<p>
 *
 *  Synopsis	：<p>
 *
 *  @author	：林利澍
 *  Creation Time	：2013-2-27 下午1:35:10 
 *  Current Version：v1.0
 *  Historical Records	:
 *  Date	: 2013-2-27 下午1:35:10 	@author:
 *  Description	:
 */
public class BootBroadcastReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(Context context, Intent intent) {
        // TODO Auto-generated method stub
        Intent i = new Intent(context,SplashActivity.class);
        i.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        context.startActivity(i);
        LogUtils.printLog("start SplashActivity");
    }
}
