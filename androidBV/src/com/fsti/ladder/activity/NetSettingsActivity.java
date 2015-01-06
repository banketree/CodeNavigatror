package com.fsti.ladder.activity;

import android.content.Intent;
import android.view.KeyEvent;
import android.widget.GridView;
import com.fsti.ladder.R;
import com.fsti.ladder.adapter.OptionAdapter;

/**
 * 版权所有 (c)2012, 邮科
 * <p>
 * File Name ：NetSettingsActivity.java
 * <p>
 * Synopsis ：activity for net settings
 * 
 * @author ：林利澍
 *         Creation Time ：2013-3-6 上午10:05:53
 *         Current Version：v1.0
 *         Historical Records :
 *         Date : 2013-3-6 上午10:05:53 修改人：
 *         Description :
 */
public class NetSettingsActivity extends BaseBackActivity {
    private GridView gv_netsettings_option;

    @Override
    protected void initView() {
        // TODO Auto-generated method stub
        setContentView(R.layout.activity_net_settings);
        gv_netsettings_option = (GridView) findViewById(R.id.gv_netsettings_option);
        String[] options = getResources().getStringArray(R.array.net_settings_items);
        gv_netsettings_option.setAdapter(new OptionAdapter(this, options));
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        // TODO Auto-generated method stub
        if (event.getRepeatCount() == 0) {
            Intent intent = null;
            switch (keyCode) {
                case KeyEvent.KEYCODE_1 :
                    intent = new Intent(this, RelativeInfoActivity.class);
                    intent.putExtra("which", 3);
                    startActivity(intent);
                    break;
                case KeyEvent.KEYCODE_2 :
                    intent = new Intent(this, NetConnectTypeActivity.class);
                    startActivity(intent);
                    break;
                case KeyEvent.KEYCODE_3 :
                    intent = new Intent(this, NetModificationActivity.class);
                    intent.putExtra("which", 1);
                    startActivity(intent);
                    break;
                case KeyEvent.KEYCODE_4 :
                    intent = new Intent(this, NetModificationActivity.class);
                    intent.putExtra("which", 2);
                    startActivity(intent);
                    break;
                case KeyEvent.KEYCODE_5 :
                    intent = new Intent(this, NetModificationActivity.class);
                    intent.putExtra("which", 3);
                    startActivity(intent);
                    break;
                case KeyEvent.KEYCODE_6 :
                    intent = new Intent(this, NetModificationActivity.class);
                    intent.putExtra("which", 4);
                    startActivity(intent);
                    break;
                case KeyEvent.KEYCODE_7 :
                    intent = new Intent(this, PasswordModificationActivity.class);
                    intent.putExtra("which", 4);
                    startActivity(intent);
                    break;
                case KeyEvent.KEYCODE_8 :
                    intent = new Intent(this, PasswordModificationActivity.class);
                    intent.putExtra("which", 5);
                    startActivity(intent);
                    break;
                case KeyEvent.KEYCODE_9 :
                    intent = new Intent(this, PasswordModificationActivity.class);
                    intent.putExtra("which", 19);
                    startActivity(intent);
                    break;
                default :
                    break;
            }
        }
        return super.onKeyDown(keyCode, event);
    }
}
