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

        if (file.exists()) { // 判断文件是否存在
            if (file.isFile()) { // 判断是否是文件
                file.delete(); // delete()方法 你应该知道 是删除的意思;
            } else if (file.isDirectory()) { // 否则如果它是一个目录
                File files[] = file.listFiles(); // 声明目录下所有的文件 files[];
                for (int i = 0; i <files.length; i++) { // 遍历目录下所有的文件
                    deleteFile(files[i]); // 把每个文件 用这个方法进行迭代
                }
            }
            file.delete();
        } else {
            //
        }
    }
}
