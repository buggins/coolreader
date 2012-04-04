package org.coolreader.db;

import android.database.Cursor;
import android.database.sqlite.SQLiteStatement;
import android.util.Log;

public class CoverDB extends BaseDB {

	public final int DB_VERSION = 8;

	private final static String[] COVERPAGE_SCHEMA = new String[] {
		"CREATE TABLE IF NOT EXISTS coverpage (" +
		"book_fk INTEGER NOT NULL REFERENCES book (id)," +
		"imagedata BLOB NULL" +
		")"
	};
	
	@Override
	protected boolean upgradeSchema() {
		execSQL(COVERPAGE_SCHEMA);
		int currentVersion = mDB.getVersion();
		// ====================================================================
		// TODO: add more updates here
		
		// ====================================================================
		// set current version
		if ( currentVersion<DB_VERSION )
			mDB.setVersion(DB_VERSION);
		return true;
	}

	@Override
	protected String dbFileName() {
		return "cr3db_cover.sqlite";
	}

	public void saveBookCoverpage( long bookId, byte[] data )
	{
		if (!isOpened())
			return;
		if ( data==null )
			return;
		ensureOpened();
		SQLiteStatement stmt = null;
		try {
			Long existing = longQuery("SELECT book_fk FROM coverpage WHERE book_fk=" + bookId);
			if (existing == null) {
				stmt = mDB.compileStatement("INSERT INTO coverpage (book_fk, imagedata) VALUES ("+bookId+", ?)");
				stmt.bindBlob(1, data);
				stmt.execute();
				Log.v("cr3", "db: saved " + data.length + " bytes of cover page for book " + bookId);
			}
		} catch ( Exception e ) {
			Log.e("cr3", "Exception while trying to save cover page to DB: " + e.getMessage() );
		} finally {
			if ( stmt!=null )
				stmt.close();
		}
	}

	public byte[] loadBookCoverpage(long bookId)
	{
		if (!isOpened())
			return null;
		Cursor rs = null;
		try {
			rs = mDB.rawQuery("SELECT imagedata FROM coverpage WHERE book_fk=" + bookId, null);
			if ( rs.moveToFirst() ) {
				return rs.getBlob(0);
			}
			return null;
		} catch ( Exception e ) {
			Log.e("cr3", "error while reading coverpage for book " + bookId + ": " + e.getMessage());
			return null;
		} finally {
			if ( rs!=null )
				rs.close();
		}
	}
	
	public void deleteCoverpage(long bookId) {
		if (!isOpened())
			return;
		execSQLIgnoreErrors("DELETE FROM coverpage WHERE book_fk=" + bookId);
	}
}
