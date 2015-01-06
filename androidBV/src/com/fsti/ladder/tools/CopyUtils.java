package com.fsti.ladder.tools;

import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import android.content.Context;
import android.content.res.AssetManager;

public class CopyUtils {
    private Context _context;
    public CopyUtils(Context ctx){
        _context = ctx;
    }
    public void copyAssets(String dirname) {//, String renFrom, String renTo
        AssetManager assetManager = _context.getAssets();
        String[] files = null;
        try {
            files = assetManager.list(dirname);
            for(String s : files){
                LogUtils.printLog("files:"+s);
            }
        } catch (IOException e) {
            
        }
        InputStream in = null;
        OutputStream out = null;
        for(int i=0; i<files.length; i++) {
            try {
              in = assetManager.open(dirname + "/" + files[i]);
//              out = new FileOutputStream(FileUtils.PACKAGE_PATH +"/"+ rename(files[i],renFrom,renTo));
              out = new FileOutputStream(FileUtils.PACKAGE_PATH + files[i] );
              copyFile(in, out);
              in.close();
              in = null;
              out.flush();
              out.close();
              out = null;
              LogUtils.printLog("copy success");
            } catch(IOException e) {
                LogUtils.printLog("copy fail");
            }
        }
    }
    private void copyFile(InputStream in, OutputStream out) throws IOException {
        byte[] buffer = new byte[1024];
        int read;
        while((read = in.read(buffer)) != -1){
          out.write(buffer, 0, read);
        }
    }
    private String rename(String filename,String renFrom,String renTo)
    {
            return filename.replaceAll(renFrom,renTo);
    }
}
