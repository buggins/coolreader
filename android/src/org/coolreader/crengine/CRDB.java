package org.coolreader.crengine;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;

import android.database.Cursor;
import android.database.DatabaseUtils;
import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteDiskIOException;
import android.database.sqlite.SQLiteStatement;
import android.util.Log;

public class CRDB {
	static final boolean DROP_TABLES = false; // for debug purposes
	SQLiteDatabase mDB;
	File mDBFile;
	SQLiteDatabase mCoverpageDB;
	File mCoverpageDBFile;

	private boolean moveToBackup(File f) {
		Log.e("cr3", "Moving corrupted DB file to backup.");
		File f2 = null;
		for (int i=2; i<100; i++) {
			f2 = new File(f.getAbsoluteFile() + ".bak." + i);
			if (!f2.exists())
				break;
		}
		if (!f.renameTo(f2)) {
			Log.e("cr3", "Cannot rename DB file " + f + " to " + f2);
			if (!f.delete()) {
				Log.e("cr3", "Cannot remove DB file " + f);
				return false;
			}
		}
		return true;
	}

	protected boolean open( File dbfile )
	{
		L.i("Opening database from " + dbfile.getAbsolutePath());
		try {
			this.mDB = SQLiteDatabase.openOrCreateDatabase(dbfile, null);
		} catch (SQLiteDiskIOException e) {
			moveToBackup(dbfile);
			this.mDB = SQLiteDatabase.openOrCreateDatabase(dbfile, null);
		}
		this.mDBFile = dbfile;
		File coverFile = new File(dbfile.getAbsolutePath().replace(".sqlite", "_cover.sqlite"));
		try {
			this.mCoverpageDB = SQLiteDatabase.openOrCreateDatabase(coverFile, null);
		} catch (SQLiteDiskIOException e) {
			moveToBackup(coverFile);
			this.mDB = SQLiteDatabase.openOrCreateDatabase(coverFile, null);
		}
		this.mCoverpageDBFile = coverFile;
		return true;
	}

	protected void dropTables()
	{
		String[] tableNames = new String[] {
			"book_author", "bookmark", "book", "series", "author", "folder", "coverpage"	
		};
		for ( String name : tableNames )
			mDB.execSQL("DROP TABLE IF EXISTS " + name);
		mCoverpageDB.execSQL("DROP TABLE IF EXISTS coverpage");
	}
	
	private void execSQLIgnoreErrors( String... sqls )
	{
		for ( String sql : sqls ) {
			try { 
				mDB.execSQL(sql);
			} catch ( SQLException e ) {
				// ignore
				Log.w("cr3", "query failed, ignoring: " + sql);
			}
		}
	}

	private void execSQLCoverpageIgnoreErrors( String... sqls )
	{
		for ( String sql : sqls ) {
			try { 
				mCoverpageDB.execSQL(sql);
			} catch ( SQLException e ) {
				// ignore
				Log.w("cr3", "cp query failed, ignoring: " + sql);
			}
		}
	}

	private void execSQL( String... sqls )
	{
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

	private void execCoverpageSQL( String... sqls )
	{
		for ( String sql : sqls ) {
			try { 
				mCoverpageDB.execSQL(sql);
			} catch ( SQLException e ) {
				// ignore
				Log.w("cr3", "cp query failed: " + sql);
				throw e;
			}
		}
	}

	private final static String[] COVERPAGE_SCHEMA = new String[] {
		"CREATE TABLE IF NOT EXISTS coverpage (" +
		"book_fk INTEGER NOT NULL REFERENCES book (id)," +
		"imagedata BLOB NULL" +
		")"
	};
	
	public final int DB_VERSION = 7;
	protected boolean updateSchema()
	{
		if (DROP_TABLES)
			dropTables();
		execSQL("CREATE TABLE IF NOT EXISTS author (" +
				"id INTEGER PRIMARY KEY AUTOINCREMENT," +
				"name VARCHAR NOT NULL COLLATE NOCASE" +
				")");
		execSQL("CREATE INDEX IF NOT EXISTS " +
                "author_name_index ON author (name) ");
		execSQL("CREATE TABLE IF NOT EXISTS series (" +
				"id INTEGER PRIMARY KEY AUTOINCREMENT," +
				"name VARCHAR NOT NULL COLLATE NOCASE" +
				")");
		execSQL("CREATE INDEX IF NOT EXISTS " +
		        "series_name_index ON series (name) ");
		execSQL("CREATE TABLE IF NOT EXISTS folder (" +
				"id INTEGER PRIMARY KEY AUTOINCREMENT," +
				"name VARCHAR NOT NULL" +
				")");
		execSQL("CREATE INDEX IF NOT EXISTS " +
				"folder_name_index ON folder (name) ");
		execSQL("CREATE TABLE IF NOT EXISTS book (" +
				"id INTEGER PRIMARY KEY AUTOINCREMENT," +
				"pathname VARCHAR NOT NULL," +
				"folder_fk INTEGER REFERENCES folder (id)," +
				"filename VARCHAR NOT NULL," +
				"arcname VARCHAR," +
				"title VARCHAR COLLATE NOCASE," +
				"series_fk INTEGER REFERENCES series (id)," +
				"series_number INTEGER," +
				"format INTEGER," +
				"filesize INTEGER," +
				"arcsize INTEGER," +
				"create_time INTEGER," +
				"last_access_time INTEGER, " +
				"flags INTEGER DEFAULT 0" +
				")");
		execSQL("CREATE INDEX IF NOT EXISTS " +
				"book_folder_index ON book (folder_fk) ");
		execSQL("CREATE INDEX IF NOT EXISTS " +
				"book_pathname_index ON book (pathname) ");
		execSQL("CREATE INDEX IF NOT EXISTS " +
				"book_filename_index ON book (filename) ");
		execSQL("CREATE INDEX IF NOT EXISTS " +
				"book_title_index ON book (title) ");
		execSQL("CREATE INDEX IF NOT EXISTS " +
				"book_last_access_time_index ON book (last_access_time) ");
		execSQL("CREATE INDEX IF NOT EXISTS " +
				"book_title_index ON book (title) ");
		execSQL("CREATE TABLE IF NOT EXISTS book_author (" +
				"book_fk INTEGER NOT NULL REFERENCES book (id)," +
				"author_fk INTEGER NOT NULL REFERENCES author (id)," +
				"PRIMARY KEY (book_fk, author_fk)" +
				")");
		execSQL("CREATE UNIQUE INDEX IF NOT EXISTS " +
				"author_book_index ON book_author (author_fk, book_fk) ");
		execSQL("CREATE TABLE IF NOT EXISTS bookmark (" +
				"id INTEGER PRIMARY KEY AUTOINCREMENT," +
				"book_fk INTEGER NOT NULL REFERENCES book (id)," +
				"type INTEGER NOT NULL DEFAULT 0," +
				"percent INTEGER DEFAULT 0," +
				"shortcut INTEGER DEFAULT 0," +
				"time_stamp INTEGER DEFAULT 0," +
				"start_pos VARCHAR NOT NULL," +
				"end_pos VARCHAR," +
				"title_text VARCHAR," +
				"pos_text VARCHAR," +
				"comment_text VARCHAR" +
				")");
		execSQL("CREATE INDEX IF NOT EXISTS " +
		"bookmark_book_index ON bookmark (book_fk) ");
		execCoverpageSQL(COVERPAGE_SCHEMA);
		int currentVersion = mDB.getVersion();
		// version 1 updates ====================================================================
		if ( currentVersion<1 )
			execSQLIgnoreErrors("ALTER TABLE bookmark ADD COLUMN shortcut INTEGER DEFAULT 0");
		if ( currentVersion<3 )
			execSQLIgnoreErrors(COVERPAGE_SCHEMA);
		if ( currentVersion<4 )
			execSQLIgnoreErrors("ALTER TABLE book ADD COLUMN flags INTEGER DEFAULT 0");
		if ( currentVersion>0 && currentVersion<5 )
			migrateCoverpages();
		if ( currentVersion<6 )
			execSQL("CREATE TABLE IF NOT EXISTS opds_catalog (" +
					"id INTEGER PRIMARY KEY AUTOINCREMENT, " +
					"name VARCHAR NOT NULL COLLATE NOCASE, " +
					"url VARCHAR NOT NULL COLLATE NOCASE" +
					")");
		if ( currentVersion<7 )
			addOPDSCatalogs(DEF_OPDS_URLS1);
		// TODO: add more updates here
			
		// set current version
		if ( currentVersion<DB_VERSION )
			mDB.setVersion(DB_VERSION);
		return true;
	}
	
	private final static String[] DEF_OPDS_URLS1 = {
			"http://www.feedbooks.com/catalog.atom", "Feedbooks",
			"http://bookserver.archive.org/catalog/", "Internet Archive",
			"http://m.gutenberg.org/", "Project Gutenberg", 
//			"http://ebooksearch.webfactional.com/catalog.atom", "eBookSearch", 
			"http://bookserver.revues.org/", "Revues.org", 
			"http://www.legimi.com/opds/root.atom", "Legimi",
			"http://www.ebooksgratuits.com/opds/", "Ebooks libres et gratuits",
			"http://flibusta.net/opds/", "Flibusta", 
//			"http://lib.ololo.cc/opds/", "lib.ololo.cc",
	};
	
	private void addOPDSCatalogs(String[] catalogs) {
		for (int i=0; i<catalogs.length-1; i+=2) {
			String url = catalogs[i];
			String name = catalogs[i+1];
			saveOPDSCatalog(null, url, name);
		}
	}

	private static String quoteSqlString(String src) {
		if (src==null)
			return "null";
		String s = src.replaceAll("\\'", "\\\\'");
		return "'" + s + "'";
	}
	
	public boolean saveOPDSCatalog(Long id, String url, String name) {
		if (url==null || name==null)
			return false;
		url = url.trim();
		name = name.trim();
		if (url.length()==0 || name.length()==0)
			return false;
		try {
			Long existingIdByUrl = longQuery("SELECT id FROM opds_catalog WHERE url=" + quoteSqlString(url));
			Long existingIdByName = longQuery("SELECT id FROM opds_catalog WHERE name=" + quoteSqlString(name));
			if (existingIdByUrl!=null && existingIdByName!=null && !existingIdByName.equals(existingIdByUrl))
				return false; // duplicates detected
			if (id==null) {
				id = existingIdByUrl;
				if (id==null)
					id = existingIdByName;
			}
			if (id==null) {
				// insert new
				execSQL("INSERT INTO opds_catalog (name, url) VALUES ("+quoteSqlString(name)+", "+quoteSqlString(url)+")");
			} else {
				// update existing
				execSQL("UPDATE opds_catalog SET name="+quoteSqlString(name)+", url="+quoteSqlString(url)+" WHERE id=" + id);
			}
				
		} catch (Exception e) {
			Log.e("cr3", "exception while saving OPDS catalog item", e);
			return false;
		}
		return true;
	}

	public boolean loadOPDSCatalogs(FileInfo parent) {
		Log.i("cr3", "loadOPDSCatalogs()");
		boolean found = false;
		Cursor rs = null;
		try {
			String sql = "SELECT id, name, url FROM opds_catalog";
			rs = mDB.rawQuery(sql, null);
			if ( rs.moveToFirst() ) {
				// remove existing entries
				parent.clear();
				// read DB
				do {
					Long id = rs.getLong(0);
					String name = rs.getString(1);
					String url = rs.getString(2);
					FileInfo opds = new FileInfo();
					opds.isDirectory = true;
					opds.pathname = FileInfo.OPDS_DIR_PREFIX + url;
					opds.filename = name;
					opds.isListed = true;
					opds.isScanned = true;
					opds.parent = parent;
					opds.id = id;
					parent.addDir(opds);
					found = true;
				} while (rs.moveToNext());
			}
		} catch (Exception e) {
			Log.e("cr3", "exception while loading list of OPDS catalogs", e);
		} finally {
			if ( rs!=null )
				rs.close();
		}
		return found;
	}
	
	public void removeOPDSCatalog(Long id) {
		Log.i("cr3", "removeOPDSCatalog(" + id + ")");
		execSQLIgnoreErrors("DELETE FROM opds_catalog WHERE id = " + id);
	}

	private void migrateCoverpages() {
		Thread migrationThread = new Thread() {
			@Override
			public void run() {
				Log.i("cr3", "Migration thread is started");
				try {
					String sql = "SELECT book_fk, imagedata FROM coverpage";
					Cursor rs = null;
					try {
						rs = mDB.rawQuery(sql, null);
						if ( rs.moveToFirst() ) {
							do {
								long id = rs.getLong(0);
								byte[] data = rs.getBlob(1);
								if (data!=null && data.length>0) {
									Log.i("cr3", "Moving coverpage for bookId=" + id + " (" + data.length + " bytes)");
									saveBookCoverpage(id, data);
								}
							} while (rs.moveToNext());
							execSQLIgnoreErrors("DROP TABLE IF EXISTS coverpage");
						}
					} finally {
						if (rs!=null)
							rs.close();
					}
				} catch (Exception e) {
					Log.e("cr3", "Exception while moving cover pages", e);
				}
				Log.i("cr3", "Migration thread is finished");
			}
			
		};
		migrationThread.start();
	}
	
	public CRDB( File dbfile )
	{
		open(dbfile);
		updateSchema();
		dumpStatistics();
	}
	
	public boolean findByPathname( FileInfo fileInfo )
	{
		return findBy( fileInfo, "pathname", fileInfo.getPathName());
	}

	public boolean findById( FileInfo fileInfo )
	{
		return findBy( fileInfo, "b.id", fileInfo.id);
	}

	private static final String READ_BOOKMARK_SQL = 
		"SELECT " +
		"id, type, percent, shortcut, time_stamp, " + 
		"start_pos, end_pos, title_text, pos_text, comment_text " +
		"FROM bookmark b ";
	private void readBookmarkFromCursor( Bookmark v, Cursor rs )
	{
		int i=0;
		v.setId( rs.getLong(i++) );
		v.setType( (int)rs.getLong(i++) );
		v.setPercent( (int)rs.getLong(i++) );
		v.setShortcut( (int)rs.getLong(i++) );
		v.setTimeStamp( rs.getLong(i++) );
		v.setStartPos( rs.getString(i++) );
		v.setEndPos( rs.getString(i++) );
		v.setTitleText( rs.getString(i++) );
		v.setPosText( rs.getString(i++) );
		v.setCommentText( rs.getString(i++) );
		v.setModified(false);
	}
	synchronized public boolean findBy( Bookmark v, String condition )
	{
		boolean found = false;
		Cursor rs = null;
		try {
			condition = " WHERE " + condition;
			rs = mDB.rawQuery(READ_BOOKMARK_SQL +
					condition, null);
			if ( rs.moveToFirst() ) {
				readBookmarkFromCursor( v, rs );
				found = true;
			}
		} finally {
			if ( rs!=null )
				rs.close();
		}
		return found;
	}

	synchronized public boolean load( ArrayList<Bookmark> list, String condition )
	{
		boolean found = false;
		Cursor rs = null;
		try {
			condition = " WHERE " + condition;
			rs = mDB.rawQuery(READ_BOOKMARK_SQL +
					condition, null);
			if ( rs.moveToFirst() ) {
				do {
					Bookmark v = new Bookmark();
					readBookmarkFromCursor( v, rs );
					list.add(v);
					found = true;
				} while ( rs.moveToNext() );
			}
		} finally {
			if ( rs!=null )
				rs.close();
		}
		return found;
	}

	private static final String READ_FILEINFO_FIELDS = 
		"b.id AS id, pathname," +
		"f.name as path, " +
		"filename, arcname, title, " +
		"(SELECT GROUP_CONCAT(a.name,'|') FROM author a JOIN book_author ba ON a.id=ba.author_fk WHERE ba.book_fk=b.id) as authors, " +
		"s.name as series_name, " +
		"series_number, " +
		"format, filesize, arcsize, " +
		"create_time, last_access_time, flags ";
	
	private static final String READ_FILEINFO_SQL = 
		"SELECT " +
		READ_FILEINFO_FIELDS +
		"FROM book b " +
		"LEFT JOIN series s ON s.id=b.series_fk " +
		"LEFT JOIN folder f ON f.id=b.folder_fk ";
	private void readFileInfoFromCursor( FileInfo fileInfo, Cursor rs )
	{
		int i=0;
		fileInfo.id = rs.getLong(i++);
		String pathName = rs.getString(i++);
		String[] parts = FileInfo.splitArcName(pathName);
		fileInfo.pathname = parts[0];
		fileInfo.path = rs.getString(i++);
		fileInfo.filename = rs.getString(i++);
		fileInfo.arcname = rs.getString(i++);
		fileInfo.title = rs.getString(i++);
		fileInfo.authors = rs.getString(i++);
		fileInfo.series = rs.getString(i++);
		fileInfo.seriesNumber = rs.getInt(i++);
		fileInfo.format = DocumentFormat.byId(rs.getInt(i++));
		fileInfo.size = rs.getInt(i++);
		fileInfo.arcsize = rs.getInt(i++);
		fileInfo.createTime = rs.getInt(i++);
		fileInfo.lastAccessTime = rs.getInt(i++);
		fileInfo.flags = rs.getInt(i++);
		fileInfo.isArchive = fileInfo.arcname!=null; 
	}
	
	
	synchronized public boolean findBy( FileInfo fileInfo, String fieldName, Object fieldValue )
	{
		String condition;
		StringBuilder buf = new StringBuilder(" WHERE ");
		buf.append(fieldName);
		if ( fieldValue==null ) {
			buf.append(" IS NULL ");
		} else {
			buf.append("=");
			DatabaseUtils.appendValueToSql(buf, fieldValue);
			buf.append(" ");
		}
		condition = buf.toString();
		boolean found = false;
		Cursor rs = null;
		try { 
			rs = mDB.rawQuery(READ_FILEINFO_SQL +
					condition, null);
			if ( rs.moveToFirst() ) {
				readFileInfoFromCursor( fileInfo, rs );
				found = true;
			}
		} finally {
			if ( rs!=null )
				rs.close();
		}
		return found;
	}

	synchronized public FileInfo[] findByPatterns( int maxCount, String author, String title, String series, String filename )
	{
		ArrayList<FileInfo> list = new ArrayList<FileInfo>();
		
		StringBuilder buf = new StringBuilder();
		if ( author!=null && author.length()>0 ) {
			if ( buf.length()>0 )
				buf.append(" AND ");
			buf.append(" b.id IN (SELECT ba.book_fk FROM author a JOIN book_author ba ON a.id=ba.author_fk WHERE a.name LIKE ");
			DatabaseUtils.appendValueToSql(buf, author);
			buf.append(") ");
		}
		if ( series!=null && series.length()>0 ) {
			if ( buf.length()>0 )
				buf.append(" AND ");
			buf.append(" b.series_fk IN (SELECT s.name FROM series s WHERE s.name LIKE ");
			DatabaseUtils.appendValueToSql(buf, series);
			buf.append(") ");
		}
		if ( title!=null && title.length()>0 ) {
			if ( buf.length()>0 )
				buf.append(" AND ");
			buf.append(" b.title LIKE ");
			DatabaseUtils.appendValueToSql(buf, title);
			buf.append(" ");
		}
		if ( filename!=null && filename.length()>0 ) {
			if ( buf.length()>0 )
				buf.append(" AND ");
			buf.append(" b.filename LIKE ");
			DatabaseUtils.appendValueToSql(buf, filename);
			buf.append(" ");
		}
		if ( buf.length()==0 )
			return new FileInfo[0];
		
		String condition = " WHERE " + buf.toString();
		String sql = READ_FILEINFO_SQL + condition;
		Log.d("cr3", "sql: " + sql );
		if ( condition.length()==0 )
			return new FileInfo[] { };
		Cursor rs = null;
		try { 
			rs = mDB.rawQuery(sql, null);
			if ( rs.moveToFirst() ) {
				int count = 0;
				do {
					FileInfo fi = new FileInfo(); 
					readFileInfoFromCursor( fi, rs );
					list.add(fi);
					count++;
				} while ( count<maxCount && rs.moveToNext() );
			}
		} finally {
			if ( rs!=null )
				rs.close();
		}
		return list.toArray(new FileInfo[list.size()]);
	}
	
	synchronized public boolean findRecentBooks( ArrayList<FileInfo> list, int maxCount, int limit )
	{
		String sql = READ_FILEINFO_SQL + " WHERE last_access_time>0 ORDER BY last_access_time DESC LIMIT " + limit;
		Cursor rs = null;
		boolean found = false;
		try {
			rs = mDB.rawQuery(sql, null);
			if ( rs.moveToFirst() ) {
				do {
					FileInfo fileInfo = new FileInfo();
					readFileInfoFromCursor( fileInfo, rs );
					if ( !fileInfo.fileExists() )
						continue;
					list.add(fileInfo);
					found = true;
					if ( list.size()>maxCount )
						break;
				} while (rs.moveToNext());
			}
		} finally {
			rs.close();
		}
		return found;
	}
	
	private Long longQuery( String sql )
	{
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
	
	private Long longCoverpageQuery( String sql )
	{
		SQLiteStatement stmt = null;
		try {
			stmt = mCoverpageDB.compileStatement(sql);
			return stmt.simpleQueryForLong();
		} catch ( Exception e ) {
			// not found or error
			return null;
		} finally {
			if (stmt != null)
				stmt.close();
		}
	}
	
	synchronized public void saveBookCoverpage( long bookId, byte[] data )
	{
		if ( data==null )
			return;
		SQLiteStatement stmt = null;
		try { 
			Long existing = longCoverpageQuery("SELECT book_fk FROM coverpage WHERE book_fk=" + bookId);
			if ( existing==null ) {
				stmt = mCoverpageDB.compileStatement("INSERT INTO coverpage (book_fk, imagedata) VALUES ("+bookId+", ?)");
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
	synchronized public byte[] loadBookCoverpage( long bookId )
	{
		Cursor rs = null;
		try {
			rs = mCoverpageDB.rawQuery("SELECT imagedata FROM coverpage WHERE book_fk=" + bookId, null);
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
	
	public void dumpStatistics()
	{
		Log.i("cr3db", "DB: " + longQuery("SELECT count(*) FROM author") + " authors, "
				 + longQuery("SELECT count(*) FROM series") + " series, "
				 + longQuery("SELECT count(*) FROM book") + " books, "
				 + longQuery("SELECT count(*) FROM bookmark") + " bookmarks"
				 + longQuery("SELECT count(*) FROM folder") + " folders"
				 + longCoverpageQuery("SELECT count(*) FROM coverpage") + " coverpages"
				 );
	}

	private SQLiteStatement seriesStmt;
	private SQLiteStatement seriesSelectStmt;
	private HashMap<String,Long> seriesCache = new HashMap<String,Long>();
	synchronized public Long getSeriesId( String seriesName )
	{
		if ( seriesName==null || seriesName.trim().length()==0 )
			return null;
		Long id = seriesCache.get(seriesName); 
		if ( id!=null )
			return id;
		if ( seriesSelectStmt==null )
			seriesSelectStmt = mDB.compileStatement("SELECT id FROM series WHERE name=?");
		try {
			seriesSelectStmt.bindString(1, seriesName);
			return seriesSelectStmt.simpleQueryForLong();
		} catch ( Exception e ) {
			// not found
		}
		if ( seriesStmt==null )
			seriesStmt = mDB.compileStatement("INSERT INTO series (id, name) VALUES (NULL,?)");
		seriesStmt.bindString(1, seriesName);
		id = seriesStmt.executeInsert();
		seriesCache.put( seriesName, id );
		return id;
	}
	
	private SQLiteStatement folderStmt;
	private SQLiteStatement folderSelectStmt;
	private HashMap<String,Long> folderCache = new HashMap<String,Long>();
	synchronized public Long getFolderId( String folderName )
	{
		if ( folderName==null || folderName.trim().length()==0 )
			return null;
		Long id = folderCache.get(folderName); 
		if ( id!=null )
			return id;
		if ( folderSelectStmt==null )
			folderSelectStmt = mDB.compileStatement("SELECT id FROM folder WHERE name=?");
		try {
			folderSelectStmt.bindString(1, folderName);
			return folderSelectStmt.simpleQueryForLong();
		} catch ( Exception e ) {
			// not found
		}
		if ( folderStmt==null )
			folderStmt = mDB.compileStatement("INSERT INTO folder (id, name) VALUES (NULL,?)");
		folderStmt.bindString(1, folderName);
		id = folderStmt.executeInsert();
		folderCache.put( folderName, id );
		return id;
	}
	
	private SQLiteStatement authorStmt;
	private SQLiteStatement authorSelectStmt;
	private HashMap<String,Long> authorCache = new HashMap<String,Long>();
	synchronized public Long getAuthorId( String authorName )
	{
		if ( authorName==null || authorName.trim().length()==0 )
			return null;
		Long id = authorCache.get(authorName); 
		if ( id!=null )
			return id;
		if ( authorSelectStmt==null )
			authorSelectStmt = mDB.compileStatement("SELECT id FROM author WHERE name=?");
		try {
			authorSelectStmt.bindString(1, authorName);
			return authorSelectStmt.simpleQueryForLong();
		} catch ( Exception e ) {
			// not found
		}
		if ( authorStmt==null )
			authorStmt = mDB.compileStatement("INSERT INTO author (id, name) VALUES (NULL,?)");
		authorStmt.bindString(1, authorName);
		id = authorStmt.executeInsert();
		authorCache.put( authorName, id );
		return id;
	}
	synchronized public Long[] getAuthorIds( String authorNames )
	{
		if ( authorNames==null || authorNames.trim().length()==0 )
			return null;
		String[] names = authorNames.split("\\|");
		if ( names==null || names.length==0 )
			return null;
		ArrayList<Long> ids = new ArrayList<Long>(names.length);
		for ( String name : names ) {
			Long id = getAuthorId(name);
			if ( id!=null )
				ids.add(id);
		}
		if ( ids.size()>0 )
			return ids.toArray(new Long[ids.size()]);
		return null;
	}
	
	synchronized public void saveBookAuthors( Long bookId, Long[] authors)
	{
		if ( authors==null || authors.length==0 )
			return;
		String insertQuery = "INSERT OR IGNORE INTO book_author (book_fk,author_fk) VALUES ";
		for ( Long id : authors ) {
			String sql = insertQuery + "(" + bookId + "," + id + ")"; 
			//Log.v("cr3", "executing: " + sql);
			mDB.execSQL(sql);
		}
	}

	public static boolean eq(String s1, String s2)
	{
		if ( s1!=null )
			return s1.equals(s2);
		return s2==null;
	}
	
	public class QueryHelper {
		String tableName;
		QueryHelper(String tableName)
		{
			this.tableName = tableName;
		}
		ArrayList<String> fields = new ArrayList<String>(); 
		ArrayList<Object> values = new ArrayList<Object>();
		QueryHelper add(String fieldName, int value, int oldValue )
		{
			if ( value!=oldValue ) {
				fields.add(fieldName);
				values.add(Long.valueOf(value));
			}
			return this;
		}
		QueryHelper add(String fieldName, Long value, Long oldValue )
		{
			if ( value!=null && (oldValue==null || !oldValue.equals(value))) {
				fields.add(fieldName);
				values.add(value);
			}
			return this;
		}
		QueryHelper add(String fieldName, String value, String oldValue)
		{
			if ( value!=null && (oldValue==null || !oldValue.equals(value))) {
				fields.add(fieldName);
				values.add(value);
			}
			return this;
		}
		QueryHelper add(String fieldName, Double value, Double oldValue)
		{
			if ( value!=null && (oldValue==null || !oldValue.equals(value))) {
				fields.add(fieldName);
				values.add(value);
			}
			return this;
		}
		Long insert()
		{
			if ( fields.size()==0 )
				return null;
			StringBuilder valueBuf = new StringBuilder();
			try {
				String ignoreOption = ""; //"OR IGNORE ";
				StringBuilder buf = new StringBuilder("INSERT " + ignoreOption + " INTO ");
				buf.append(tableName);
				buf.append(" (id");
				for ( String field : fields ) {
					buf.append(",");
					buf.append(field);
				}
				buf.append(") VALUES (NULL");
				for ( String field : fields ) {
					buf.append(",");
					buf.append("?");
				}
				buf.append(")");
				String sql = buf.toString();
				Log.d("cr3db", "going to execute " + sql);
				SQLiteStatement stmt = null;
				Long id = null;
				try {
					stmt = mDB.compileStatement(sql);
					for ( int i=1; i<=values.size(); i++ ) {
						Object v = values.get(i-1);
						valueBuf.append(v!=null ? v.toString() : "null");
						valueBuf.append(",");
						if ( v==null )
							stmt.bindNull(i);
						else if (v instanceof String)
							stmt.bindString(i, (String)v);
						else if (v instanceof Long)
							stmt.bindLong(i, (Long)v);
						else if (v instanceof Double)
							stmt.bindDouble(i, (Double)v);
					}
					id = stmt.executeInsert();
					Log.d("cr3db", "added book, id=" + id + ", query=" + sql);
				} finally {
					if ( stmt!=null )
						stmt.close();
				}
				return id;
			} catch ( Exception e ) {
				Log.e("cr3db", "insert failed: " + e.getMessage());
				Log.e("cr3db", "values: " + valueBuf.toString());
				return null;
			}
		}
		boolean update( Long id )
		{
			if ( fields.size()==0 )
				return false;
			StringBuilder buf = new StringBuilder("UPDATE ");
			buf.append(tableName);
			buf.append(" SET ");
			boolean first = true;
			for ( String field : fields ) {
				if ( !first )
					buf.append(",");
				buf.append(field);
				buf.append("=?");
				first = false;
			}
			buf.append(" WHERE id=" + id );
			mDB.execSQL(buf.toString(), values.toArray());
			return true;
		}
		Long fromFormat( DocumentFormat f )
		{
			if ( f==null )
				return null;
			return (long)f.ordinal();
		}
		QueryHelper( FileInfo newValue, FileInfo oldValue )
		{
			this("book");
			add("pathname", newValue.getPathName(), oldValue.getPathName());
			add("folder_fk", getFolderId(newValue.path), getFolderId(oldValue.path));
			add("filename", newValue.filename, oldValue.filename);
			add("arcname", newValue.arcname, oldValue.arcname);
			add("title", newValue.title, oldValue.title);
			add("series_fk", getSeriesId(newValue.series), getSeriesId(oldValue.series));
			add("series_number", (long)newValue.seriesNumber, (long)oldValue.seriesNumber);
			add("format", fromFormat(newValue.format), fromFormat(oldValue.format));
			add("filesize", (long)newValue.size, (long)oldValue.size);
			add("arcsize", (long)newValue.arcsize, (long)oldValue.arcsize);
			add("last_access_time", (long)newValue.lastAccessTime, (long)oldValue.lastAccessTime);
			add("create_time", (long)newValue.createTime, (long)oldValue.createTime);
			add("flags", (long)newValue.flags, (long)oldValue.flags);
		}
		QueryHelper( Bookmark newValue, Bookmark oldValue, long bookId )
		{
			this("bookmark");
			add("book_fk", bookId, oldValue.getId()!=null ? bookId : null);
			add("type", newValue.getType(), oldValue.getType());
			add("percent", newValue.getPercent(), oldValue.getPercent());
			add("shortcut", newValue.getShortcut(), oldValue.getShortcut());
			add("start_pos", newValue.getStartPos(), oldValue.getStartPos());
			add("end_pos", newValue.getEndPos(), oldValue.getEndPos());
			add("title_text", newValue.getTitleText(), oldValue.getTitleText());
			add("pos_text", newValue.getPosText(), oldValue.getPosText());
			add("comment_text", newValue.getCommentText(), oldValue.getCommentText());
			add("time_stamp", newValue.getTimeStamp(), oldValue.getTimeStamp());
		}
	}

	/**
	 * @param fileList
	 * @param maxCount
	 * @return
	 */
	public ArrayList<BookInfo> loadRecentBooks( HashMap<String, FileInfo> fileList, int maxCount )
	{
		ArrayList<FileInfo> list = new ArrayList<FileInfo>();
		findRecentBooks( list, maxCount, maxCount*10 );
		ArrayList<BookInfo> res = new ArrayList<BookInfo>(list.size());
		for ( FileInfo file : list ) {
			fileList.put(file.getPathName(), file);
			BookInfo item = new BookInfo( file );
			ArrayList<Bookmark> bookmarks = new ArrayList<Bookmark>(); 
			if ( load( bookmarks, "book_fk=" + file.id + " ORDER BY type" ) ) {
				item.setBookmarks(bookmarks);
			}
			res.add(item);
		}
		return res;
	}

	synchronized public boolean save( BookInfo bookInfo )
	{
		if ( mDB==null ) {
			Log.e("cr3db", "cannot save book info : DB is closed");
			return false;
		}
		if (bookInfo==null || bookInfo.getFileInfo()==null)
			return false;
		boolean res = true;
		if (bookInfo.getFileInfo().isModified || bookInfo.getFileInfo().id==null) {
			res = save(bookInfo.getFileInfo()) && res;
			Log.d("cr3db", "saving Book info id=" + bookInfo.getFileInfo().id);
		}
		for ( int i=0; i<bookInfo.getBookmarkCount(); i++ ) {
			 Bookmark bmk  = bookInfo.getBookmark(i);
			 if (bmk.isModified())
			 	res = save(bmk, bookInfo.getFileInfo().id) || res;
		}
		if ( bookInfo.getLastPosition()!=null && bookInfo.getLastPosition().isModified() )
			res = save(bookInfo.getLastPosition(), bookInfo.getFileInfo().id) || res;
		return res;
	}

	private boolean save( Bookmark v, long bookId )
	{
		if ( !v.isModified() )
			return false;
		Log.d("cr3db", "saving bookmark id=" + v.getId() + ", bookId=" + bookId + ", pos=" + v.getStartPos());
		if ( v.getId()!=null ) {
			// update
			Bookmark oldValue = new Bookmark();
			oldValue.setId(v.getId());
			if ( findBy(oldValue, "book_fk=" + bookId + " AND id=" + v.getId()) ) {
				// found, updating
				QueryHelper h = new QueryHelper(v, oldValue, bookId);
				h.update(v.getId());
			} else {
				oldValue = new Bookmark();
				QueryHelper h = new QueryHelper(v, oldValue, bookId);
				v.setId( h.insert() );
			}
		} else {
			Bookmark oldValue = new Bookmark();
			QueryHelper h = new QueryHelper(v, oldValue, bookId);
			v.setId( h.insert() );
		}
		v.setModified(false);
		return true;
	}

	synchronized public void deleteRecentPosition( FileInfo fileInfo )
	{
		if ( fileInfo==null || fileInfo.id==0 )
			return;
		execSQLIgnoreErrors("DELETE FROM bookmark WHERE book_fk=" + fileInfo.id + " AND type=0");
		execSQLIgnoreErrors("UPDATE book SET last_access_time=0 WHERE id=" + fileInfo.id);
	}
	
	synchronized public void deleteBookmark( Bookmark bm )
	{
		if ( bm.getId()==null )
			return;
		execSQLIgnoreErrors("DELETE FROM bookmark WHERE id=" + bm.getId());
	}
	
	synchronized public void deleteBook( FileInfo fileInfo )
	{
		if ( fileInfo==null || fileInfo.id==0 )
			return;
		execSQLIgnoreErrors("DELETE FROM bookmark WHERE book_fk=" + fileInfo.id);
		execSQLIgnoreErrors("DELETE FROM coverpage WHERE book_fk=" + fileInfo.id);
		execSQLIgnoreErrors("DELETE FROM book WHERE id=" + fileInfo.id);
	}
	
	synchronized public boolean save( FileInfo fileInfo )
	{
		boolean authorsChanged = true;
		if ( fileInfo.id!=null ) {
			// update
			FileInfo oldValue = new FileInfo();
			oldValue.id = fileInfo.id;
			if ( findById(oldValue) ) {
				// found, updating
				QueryHelper h = new QueryHelper(fileInfo, oldValue);
				h.update(fileInfo.id);
				authorsChanged = !eq(fileInfo.authors, oldValue.authors);
			} else {
				oldValue = new FileInfo();
				QueryHelper h = new QueryHelper(fileInfo, oldValue);
				fileInfo.id = h.insert();
			}
		} else {
			FileInfo oldValue = new FileInfo();
			QueryHelper h = new QueryHelper(fileInfo, oldValue);
			fileInfo.id = h.insert();
		}
		fileInfo.setModified(false);
		if ( fileInfo.id!=null ) {
			if ( authorsChanged ) {
				Long[] authorIds = getAuthorIds(fileInfo.authors);
				saveBookAuthors(fileInfo.id, authorIds);
			}
			return true;
		}
		return false;
	}

    public void flush()
    {
        Log.i("cr3db", "Flushing DB");
        if ( seriesStmt!=null) {
            seriesStmt.close();
            seriesStmt = null;
        }
        if ( authorStmt!=null) {
            authorStmt.close();
            authorStmt = null;
        }
        if ( seriesSelectStmt!=null) {
            seriesSelectStmt.close();
            seriesSelectStmt = null;
        }
        if ( authorSelectStmt!=null) {
            authorSelectStmt.close();
            authorSelectStmt = null;
        }
        SQLiteDatabase.releaseMemory();
    }
    
	public void close()
	{
	    flush();
		Log.i("cr3db", "Closing DB");
		if ( mDB!=null && mDB.isOpen() ) {
			mDB.close();
			mDB = null;
		}
		if ( mCoverpageDB!=null && mCoverpageDB.isOpen() ) {
			mCoverpageDB.close();
			mCoverpageDB = null;
		}
	}
}
