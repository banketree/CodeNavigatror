package com.fsti.ladder.activity;

import java.util.ArrayList;

import android.R.integer;
import android.app.Activity;
import android.content.Context;
import android.media.AudioManager;
import android.os.Bundle;
import android.view.KeyEvent;
import android.widget.ListView;
import com.fsti.ladder.R;
import com.fsti.ladder.adapter.RelativeInfoAdapter;
import com.fsti.ladder.bean.RelativeInfo;
import com.fsti.ladder.engine.IDBTEngine;
import com.fsti.ladder.tools.DeviceUtils;

/**
 * 版权所有 (c)2012, 邮科
 * <p>
 * File Name ：RelativeInfoActivity.java
 * <p>
 * Synopsis ：
 * <p>
 * 
 * @author ：林利澍
 *         Creation Time ：2013-3-7 上午8:46:30
 *         Current Version：v1.0
 *         Historical Records :
 *         Date : 2013-3-7 上午8:46:30 修改人：
 *         Description :
 */
public class RelativeInfoActivity extends Activity {
    private ListView lv_device_info;// 设备信息选项
    private String[] infoName = null;
    private String[] infoValue = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // TODO Auto-generated method stub
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_device_info);
        lv_device_info = (ListView) findViewById(R.id.lv_device_info);
        int which = getIntent().getIntExtra("which", MODE_PRIVATE);
        switch (which) {
            case 1 :
                infoName = getResources().getStringArray(R.array.device_info_items_name);
                infoValue = IDBTEngine.getDevInfo().split("\\|");
                break;
            case 2 :
                infoName = getResources().getStringArray(R.array.version_info_items_name);
                String softVersion = DeviceUtils.getVersion(this);
//                String[] hardVersion = IDBTEngine.getVersionInfo().split("\\|",3);
                String[] hardVersion = IDBTEngine.getVersionInfo().split("\\|");
                infoValue = new String[]{hardVersion[0], hardVersion[1], hardVersion[2]};
                break;
            case 3 :
                infoName = getResources().getStringArray(R.array.net_info_items_name);
                infoValue = IDBTEngine.getNetworkInfo().split("\\|");
                if ("1".equals(infoValue[0])) {
                	infoValue[0] = "DHCP网络接入";
                } else if ("2".equals(infoValue[0])) {
                	 infoValue[0] = "PPPOE网络接入";
                } else if ("3".equals(infoValue[0])) {
                    infoValue[0] = "固定IP网络接入";
                }
                if ("0".equals(infoValue[8])) {
                	infoValue[8] = "公网环境";
                } else if ("1".equals(infoValue[8])) {
                	 infoValue[8] = "局域网环境";
                }
                break;
            case 4 :
                infoName = getResources().getStringArray(R.array.platform_info_items_name);
                infoValue = IDBTEngine.getPlatformInfo().split("\\|");
                break;
            case 5 :
                infoName = getResources().getStringArray(R.array.sip_info_items_name);
                infoValue = IDBTEngine.getSIPInfo().split("\\|");
                break;           
            case 6 :
                infoName = getResources().getStringArray(R.array.audio_video_items_name);
                AudioManager am = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
                int maxMusic = am.getStreamMaxVolume( AudioManager.STREAM_MUSIC );
                int currentMusic = am.getStreamVolume( AudioManager.STREAM_MUSIC );
                int maxCall = am.getStreamMaxVolume( AudioManager.STREAM_VOICE_CALL  );
                int currentCall = am.getStreamVolume( AudioManager.STREAM_VOICE_CALL  );
                int iVedioQuality = IDBTEngine.getVedioQuality();
                int iVedioFormat1 = IDBTEngine.getVideoFormat();
                int iNegotiateMode = IDBTEngine.getNegotiateMode();
                int iVedioRotation = IDBTEngine.getVedioRotation();
                infoValue = new String[]{String.valueOf(currentMusic)+"/"+  String.valueOf(maxMusic), 
                		String.valueOf(currentCall)+"/"+  String.valueOf(maxCall),
                		iVedioQuality+"/50", iVedioFormat1 + "/1" , iNegotiateMode + "/1" ,  iVedioRotation+"/1"};
//                infoValue = IDBTEngine.getPlatformInfo().split("\\|");
                break;
            case 7:
            	infoName = getResources().getStringArray(R.array.position_info_items_name);
                infoValue = IDBTEngine.getDevicePosition().split("\\|");
                if(infoValue[0] != null)
                {
                	infoValue[0] += "栋";
                }
                if(infoValue[1] != null)
                {
                	infoValue[1] += "单元";
                }
                break;
            case 8 :
                infoName = getResources().getStringArray(R.array.net_monitor_items_name);
                infoValue = IDBTEngine.getNetMonitorInfo().split("\\|");
                break;   
            default :
                finish();
                break;
        }
        if (infoName != null) {
            ArrayList<RelativeInfo> lstInfo = new ArrayList<RelativeInfo>();
            for (int i = 0; i < infoName.length; i++) {
                RelativeInfo rInfo = new RelativeInfo();
                rInfo.itemName = infoName[i];
                try {
                    rInfo.itemValue = infoValue[i];
                }
                catch (Exception e) {
                    rInfo.itemValue = "";
                }
                lstInfo.add(rInfo);
            }
            lv_device_info.setAdapter(new RelativeInfoAdapter(this, lstInfo));
        }
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        // TODO Auto-generated method stub
        if (keyCode == KeyEvent.KEYCODE_DEL && event.getRepeatCount() == 0) {
            finish();
        }
        return super.onKeyDown(keyCode, event);
    }
}
