package org.coolreader.db;

import org.coolreader.crengine.L;
import org.coolreader.crengine.Logger;

import android.database.Cursor;
import android.database.sqlite.SQLiteStatement;
import android.util.Log;

public class CoverDB extends BaseDB {

	public static final Logger log = L.create("cdb");
	
	public final int DB_VERSION = 8;
	private final static boolean CLEAR_ON_START = false;

	private final static String[] COVERPAGE_SCHEMA = new String[] {
		"CREATE TABLE IF NOT EXISTS coverpage (" +
		"book_fk INTEGER NOT NULL REFERENCES book (id)," +
		"imagedata BLOB NULL" +
		")"
	};
	
	@Override
	protected boolean upgradeSchema() {
		if (mDB.needUpgrade(DB_VERSION)) {
			execSQL(COVERPAGE_SCHEMA);
			int currentVersion = mDB.getVersion();
			// ====================================================================
			// add more updates here
			
			// ====================================================================
			// set current version
			if ( currentVersion<DB_VERSION )
				mDB.setVersion(DB_VERSION);
		}

		dumpStatistics();
	
		if (CLEAR_ON_START) {
			log.w("CLEAR_ON_START is ON: removing all coverpages from DB");
			execSQLIgnoreErrors("DELETE FROM coverpage");
		}
		
		return true;
	}

	@Override
	protected String dbFileName() {
		return "cr3db_cover.sqlite";
	}

	private void dumpStatistics() {
		log.i("coverDB: " + longQuery("SELECT count(*) FROM coverpage") + " coverpages");
	}

	public void clearCaches() {
		coverpageCache.clear();
	}
	
    private static final int COVERPAGE_CACHE_SIZE = 512 * 1024;
    private ByteArrayCache coverpageCache = new ByteArrayCache(COVERPAGE_CACHE_SIZE);
    
	public void saveBookCoverpage( long bookId, byte[] data )
	{
		byte[] oldData = coverpageCache.get(bookId);
		if (oldData != null)
			return; // already in cache
		// update cache and DB
		coverpageCache.put(bookId, data);
		
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
		byte[] data = coverpageCache.get(bookId);
		if (data != null)
			return data;
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
		coverpageCache.remove(bookId);
		if (!isOpened())
			return;
		execSQLIgnoreErrors("DELETE FROM coverpage WHERE book_fk=" + bookId);
	}
}
