package com.fsti.ladder.tools;

import java.io.IOException;
import java.net.UnknownHostException;
import android.os.Handler;

public class NetTools {
    public static void isNetWorkConnected(Handler mHandler){
        try {
            new java.net.Socket("baidu.com", 80);
            LogUtils.printLog("socket ok");
            mHandler.sendEmptyMessage(11);
        } catch (UnknownHostException e) {
        	LogUtils.printLog("socket UnknownHostException");
            mHandler.sendEmptyMessage(12);
        } catch (IOException e) {
        	LogUtils.printLog("socket IOException");
            mHandler.sendEmptyMessage(12);
        }
    }
}
