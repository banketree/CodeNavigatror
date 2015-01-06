package com.fsti.ladder.activity;

import android.app.Activity;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnKeyListener;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;
import com.fsti.ladder.R;
import com.fsti.ladder.engine.IDBTEngine;
import com.fsti.ladder.tools.LogUtils;

/**
 * 版权所有 (c)2012, 邮科
 * <p>
 * File Name ：NetModificationActivity.java
 * <p>
 * Synopsis ：settings host ip、 realm dns
 * 
 * @author ：林利澍
 *         Creation Time ：2013-3-8 下午1:26:00
 *         Current Version：v1.0
 *         Historical Records :
 *         Date : 2013-3-8 下午1:26:00 修改人：
 *         Description :
 */
public class NetModificationActivity extends Activity implements OnKeyListener {
    private EditText et_input_content1, et_input_content2, et_input_content3, et_input_content4;
    private boolean isFirst = false;
    private int which = 0;
    private final static int INPUT_CONTENT_EMPTY = 0;
    private final static int INPUT_CONTENT_FULL = 3;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // TODO Auto-generated method stub
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_net_modification);
        initView();
    }

    private void initView() {
        TextView tv_hint_content = (TextView) findViewById(R.id.tv_hint_content);
        which = getIntent().getIntExtra("which", MODE_PRIVATE);
        switch (which) {
            case 1 :// ip设置
                tv_hint_content.setText(R.string.input_ip_address);
                break;
            case 2 :// 子网掩码
                tv_hint_content.setText(R.string.input_subnet_mask);
                break;
            case 3 :// 网关地址
                tv_hint_content.setText(R.string.input_gateway_address);
                break;
            case 4 :// DNS服务器
                tv_hint_content.setText(R.string.input_dns_address);
                break;
            case 5 :// 监测网络IP
                tv_hint_content.setText(R.string.input_monitor_ip);
                break;
            case 6 :// 平台域名IP
                tv_hint_content.setText(R.string.please_input_platform_ip);
                break;
            default :
                finish();
                break;
        }
        et_input_content1 = (EditText) findViewById(R.id.et_input_content1);
        et_input_content1.setOnKeyListener(this);
        et_input_content2 = (EditText) findViewById(R.id.et_input_content2);
        et_input_content2.setOnKeyListener(this);
        et_input_content3 = (EditText) findViewById(R.id.et_input_content3);
        et_input_content3.setOnKeyListener(this);
        et_input_content4 = (EditText) findViewById(R.id.et_input_content4);
        et_input_content4.setOnKeyListener(this);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        // TODO Auto-generated method stub
        if (keyCode == KeyEvent.KEYCODE_DEL && event.getRepeatCount() == 3) {
            LogUtils.printLog("onKeyDown");
            finish();
        }
        return super.onKeyDown(keyCode, event);
    }

    /**
     * @function : setAddress
     * @param：
     * @param which
     * @return
     * @return：
     *          int
     * @history：
     * @time：2013-3-19 上午10:13:13 modifier：
     * @describe ：
     */
    private int setAddress(String inputContent) {
        int result = 0;
        switch (which) {
            case 1 :
                result = IDBTEngine.setIPAddress(inputContent);
                break;
            case 2 :
                result = IDBTEngine.setNetMask(inputContent);
                break;
            case 3 :
                result = IDBTEngine.setGateway(inputContent);
                break;
            case 4 :
                result = IDBTEngine.setDNS(inputContent);
                break;
            case 5 :
                result = IDBTEngine.setMonitorIP(inputContent);
                break;
            case 6 :
                result = IDBTEngine.setPlatformIP(inputContent);
                break;
        }
        return result;
    }

    @Override
    public boolean onKey(View v, int keyCode, KeyEvent event) {
        // TODO Auto-generated method stub
        switch (v.getId()) {
            case R.id.et_input_content1 :
                if (getInputTxetLength(et_input_content1) == INPUT_CONTENT_EMPTY && keyCode == KeyEvent.KEYCODE_DEL && event
                        .getRepeatCount() == 0) {
                    if (isFirst) {
                        isFirst = false;
                    } else {
                        finish();
                    }
                } else if (getInputTxetLength(et_input_content1) == INPUT_CONTENT_FULL && keyCode != KeyEvent.KEYCODE_DEL && event
                        .getRepeatCount() == 0) {
                    isFirst = true;
                    et_input_content1.setFocusable(false);
                    et_input_content3.setFocusable(false);
                    et_input_content4.setFocusable(false);
                    et_input_content2.setFocusable(true);
                } else {
                    isFirst = true;
                }
                break;
            case R.id.et_input_content2 :
                if (keyCode == KeyEvent.KEYCODE_DEL && event.getRepeatCount() == 0 && getInputTxetLength(et_input_content2) == INPUT_CONTENT_EMPTY) {
                    et_input_content2.setFocusable(false);
                    et_input_content3.setFocusable(false);
                    et_input_content4.setFocusable(false);
                    et_input_content1.setFocusable(true);
                } else if (event.getRepeatCount() == 0 && getInputTxetLength(et_input_content2) == INPUT_CONTENT_FULL && keyCode != KeyEvent.KEYCODE_DEL) {
                    et_input_content1.setFocusable(false);
                    et_input_content2.setFocusable(false);
                    et_input_content4.setFocusable(false);
                    et_input_content3.setFocusable(true);
                }
                break;
            case R.id.et_input_content3 :
                if (getInputTxetLength(et_input_content3) == INPUT_CONTENT_EMPTY && keyCode == KeyEvent.KEYCODE_DEL && event
                        .getRepeatCount() == 0) {
                	et_input_content1.setFocusable(false);
                	et_input_content3.setFocusable(false);
                    et_input_content4.setFocusable(false);
                    et_input_content2.setFocusable(true);
                } else if (getInputTxetLength(et_input_content3) == INPUT_CONTENT_FULL && keyCode != KeyEvent.KEYCODE_DEL && event
                        .getRepeatCount() == 0) {
                    et_input_content1.setFocusable(false);
                    et_input_content2.setFocusable(false);
                    et_input_content3.setFocusable(false);
                    et_input_content4.setFocusable(true);
                }
                break;
            case R.id.et_input_content4 :
                if (getInputTxetLength(et_input_content4) == INPUT_CONTENT_EMPTY && keyCode == KeyEvent.KEYCODE_DEL && event
                        .getRepeatCount() == 0) {
                	et_input_content1.setFocusable(false);
                	et_input_content2.setFocusable(false);
                    et_input_content4.setFocusable(false);
                    et_input_content3.setFocusable(true);
                } else if (keyCode == KeyEvent.KEYCODE_ENTER && event.getRepeatCount() == 0 && getInputTxetLength(et_input_content4) != INPUT_CONTENT_EMPTY) {
                    String str1 = getInputText(et_input_content1);
                    String str2 = getInputText(et_input_content2);
                    String str3 = getInputText(et_input_content3);
                    String str4 = getInputText(et_input_content4);
                    int num1 = Integer.valueOf(str1);
                    int num2 = Integer.valueOf(str2);
                    int num3 = Integer.valueOf(str3);
                    int num4 = Integer.valueOf(str4);
                    if(num1 > 255 || num2 > 255 || num3 > 255 || num4 > 255)
                    {
                    	 Toast.makeText(NetModificationActivity.this, "输入数值超过255,请重输", Toast.LENGTH_SHORT).show();
                         break;
                    }
//                	int result = setAddress(et_input_content1.getText().toString().trim() + "." + et_input_content2
//                            .getText().toString().trim() + "." + et_input_content3.getText().toString().trim() + "." + et_input_content4
//                            .getText().toString().trim());
                	int result = setAddress(Integer.toString(num1) + "." + Integer.toString(num2) + "." + 
                			Integer.toString(num3) + "." + Integer.toString(num4));
                    switch (result) {
                        case IDBTEngine.IDBT_OK :
                        	if (which == 5 || which == 6) {
    							Toast.makeText(NetModificationActivity.this,
    									"设置成功,重启后生效", Toast.LENGTH_SHORT).show();
                        	}
                        	else
                        	{
                                Toast.makeText(NetModificationActivity.this, "设置成功", Toast.LENGTH_SHORT).show();
                        	}
                            finish();
                            break;
                        case IDBTEngine.IDBT_FAIL :
                            Toast.makeText(NetModificationActivity.this, "设置失败", Toast.LENGTH_SHORT).show();
                            break;
                    }
                }
                break;
            default :
                break;
        }
        return false;
    }

    /**
     * get text from EditText input content
     * 
     * @param：
     * @param mEditText
     * @return
     * @return：
     *          String
     */
    private String getInputText(EditText mEditText) {
        String text = null;
        if (mEditText != null) {
            text = mEditText.getText().toString().trim();
        }
        return text;
    }

    private int getInputTxetLength(EditText mEditText) {
        String text = getInputText(mEditText);
        int length = 0;
        if (text != null) {
            length = text.length();
        }
        return length;
    }
}
