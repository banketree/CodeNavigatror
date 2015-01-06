package com.fsti.ladder.activity;

import android.content.Intent;
import android.view.KeyEvent;
import android.widget.GridView;
import com.fsti.ladder.R;
import com.fsti.ladder.adapter.OptionAdapter;

public class DeviceSettingActivity extends BaseBackActivity {
	@Override
    protected void initView() {
        // TODO Auto-generated method stub
        setContentView(R.layout.activity_config_settings);
        GridView gvSettings = (GridView) findViewById(R.id.gvSettings);
        String[] option = getResources().getStringArray(R.array.device_settings_items);
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
                    intent.putExtra("which", 1);
                    startActivity(intent);
                    break;
                case KeyEvent.KEYCODE_2 :
                    intent = new Intent(this, RelativeInfoActivity.class);
                    intent.putExtra("which", 7);
                    startActivity(intent);
                    break;
                case KeyEvent.KEYCODE_3 :
                    intent = new Intent(this, PasswordModificationActivity.class);
                    intent.putExtra("which", 11);
                    startActivity(intent);
                    break;
                case KeyEvent.KEYCODE_4 :
                    intent = new Intent(this, PasswordModificationActivity.class);
                    intent.putExtra("which", 12);
                    startActivity(intent);
                    break;
                case KeyEvent.KEYCODE_5 :
                    intent = new Intent(this, RelativeInfoActivity.class);
                    intent.putExtra("which", 8);
                    startActivity(intent);
                    break;
                case KeyEvent.KEYCODE_6 :
                    intent = new Intent(this, NetModificationActivity.class);
                    intent.putExtra("which", 5);
                    startActivity(intent);
                    break;
                case KeyEvent.KEYCODE_7 :
                    intent = new Intent(this, PasswordModificationActivity.class);
                    intent.putExtra("which", 17);
                    startActivity(intent);
                    break;
                case KeyEvent.KEYCODE_8 :
                    intent = new Intent(this, PasswordModificationActivity.class);
                    intent.putExtra("which", 18);
                    startActivity(intent);
                    break;
                default :
                    break;
            }
        }
        return super.onKeyDown(keyCode, event);
    }
}
