package com.fsti.ladder.activity;

import android.R.integer;
import android.app.Activity;
import android.content.Context;
import android.media.AudioManager;
import android.os.Bundle;
import android.text.Editable;
import android.text.InputFilter;
import android.text.InputType;
import android.text.TextWatcher;
import android.text.InputFilter.LengthFilter;
import android.text.method.HideReturnsTransformationMethod;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnKeyListener;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;
import com.fsti.ladder.R;
import com.fsti.ladder.engine.IDBTEngine;

public class PasswordModificationActivity extends Activity {
	private final static int USB_DEBUG_MODE = 0;
	private final static int NET_DEBUG_MODE = 1;
	private TextView tv_hint_content;
	private EditText et_input_content;
	private static int iDebugMode = USB_DEBUG_MODE;
	private boolean isExit = true;
	private boolean isFirst = false;
	private int which = 0;
	AudioManager am = null;
	int max = 0;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_password_modification);
		am = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
		initView();
	}

	private void initView() {
		tv_hint_content = (TextView) findViewById(R.id.tv_hint_content);
		et_input_content = (EditText) findViewById(R.id.et_input_content);
		which = getIntent().getIntExtra("which", MODE_PRIVATE);
		switch (which) {
		case 2:// 设备编号变更
			tv_hint_content.setText(R.string.please_input_device_code);
			et_input_content.setInputType(InputType.TYPE_CLASS_TEXT);
			et_input_content
					.setTransformationMethod(HideReturnsTransformationMethod
							.getInstance());
			break;
		case 3:// 平台域名端口变更
			tv_hint_content.setText(R.string.please_input_platform_port);
			et_input_content.setFilters(new InputFilter[]{new LengthFilter(5)});
			et_input_content
					.setTransformationMethod(HideReturnsTransformationMethod
							.getInstance());
			break;
		case 4:// pppoe账号变更设置
			tv_hint_content.setText(R.string.please_input_pppoe_account);
			et_input_content.setInputType(InputType.TYPE_CLASS_TEXT);
			et_input_content
					.setTransformationMethod(HideReturnsTransformationMethod
							.getInstance());
			break;
		case 5:// PPPOE密码变更设置
			tv_hint_content.setText(R.string.please_input_pppoe_pwd);
			et_input_content.setInputType(InputType.TYPE_CLASS_TEXT);
			break;
		case 6:// 管理密码变更
			tv_hint_content.setText(R.string.please_input_manage_pwd);
			break;
		case 7:// 开门密码变更
			tv_hint_content.setText(R.string.please_input_opendoor_pwd);
			break;
		case 8:// 媒体音量变更
			max = am.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
			tv_hint_content.setText(R.string.please_input_change_audio);
			et_input_content.setTextSize(30);
			et_input_content.setHint("媒体音量最大值为"+max);
			et_input_content
					.setTransformationMethod(HideReturnsTransformationMethod
							.getInstance());
			et_input_content.setFilters(new InputFilter[]{new LengthFilter(2)});
			break;
		case 9:// 通话音量变更
			max = am.getStreamMaxVolume(AudioManager.STREAM_VOICE_CALL);
			tv_hint_content.setText(R.string.please_input_change_call);
			et_input_content.setTextSize(30);
			et_input_content.setHint("通话音量最大值为"+max);
			et_input_content
					.setTransformationMethod(HideReturnsTransformationMethod
							.getInstance());
			et_input_content.setFilters(new InputFilter[]{new LengthFilter(2)});
			break;
		case 10:// 视频参数变更
			tv_hint_content.setText(R.string.please_input_change_video);
			et_input_content.setTextSize(30);
			et_input_content.setHint("视频质量最大值为50");
			et_input_content
					.setTransformationMethod(HideReturnsTransformationMethod
							.getInstance());
			et_input_content.setFilters(new InputFilter[]{new LengthFilter(2)});
			break;
		case 11: //设置楼栋号
			tv_hint_content.setText(R.string.please_input_change_block);
			et_input_content.setTextSize(30);
			et_input_content.setHint("请输入楼栋号");
			et_input_content
					.setTransformationMethod(HideReturnsTransformationMethod
							.getInstance());
			et_input_content.setFilters(new InputFilter[]{new LengthFilter(4)});
			break;
		case 12: //设置单元号
			tv_hint_content.setText(R.string.please_input_change_unit);
			et_input_content.setTextSize(30);
			et_input_content.setHint("请输入单元号");
			et_input_content
					.setTransformationMethod(HideReturnsTransformationMethod
							.getInstance());
			et_input_content.setFilters(new InputFilter[]{new LengthFilter(2)});
			break;
		case 14: //梯口机账号变更
			tv_hint_content.setText(R.string.please_input_platform_username);
			et_input_content.setInputType(InputType.TYPE_CLASS_TEXT);
			et_input_content
					.setTransformationMethod(HideReturnsTransformationMethod
							.getInstance());
			break;
		case 15: //梯口机密码变更
			tv_hint_content.setText(R.string.please_input_platform_password);
			et_input_content.setInputType(InputType.TYPE_CLASS_TEXT);
			break;
		case 16: //视频翻转变更
			tv_hint_content.setText(R.string.please_input_change_rotation);
			et_input_content.setTextSize(30);
			et_input_content.setHint("请输入0或1");
			et_input_content
					.setTransformationMethod(HideReturnsTransformationMethod
							.getInstance());
			et_input_content.setFilters(new InputFilter[]{new LengthFilter(1)});
			break;
		case 17://监测网络端口变更
			tv_hint_content.setText(R.string.please_input_monitor_port);
			et_input_content
					.setTransformationMethod(HideReturnsTransformationMethod
							.getInstance());
			et_input_content.setFilters(new InputFilter[]{new LengthFilter(5)});
			break;
		case 18://调试模式变更
			tv_hint_content.setText(R.string.please_input_debug_mode);
			et_input_content.setTextSize(26);
			et_input_content.setHint("当前值为"+iDebugMode+",0为usb调试,1为网络调试");
			et_input_content
					.setTransformationMethod(HideReturnsTransformationMethod
							.getInstance());
			et_input_content.setFilters(new InputFilter[]{new LengthFilter(1)});
			break;
		case 19://网络环境变更
			tv_hint_content.setText(R.string.please_input_net_mode);
			et_input_content.setTextSize(30);
			et_input_content.setHint("0为公网环境,1为局域网环境");
			et_input_content
					.setTransformationMethod(HideReturnsTransformationMethod
							.getInstance());
			et_input_content.setFilters(new InputFilter[]{new LengthFilter(1)});
			break;
		case 20: //视频翻转变更 add by chensq 20130911
			tv_hint_content.setText(R.string.please_input_change_videoformat);
			et_input_content.setTextSize(30);
			et_input_content.setHint("请输入0(CIF)或1(D1)");
			et_input_content
					.setTransformationMethod(HideReturnsTransformationMethod
							.getInstance());
			et_input_content.setFilters(new InputFilter[]{new LengthFilter(1)});
			break;
	case 21: //视频模式变更 add by chensq 20130924
		tv_hint_content.setText(R.string.please_input_change_negotiatemode);
		et_input_content.setTextSize(30);
		et_input_content.setHint("请输入0(手动)或1(自动)");
		et_input_content
				.setTransformationMethod(HideReturnsTransformationMethod
						.getInstance());
		et_input_content.setFilters(new InputFilter[]{new LengthFilter(1)});
		break;
	default :
		break;
	}
		et_input_content.addTextChangedListener(new TextWatcher() {
			@Override
			public void onTextChanged(CharSequence s, int start, int before,
					int count) {
				// TODO Auto-generated method stub
			}

			@Override
			public void beforeTextChanged(CharSequence s, int start, int count,
					int after) {
				// TODO Auto-generated method stub
			}

			@Override
			public void afterTextChanged(Editable s) {
				// TODO Auto-generated method stub
				if (s.length() == 0) {
					isExit = true;
				} else {
					isExit = false;
					isFirst = true;
				}
			}
		});
		et_input_content.setOnKeyListener(new OnKeyListener() {
			@Override
			public boolean onKey(View v, int keyCode, KeyEvent event) {
				// TODO Auto-generated method stub
				if (isExit && keyCode == KeyEvent.KEYCODE_DEL
						&& event.getRepeatCount() == 0) {
					if (isFirst) {
						isFirst = false;
					} else {
						finish();
					}
				} else if (keyCode == KeyEvent.KEYCODE_ENTER
						&& event.getRepeatCount() == 0 && event.getAction() == KeyEvent.ACTION_UP) {
					int result = setConfigParam(which, et_input_content
							.getText().toString().trim());
					if (result == 0) {
						if (which == 2 || which == 3 || which == 13 || which == 14 || which == 15 || which == 17) {
							Toast.makeText(PasswordModificationActivity.this,
									"设置成功,重启后生效", Toast.LENGTH_SHORT).show();
						} else {
							Toast.makeText(PasswordModificationActivity.this,
									"设置成功", Toast.LENGTH_SHORT).show();
						}
						finish();
					} else {
						Toast.makeText(PasswordModificationActivity.this,
								"设置失败", Toast.LENGTH_SHORT).show();
					}
				} else if (keyCode == KeyEvent.KEYCODE_V) {
					if (which == 2) {
						if (event.getRepeatCount() == 0
								&& event.getAction() == KeyEvent.ACTION_UP) {
							et_input_content.append("+");
						}
						return true;
					}
				}
				return false;
			}
		});
	}

	private int setConfigParam(int where, String input) {
		int result = 1;
		int setVol = 0;
		if(input.length() == 0)
		{
			return result;
		}
		switch (where) {
		case 2:// 设备修改
			result = IDBTEngine.setDevNum(input);
			break;
		case 3:// 平台域名端口变更
			setVol = Integer.parseInt(input);
			if (setVol >= 0 && setVol <= 65535) {
				result = IDBTEngine.setPlatformPort(input);
			}
			break;
		case 4:// pppoe账号变更设置
			result = IDBTEngine.setPPPOEUsername(input);
			break;
		case 5:// PPPOE密码变更设置
			result = IDBTEngine.setPPPOEPassword(input);
			break;
		case 6:// 管理密码变更
			result = IDBTEngine.setManagementPassword(input);
			break;
		case 7:// 开门密码变更
			result = IDBTEngine.setOpendoorPassword(input);
			break;
		case 8:// 媒体音量变更
			max = am.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
			setVol = Integer.parseInt(input);
			if (setVol >= 0 && setVol <= max) {
				am.setStreamVolume(AudioManager.STREAM_MUSIC, setVol, 0);
				result = 0;
			}
//			else
//			{
//				Toast.makeText(PasswordModificationActivity.this,
//						"媒体音量最大值为"+max, Toast.LENGTH_SHORT).show();
//			}
			break;		
		case 9:// 通话音量变更
			max = am.getStreamMaxVolume(AudioManager.STREAM_VOICE_CALL);
			setVol = Integer.parseInt(input);
			if (setVol >= 0 && setVol <= max) {
				am.setStreamVolume(AudioManager.STREAM_VOICE_CALL, setVol, 0);
				result = 0;
			}
//			else
//			{
//				Toast.makeText(PasswordModificationActivity.this,
//						"通话音量最大值为"+max, Toast.LENGTH_SHORT).show();
//			}
			break;
		case 10:// 视频质量变更
			setVol = Integer.parseInt(input);
			if (setVol >= 0 && setVol <= 50) {
				result = IDBTEngine.setVedioQuality(input);
			}
//			else
//			{
//				Toast.makeText(PasswordModificationActivity.this,
//						"视频质量最大值为50", Toast.LENGTH_SHORT).show();
//			}
			break;
		case 11:// 楼栋号变更
			result = IDBTEngine.setBlockNum(input);
			break;	
		case 12:// 单元号变更
			result = IDBTEngine.setUnitNum(input);
			break;
		case 14:// 梯口账号变更
			result = IDBTEngine.setPlatformUserName(input);
			break;
		case 15:// 梯口密码变更
			result = IDBTEngine.setPlatformPassword(input);
			break;
		case 16:// 翻转参数变更
			setVol = Integer.parseInt(input);
			if (setVol == 0 || setVol == 1) {
				result = IDBTEngine.setRotationValue(input);
			}
			break;	
		case 17:// 监测网络端口变更
			setVol = Integer.parseInt(input);
			if (setVol >= 0 && setVol <= 65535) {
				result = IDBTEngine.setMonitorPort(input);
			}
			break;	
		case 18:// 调试模式变更
			setVol = Integer.parseInt(input);
			if (setVol == 0 || setVol == 1) {
				iDebugMode = setVol;
				result = IDBTEngine.setDebugMode(input);
			}
			break;	
		case 19:// 网络环境变更
			setVol = Integer.parseInt(input);
			if (setVol == 0 || setVol == 1) {
				result = IDBTEngine.setNetMode(input);
			}
			break;	
		case 20:// 视频分辨率变更 add by chensq
			setVol = Integer.parseInt(input);
			if (setVol == 0 || setVol == 1 || setVol == 2) {
				result = IDBTEngine.setVideoFormat(input);
				System.out.printf("CSQ setVideoFormat result:" + result + "\n");
			}
			break;	
		case 21:// 协商模式变更 add by chensq
			setVol = Integer.parseInt(input);
			if (setVol == 0 || setVol == 1) {
				result = IDBTEngine.setNegotiateMode(input);
				System.out.printf("CSQ setNegotiateMode result:" + result + "\n");
			}
			break;	
		}
		return result;
	}

}
