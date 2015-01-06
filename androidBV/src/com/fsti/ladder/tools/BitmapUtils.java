package com.fsti.ladder.tools;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;

/**
 * 版权所有 (c)2012, 邮科
 * <p>
 * File Name ：BitmapUtils.java
 * <p>
 * Synopsis ：
 * <p>
 * 
 * @author ：林利澍
 *         Creation Time ：2013-2-27 上午10:04:58
 *         Current Version：v1.0
 *         Historical Records :
 *         Date : 2013-2-27 上午10:04:58 修改人：
 *         Description :
 */
public class BitmapUtils {
    
    public static Bitmap getBitmap(String pathName) {
        Bitmap bitmap = null;
        try {
            bitmap = BitmapFactory.decodeFile(pathName);
        }
        catch (OutOfMemoryError e) {
            if (bitmap == null) {
                System.gc();
                System.runFinalization();
                bitmap = BitmapFactory.decodeFile(pathName);
            }
        }
        return bitmap;
    }

    public static void downloadImage() {}
}
