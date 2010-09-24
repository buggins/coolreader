package org.coolreader.crengine;

import java.io.File;
import java.util.ArrayList;
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
			"bookmark", "book", "series", "author"	
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
		db.execSQL("CREATE TABLE IF NOT EXISTS series (" +
				"id INTEGER PRIMARY KEY AUTOINCREMENT," +
				"name VARCHAR NOT NULL" +
				")");
		db.execSQL("CREATE TABLE IF NOT EXISTS book (" +
				"id INTEGER PRIMARY KEY AUTOINCREMENT," +
				"pathname VARCHAR NOT NULL," +
				"path VARCHAR NOT NULL," +
				"filename VARCHAR NOT NULL," +
				"arcname VARCHAR," +
				"title VARCHAR," +
				"series_fk INTEGER REFERENCES series (id)," +
				"series_number INTEGER," +
				"format INTEGER," +
				"filesize INTEGER," +
				"arcsize INTEGER" +
				")");
		db.execSQL("CREATE INDEX IF NOT EXISTS " +
				"book_path_index ON book (path) ");
		db.execSQL("CREATE INDEX IF NOT EXISTS " +
				"book_pathname_index ON book (pathname) ");
		db.execSQL("CREATE INDEX IF NOT EXISTS " +
				"book_filename_index ON book (filename) ");
		db.execSQL("CREATE INDEX IF NOT EXISTS " +
				"book_title_index ON book (title) ");
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
				"type INTEGER NOT NULL," +
				"percent INTEGER," +
				"page INTEGER," +
				"time_stamp INTEGER," +
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
		return findBy( fileInfo, "pathname", fileInfo.pathname);
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
		Cursor rs = db.rawQuery("SELECT " +
				"b.id AS id, pathname, path, " +
				"filename, arcname, title, " +
				"(SELECT GROUP_CONCAT(a.name,'|') FROM author a JOIN book_author ba ON a.id=ba.author_fk WHERE ba.book_fk=b.id) as authors, " +
				"s.name as series_name, " +
				"series_number, " +
				"format, filesize, arcsize " +
				"FROM book b " +
				"LEFT JOIN series s ON s.id=b.series_fk " +
				condition, null);
		boolean found = false;
		if ( rs.moveToFirst() ) {
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
				 + longQuery("SELECT count(*) FROM book") + " books");
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
			add("path", newValue.path, oldValue.path);
			add("filename", newValue.filename, oldValue.filename);
			add("arcname", newValue.arcname, oldValue.arcname);
			add("title", newValue.title, oldValue.title);
			add("series_fk", getSeriesId(newValue.series), getSeriesId(oldValue.series));
			add("series_number", (long)newValue.seriesNumber, (long)oldValue.seriesNumber);
			add("format", fromFormat(newValue.format), fromFormat(oldValue.format));
			add("filesize", (long)newValue.size, (long)oldValue.size);
			add("arcsize", (long)newValue.arcsize, (long)oldValue.arcsize);
		}
		
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
