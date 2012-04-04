package org.coolreader.db;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;

import org.coolreader.crengine.BookInfo;
import org.coolreader.crengine.Bookmark;
import org.coolreader.crengine.DeviceInfo;
import org.coolreader.crengine.DocumentFormat;
import org.coolreader.crengine.FileInfo;
import org.coolreader.crengine.Utils;

import android.database.Cursor;
import android.util.Log;

public class MainDB extends BaseDB {
	
	public final int DB_VERSION = 8;
	@Override
	protected boolean upgradeSchema() {
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
		execSQL("CREATE UNIQUE INDEX IF NOT EXISTS " +
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
		int currentVersion = mDB.getVersion();
		// version 1 updates ====================================================================
		if ( currentVersion<1 )
			execSQLIgnoreErrors("ALTER TABLE bookmark ADD COLUMN shortcut INTEGER DEFAULT 0");
		if ( currentVersion<4 )
			execSQLIgnoreErrors("ALTER TABLE book ADD COLUMN flags INTEGER DEFAULT 0");
		if ( currentVersion<6 )
			execSQL("CREATE TABLE IF NOT EXISTS opds_catalog (" +
					"id INTEGER PRIMARY KEY AUTOINCREMENT, " +
					"name VARCHAR NOT NULL COLLATE NOCASE, " +
					"url VARCHAR NOT NULL COLLATE NOCASE" +
					")");
		if ( currentVersion<7 ) {
			addOPDSCatalogs(DEF_OPDS_URLS1);
			if (!DeviceInfo.NOFLIBUSTA)
				addOPDSCatalogs(DEF_OPDS_URLS1A);
		}
		if ( currentVersion<8 )
			addOPDSCatalogs(DEF_OPDS_URLS2);
		// TODO: add more updates here
			
		// set current version
		if ( currentVersion<DB_VERSION )
			mDB.setVersion(DB_VERSION);
		return true;
	}

	@Override
	protected String dbFileName() {
		return "cr3db.sqlite";
	}

	private final static String[] DEF_OPDS_URLS1 = {
		"http://www.feedbooks.com/catalog.atom", "Feedbooks",
		"http://bookserver.archive.org/catalog/", "Internet Archive",
		"http://m.gutenberg.org/", "Project Gutenberg", 
//		"http://ebooksearch.webfactional.com/catalog.atom", "eBookSearch", 
		"http://bookserver.revues.org/", "Revues.org", 
		"http://www.legimi.com/opds/root.atom", "Legimi",
		"http://www.ebooksgratuits.com/opds/", "Ebooks libres et gratuits",
	};

	private final static String[] DEF_OPDS_URLS1A = {
		"http://flibusta.net/opds/", "Flibusta", 
	};
	
	private final static String[] DEF_OPDS_URLS2 = {
		"http://www.shucang.com/s/index.php", "ShuCang.com",
	};

	private void addOPDSCatalogs(String[] catalogs) {
		for (int i=0; i<catalogs.length-1; i+=2) {
			String url = catalogs[i];
			String name = catalogs[i+1];
			saveOPDSCatalog(null, url, name);
		}
	}

	public boolean saveOPDSCatalog(Long id, String url, String name) {
		if (!isOpened())
			return false;
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

	public boolean loadOPDSCatalogs(ArrayList<FileInfo> list) {
		Log.i("cr3", "loadOPDSCatalogs()");
		boolean found = false;
		Cursor rs = null;
		try {
			String sql = "SELECT id, name, url FROM opds_catalog";
			rs = mDB.rawQuery(sql, null);
			if ( rs.moveToFirst() ) {
				// remove existing entries
				list.clear();
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
					opds.id = id;
					list.add(opds);
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
	

	//=======================================================================================
    // Bookmarks access code
    //=======================================================================================
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

	public boolean load( ArrayList<Bookmark> list, String condition )
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

	public boolean loadBookmarks(BookInfo book)
	{
		if (book.getFileInfo().id == null)
			return false; // unknown book id
		boolean found = false;
		ArrayList<Bookmark> bookmarks = new ArrayList<Bookmark>(); 
		if ( load( bookmarks, "book_fk=" + book.getFileInfo().id + " ORDER BY type" ) ) {
			found = true;
			book.setBookmarks(bookmarks);
		}
		return found;
	}

	
	
	//=======================================================================================
    // Item groups access code
    //=======================================================================================
	
	/// add items range to parent dir
	private static void addItems(FileInfo parent, ArrayList<FileInfo> items, int start, int end) {
		for (int i=start; i<end; i++) {
			items.get(i).parent = parent;
			parent.addDir(items.get(i));
		}
	}
	
	private static abstract class ItemGroupExtractor {
		public abstract String getComparisionField(FileInfo item);
		public String getItemFirstLetters(FileInfo item, int level) {
			String name = getComparisionField(item); //.filename;
			int l = name == null ? 0 : (name.length() < level ? name.length() : level);  
			return l > 0 ? name.substring(0, l).toUpperCase() : "_";
		}
	}

	private static class ItemGroupFilenameExtractor extends ItemGroupExtractor {
		@Override
		public String getComparisionField(FileInfo item) {
			return item.filename;
		}
	}

	private static class ItemGroupTitleExtractor extends ItemGroupExtractor {
		@Override
		public String getComparisionField(FileInfo item) {
			return item.title;
		}
	}
	
	private FileInfo createItemGroup(String groupPrefix, String groupPrefixTag) {
		FileInfo groupDir = new FileInfo();
		groupDir.isDirectory = true;
		groupDir.pathname = groupPrefixTag + groupPrefix;
		groupDir.filename = groupPrefix + "...";
		groupDir.isListed = true;
		groupDir.isScanned = true;
		groupDir.id = 0l;
		return groupDir;
	}
	
	private void sortItems(ArrayList<FileInfo> items, final ItemGroupExtractor extractor) {
		Collections.sort(items, new Comparator<FileInfo>() {
			@Override
			public int compare(FileInfo lhs, FileInfo rhs) {
				String l = extractor.getComparisionField(lhs) != null ? extractor.getComparisionField(lhs).toUpperCase() : "";
				String r = extractor.getComparisionField(rhs) != null ? extractor.getComparisionField(rhs).toUpperCase() : "";
				return l.compareTo(r);
			}
		});
	}
	
	private void addGroupedItems(FileInfo parent, ArrayList<FileInfo> items, int start, int end, String groupPrefixTag, int level, final ItemGroupExtractor extractor) {
		int itemCount = end - start;
		if (itemCount < 1)
			return;
		// for nested level (>1), create base subgroup, otherwise use parent 
		if (level > 1 && itemCount > 1) {
			String baseFirstLetter = extractor.getItemFirstLetters(items.get(start), level - 1);
			FileInfo newGroup = createItemGroup(baseFirstLetter, groupPrefixTag);
			newGroup.parent = parent;
			parent.addDir(newGroup);
			parent = newGroup;
		}
		
		// check group count
		int topLevelGroupsCount = 0;
		String lastFirstLetter = "";
		for (int i=start; i<end; i++) {
			String firstLetter = extractor.getItemFirstLetters(items.get(i), level);
			if (!firstLetter.equals(lastFirstLetter)) {
				topLevelGroupsCount++;
				lastFirstLetter = firstLetter;
			}
		}
		if (itemCount <= topLevelGroupsCount * 11 / 10 || itemCount < 8) {
			// small number of items: add as is
			addItems(parent, items, start, end); 
			return;
		}

		// divide items into groups
		for (int i=start; i<end; ) {
			String firstLetter = extractor.getItemFirstLetters(items.get(i), level);
			int groupEnd = i + 1;
			for (; groupEnd < end; groupEnd++) {
				String firstLetter2 = groupEnd < end ? extractor.getItemFirstLetters(items.get(groupEnd), level) : "";
				if (!firstLetter.equals(firstLetter2))
					break;
			}
			// group is i..groupEnd
			addGroupedItems(parent, items, i, groupEnd, groupPrefixTag, level + 1, extractor);
			i = groupEnd;
		}
	}
	
	private boolean loadItemList(ArrayList<FileInfo> list, String sql, String groupPrefixTag) {
		boolean found = false;
		Cursor rs = null;
		try {
			rs = mDB.rawQuery(sql, null);
			if ( rs.moveToFirst() ) {
				// read DB
				do {
					long id = rs.getLong(0);
					String name = rs.getString(1);
					if (FileInfo.AUTHOR_PREFIX.equals(groupPrefixTag))
						name = Utils.authorNameFileAs(name);
					Integer bookCount = rs.getInt(2);
					
					FileInfo item = new FileInfo();
					item.isDirectory = true;
					item.pathname = groupPrefixTag + id;
					item.filename = name;
					item.isListed = true;
					item.isScanned = true;
					item.id = id;
					item.tag = bookCount;
					
					list.add(item);
					found = true;
				} while (rs.moveToNext());
			}
		} catch (Exception e) {
			Log.e("cr3", "exception while loading list of authors", e);
		} finally {
			if ( rs!=null )
				rs.close();
		}
		sortItems(list, new ItemGroupFilenameExtractor());
		return found;
	}
	
	public boolean loadAuthorsList(FileInfo parent) {
		Log.i("cr3", "loadAuthorsList()");
		parent.clear();
		ArrayList<FileInfo> list = new ArrayList<FileInfo>();
		String sql = "SELECT author.id, author.name, count(*) as book_count FROM author INNER JOIN book_author ON  book_author.author_fk = author.id GROUP BY author.name, author.id ORDER BY author.name";
		boolean found = loadItemList(list, sql, FileInfo.AUTHOR_PREFIX);
		addGroupedItems(parent, list, 0, list.size(), FileInfo.AUTHOR_GROUP_PREFIX, 1, new ItemGroupFilenameExtractor());
		return found;
	}

	public boolean loadSeriesList(FileInfo parent) {
		Log.i("cr3", "loadSeriesList()");
		parent.clear();
		ArrayList<FileInfo> list = new ArrayList<FileInfo>();
		String sql = "SELECT series.id, series.name, count(*) as book_count FROM series INNER JOIN book ON book.series_fk = series.id GROUP BY series.name, series.id ORDER BY series.name";
		boolean found = loadItemList(list, sql, FileInfo.SERIES_PREFIX);
		addGroupedItems(parent, list, 0, list.size(), FileInfo.SERIES_GROUP_PREFIX, 1, new ItemGroupFilenameExtractor());
		return found;
	}
	
	public boolean loadTitleList(FileInfo parent) {
		Log.i("cr3", "loadTitleList()");
		parent.clear();
		ArrayList<FileInfo> list = new ArrayList<FileInfo>();
		String sql = READ_FILEINFO_SQL + " WHERE b.title IS NOT NULL AND b.title != '' ORDER BY b.title";
		boolean found = findBooks(sql, list);
		sortItems(list, new ItemGroupTitleExtractor());
		// remove duplicate titles
		for (int i=list.size() - 1; i>0; i--) {
			String title = list.get(i).title; 
			if (title == null) {
				list.remove(i);
				continue;
			}
			String prevTitle = list.get(i - 1).title;
			if (title.equals(prevTitle))
				list.remove(i);
		}
		addGroupedItems(parent, list, 0, list.size(), FileInfo.TITLE_GROUP_PREFIX, 1, new ItemGroupTitleExtractor());
		return found;
	}
	
	//=======================================================================================
    // File info access code
    //=======================================================================================
	
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

	private boolean findBooks(String sql, ArrayList<FileInfo> list) {
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
				} while (rs.moveToNext());
			}
		} finally {
			if (rs != null)
				rs.close();
		}
		return found;
	}

}
