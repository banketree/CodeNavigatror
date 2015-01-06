package com.fsti.ladder.common;

import android.content.Context;
import android.database.Cursor;
import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteDatabase.CursorFactory;
import android.database.sqlite.SQLiteOpenHelper;
import android.util.Log;

public class CreateCacheDB extends SQLiteOpenHelper {

	private Context context;
	public static final String DB_NAME = "idbtdb";
	public static final String DB_TABLE_NAME = "increment_service";
	public static final int DB_VERSION = 1;
	private static final String DB_CREATE = "CREATE TABLE "
			+ DB_TABLE_NAME
			+ " (_url TEXT PRIMARY KEY,_id TEXT UNIQUE NOT NULL,"
			+ "_end TEXT);";
	final static String tag = "huangbin";
	
	public CreateCacheDB(Context context, String name, CursorFactory factory,
			int version) {
		super(context, name, factory, version);
		// TODO Auto-generated constructor stub
		this.context = context;
	}

	@Override
	public void onCreate(SQLiteDatabase db) {
		// TODO Auto-generated method stub

		if (!isTableExist(db, "TABLE_SECURITY")) {
			try {
				db.execSQL(DB_CREATE);
			} catch (SQLException e) {
				Log.e(tag, "", e);
			}
		}
	}

	@Override
	public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
		// TODO Auto-generated method stub

	}

	/**
	 * 判断该db中某个表是否存在
	 * 
	 * @param db
	 * @param name
	 * @return
	 */
	private boolean isTableExist(SQLiteDatabase db, String name) {
		boolean isExist = false;
		try {
			Cursor cursor = null;
			String sql = "select count(1) as c from sqlite_master where type ='table' and name ='"
					+ name.trim() + "' ";
			cursor = db.rawQuery(sql, null);
			if (cursor.moveToNext()) {
				int count = cursor.getInt(0);
				if (count > 0) {
					isExist = true;
				}
			}
			cursor.close();

		} catch (Exception e) {
			// TODO: handle exception
		}
		return isExist;
	}

}