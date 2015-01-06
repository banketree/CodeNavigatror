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
 *	版权所有  (c)2012,   邮科<p>	
 *  文件名称	：SplashActivity.java<p>
 *
 *  内容摘要	：activity for welcome start application
 *
 *  作者	：林利澍
 *  创建时间	：2012-12-31 上午10:51:02 
 *  当前版本号：v1.0
 *  历史记录	:
 *  日期	: 2012-12-31 上午10:51:02 	修改人：
 *  描述	:
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
