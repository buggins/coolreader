package org.coolreader.db;

import org.coolreader.crengine.L;
import org.coolreader.crengine.Logger;

import android.database.Cursor;
import android.database.sqlite.SQLiteStatement;
import android.util.Log;

public class CoverDB extends BaseDB {

	public static final Logger log = L.create("cdb");
	
	public final int DB_VERSION = 9;
	private final static boolean CLEAR_ON_START = false;

	private final static String[] COVERPAGE_SCHEMA = new String[] {
		"CREATE TABLE IF NOT EXISTS coverpages (" +
		"book_path VARCHAR NOT NULL PRIMARY KEY," +
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
			
			if (currentVersion < 9)
				execSQLIgnoreErrors("DROP TABLE coverpage");
			// ====================================================================
			// set current version
			if ( currentVersion<DB_VERSION )
				mDB.setVersion(DB_VERSION);
		}

		dumpStatistics();
	
		if (CLEAR_ON_START) {
			log.w("CLEAR_ON_START is ON: removing all coverpages from DB");
			execSQLIgnoreErrors("DELETE FROM coverpages");
		}
		
		return true;
	}

	@Override
	protected String dbFileName() {
		return "cr3db_cover.sqlite";
	}

	private void dumpStatistics() {
		log.i("coverDB: " + longQuery("SELECT count(*) FROM coverpages") + " coverpages");
	}

	public void clearCaches() {
		coverpageCache.clear();
	}
	
    private static final int COVERPAGE_CACHE_SIZE = 512 * 1024;
    private ByteArrayCache coverpageCache = new ByteArrayCache(COVERPAGE_CACHE_SIZE);
    
	public void saveBookCoverpage(String bookId, byte[] data)
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
			String existing = stringQuery("SELECT book_path FROM coverpages WHERE book_path=" + quoteSqlString(bookId));
			if (existing == null) {
				stmt = mDB.compileStatement("INSERT INTO coverpages (book_path, imagedata) VALUES (?, ?)");
				stmt.bindString(1, bookId);
				stmt.bindBlob(2, data);
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

	public byte[] loadBookCoverpage(String bookId)
	{
		byte[] data = coverpageCache.get(bookId);
		if (data != null)
			return data;
		if (!isOpened())
			return null;
		Cursor rs = null;
		try {
			rs = mDB.rawQuery("SELECT imagedata FROM coverpages WHERE book_path=" + quoteSqlString(bookId), null);
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
	
	public void deleteCoverpage(String bookId) {
		coverpageCache.remove(bookId);
		if (!isOpened())
			return;
		execSQLIgnoreErrors("DELETE FROM coverpages WHERE book_path=" + quoteSqlString(bookId));
	}
}
