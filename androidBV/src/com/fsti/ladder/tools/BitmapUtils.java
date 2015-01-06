package com.fsti.ladder.tools;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;

/**
 * ��Ȩ���� (c)2012, �ʿ�
 * <p>
 * File Name ��BitmapUtils.java
 * <p>
 * Synopsis ��
 * <p>
 * 
 * @author ��������
 *         Creation Time ��2013-2-27 ����10:04:58
 *         Current Version��v1.0
 *         Historical Records :
 *         Date : 2013-2-27 ����10:04:58 �޸��ˣ�
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
