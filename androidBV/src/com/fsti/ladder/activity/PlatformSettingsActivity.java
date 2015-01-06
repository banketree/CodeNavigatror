package com.fsti.ladder.activity;

import android.content.Intent;
import android.view.KeyEvent;
import android.widget.GridView;
import com.fsti.ladder.R;
import com.fsti.ladder.adapter.OptionAdapter;

public class PlatformSettingsActivity extends BaseBackActivity {
    @Override
    protected void initView() {
        // TODO Auto-generated method stub
        setContentView(R.layout.activity_config_settings);
        GridView gvSettings = (GridView) findViewById(R.id.gvSettings);
        String[] option = getResources().getStringArray(R.array.platform_settings_items);
        gvSettings.setAdapter(new OptionAdapter(this, option));
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        // TODO Auto-generated method stub
        if (event.getRepeatCount() == 0) {
            Intent intent = null;
            switch (keyCode) {
                case KeyEvent.KEYCODE_1 :      
                    intent = new Intent(this,RelativeInfoActivity.class);
                    intent.putExtra("which", 4);
                    startActivity(intent);
                    break;
                case KeyEvent.KEYCODE_2 :
                    intent = new Intent(this, PasswordModificationActivity.class);
                    intent.putExtra("which", 2);
                    startActivity(intent);
                    break;
                case KeyEvent.KEYCODE_3 :
                    intent = new Intent(this, NetModificationActivity.class);
                    intent.putExtra("which", 6);
                    startActivity(intent);
                    break;
                case KeyEvent.KEYCODE_4 :
                    intent = new Intent(this, PasswordModificationActivity.class);
                    intent.putExtra("which", 3);
                    startActivity(intent);
                    break;
                case KeyEvent.KEYCODE_5 :
                    intent = new Intent(this, PasswordModificationActivity.class);
                    intent.putExtra("which", 14);
                    startActivity(intent);
                    break;
                case KeyEvent.KEYCODE_6 :
                    intent = new Intent(this, PasswordModificationActivity.class);
                    intent.putExtra("which", 15);
                    startActivity(intent);
                    break;
                default :
                    break;
            }
        }
        return super.onKeyDown(keyCode, event);
    }
}
