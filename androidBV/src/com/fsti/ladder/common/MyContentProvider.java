package com.fsti.ladder.common;

import java.util.HashMap;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.UriMatcher;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.net.Uri;

/**
 * 
 * 版权所有 (c)2012, 邮科
 * <p>
 * 文件名称 ：IDoorCacheContentProvider.java
 * <p>
 * 内容摘要 ：本地数据库内容提供器，用于对本地缓存数据库的增删改查 作者 ：卞勇屏
 */
public class MyContentProvider extends ContentProvider {
	public static final String AUTHORITY = "com.fsti.ladder.common";
	public static final String PATH_SINGLE = "increment_service/#";
	public static final String PATH_MULTIPLE = "increment_service";
	public static final Uri content_URI = Uri.parse("content://" + AUTHORITY
			+ "/" + PATH_MULTIPLE);
	public static final String DEFAULT_SORT_ORDER = "name DESC";
	public static final String _ID = "_id";
	public static final String _URL = "_url";
	public static final String _END = "_end";
	public static final int SINGLE_NUM = 1;
	public static final int MULTIPLE_NUM = 2;
	private static UriMatcher URI_MATCHER;
	private static HashMap<String, String> PROJECTION_MAP;
	public static String DB_NAME = "idbtdb";
	public static String DB_TABLE_NAME = "increment_service";
	private CreateCacheDB cacheDB = null;
	private SQLiteDatabase mDatabase;

	@Override
	public boolean onCreate() {
		// TODO Auto-generated method stub
		cacheDB = new CreateCacheDB(this.getContext(), DB_NAME, null, 1);
		return false;
	}

	@Override
	public Cursor query(Uri uri, String[] projection, String selection,
			String[] selectionArgs, String sortOrder) {
		// TODO Auto-generated method stub

		mDatabase = cacheDB.getReadableDatabase();

		// Log.v("db_path", mDatabase.getPath());
		String tablename = uri.getPath().substring(1);
		Cursor cur = mDatabase.query(tablename, projection, selection,
				selectionArgs, null, null, sortOrder);
		return cur;
	}

	@Override
	public String getType(Uri uri) {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public Uri insert(Uri uri, ContentValues values) {
		// TODO Auto-generated method stub
		mDatabase = cacheDB.getReadableDatabase();
		String tablename = uri.getPath().substring(1);
		mDatabase.insert(tablename, null, values);
		return null;
	}

	@Override
	public int delete(Uri uri, String selection, String[] selectionArgs) {
		// TODO Auto-generated method stub
		mDatabase = cacheDB.getReadableDatabase();
		String tablename = uri.getPath().substring(1);
		mDatabase.delete(tablename, selection, selectionArgs);
		return 0;
	}

	@Override
	public int update(Uri uri, ContentValues values, String selection,
			String[] selectionArgs) {
		// TODO Auto-generated method stub
		mDatabase = cacheDB.getReadableDatabase();
		String tablename = uri.getPath().substring(1);
		mDatabase.update(tablename, values, selection, selectionArgs);
		return 0;
	}

}

