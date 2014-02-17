package org.coolreader.db;

import android.database.Cursor;
import android.database.DatabaseUtils;
import android.database.sqlite.SQLiteException;
import android.database.sqlite.SQLiteStatement;
import android.util.Log;
import org.coolreader.crengine.*;

import java.util.*;

public class MainDB extends BaseDB {
	public static final Logger log = L.create("mdb");
	public static final Logger vlog = L.create("mdb", Log.VERBOSE);
	
	private boolean pathCorrectionRequired = false;
	public final int DB_VERSION = 21;
	@Override
	protected boolean upgradeSchema() {
		if (mDB.needUpgrade(DB_VERSION)) {
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
					"flags INTEGER DEFAULT 0, " +
					"language VARCHAR DEFAULT NULL" +
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
					"comment_text VARCHAR, " +
					"time_elapsed INTEGER DEFAULT 0" +
					")");
			execSQL("CREATE INDEX IF NOT EXISTS " +
			"bookmark_book_index ON bookmark (book_fk) ");
			int currentVersion = mDB.getVersion();
			// ====================================================================
			if ( currentVersion<1 )
				execSQLIgnoreErrors("ALTER TABLE bookmark ADD COLUMN shortcut INTEGER DEFAULT 0");
			if ( currentVersion<4 )
				execSQLIgnoreErrors("ALTER TABLE book ADD COLUMN flags INTEGER DEFAULT 0");
			if ( currentVersion<6 )
				execSQL("CREATE TABLE IF NOT EXISTS opds_catalog (" +
                        "id INTEGER PRIMARY KEY AUTOINCREMENT, " +
                        "name VARCHAR NOT NULL COLLATE NOCASE, " +
                        "url VARCHAR NOT NULL COLLATE NOCASE, " +
                        "last_usage INTEGER DEFAULT 0" +
                        ")");
			if (currentVersion < 7) {
				addOPDSCatalogs(DEF_OPDS_URLS1);
			}
			if (currentVersion < 8)
				addOPDSCatalogs(DEF_OPDS_URLS2);
			if (currentVersion < 13)
			    execSQLIgnoreErrors("ALTER TABLE book ADD COLUMN language VARCHAR DEFAULT NULL");
			if (currentVersion < 14)
				pathCorrectionRequired = true;
			if (currentVersion < 15)
			    execSQLIgnoreErrors("ALTER TABLE opds_catalog ADD COLUMN last_usage INTEGER DEFAULT 0");
			if (currentVersion < 16)
				execSQLIgnoreErrors("ALTER TABLE bookmark ADD COLUMN time_elapsed INTEGER DEFAULT 0");
			if (currentVersion < 17)
				pathCorrectionRequired = true; // chance to correct paths under Android 4.2
			if (currentVersion < 20)
				removeOPDSCatalogsFromBlackList(); // BLACK LIST enforcement, by LitRes request
            if (currentVersion < 21)
                execSQL("CREATE TABLE IF NOT EXISTS favorite_folders (" +
                        "id INTEGER PRIMARY KEY AUTOINCREMENT, " +
                        "path VARCHAR NOT NULL, " +
                        "position INTEGER NOT NULL default 0" +
                        ")");

			//==============================================================
			// add more updates above this line
				
			// set current version
			if (currentVersion < DB_VERSION)
				mDB.setVersion(DB_VERSION);
		}
		
		dumpStatistics();
		
		return true;
	}

	private void dumpStatistics() {
		log.i("mainDB: " + longQuery("SELECT count(*) FROM author") + " authors, "
				 + longQuery("SELECT count(*) FROM series") + " series, "
				 + longQuery("SELECT count(*) FROM book") + " books, "
				 + longQuery("SELECT count(*) FROM bookmark") + " bookmarks"
				 + longQuery("SELECT count(*) FROM folder") + " folders"
		);
	}

	@Override
	protected String dbFileName() {
		return "cr3db.sqlite";
	}
	
	public void clearCaches() {
		seriesCache.clear();
		authorCache.clear();
		folderCache.clear();
		fileInfoCache.clear();
	}

	public void flush() {
        super.flush();
        if (seriesStmt != null) {
            seriesStmt.close();
            seriesStmt = null;
        }
        if (folderStmt != null) {
        	folderStmt.close();
        	folderStmt = null;
        }
        if (authorStmt != null) {
            authorStmt.close();
            authorStmt = null;
        }
        if (seriesSelectStmt != null) {
            seriesSelectStmt.close();
            seriesSelectStmt = null;
        }
        if (folderSelectStmt != null) {
        	folderSelectStmt.close();
        	folderSelectStmt = null;
        }
        if (authorSelectStmt != null) {
            authorSelectStmt.close();
            authorSelectStmt = null;
        }
	}
	
	//=======================================================================================
    // OPDS access code
    //=======================================================================================
	private final static String[] DEF_OPDS_URLS1 = {
		"http://www.feedbooks.com/catalog.atom", "Feedbooks",
		"http://bookserver.archive.org/catalog/", "Internet Archive",
		"http://m.gutenberg.org/", "Project Gutenberg", 
//		"http://ebooksearch.webfactional.com/catalog.atom", "eBookSearch", 
		"http://bookserver.revues.org/", "Revues.org", 
		"http://www.legimi.com/opds/root.atom", "Legimi",
		"http://www.ebooksgratuits.com/opds/", "Ebooks libres et gratuits",
	};

	private final static String[] DEF_OPDS_URLS2 = {
		"http://www.shucang.org/s/index.php", "ShuCang.org",
	};

	private void addOPDSCatalogs(String[] catalogs) {
		for (int i=0; i<catalogs.length-1; i+=2) {
			String url = catalogs[i];
			String name = catalogs[i+1];
			saveOPDSCatalog(null, url, name);
		}
	}

	public void removeOPDSCatalogsFromBlackList() {
		if (OPDSConst.BLACK_LIST_MODE != OPDSConst.BLACK_LIST_MODE_FORCE) {
		    execSQLIgnoreErrors("DELETE FROM opds_catalog WHERE url='http://flibusta.net/opds/'");
		} else {
			for (String url : OPDSConst.BLACK_LIST) {
			    execSQLIgnoreErrors("DELETE FROM opds_catalog WHERE url=" + quoteSqlString(url));
			}
		}
	}
	
	public void updateOPDSCatalogLastUsage(String url) {
		try {
			Long existingIdByUrl = longQuery("SELECT id FROM opds_catalog WHERE url=" + quoteSqlString(url));
			if (existingIdByUrl == null)
				return;
			// update existing
			Long lastUsage = longQuery("SELECT max(last_usage) FROM opds_catalog");
			if (lastUsage == null)
				lastUsage = 1L;
			else
				lastUsage = lastUsage + 1;
			execSQL("UPDATE opds_catalog SET last_usage="+ lastUsage +" WHERE id=" + existingIdByUrl);
		} catch (Exception e) {
			log.e("exception while updating OPDS catalog item", e);
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
			updateOPDSCatalogLastUsage(url);
				
		} catch (Exception e) {
			log.e("exception while saving OPDS catalog item", e);
			return false;
		}
		return true;
	}

	public boolean loadOPDSCatalogs(ArrayList<FileInfo> list) {
		log.i("loadOPDSCatalogs()");
		boolean found = false;
		Cursor rs = null;
		try {
			String sql = "SELECT id, name, url FROM opds_catalog ORDER BY last_usage DESC, name";
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
	
	public void removeOPDSCatalog(Long id) {
		log.i("removeOPDSCatalog(" + id + ")");
		execSQLIgnoreErrors("DELETE FROM opds_catalog WHERE id = " + id);
	}

    public ArrayList<FileInfo> loadFavoriteFolders() {
        log.i("loadFavoriteFolders()");
        Cursor rs = null;
        ArrayList<FileInfo> list = new ArrayList<FileInfo>();
        try {
            String sql = "SELECT id, path, position FROM favorite_folders ORDER BY position, path";
            rs = mDB.rawQuery(sql, null);
            if ( rs.moveToFirst() ) {
                do {
                    Long id = rs.getLong(0);
                    String path = rs.getString(1);
                    int pos = rs.getInt(2);
                    FileInfo favorite = new FileInfo(path);
                    favorite.id = id;
                    favorite.seriesNumber = pos;
                    favorite.setType(FileInfo.TYPE_NOT_SET);
                    list.add(favorite);
                } while (rs.moveToNext());
            }
        } catch (Exception e) {
            Log.e("cr3", "exception while loading list of favorite folders", e);
        } finally {
            if ( rs!=null )
                rs.close();
        }
        return list;
    }

    public void deleteFavoriteFolder(FileInfo folder){
        execSQLIgnoreErrors("DELETE FROM favorite_folders WHERE id = "+ folder.id);
    }

    public void updateFavoriteFolder(FileInfo folder){
        SQLiteStatement stmt = null;
        try {
            stmt = mDB.compileStatement("UPDATE favorite_folders SET position = ?, path = ? WHERE id = ?");
            stmt.bindLong(1, folder.seriesNumber);
            stmt.bindString(2, folder.pathname);
            stmt.bindLong(3, folder.id);
            stmt.execute();
        } finally {
            if(stmt!= null)
                stmt.close();
        }
    }

    public void createFavoritesFolder(FileInfo folder){
        SQLiteStatement stmt = null;
        try {
            stmt = mDB.compileStatement("INSERT INTO favorite_folders (id, path, position) VALUES (NULL, ?, ?)");
            stmt.bindString(1, folder.pathname);
            stmt.bindLong(2, folder.seriesNumber);
            folder.id = stmt.executeInsert();
        } finally {
            if(stmt!= null)
                stmt.close();
        }
    }

	//=======================================================================================
    // Bookmarks access code
    //=======================================================================================
	private static final String READ_BOOKMARK_SQL = 
		"SELECT " +
		"id, type, percent, shortcut, time_stamp, " + 
		"start_pos, end_pos, title_text, pos_text, comment_text, time_elapsed " +
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
		v.setTimeElapsed( rs.getLong(i++) );
	}

	public boolean findBy( Bookmark v, String condition ) {
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
		beginReading();
		parent.clear();
		ArrayList<FileInfo> list = new ArrayList<FileInfo>();
		String sql = "SELECT author.id, author.name, count(*) as book_count FROM author INNER JOIN book_author ON  book_author.author_fk = author.id GROUP BY author.name, author.id ORDER BY author.name";
		boolean found = loadItemList(list, sql, FileInfo.AUTHOR_PREFIX);
		addGroupedItems(parent, list, 0, list.size(), FileInfo.AUTHOR_GROUP_PREFIX, 1, new ItemGroupFilenameExtractor());
		endReading();
		return found;
	}

	public boolean loadSeriesList(FileInfo parent) {
		Log.i("cr3", "loadSeriesList()");
		beginReading();
		parent.clear();
		ArrayList<FileInfo> list = new ArrayList<FileInfo>();
		String sql = "SELECT series.id, series.name, count(*) as book_count FROM series INNER JOIN book ON book.series_fk = series.id GROUP BY series.name, series.id ORDER BY series.name";
		boolean found = loadItemList(list, sql, FileInfo.SERIES_PREFIX);
		addGroupedItems(parent, list, 0, list.size(), FileInfo.SERIES_GROUP_PREFIX, 1, new ItemGroupFilenameExtractor());
		endReading();
		return found;
	}
	
	public boolean loadTitleList(FileInfo parent) {
		Log.i("cr3", "loadTitleList()");
		beginReading();
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
		endReading();
		return found;
	}
	
	public boolean findAuthorBooks(ArrayList<FileInfo> list, long authorId)
	{
		if (!isOpened())
			return false;
		String sql = READ_FILEINFO_SQL + " INNER JOIN book_author ON book_author.book_fk = b.id WHERE book_author.author_fk = " + authorId + " ORDER BY b.title";
		return findBooks(sql, list);
	}
	
	public boolean findSeriesBooks(ArrayList<FileInfo> list, long seriesId)
	{
		if (!isOpened())
			return false;
		String sql = READ_FILEINFO_SQL + " INNER JOIN series ON series.id = b.series_fk WHERE series.id = " + seriesId + " ORDER BY b.series_number, b.title";
		return findBooks(sql, list);
	}
	
	public boolean findBooksByRating(ArrayList<FileInfo> list, int minRate, int maxRate)
	{
		if (!isOpened())
			return false;
		String sql = READ_FILEINFO_SQL + " WHERE ((flags>>20)&15) BETWEEN " + minRate + " AND " + maxRate + " ORDER BY ((flags>>20)&15) DESC, b.title LIMIT 1000";
		return findBooks(sql, list);
	}
	
	public boolean findBooksByState(ArrayList<FileInfo> list, int state)
	{
		if (!isOpened())
			return false;
		String sql = READ_FILEINFO_SQL + " WHERE ((flags>>16)&15) = " + state + " ORDER BY b.title LIMIT 1000";
		return findBooks(sql, list);
	}
	
	private String findAuthors(int maxCount, String authorPattern) {
		StringBuilder buf = new StringBuilder();
		String sql = "SELECT id, name FROM author";
		Cursor rs = null;
		int count = 0;
		try {
			rs = mDB.rawQuery(sql, null);
			if ( rs.moveToFirst() ) {
				do {
					long id = rs.getLong(0);
					String name = rs.getString(1);
					if (Utils.matchPattern(name, authorPattern)) {
						if (buf.length() != 0)
							buf.append(",");
						buf.append(id);
						count++;
						if (count >= maxCount)
							break;
					}
				} while (rs.moveToNext());
			}
		} finally {
			if ( rs!=null )
				rs.close();
		}
		return buf.toString();
	}
	
	
	//=======================================================================================
    // Series access code
    //=======================================================================================
	
	private SQLiteStatement seriesStmt;
	private SQLiteStatement seriesSelectStmt;
	private HashMap<String,Long> seriesCache = new HashMap<String,Long>();
	public Long getSeriesId( String seriesName )
	{
		if ( seriesName==null || seriesName.trim().length()==0 )
			return null;
		Long id = seriesCache.get(seriesName); 
		if ( id!=null )
			return id;
		if (seriesSelectStmt == null)
			seriesSelectStmt = mDB.compileStatement("SELECT id FROM series WHERE name=?");
		try {
			seriesSelectStmt.bindString(1, seriesName);
			return seriesSelectStmt.simpleQueryForLong();
		} catch ( Exception e ) {
			// not found
		}
		if (seriesStmt == null)
			seriesStmt = mDB.compileStatement("INSERT INTO series (id, name) VALUES (NULL,?)");
		seriesStmt.bindString(1, seriesName);
		id = seriesStmt.executeInsert();
		seriesCache.put( seriesName, id );
		return id;
	}
	
	//=======================================================================================
    // Folder access code
    //=======================================================================================
	
	private SQLiteStatement folderStmt;
	private SQLiteStatement folderSelectStmt;
	private HashMap<String,Long> folderCache = new HashMap<String,Long>();
	public Long getFolderId( String folderName )
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
	
	//=======================================================================================
    // Author access code
    //=======================================================================================
	
	private SQLiteStatement authorStmt;
	private SQLiteStatement authorSelectStmt;
	private HashMap<String,Long> authorCache = new HashMap<String,Long>();
	private Long getAuthorId( String authorName ) {
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
	
	private Long[] getAuthorIds( String authorNames ) {
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
	
	public void saveBookAuthors( Long bookId, Long[] authors) {
		if ( authors==null || authors.length==0 )
			return;
		String insertQuery = "INSERT OR IGNORE INTO book_author (book_fk,author_fk) VALUES ";
		for ( Long id : authors ) {
			String sql = insertQuery + "(" + bookId + "," + id + ")"; 
			//Log.v("cr3", "executing: " + sql);
			mDB.execSQL(sql);
		}
	}

	private static boolean eq(String s1, String s2)
	{
		if ( s1!=null )
			return s1.equals(s2);
		return s2==null;
	}

	private final static int FILE_INFO_CACHE_SIZE = 3000;
	private FileInfoCache fileInfoCache = new FileInfoCache(FILE_INFO_CACHE_SIZE); 
	
	private FileInfo findMovedFileInfo(String path) {
		ArrayList<FileInfo> list = new ArrayList<FileInfo>();
		FileInfo fi = new FileInfo(path);
		if (fi.exists()) {
			if (findAllBy(list, "filename", fi.filename)) {
				for (FileInfo item : list) {
					if (item.exists())
						continue;
					if (item.size == fi.size) {
						log.i("Found record for file of the same name and size: treat as moved " + item.filename + " " + item.size);
						// fix and save
						item.pathname = fi.pathname;
						item.arcname = fi.arcname;
						item.arcsize = fi.arcsize;
						item.path = fi.path;
						item.createTime = fi.createTime;
						save(item);
						fileInfoCache.put(item);
						return item;
					}
				}
			}
		}
		return null;
	}
	
	private FileInfo findFileInfoByPathname(String path, boolean detectMoved)
	{
		FileInfo existing = fileInfoCache.get(path);
		if (existing != null)
			return existing;
		FileInfo fileInfo = new FileInfo(); 
		if (findBy(fileInfo, "pathname", path)) {
			fileInfoCache.put(fileInfo);
			return fileInfo;
		}
		if (!detectMoved)
			return null;
		return findMovedFileInfo(path);
	}

	private FileInfo findFileInfoById(Long id)
	{
		if (id == null)
			return null;
		FileInfo existing = fileInfoCache.get(id);
		if (existing != null)
			return existing;
		FileInfo fileInfo = new FileInfo(); 
		if (findBy( fileInfo, "b.id", id)) {
			return fileInfo;
		}
		return null;
	}

	private boolean findBy( FileInfo fileInfo, String fieldName, Object fieldValue )
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

	private boolean findAllBy( ArrayList<FileInfo> result, String fieldName, Object fieldValue )
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
			if (rs.moveToFirst()) {
				do {
					FileInfo fileInfo = new FileInfo();
					readFileInfoFromCursor( fileInfo, rs );
					result.add(fileInfo);
					found = true;
				} while (rs.moveToNext());
			}
		} finally {
			if ( rs!=null )
				rs.close();
		}
		return found;
	}

	private HashMap<String, Bookmark> loadBookmarks(FileInfo fileInfo) {
		HashMap<String, Bookmark> map = new HashMap<String, Bookmark>();
		if (fileInfo.id != null) {
			ArrayList<Bookmark> bookmarks = new ArrayList<Bookmark>();
			if (load(bookmarks, "book_fk=" + fileInfo.id + " ORDER BY type")) {
				for (Bookmark b : bookmarks) {
					// delete non-unique bookmarks
					String key = b.getUniqueKey();
					if (!map.containsKey(key))
						map.put(key, b);
					else {
						log.w("Removing non-unique bookmark " + b + " for " + fileInfo.getPathName());
						deleteBookmark(b);
					}
				}
			}
		}
		return map;
	}

	private boolean save( Bookmark v, long bookId )
	{
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
		return true;
	}

	public void saveBookInfo(BookInfo bookInfo)	{
		if (!isOpened()) {
			Log.e("cr3db", "cannot save book info : DB is closed");
			return;
		}
		if (bookInfo == null || bookInfo.getFileInfo() == null)
			return;
		
		// save main data
		save(bookInfo.getFileInfo());
		fileInfoCache.put(bookInfo.getFileInfo());
		
		// save bookmarks
		HashMap<String, Bookmark> existingBookmarks = loadBookmarks(bookInfo.getFileInfo());
		int changed = 0;
		int removed = 0;
		int added = 0;
		for (Bookmark bmk : bookInfo.getAllBookmarks()) {
			 Bookmark existing = existingBookmarks.get(bmk.getUniqueKey());
			 if (existing != null) {
				 bmk.setId(existing.getId());
				 if (!bmk.equals(existing)) {
					 save(bmk, bookInfo.getFileInfo().id);
					 changed++;
				 }
				 existingBookmarks.remove(bmk.getUniqueKey()); // saved
			 } else {
				 // create new
			 	 save(bmk, bookInfo.getFileInfo().id);
			 	 added++;
			 }
		}
		if (existingBookmarks.size() > 0) {
			// remove bookmarks not found in new object
			for (Bookmark bmk : existingBookmarks.values()) {
				deleteBookmark(bmk);
				removed++;
			}
		}
		if (added + changed + removed > 0)
			vlog.i("bookmarks added:" + added + ", updated: " + changed + ", removed:" + removed);
	}

	private boolean save(FileInfo fileInfo)	{
		boolean authorsChanged = true;
		try {
			FileInfo oldValue = findFileInfoByPathname(fileInfo.getPathName(), false);
			if (oldValue == null && fileInfo.id != null)
				oldValue = findFileInfoById(fileInfo.id);
			if (oldValue != null && fileInfo.id == null && oldValue.id != null)
				fileInfo.id = oldValue.id;
			if (oldValue != null) {
				// found, updating
				if (!fileInfo.equals(oldValue)) {
					vlog.d("updating file " + fileInfo.getPathName());
					beginChanges();
					QueryHelper h = new QueryHelper(fileInfo, oldValue);
					h.update(fileInfo.id);
				}
				authorsChanged = !eq(fileInfo.authors, oldValue.authors);
			} else {
				// inserting
				vlog.d("inserting new file " + fileInfo.getPathName());
				beginChanges();
				QueryHelper h = new QueryHelper(fileInfo, new FileInfo());
				fileInfo.id = h.insert();
				authorsChanged = true;
			}
			
			fileInfoCache.put(fileInfo);
			if (fileInfo.id != null) {
				if ( authorsChanged ) {
					vlog.d("updating authors for file " + fileInfo.getPathName());
					beginChanges();
					Long[] authorIds = getAuthorIds(fileInfo.authors);
					saveBookAuthors(fileInfo.id, authorIds);
				}
				return true;
			}
			return false;
		} catch (SQLiteException e) {
			log.e("error while writing to DB", e);
			return false;
		}
	}

	public void saveFileInfos(Collection<FileInfo> list)
	{
		Log.v("cr3db", "save BookInfo collection: " + list.size() + " items");
		if (!isOpened()) {
			Log.e("cr3db", "cannot save book info : DB is closed");
			return;
		}
		for (FileInfo fileInfo : list) {
			save(fileInfo);
		}
	}
	
	/**
	 * Load recent books list, with bookmarks
	 * @param maxCount is max number of recent books to get
	 * @return list of loaded books
	 */
	public ArrayList<BookInfo> loadRecentBooks(int maxCount)
	{
		ArrayList<FileInfo> list = new ArrayList<FileInfo>();
		if (!isOpened())
			return null;
		beginReading();
		findRecentBooks(list, maxCount, maxCount*10);
		ArrayList<BookInfo> res = new ArrayList<BookInfo>(list.size());
		for (FileInfo f : list) {
			FileInfo file = fileInfoCache.get(f.getPathName()); // try using cached value instead
			if (file == null) {
				file = f;
				fileInfoCache.put(file);
			}
			BookInfo item = new BookInfo(new FileInfo(file));
			loadBookmarks(item);
			res.add(item);
		}
		endReading();
		return res;
	}

	private boolean findRecentBooks( ArrayList<FileInfo> list, int maxCount, int limit )
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
					fileInfoCache.put(fileInfo);
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
	
	
	//=======================================================================================
    // File info access code
    //=======================================================================================
	
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
			beginChanges();
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
				for ( @SuppressWarnings("unused") String field : fields ) {
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
			beginChanges();
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
			vlog.v("executing " + buf);
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
			add("language", newValue.language, oldValue.language);
			if (fields.size() == 0)
				vlog.v("QueryHelper: no fields to update");
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
			add("time_elapsed", newValue.getTimeElapsed(), oldValue.getTimeElapsed());
		}
	}

	private static final String READ_FILEINFO_FIELDS = 
		"b.id AS id, pathname," +
		"f.name as path, " +
		"filename, arcname, title, " +
		"(SELECT GROUP_CONCAT(a.name,'|') FROM author a JOIN book_author ba ON a.id=ba.author_fk WHERE ba.book_fk=b.id) as authors, " +
		"s.name as series_name, " +
		"series_number, " +
		"format, filesize, arcsize, " +
		"create_time, last_access_time, flags, language ";
	
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
	    fileInfo.language = rs.getString(i++);
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
					fileInfoCache.put(fileInfo);
					list.add(new FileInfo(fileInfo));
					found = true;
				} while (rs.moveToNext());
			}
		} finally {
			if (rs != null)
				rs.close();
		}
		return found;
	}

	private String findSeries(int maxCount, String seriesPattern) {
		StringBuilder buf = new StringBuilder();
		String sql = "SELECT id, name FROM series";
		Cursor rs = null;
		int count = 0;
		try {
			rs = mDB.rawQuery(sql, null);
			if ( rs.moveToFirst() ) {
				do {
					long id = rs.getLong(0);
					String name = rs.getString(1);
					if (Utils.matchPattern(name, seriesPattern)) {
						if (buf.length() != 0)
							buf.append(",");
						buf.append(id);
						count++;
						if (count >= maxCount)
							break;
					}
				} while (rs.moveToNext());
			}
		} finally {
			if ( rs!=null )
				rs.close();
		}
		return buf.toString();
	}
	
	public ArrayList<FileInfo> findByPatterns(int maxCount, String author, String title, String series, String filename)
	{
		beginReading();
		ArrayList<FileInfo> list = new ArrayList<FileInfo>();
		
		StringBuilder buf = new StringBuilder();
		boolean hasCondition = false;
		if ( author!=null && author.length()>0 ) {
			String authorIds = findAuthors(maxCount, author);
			if (authorIds == null || authorIds.length() == 0)
				return list;
			if ( buf.length()>0 )
				buf.append(" AND ");
			buf.append(" b.id IN (SELECT ba.book_fk FROM book_author ba WHERE ba.author_fk IN (" + authorIds + ")) ");
			hasCondition = true;
		}
		if ( series!=null && series.length()>0 ) {
			String seriesIds = findSeries(maxCount, series);
			if (seriesIds == null || seriesIds.length() == 0)
				return list;
			if ( buf.length()>0 )
				buf.append(" AND ");
			buf.append(" b.series_fk IN (" + seriesIds + ") ");
			hasCondition = true;
		}
		if ( title!=null && title.length()>0 ) {
			hasCondition = true;
		}
		if ( filename!=null && filename.length()>0 ) {
			hasCondition = true;
		}
		if (!hasCondition)
			return list;
		
		String condition = buf.length()==0 ? "" : " WHERE " + buf.toString();
		String sql = READ_FILEINFO_SQL + condition;
		Log.d("cr3", "sql: " + sql );
		Cursor rs = null;
		try { 
			rs = mDB.rawQuery(sql, null);
			if ( rs.moveToFirst() ) {
				int count = 0;
				do {
					if ( title!=null && title.length()>0 )
						if (!Utils.matchPattern(rs.getString(5), title))
							continue;
					if ( filename!=null && filename.length()>0 )
						if (!Utils.matchPattern(rs.getString(3), filename))
							continue;
					FileInfo fi = new FileInfo(); 
					readFileInfoFromCursor( fi, rs );
					list.add(fi);
					fileInfoCache.put(fi);
					count++;
				} while ( count<maxCount && rs.moveToNext() );
			}
		} finally {
			if ( rs!=null )
				rs.close();
		}
		endReading();
		return list;
	}

	public ArrayList<FileInfo> loadFileInfos(ArrayList<String> pathNames) {
		ArrayList<FileInfo> list = new ArrayList<FileInfo>();
		if (!isOpened())
			return list;
		try {
			beginReading();
			for (String path : pathNames) {
				FileInfo file = findFileInfoByPathname(path, true);
				if (file != null)
					list.add(new FileInfo(file));
			}
			endReading();
		} catch (Exception e) {
			log.e("Exception while loading books from DB", e);
		}
		return list;
	}

	public void deleteRecentPosition( FileInfo fileInfo ) {
		Long bookId = getBookId(fileInfo);
		if (bookId == null)
			return;
		execSQLIgnoreErrors("DELETE FROM bookmark WHERE book_fk=" + bookId + " AND type=0");
		execSQLIgnoreErrors("UPDATE book SET last_access_time=0 WHERE id=" + bookId);
	}
	
	public void deleteBookmark(Bookmark bm) {
		if (bm.getId() == null)
			return;
		execSQLIgnoreErrors("DELETE FROM bookmark WHERE id=" + bm.getId());
	}

	public BookInfo loadBookInfo(FileInfo fileInfo) {
		if (!isOpened())
			return null;
		try {
			FileInfo cached = fileInfoCache.get(fileInfo.getPathName());
			if (cached != null) {
				BookInfo book = new BookInfo(new FileInfo(cached));
				loadBookmarks(book);
				return book;
			}
			if (loadByPathname(fileInfo)) {
				BookInfo book = new BookInfo(new FileInfo(fileInfo));
				loadBookmarks(book);
				return book;
			}
		} catch (Exception e) {
			// ignore
		}
		return null;
	}
	
	public FileInfo loadFileInfo(String pathName) {
		if (!isOpened())
			return null;
		try {
			FileInfo cached = fileInfoCache.get(pathName);
			if (cached != null) {
				return new FileInfo(cached);
			}
			FileInfo fileInfo = new FileInfo(pathName);
			if (loadByPathname(fileInfo)) {
				fileInfoCache.put(fileInfo);
				return new FileInfo(fileInfo);
			}
		} catch (Exception e) {
			// ignore
		}
		return null;
	}
	
	private boolean loadByPathname(FileInfo fileInfo) {
		if (findBy(fileInfo, "pathname", fileInfo.getPathName())) {
			fileInfoCache.put(fileInfo);
			return true;
		}
		
		FileInfo moved = findMovedFileInfo(fileInfo.getPathName());
		if (moved != null) {
			fileInfo.assign(moved);
			return true;
		}
		
		return false;
	}

//	private boolean loadById( FileInfo fileInfo ) {
//		return findBy(fileInfo, "b.id", fileInfo.id);
//	}

	private Long getBookId(FileInfo fileInfo) {
		Long bookId = null;
		if (fileInfo == null)
			return bookId;
		String pathName = fileInfo.getPathName();
		FileInfo cached = fileInfoCache.get(pathName);
		if (cached != null) {
			bookId = cached.id;
		}
		if (bookId == null)
			bookId = fileInfo.id;
		if (bookId == null)
			loadByPathname(fileInfo);
		return bookId;
	}
	public Long deleteBook(FileInfo fileInfo)
	{
		if (fileInfo == null)
			return null;
		Long bookId = getBookId(fileInfo);
		fileInfoCache.remove(fileInfo);
		if (bookId == null)
			return null;
		execSQLIgnoreErrors("DELETE FROM bookmark WHERE book_fk=" + bookId);
		execSQLIgnoreErrors("DELETE FROM book WHERE id=" + bookId);
		return bookId;
	}
	
	public void correctFilePaths() {
		Log.i("cr3", "checking data for path correction");
		beginReading();
		int rowCount = 0;
		Map<String, Long> map = new HashMap<String, Long>();
		Cursor rs = null;
		try {
			String sql = "SELECT id, pathname FROM book";
			rs = mDB.rawQuery(sql, null);
			if ( rs.moveToFirst() ) {
				// read DB
				do {
					Long id = rs.getLong(0);
					String pathname = rs.getString(1);
					String corrected = pathCorrector.normalize(pathname);
					if (pathname == null)
						continue;
					rowCount++;
					if (corrected == null) {
						Log.w("cr3", "DB contains unknown path " + pathname);
					} else if (!pathname.equals(corrected)) {
						map.put(pathname, id);
					}
				} while (rs.moveToNext());
			}
		} catch (Exception e) {
			Log.e("cr3", "exception while loading list books to correct paths", e);
		} finally {
			if ( rs!=null )
				rs.close();
		}
		Log.i("cr3", "Total rows: " + rowCount + ", " + (map.size() > 0 ? "need to correct " + map.size() + " items" : "no corrections required"));
		if (map.size() > 0) {
			beginChanges();
			int count = 0;
			for (Map.Entry<String, Long> entry : map.entrySet()) {
				String pathname = entry.getKey();
				String corrected = pathCorrector.normalize(pathname);
				if (corrected != null && !corrected.equals(pathname)) {
					count++;
					execSQLIgnoreErrors("update book set pathname=" + quoteSqlString(corrected) + " WHERE id=" + entry.getValue());
				}
			}
			flush();
			log.i("Finished. Rows corrected: " + count);
		}
	}
	
	private MountPathCorrector pathCorrector;
	public void setPathCorrector(MountPathCorrector corrector) {
		this.pathCorrector = corrector;
		if (pathCorrectionRequired) {
			correctFilePaths();
			pathCorrectionRequired = false;
		}
	}

}
