package org.coolreader.crengine;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;

import android.database.Cursor;
import android.database.DatabaseUtils;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteStatement;
import android.util.Log;

public class CRDB {
	SQLiteDatabase db;
	File dbfile;
	protected boolean open( File dbfile )
	{
		db = SQLiteDatabase.openOrCreateDatabase(dbfile, null);
		this.dbfile = dbfile;
		return true;
	}
	static boolean DROP_TABLES = false;
	protected void dropTables()
	{
		String[] tableNames = new String[] {
			"bookmark", "book", "series", "author", "folder"	
		};
		for ( String name : tableNames )
			db.execSQL("DROP TABLE IF EXISTS " + name);
	}
	protected boolean updateSchema()
	{
		if (DROP_TABLES)
			dropTables();
		db.execSQL("CREATE TABLE IF NOT EXISTS author (" +
				"id INTEGER PRIMARY KEY AUTOINCREMENT," +
				"name VARCHAR NOT NULL" +
				")");
		db.execSQL("CREATE INDEX IF NOT EXISTS " +
                "author_name_index ON author (name) ");
		db.execSQL("CREATE TABLE IF NOT EXISTS series (" +
				"id INTEGER PRIMARY KEY AUTOINCREMENT," +
				"name VARCHAR NOT NULL" +
				")");
		db.execSQL("CREATE INDEX IF NOT EXISTS " +
		        "series_name_index ON series (name) ");
		db.execSQL("CREATE TABLE IF NOT EXISTS folder (" +
				"id INTEGER PRIMARY KEY AUTOINCREMENT," +
				"name VARCHAR NOT NULL" +
				")");
		db.execSQL("CREATE INDEX IF NOT EXISTS " +
				"folder_name_index ON folder (name) ");
		db.execSQL("CREATE TABLE IF NOT EXISTS book (" +
				"id INTEGER PRIMARY KEY AUTOINCREMENT," +
				"pathname VARCHAR NOT NULL," +
				"folder_fk INTEGER REFERENCES folder (id)," +
				"filename VARCHAR NOT NULL," +
				"arcname VARCHAR," +
				"title VARCHAR," +
				"series_fk INTEGER REFERENCES series (id)," +
				"series_number INTEGER," +
				"format INTEGER," +
				"filesize INTEGER," +
				"arcsize INTEGER," +
				"create_time INTEGER," +
				"last_access_time INTEGER" +
				")");
		db.execSQL("CREATE INDEX IF NOT EXISTS " +
				"book_folder_index ON book (folder_fk) ");
		db.execSQL("CREATE INDEX IF NOT EXISTS " +
				"book_pathname_index ON book (pathname) ");
		db.execSQL("CREATE INDEX IF NOT EXISTS " +
				"book_filename_index ON book (filename) ");
		db.execSQL("CREATE INDEX IF NOT EXISTS " +
				"book_title_index ON book (title) ");
		db.execSQL("CREATE INDEX IF NOT EXISTS " +
				"book_last_access_time_index ON book (last_access_time) ");
		db.execSQL("CREATE INDEX IF NOT EXISTS " +
				"book_title_index ON book (title) ");
		db.execSQL("CREATE TABLE IF NOT EXISTS book_author (" +
				"book_fk INTEGER NOT NULL REFERENCES book (id)," +
				"author_fk INTEGER NOT NULL REFERENCES author (id)," +
				"PRIMARY KEY (book_fk, author_fk)" +
				")");
		db.execSQL("CREATE UNIQUE INDEX IF NOT EXISTS " +
				"author_book_index ON book_author (author_fk, book_fk) ");
		db.execSQL("CREATE TABLE IF NOT EXISTS bookmark (" +
				"id INTEGER PRIMARY KEY AUTOINCREMENT," +
				"book_fk INTEGER REFERENCES book (id)," +
				"type INTEGER NOT NULL DEFAULT 0," +
				"percent INTEGER DEFAULT 0," +
				"time_stamp INTEGER DEFAULT 0," +
				"start_pos VARCHAR NOT NULL," +
				"end_pos VARCHAR," +
				"title_text VARCHAR," +
				"pos_text VARCHAR," +
				"comment_text VARCHAR" +
				")");
		return true;
	}
	
	public CRDB( File dbfile )
	{
		open(dbfile);
		updateSchema();
		dumpStatistics();
	}
	
	public boolean findByPathname( FileInfo fileInfo )
	{
		return findBy( fileInfo, "pathname", fileInfo.pathname);
	}

	public boolean findById( FileInfo fileInfo )
	{
		return findBy( fileInfo, "b.id", fileInfo.id);
	}

	private static final String READ_BOOKMARK_SQL = 
		"SELECT " +
		"id, type, percent, time_stamp, " + 
		"start_pos, end_pos, title_text, pos_text, comment_text " +
		"FROM bookmark b ";
	private void readBookmarkFromCursor( Bookmark v, Cursor rs )
	{
		int i=0;
		v.setId( rs.getLong(i++) );
		v.setType( (int)rs.getLong(i++) );
		v.setPercent( (int)rs.getLong(i++) );
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
		condition = " WHERE " + condition;
		Cursor rs = db.rawQuery(READ_BOOKMARK_SQL +
				condition, null);
		boolean found = false;
		if ( rs.moveToFirst() ) {
			readBookmarkFromCursor( v, rs );
			found = true;
		}
		rs.close();
		return found;
	}

	synchronized public boolean load( ArrayList<Bookmark> list, String condition )
	{
		condition = " WHERE " + condition;
		Cursor rs = db.rawQuery(READ_BOOKMARK_SQL +
				condition, null);
		boolean found = false;
		if ( rs.moveToFirst() ) {
			do {
				Bookmark v = new Bookmark();
				readBookmarkFromCursor( v, rs );
				list.add(v);
				found = true;
			} while ( rs.moveToNext() );
		}
		rs.close();
		return found;
	}

	private static final String READ_FILEINFO_SQL = 
		"SELECT " +
		"b.id AS id, pathname," +
		"f.name as path, " +
		"filename, arcname, title, " +
		"(SELECT GROUP_CONCAT(a.name,'|') FROM author a JOIN book_author ba ON a.id=ba.author_fk WHERE ba.book_fk=b.id) as authors, " +
		"s.name as series_name, " +
		"series_number, " +
		"format, filesize, arcsize, " +
		"create_time, last_access_time " +
		"FROM book b " +
		"LEFT JOIN series s ON s.id=b.series_fk " +
		"LEFT JOIN folder f ON f.id=b.folder_fk ";
	private void readFileInfoFromCursor( FileInfo fileInfo, Cursor rs )
	{
		int i=0;
		fileInfo.id = rs.getLong(i++);
		fileInfo.pathname = rs.getString(i++);
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
		Cursor rs = db.rawQuery(READ_FILEINFO_SQL +
				condition, null);
		boolean found = false;
		if ( rs.moveToFirst() ) {
			readFileInfoFromCursor( fileInfo, rs );
			found = true;
		}
		rs.close();
		return found;
	}
	
	private Long longQuery( String sql )
	{
		SQLiteStatement stmt = db.compileStatement(sql);
		try {
			return stmt.simpleQueryForLong();
		} catch ( Exception e ) {
			// not found or error
			return null;
		}
	}
	public void dumpStatistics()
	{
		Log.i("cr3db", "DB: " + longQuery("SELECT count(*) FROM author") + " authors, "
				 + longQuery("SELECT count(*) FROM series") + " series, "
				 + longQuery("SELECT count(*) FROM book") + " books, "
				 + longQuery("SELECT count(*) FROM bookmark") + " bookmarks"
				 + longQuery("SELECT count(*) FROM folder") + " folders"
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
			seriesSelectStmt = db.compileStatement("SELECT id FROM series WHERE name=?");
		try {
			seriesSelectStmt.bindString(1, seriesName);
			return seriesSelectStmt.simpleQueryForLong();
		} catch ( Exception e ) {
			// not found
		}
		if ( seriesStmt==null )
			seriesStmt = db.compileStatement("INSERT INTO series (id, name) VALUES (NULL,?)");
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
			folderSelectStmt = db.compileStatement("SELECT id FROM folder WHERE name=?");
		try {
			folderSelectStmt.bindString(1, folderName);
			return folderSelectStmt.simpleQueryForLong();
		} catch ( Exception e ) {
			// not found
		}
		if ( folderStmt==null )
			folderStmt = db.compileStatement("INSERT INTO folder (id, name) VALUES (NULL,?)");
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
			authorSelectStmt = db.compileStatement("SELECT id FROM author WHERE name=?");
		try {
			authorSelectStmt.bindString(1, authorName);
			return authorSelectStmt.simpleQueryForLong();
		} catch ( Exception e ) {
			// not found
		}
		if ( authorStmt==null )
			authorStmt = db.compileStatement("INSERT INTO author (id, name) VALUES (NULL,?)");
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
		StringBuilder query = new StringBuilder( "INSERT OR IGNORE INTO book_author (book_fk, author_fk) VALUES " );
		boolean first = true;
		for ( Long id : authors ) {
			if ( !first )
				query.append(",");
			query.append("(");
			query.append(bookId);
			query.append(",");
			query.append(id);
			query.append(")");
			first = false;
		}
		db.execSQL(query.toString());
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
				SQLiteStatement stmt = db.compileStatement(sql);
				for ( int i=1; i<=values.size(); i++ ) {
					Object v = values.get(i-1);
					if ( v==null )
						stmt.bindNull(i);
					else if (v instanceof String)
						stmt.bindString(i, (String)v);
					else if (v instanceof Long)
						stmt.bindLong(i, (Long)v);
					else if (v instanceof Double)
						stmt.bindDouble(i, (Double)v);
				}
				Long id = stmt.executeInsert();
				Log.d("cr3db", "added book, id=" + id + ", query=" + sql);
				stmt.close();
				return id;
			} catch ( Exception e ) {
				Log.e("cr3db", "insert failed: " + e.getMessage());
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
			db.execSQL(buf.toString(), values.toArray());
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
			add("pathname", newValue.pathname, oldValue.pathname);
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

	public ArrayList<BookInfo> loadRecentBooks( ArrayList<FileInfo> fileList, int maxCount )
	{
		ArrayList<FileInfo> list = new ArrayList<FileInfo>(fileList.size());
		for ( FileInfo item : fileList )
			if ( item.lastAccessTime!=0 && item.id!=null )
				list.add(item);
		// sort by access time, most recent at beginning
		Collections.sort(list, new Comparator<FileInfo>() {
			public int compare(FileInfo v1, FileInfo v2) {
				if ( v1.lastAccessTime>v2.lastAccessTime )
					return -1;
				else if ( v1.lastAccessTime<v2.lastAccessTime )
					return 1;
				return 0;
			}
		});
		// remove tail
		for ( int i=list.size()-1; i>=maxCount; i-- )
			list.remove(i);
		//
		ArrayList<BookInfo> res = new ArrayList<BookInfo>(list.size());
		for ( FileInfo file : list ) {
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
		Log.d("cr3db", "saving Book info id=" + bookInfo.getFileInfo().id);
		boolean res = save(bookInfo.getFileInfo());
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

	public void close()
	{
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
		if ( db!=null && db.isOpen() ) {
			db.close();
			db = null;
		}
		
	}
}
