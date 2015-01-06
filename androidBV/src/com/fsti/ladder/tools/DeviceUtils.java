package com.fsti.ladder.tools;

import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;

public class DeviceUtils {
    /**
     * 
     *  函数名称 : getVersion
     *  功能描述 :  获取软件版本号
     *  @param：
     *  	@param ctx
     *  	@return
     *  @return：
     *  	String
     *  修改记录：
     *  日期：2013-3-7 下午5:36:32	修改人：林利澍
     *  描述	：
     *
     */
    public static String getVersion(Context ctx) {
        PackageManager packageManager = ctx.getPackageManager();
        PackageInfo packInfo = null;
        try {
            packInfo = packageManager.getPackageInfo(ctx.getPackageName(), 0);
        }
        catch (NameNotFoundException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        return packInfo.versionName;
    }
}
