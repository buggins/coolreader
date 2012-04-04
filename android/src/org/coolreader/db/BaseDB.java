package org.coolreader.db;

import java.io.File;

import org.coolreader.crengine.L;
import org.coolreader.crengine.Utils;

import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteException;
import android.database.sqlite.SQLiteStatement;
import android.util.Log;

public abstract class BaseDB {
	protected SQLiteDatabase mDB;
	private File mFileName;
	private boolean restoredFromBackup;
	private boolean error = true;

	public File getFileName() {
		return mFileName;
	}

	public boolean isOpened() {
		return mDB != null && !error;
	}

	public boolean open(File dir) {
		error = true;
		File dbFile = new File(dir, dbFileName());
		mFileName = dbFile;
		mDB = openDB(dbFile);
		if (mDB == null)
			return false;
		boolean res = checkSchema();
		if (!res) {
			L.e("Closing DB due error while upgrade of schema: " + dbFile.getAbsolutePath());
			close();
			Utils.moveCorruptedFileToBackup(dbFile);
			if (!restoredFromBackup)
				Utils.restoreFromBackup(dbFile);
			mDB = openDB(dbFile);
			res = checkSchema();
			if (!res)
				close();
		}
		if (mDB != null) {
			error = false;
			return true;
		}
		return false;
	}

	public boolean close() {
		if (mDB != null) {
			try {
				mDB.close();
				mDB = null;
				return true;
			} catch (SQLiteException e) {
				L.e("Error while closing DB " + mFileName);
			}
			mDB = null;
		}
		return false;
	}

	protected boolean checkSchema() {
		try {
			upgradeSchema();
			return true;
		} catch (SQLiteException e) {
			return false;
		}
	}

	protected abstract boolean upgradeSchema();

	protected abstract String dbFileName();
	
	private SQLiteDatabase openDB(File dbFile) {
		restoredFromBackup = false;
		SQLiteDatabase db = null;
		try {
			db = SQLiteDatabase.openOrCreateDatabase(dbFile, null);
			return db;
		} catch (SQLiteException e) {
			L.e("Error while opening DB " + dbFile.getAbsolutePath());
			Utils.moveCorruptedFileToBackup(dbFile);
			restoredFromBackup = Utils.restoreFromBackup(dbFile);
			try {
				db = SQLiteDatabase.openOrCreateDatabase(dbFile, null);
				return db;
			} catch (SQLiteException ee) {
				L.e("Error while opening DB " + dbFile.getAbsolutePath());
			}
		}
		return null;
	}

	public void execSQLIgnoreErrors( String... sqls )
	{
		for ( String sql : sqls ) {
			try { 
				mDB.execSQL(sql);
			} catch ( SQLException e ) {
				// ignore
				Log.w("cr3db", "query failed, ignoring: " + sql);
			}
		}
	}
	
	protected void ensureOpened() {
		if (!isOpened())
			throw new DBRuntimeException("DB is not opened");
	}

	public void execSQL( String... sqls )
	{
		ensureOpened();
		for ( String sql : sqls ) {
			try { 
				mDB.execSQL(sql);
			} catch ( SQLException e ) {
				// ignore
				Log.w("cr3", "query failed: " + sql);
				throw e;
			}
		}
	}

	public Long longQuery( String sql )
	{
		ensureOpened();
		SQLiteStatement stmt = null;
		try {
			stmt = mDB.compileStatement(sql);
			return stmt.simpleQueryForLong();
		} catch ( Exception e ) {
			// not found or error
			return null;
		} finally {
			if (stmt != null)
				stmt.close();
		}
	}
	
	public static String quoteSqlString(String src) {
		if (src==null)
			return "null";
		String s = src.replaceAll("\\'", "\\\\'");
		return "'" + s + "'";
	}
	
}
