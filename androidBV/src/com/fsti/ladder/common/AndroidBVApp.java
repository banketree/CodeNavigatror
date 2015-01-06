package com.fsti.ladder.common;

import android.app.Application;

public class AndroidBVApp extends Application {
	private static AndroidBVApp sInstance;

	public AndroidBVApp() {

		sInstance = this;
	}

	public static AndroidBVApp getInstance() {

		return sInstance;
	}
}
