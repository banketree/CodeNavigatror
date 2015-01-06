package com.fsti.ladder.service;

import java.io.File;
import java.util.Calendar;
import java.util.Date;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;

import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.Cursor;
import android.os.IBinder;
import android.widget.Toast;
import com.fsti.ladder.R;
import com.fsti.ladder.common.AlertSound;
import com.fsti.ladder.common.AndroidBVApp;
import com.fsti.ladder.common.BVAction;
import com.fsti.ladder.common.MyContentProvider;
import com.fsti.ladder.common.MyJavaCallback;
import com.fsti.ladder.common.SoundManager;
import com.fsti.ladder.engine.IDBTEngine;
import com.fsti.ladder.tools.CopyUtils;
import com.fsti.ladder.tools.FileUtils;
import com.fsti.ladder.tools.LogUtils;
import com.fsti.ladder.tools.ZipUtils;

/**
 * 版权所有 (c)2012, 邮科
 * <p>
 * 文件名称 ：IDBTService.java
 * <p>
 * 内容摘要 ：IDBT Service 作者 ：林利澍 创建时间 ：2013-1-5 下午6:21:57 当前版本号：v1.0 历史记录 : 日期 :
 * 2013-1-5 下午6:21:57 修改人： 描述 :
 */
public class IDBTService extends Service {
    private BroadcastReceiver mIDBTReceiver;
    private SoundManager soundMgr;
    private AlertSound alertCardInvail;
    private Timer countTimer = null;
    public long integral_time = 0;
    @Override
    public void onCreate() {
        // TODO Auto-generated method stub
        super.onCreate();
        soundMgr = new SoundManager();
        soundMgr.initSounds(this);
        soundMgr.addSound(1, R.raw.card_fail);
        soundMgr.addSound(2, R.raw.card_succ);
        soundMgr.addSound(3, R.raw.door_opened);
        alertCardInvail = new AlertSound(getBaseContext());
        alertCardInvail.setResource(R.raw.card_unauthorized);
        initBroadcastReceiver();
//        Calendar cal = Calendar.getInstance();
//        cal.set(Calendar.SECOND, 0);
//        cal.set(Calendar.MINUTE, 0);
//        cal.set(Calendar.MILLISECOND, 0);
//        integral_time = cal.getTimeInMillis() + 1*60*60*1000;
        new Thread() {
            public void run() {
            	//配置文件是否存在
                File tmFile = new File(FileUtils.PACKAGE_PATH + "TMData.xml");
                if(!tmFile.exists()){
                    CopyUtils cus = new CopyUtils(IDBTService.this);
                    cus.copyAssets("config");
                }
                //启动JNI 消息队列
                int result = IDBTEngine.IDBTInit();
                LogUtils.printLog("idbt  init result:" + result);
            };
        }.start();
        //进行定期查看增值业务有效期 生效添加链表 失效从数据库删除
        countTimer = new Timer();
        countTimer.schedule(mTimerTaskInCall, 0, 60*1000);
    }

    @Override
    public IBinder onBind(Intent intent) {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public void onDestroy() {
        // TODO Auto-generated method stub
    	uninitAlertSound();
        unitBroadcastReceiver();
        if (countTimer != null) {
            countTimer.cancel();
            countTimer = null;
        }
        super.onDestroy();
    }
    
    private void uninitAlertSound() {
        if (alertCardInvail != null) {
        	alertCardInvail.releaseSound();
        	alertCardInvail = null;
        }
    }

    private void initBroadcastReceiver() {
        mIDBTReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                // TODO Auto-generated method stub
                String action = intent.getAction();
                if (action.equals(BVAction.ACTION_IDBT_STATE)) {
                    int state = intent.getIntExtra("state", Context.MODE_PRIVATE);
                    Intent advertUpdate = new Intent(BVAction.ACTION_ADVERT_UPDATE);
                    switch (state) {
                        case MyJavaCallback.JNI_DEV_RFIDCARD_VALIDATA_STATUS_VALID :
                            Toast.makeText(context, R.string.card_vail_door_opened, Toast.LENGTH_SHORT).show();
                            if (soundMgr != null) {
                                soundMgr.playSound(2);
                                soundMgr.playSound(3);
                            }
                            break;
                        case MyJavaCallback.JNI_DEV_RFIDCARD_VALIDATE_STATUS_INVALID :
                            Toast.makeText(context, R.string.card_invail, Toast.LENGTH_SHORT).show();
                            if (soundMgr != null) {
                                soundMgr.playSound(1);
                                alertCardInvail.starSound();
                            }
                            break;
                        case MyJavaCallback.JNI_DEV_ADVERT_UPDATE :
                            LogUtils.printLog("advert_update");
//                            new Thread() {
//                                public void run() {
                                    String zipFilePath = FileUtils.PACKAGE_PATH + FileUtils.ZIP_FILE_NAME;
                                    String upZipFile = FileUtils.PACKAGE_PATH + FileUtils.ADVERT_FILE;
                                    File targetFile = new File(upZipFile);
                                    if (targetFile.exists()) {
                                        LogUtils.printLog("targetFile  exists");
                                        FileUtils.deleteFile(targetFile);
                                    }
                                    ZipUtils.unZip(zipFilePath,FileUtils.PACKAGE_PATH);
                                    sendBroadcast(advertUpdate);
//                                };
//                            }.start();
                            break;
                        case MyJavaCallback.JNI_DEV_DTMF_OPENDOOR:
                            Toast.makeText(IDBTService.this, R.string.door_opened, Toast.LENGTH_SHORT).show();
                            if(soundMgr != null){
                            	 soundMgr.playSound(2);
                                 soundMgr.playSound(3);
                            }
                            break;
                        case MyJavaCallback.JNI_ADV_DOWNLOAD:
                        case MyJavaCallback.JNI_ANN_DOWNLOAD:
                            //读取数据并保持到数据库
                        	ContentResolver contentResolver = AndroidBVApp.getInstance().getBaseContext().getContentResolver();
                        	String strID = intent.getStringExtra("id");
                        	String strUrl = intent.getStringExtra("url");
                        	String strEnd = intent.getStringExtra("end");
                        	ContentValues values = new ContentValues();
                            values.put(MyContentProvider._ID, strID);
                            values.put(MyContentProvider._URL, strUrl);
                            values.put(MyContentProvider._END,strEnd);
                            contentResolver.insert(MyContentProvider.content_URI, values);
                        	//生效图片添加链表
                            sendBroadcast(advertUpdate);
                        break;
                        default :
                            break;
                    }
                }
            }
        };
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(BVAction.ACTION_IDBT_STATE);
        registerReceiver(mIDBTReceiver, intentFilter);
    }

    private void unitBroadcastReceiver() {
        if (mIDBTReceiver != null) {
            unregisterReceiver(mIDBTReceiver);
        }
    }
    
    private final TimerTask mTimerTaskInCall = new TimerTask() {
        @Override
        public void run() {
            // TODO Auto-generated method stub
//            final long duration = new Date().getTime() - startTime;
        	//时区问题 差8小时
        	boolean bHaveChanged = false;
	        long now_time = new Date().getTime();
        	final long duration = new Date().getTime() - integral_time;
        	LogUtils.printLog("mTimerTaskInCall now time : "+ new Date().toGMTString() + "; now_msec : " + now_time +" ; integral_time : " + integral_time + "; duration : " + duration);
        	if(duration < 0)
        	{
        		return ;
        	}
        	else
        	{
        		Calendar cal = Calendar.getInstance();
                cal.set(Calendar.SECOND, 0);
                cal.set(Calendar.MINUTE, 0);
    	        cal.set(Calendar.MILLISECOND, 0);
    	        integral_time = cal.getTimeInMillis() + 1*60*60*1000;
    	        LogUtils.printLog("mTimerTaskInCall now time !! : " + new Date().toGMTString());
        		ContentResolver contentResolver = AndroidBVApp.getInstance().getBaseContext().getContentResolver();
        		//数据库查找
        		Cursor c = contentResolver.query(MyContentProvider.content_URI, 
        				new String[] { MyContentProvider._URL,MyContentProvider._ID,
        				MyContentProvider._END }, null, null, null);
        		//打开
        		c.moveToFirst();
        		while (!c.isAfterLast()) 
        		{
            		//查找结束时间
        			long end = c.getLong(c.getColumnIndexOrThrow(MyContentProvider._END));
        			if(now_time > end)
        			{
        				if(bHaveChanged == false)
        				{
        					bHaveChanged = true;
        				}
                		//达到时间处理
        				//删除
        				String id = c.getString(c.getColumnIndexOrThrow(MyContentProvider._ID));
        				String url = c.getString(c.getColumnIndexOrThrow(MyContentProvider._URL));
        				File targetFile = new File(url);
                        if (targetFile.exists()) {
                            LogUtils.printLog("targetFile  exists");
                            FileUtils.deleteFile(targetFile);
                        }
                        //从数据库中删除
                        contentResolver.delete(MyContentProvider.content_URI, "_url=?", new String[]{url});
//                        contentResolver.delete(MyContentProvider.content_URI, "_id=? and name=?", new String[]{id});
        			}
            		c.moveToNext();
        		}
        		c.close();
        		if(bHaveChanged == true)
        		{
    				//发送广播通知
    				Intent advertUpdate = new Intent(BVAction.ACTION_ADVERT_UPDATE);
                    sendBroadcast(advertUpdate);
        		}
        	}
        }
    };

}
