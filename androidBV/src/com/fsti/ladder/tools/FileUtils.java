package com.fsti.ladder.tools;

import java.io.File;

public class FileUtils {
    
    public final static String PACKAGE_PATH = "/data/data/com.fsti.ladder/";
    public final static String ZIP_FILE_NAME = "adv.zip";
    public final static String ADVERT_FILE = "adv";
    public final static String ADVERT_IN_STANDBY = "adv_standby";
    public final static String ADVERT_IN_CALL = "adv_call";
    public static String[] getFilePaths(String path) {
        File file = new File(path);
        String[] imgPaths = null;
        if (file != null) {
            imgPaths = file.list();
        }
        return imgPaths;
    }

    
    public static void deleteFile(File file) {

        if (file.exists()) { // �ж��ļ��Ƿ����
            if (file.isFile()) { // �ж��Ƿ����ļ�
                file.delete(); // delete()���� ��Ӧ��֪�� ��ɾ������˼;
            } else if (file.isDirectory()) { // �����������һ��Ŀ¼
                File files[] = file.listFiles(); // ����Ŀ¼�����е��ļ� files[];
                for (int i = 0; i <files.length; i++) { // ����Ŀ¼�����е��ļ�
                    deleteFile(files[i]); // ��ÿ���ļ� ������������е���
                }
            }
            file.delete();
        } else {
            //
        }
    }
}
