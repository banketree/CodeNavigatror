package com.fsti.ladder.tools;

import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;

public class DeviceUtils {
    /**
     * 
     *  �������� : getVersion
     *  �������� :  ��ȡ����汾��
     *  @param��
     *  	@param ctx
     *  	@return
     *  @return��
     *  	String
     *  �޸ļ�¼��
     *  ���ڣ�2013-3-7 ����5:36:32	�޸��ˣ�������
     *  ����	��
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
