package com.fsti.ladder.activity;

import android.content.Intent;
import android.view.KeyEvent;
import android.view.ViewGroup.LayoutParams;
import android.widget.GridView;
import com.fsti.ladder.R;
import com.fsti.ladder.adapter.OptionAdapter;

/**
 * 版权所有 (c)2012, 邮科
 * <p>
 * 文件名称 ：SettingsActivity.java
 * <p>
 * 内容摘要 ：activity for set application configuration 作者 ：林利澍 创建时间 ：2012-12-31
 * 上午11:07:19 当前版本号：v1.0 历史记录 : 日期 : 2012-12-31 上午11:07:19 修改人： 描述 :
 */
public class ConfigSettingsActivity extends BaseBackActivity {
    private GridView gvSettings;
    
    @Override
    protected void initView() {
        // TODO Auto-generated method stub
        setContentView(R.layout.activity_whole_settings);
        gvSettings = (GridView) findViewById(R.id.gvWholeSettings);
        String[] option = getResources().getStringArray(R.array.settings_name);
        gvSettings.setAdapter(new OptionAdapter(this, option));
    }
    
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        // TODO Auto-generated method stub
        if (event.getRepeatCount() == 0) {
            Intent intent = null;
            switch (keyCode) {
                case KeyEvent.KEYCODE_1 :
                    intent = new Intent(this,DeviceSettingActivity.class);
//                    intent.putExtra("which", 1);
                    startActivity(intent);
                    break;
                case KeyEvent.KEYCODE_2 :
                    intent = new Intent(this,RelativeInfoActivity.class);
                    intent.putExtra("which", 2);
                    startActivity(intent);
                    break;
                case KeyEvent.KEYCODE_3 :
                    intent = new Intent(this,NetSettingsActivity.class);
                    startActivity(intent);
                    break;
                case KeyEvent.KEYCODE_4 :
                    intent = new Intent(this,PlatformSettingsActivity.class);
                    startActivity(intent);
                    break;
                case KeyEvent.KEYCODE_5 :
                    intent = new Intent(this,RelativeInfoActivity.class);
                    intent.putExtra("which", 5);
                    startActivity(intent);
                    break; 
                case KeyEvent.KEYCODE_6 :
                	intent = new Intent(this,VideoSettingsActivity.class);
                    startActivity(intent);
                    break;
                case KeyEvent.KEYCODE_7 :
                    intent = new Intent(this, PasswordModificationActivity.class);
                    intent.putExtra("which", 6);
                    startActivity(intent);
                    break;
                case KeyEvent.KEYCODE_8 :
                    intent = new Intent(this, PasswordModificationActivity.class);
                    intent.putExtra("which", 7);
                    startActivity(intent);
                    break;
                case KeyEvent.KEYCODE_9 :
                    Intent restartIntent = new Intent(this,RestartActivity.class);
                    startActivity(restartIntent);
                    break;
                default :
                    break;
            }
        }
        return super.onKeyDown(keyCode, event);
    }    
}
