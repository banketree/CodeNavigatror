package com.fsti.ladder.activity;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.view.animation.AlphaAnimation;
import android.view.animation.Animation;
import android.view.animation.Animation.AnimationListener;
import com.fsti.ladder.R;
/**
 * 
 *	��Ȩ����  (c)2012,   �ʿ�<p>	
 *  �ļ�����	��SplashActivity.java<p>
 *
 *  ����ժҪ	��activity for welcome start application
 *
 *  ����	��������
 *  ����ʱ��	��2012-12-31 ����10:51:02 
 *  ��ǰ�汾�ţ�v1.0
 *  ��ʷ��¼	:
 *  ����	: 2012-12-31 ����10:51:02 	�޸��ˣ�
 *  ����	:
 */
public class SplashActivity extends Activity {
	private View view;
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		view = View.inflate(this, R.layout.activity_splash, null);
		setContentView(view);
		
		
	}
	@Override
	protected void onStart() {
		// TODO Auto-generated method stub
		super.onStart();
		AlphaAnimation aa = new AlphaAnimation(0.3f,1.0f);
		aa.setDuration(2000);
		view.startAnimation(aa);
		aa.setAnimationListener(new AnimationListener()
		{
			@Override
			public void onAnimationEnd(Animation arg0) {
				Intent intent = new Intent(SplashActivity.this, AdvertActivity.class);
		        startActivity(intent);
		        finish();
			}
			@Override
			public void onAnimationRepeat(Animation animation) {}
			@Override
			public void onAnimationStart(Animation animation) {
				Intent intent = new Intent("action.intent.start.IDBT");
				startService(intent);
			}
			
		});
		
	}
	

}
