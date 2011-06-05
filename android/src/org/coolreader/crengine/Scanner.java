package org.coolreader.crengine;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Set;
import java.util.zip.ZipEntry;

import org.coolreader.CoolReader;
import org.coolreader.R;
import org.coolreader.crengine.Engine.EngineTask;

import android.os.Environment;
import android.util.Log;

public class Scanner {
	
	HashMap<String, FileInfo> mFileList = new HashMap<String, FileInfo>();
	ArrayList<FileInfo> mFilesForParsing = new ArrayList<FileInfo>();
	FileInfo mRoot;
	
	boolean mHideEmptyDirs = true;
	
	void setHideEmptyDirs( boolean flgHide ) {
		mHideEmptyDirs = flgHide;
	}

//	private boolean scanDirectories( FileInfo baseDir )
//	{
//		try {
//			File dir = new File(baseDir.pathname);
//			File[] items = dir.listFiles();
//			// process normal files
//			for ( File f : items ) {
//				if ( !f.isDirectory() ) {
//					FileInfo item = new FileInfo( f );
//					if ( item.format!=null ) {
//						item.parent = baseDir;
//						baseDir.addFile(item);
//						mFileList.add(item);
//					}
//				}
//			}
//			// process directories 
//			for ( File f : items ) {
//				if ( f.isDirectory() ) {
//					FileInfo item = new FileInfo( f );
//					item.parent = baseDir;
//					scanDirectories(item);
//					if ( !item.isEmpty() ) {
//						baseDir.addDir(item);					
//					}
//				}
//			}
//			return !baseDir.isEmpty();
//		} catch ( Exception e ) {
//			L.e("Exception while scanning directory " + baseDir.pathname, e);
//			return false;
//		}
//	}
	
	private boolean dirScanEnabled = true;
	public boolean getDirScanEnabled()
	{
		return dirScanEnabled;
	}
	
	public void setDirScanEnabled(boolean dirScanEnabled)
	{
		this.dirScanEnabled = dirScanEnabled;
	}
	
	private FileInfo scanZip( FileInfo zip )
	{
		try {
			File zf = new File(zip.pathname);
			//ZipFile file = new ZipFile(zf);
			ArrayList<ZipEntry> entries = engine.getArchiveItems(zip.pathname);
			ArrayList<FileInfo> items = new ArrayList<FileInfo>();
			//for ( Enumeration<?> e = file.entries(); e.hasMoreElements(); ) {
			for ( ZipEntry entry : entries ) {
				if ( entry.isDirectory() )
					continue;
				String name = entry.getName();
				FileInfo item = new FileInfo();
				item.format = DocumentFormat.byExtension(name);
				if ( item.format==null )
					continue;
				File f = new File(name);
				item.filename = f.getName();
				item.path = f.getPath();
				item.pathname = entry.getName();
				item.size = (int)entry.getSize();
				//item.createTime = entry.getTime();
				item.createTime = zf.lastModified();
				item.arcname = zip.pathname;
				item.arcsize = (int)entry.getSize(); //getCompressedSize();
				item.isArchive = true;
				items.add(item);
			}
			if ( items.size()==0 ) {
				L.i("Supported files not found in " + zip.pathname);
				return null;
			} else if ( items.size()==1 ) {
				// single supported file in archive
				FileInfo item = items.get(0);
				item.isArchive = true;
				item.isDirectory = false;
				return item;
			} else {
				zip.isArchive = true;
				zip.isDirectory = true;
				zip.isListed = true;
				for ( FileInfo item : items ) {
					item.parent = zip;
					zip.addFile(item);
				}
				return zip;
			}
		} catch ( Exception e ) {
			L.e("IOException while opening " + zip.pathname + " " + e.getMessage());
		}
		return null;
	}
	
	/**
	 * Adds dir and file children to directory FileInfo item.
	 * @param baseDir is directory to list files and dirs for
	 * @return true if successful.
	 */
	public boolean listDirectory( FileInfo baseDir )
	{
		Set<String> knownItems = null;
		if ( baseDir.isListed ) {
			knownItems = new HashSet<String>();
			for ( int i=baseDir.itemCount()-1; i>=0; i-- ) {
				FileInfo item = baseDir.getItem(i);
				if ( !item.exists() ) {
					// remove item from list
					baseDir.removeChild(item);
				} else {
					knownItems.add(item.getBasePath());
				}
			}
		}
		try {
			File dir = new File(baseDir.pathname);
			File[] items = dir.listFiles();
			// process normal files
			if ( items!=null ) {
				for ( File f : items ) {
					if ( !f.isDirectory() ) {
						if ( f.getName().startsWith(".") )
							continue; // treat files beginning with '.' as hidden
						String pathName = f.getAbsolutePath();
						if ( knownItems!=null && knownItems.contains(pathName) )
							continue;
						boolean isZip = pathName.toLowerCase().endsWith(".zip");
						FileInfo item = mFileList.get(pathName);
						boolean isNew = false;
						if ( item==null ) {
							item = new FileInfo( f );
							if ( isZip ) {
								item = scanZip( item );
								if ( item==null )
									continue;
								if ( item.isDirectory ) {
									// many supported files in ZIP
									item.parent = baseDir;
									baseDir.addDir(item);
									for ( int i=0; i<item.fileCount(); i++ ) {
										FileInfo file = item.getFile(i);
										mFileList.put(file.getPathName(), file);
									}
								} else {
									item.parent = baseDir;
									baseDir.addFile(item);
									mFileList.put(pathName, item);
								}
								continue;
							}
							isNew = true;
						}
						if ( item.format!=null ) {
							item.parent = baseDir;
							baseDir.addFile(item);
							if ( isNew )
								mFileList.put(pathName, item);
						}
					}
				}
				// process directories 
				for ( File f : items ) {
					if ( f.isDirectory() ) {
						if ( f.getName().startsWith(".") )
							continue; // treat dirs beginning with '.' as hidden
						FileInfo item = new FileInfo( f );
						if ( knownItems!=null && knownItems.contains(item.getPathName()) )
							continue;
						item.parent = baseDir;
						baseDir.addDir(item);					
					}
				}
			}
			baseDir.isListed = true;
			return !baseDir.isEmpty();
		} catch ( Exception e ) {
			L.e("Exception while listing directory " + baseDir.pathname, e);
			baseDir.isListed = true;
			return false;
		}
	}
	
	public static class ScanControl {
		volatile private boolean stopped = false;
		public boolean isStopped() {
			return stopped;
		}
		public void stop() {
			stopped = true;
		}
	}
	
	/**
	 * Scan single directory for dir and file properties in background thread.
	 * @param baseDir is directory to scan
	 * @param readyCallback is called on completion
	 */
	public void scanDirectory( final FileInfo baseDir, final Runnable readyCallback, final boolean recursiveScan, final ScanControl scanControl )
	{
		final long startTime = System.currentTimeMillis();
		listDirectory(baseDir);
		listSubtree( baseDir, 2, android.os.SystemClock.uptimeMillis() + 700 );
		if ( (!getDirScanEnabled() || baseDir.isScanned) && !recursiveScan ) {
			readyCallback.run();
			return;
		}
		engine.execute(new EngineTask() {
			long nextProgressTime = startTime + 2000;
			boolean progressShown = false;
			void progress( int percent )
			{
				if ( recursiveScan )
					return; // no progress dialog for recursive scan
				long ts = System.currentTimeMillis();
				if ( ts>=nextProgressTime ) {
					engine.showProgress(percent, R.string.progress_scanning);
					nextProgressTime = ts + 1500;
					progressShown = true;
				}
			}
			
			public void done() {
				baseDir.isScanned = true;
				if ( progressShown )
					engine.hideProgress();
				readyCallback.run();
			}

			public void fail(Exception e) {
				L.e("Exception while scanning directory " + baseDir.pathname, e);
				baseDir.isScanned = true;
				if ( progressShown )
					engine.hideProgress();
				readyCallback.run();
			}

			public void scan( FileInfo baseDir ) {
				if ( baseDir.isRecentDir() )
					return;
				//listDirectory(baseDir);
				progress(1000);
				if ( scanControl.isStopped() )
					return;
				for ( int i=baseDir.dirCount()-1; i>=0; i-- ) {
					if ( scanControl.isStopped() )
						return;
					listDirectory(baseDir.getDir(i));
				}
				progress(2000);
				if ( mHideEmptyDirs )
					baseDir.removeEmptyDirs();
				if ( scanControl.isStopped() )
					return;
				ArrayList<FileInfo> filesForParsing = new ArrayList<FileInfo>();
				int count = baseDir.fileCount();
				for ( int i=0; i<count; i++ ) {
					FileInfo item = baseDir.getFile(i);
					boolean found = db.findByPathname(item);
					if ( found )
						Log.v("cr3db", "File " + item.pathname + " is found in DB (id="+item.id+", title=" + item.title + ", authors=" + item.authors +")");

					boolean saveToDB = true;
					if ( !found && item.format.canParseProperties() ) {
						filesForParsing.add(item);
						saveToDB = false;
					}

					if ( !found && saveToDB ) {
						db.save(item);
						Log.v("cr3db", "File " + item.pathname + " is added to DB (id="+item.id+", title=" + item.title + ", authors=" + item.authors +")");
					}
					progress( 2000 + 3000 * i / count );
				}
				// db lookup files
				count = filesForParsing.size();
				for ( int i=0; i<count; i++ ) {
					if ( scanControl.isStopped() )
						return;
					FileInfo item = filesForParsing.get(i);
					engine.scanBookProperties(item);
					db.save(item);
					Log.v("cr3db", "File " + item.pathname + " is added to DB (id="+item.id+", title=" + item.title + ", authors=" + item.authors +")");
					progress( 5000 + 5000 * i / count );
				}
				if ( recursiveScan ) {
					if ( scanControl.isStopped() )
						return;
					for ( int i=baseDir.dirCount()-1; i>=0; i-- )
						scan(baseDir.getDir(i));
				}
			}
			
			public void work() throws Exception {
				// scan (list) directories
				nextProgressTime = startTime + 1500;
				scan( baseDir );
			}
		});
	}

//	private int lastPercent = 0;
//	private long lastProgressUpdate = 0;
//	private final int PROGRESS_UPDATE_INTERVAL = 2000; // 2 seconds
//	private void updateProgress( int percent )
//	{
//		long ts = System.currentTimeMillis();
//		if ( percent!=lastPercent && ts>lastProgressUpdate+PROGRESS_UPDATE_INTERVAL ) {
//			engine.showProgress(percent, "Scanning directories...");
//			lastPercent = percent;
//			lastProgressUpdate = ts;
//		}
//	}
	
//	private void lookupDB()
//	{
//		int count = mFileList.size();
//		for ( int i=0; i<count; i++ ) {
//			FileInfo item = mFileList.get(i);
//			boolean found = db.findByPathname(item);
//			if ( found )
//				Log.v("cr3db", "File " + item.pathname + " is found in DB (id="+item.id+", title=" + item.title + ", authors=" + item.authors +")");
//
//			boolean saveToDB = true;
//			if ( !found && item.format==DocumentFormat.FB2 ) {
//				mFilesForParsing.add(item);
//				saveToDB = false;
//			}
//
//			if ( !found && saveToDB ) {
//				db.save(item);
//				Log.v("cr3db", "File " + item.pathname + " is added to DB (id="+item.id+", title=" + item.title + ", authors=" + item.authors +")");
//			}
//			updateProgress( 1000 + 4000 * i / count );
//		}
//	}
//	
//	private void parseBookProperties()
//	{
//		int count = mFilesForParsing.size();
//		for ( int i=0; i<count; i++ ) {
//			FileInfo item = mFilesForParsing.get(i);
//			engine.scanBookProperties(item);
//			db.save(item);
//			Log.v("cr3db", "File " + item.pathname + " is added to DB (id="+item.id+", title=" + item.title + ", authors=" + item.authors +")");
//			updateProgress( 5000 + 5000 * i / count );
//		}
//	}
	
	private boolean addRoot( String pathname, int resourceId, boolean listIt)
	{
		return addRoot( pathname, coolReader.getResources().getString(resourceId), listIt);
	}
	private boolean addRoot( String pathname, String filename, boolean listIt)
	{
		FileInfo dir = new FileInfo();
		dir.isDirectory = true;
		dir.pathname = pathname;
		dir.filename = filename;
		if ( mRoot.findItemByPathName(pathname)!=null )
			return false; // exclude duplicates
		if ( listIt && !listDirectory(dir) )
			return false;
		mRoot.addDir(dir);
		dir.parent = mRoot;
		if ( !listIt ) {
			dir.isListed = true;
			dir.isScanned = true;
		}
		return true;
	}
	
	private void addOPDSRoot() {
		FileInfo dir = new FileInfo();
		dir.isDirectory = true;
		dir.pathname = FileInfo.OPDS_LIST_TAG;
		dir.filename = "OPDS Catalogs";
		dir.isListed = true;
		dir.isScanned = true;
		dir.parent = mRoot;
		mRoot.addDir(dir);
		String[] urls = {
				"http://www.feedbooks.com/catalog/", "Feedbooks",
				"http://bookserver.archive.org/catalog/", "Internet Archive",
				"http://m.gutenberg.org/", "Project Gutenberg", 
				"http://ebooksearch.webfactional.com/catalog.atom", "eBookSearch", 
				"http://bookserver.revues.org/", "Revues.org", 
				"http://www.legimi.com/opds/root.atom", "Legimi",
				"http://www.ebooksgratuits.com/opds/", "Ebooks libres et gratuits",
				"http://213.5.65.159/opds/", "Flibusta", 
				"http://lib.ololo.cc/opds/", "lib.ololo.cc",
		};
		for ( int i=0; i<urls.length-1; i+=2 ) {
			String url = urls[i];
			String title = urls[i+1];
			FileInfo odps = new FileInfo();
			odps.isDirectory = true;
			odps.pathname = FileInfo.OPDS_DIR_PREFIX + url;
			odps.filename = title;
			odps.isListed = true;
			odps.isScanned = true;
			odps.parent = dir;
			dir.addDir(odps);
		}
	}
	
	/**
	 * Lists all directories from root to directory of specified file, returns found directory.
	 * @param file
	 * @param root
	 * @return
	 */
	private FileInfo findParentInternal( FileInfo file, FileInfo root )
	{
		if ( root==null || file==null || root.isRecentDir() )
			return null;
		if ( !root.isRootDir() && !file.getPathName().startsWith( root.getPathName() ) )
			return null;
		// to list all directories starting root dir
		if ( root.isDirectory && !root.isSpecialDir() )
				listDirectory(root);
		for ( int i=0; i<root.dirCount(); i++ ) {
			FileInfo found = findParentInternal( file, root.getDir(i));
			if ( found!=null )
				return found;
		}
		for ( int i=0; i<root.fileCount(); i++ ) {
			if ( root.getFile(i).getPathName().equals(file.getPathName()) )
				return root;
			if ( root.getFile(i).getPathName().startsWith(file.getPathName() + "@/") )
				return root;
		}
		return null;
	}
	
	public final static int MAX_DIR_LIST_TIME = 500; // 0.5 seconds
	
	/**
	 * Lists all directories from root to directory of specified file, returns found directory.
	 * @param file
	 * @param root
	 * @return
	 */
	public FileInfo findParent( FileInfo file, FileInfo root )
	{
		FileInfo parent = findParentInternal(file, root);
		if ( parent==null ) {
			autoAddRootForFile(new File(file.pathname) );
			parent = findParentInternal(file, root);
			if ( parent==null ) {
				L.e("Cannot find root directory for file " + file.pathname);
				return null;
			}
		}
		long maxTs = android.os.SystemClock.uptimeMillis() + MAX_DIR_LIST_TIME;
		listSubtrees(root, mHideEmptyDirs ? 5 : 1, maxTs);
		return parent;
	}
	
	/**
	 * List directories in subtree, limited by runtime and depth; remove empty branches (w/o books).  
	 * @param root is directory to start with
	 * @param maxDepth is maximum depth
	 * @param limitTs is limit for android.os.SystemClock.uptimeMillis()
	 * @return true if completed, false if stopped by limit. 
	 */
	private boolean listSubtree( FileInfo root, int maxDepth, long limitTs )
	{
		long ts = android.os.SystemClock.uptimeMillis();
		if ( ts>limitTs || maxDepth<=0 )
			return false;
		listDirectory(root);
		for ( int i=root.dirCount()-1; i>=-0; i-- ) {
			boolean res = listSubtree(root.getDir(i), maxDepth-1, limitTs);
			if ( !res )
				return false;
		}
		if ( mHideEmptyDirs )
			root.removeEmptyDirs();
		return true;
	}
	
	/**
	 * List directories in subtree, limited by runtime and depth; remove empty branches (w/o books).  
	 * @param root is directory to start with
	 * @param maxDepth is maximum depth
	 * @param limitTs is limit for android.os.SystemClock.uptimeMillis()
	 * @return true if completed, false if stopped by limit. 
	 */
	public boolean listSubtrees( FileInfo root, int maxDepth, long limitTs )
	{
		for ( int depth = 1; depth<=maxDepth; depth++ ) {
			boolean res = listSubtree( root, depth, limitTs );
			if ( res )
				return true;
			long ts = android.os.SystemClock.uptimeMillis();
			if ( ts>limitTs )
				return false; // limited by time
			// iterate deeper
		}
		return false; // limited by depth
	}
	
	public FileInfo setSearchResults( FileInfo[] results ) {
		FileInfo existingResults = null;
		for ( int i=0; i<mRoot.dirCount(); i++ ) {
			FileInfo dir = mRoot.getDir(i);
			if ( dir.isSearchDir() ) {
				existingResults = dir;
				dir.clear();
				break;
			}
		}
		if ( existingResults==null ) {
			FileInfo dir = new FileInfo();
			dir.isDirectory = true;
			dir.pathname = FileInfo.SEARCH_RESULT_DIR_TAG;
			dir.filename = coolReader.getResources().getString(R.string.dir_search_results);
			dir.parent = mRoot;
			dir.isListed = true;
			dir.isScanned = true;
			mRoot.addDir(dir);
			existingResults = dir;
		}
		for ( FileInfo item : results )
			existingResults.addFile(item);
		return existingResults;
	}

	private void autoAddRoots( String rootPath, String[] pathsToExclude )
	{
		try {
			File root = new File(rootPath);
			File[] files = root.listFiles();
			if ( files!=null ) {
				for ( File f : files ) {
					if ( !f.isDirectory() )
						continue;
					String fullPath = f.getAbsolutePath();
					if ( engine.isLink(fullPath) ) {
						L.d("skipping symlink " + fullPath);
						continue;
					}
					boolean skip = false;
					for ( String path : pathsToExclude ) {
						if ( fullPath.startsWith(path) ) {
							skip = true;
							break;
						}
					}
					if ( skip )
						continue;
					if ( !f.canWrite() )
						continue;
					L.i("Found possible mount point " + f.getAbsolutePath());
					addRoot(f.getAbsolutePath(), f.getAbsolutePath(), true);
				}
			}
		} catch ( Exception e ) {
			L.w("Exception while trying to auto add roots");
		}
	}
	
	public void initRoots()
	{
		mRoot.clear();
		// create recent books dir
		addRoot( FileInfo.RECENT_DIR_TAG, R.string.dir_recent_books, false);
		String sdpath = Environment.getExternalStorageDirectory().getAbsolutePath();
		if ( "/nand".equals(sdpath) && new File("/sdcard").isDirectory() )
			sdpath = "/sdcard";
		addRoot( sdpath, R.string.dir_sd_card, true);
		// internal SD card on Nook
		addRoot( "/system/media/sdcard", R.string.dir_internal_sd_card, true);
		// internal memory
		addRoot( "/media", R.string.dir_internal_memory, true);
		addRoot( "/nand", R.string.dir_internal_memory, true);
		// internal SD card on PocketBook 701 IQ
		addRoot( "/PocketBook701", R.string.dir_internal_sd_card, true);
		// external SD
		addRoot( "/mnt/extsd", "External SD /mnt/extsd", true);
		// external SD card Huawei S7
		addRoot( "/sdcard2", R.string.dir_sd_card_2, true);
		//addRoot( "/mnt/localdisk", "/mnt/localdisk", true);
		autoAddRoots( "/", SYSTEM_ROOT_PATHS );
		autoAddRoots( "/mnt", new String[] {} );
		
		addOPDSRoot();
	}
	
	public boolean autoAddRootForFile( File f ) {
		File p = f.getParentFile();
		while ( p!=null ) {
			if ( p.getParentFile()==null || p.getParentFile().getParentFile()==null )
				break;
			p = p.getParentFile();
		}
		if ( p!=null ) {
			L.i("Found possible mount point " + p.getAbsolutePath());
			return addRoot(p.getAbsolutePath(), p.getAbsolutePath(), true);
		}
		return false;
	}
	
	private static final String[] SYSTEM_ROOT_PATHS = {"/system", "/data", "/mnt"};
	
//	public boolean scan()
//	{
//		L.i("Started scanning");
//		long start = System.currentTimeMillis();
//		mFileList.clear();
//		mFilesForParsing.clear();
//		mRoot.clear();
//		// create recent books dir
//		FileInfo recentDir = new FileInfo();
//		recentDir.isDirectory = true;
//		recentDir.pathname = "@recent";
//		recentDir.filename = "Recent Books";
//		mRoot.addDir(recentDir);
//		recentDir.parent = mRoot;
//		// scan directories
//		lastPercent = -1;
//		lastProgressUpdate = System.currentTimeMillis() - 500;
//		boolean res = scanDirectories( mRoot );
//		// process found files
//		lookupDB();
//		parseBookProperties();
//		updateProgress(9999);
//		L.i("Finished scanning (" + (System.currentTimeMillis()-start)+ " ms)");
//		return res;
//	}
	
	
	public FileInfo getDownloadDirectory() {
		for ( int i=0; i<mRoot.dirCount(); i++ ) {
			FileInfo item = mRoot.getDir(i);
			if ( !item.isSpecialDir() && !item.isArchive ) {
				FileInfo books = item.findItemByPathName(item.pathname+"/Books");
				if ( books.exists() )
					return books;
				File dir = new File(item.getPathName());
				if ( dir.isDirectory() && dir.canWrite() ) {
					File f = new File( dir, "Books" );
					if ( f.mkdirs() ) {
						books = new FileInfo(f);
						books.parent = item;
						item.addDir(books);
						books.isScanned = true;
						books.isListed = true;
						return books;
					}
				}
			}
		}
		return null;
	}
	
	public FileInfo getRoot() 
	{
		return mRoot;
	}

	public FileInfo getOPDSRoot() 
	{
		for ( int i=0; i<mRoot.dirCount(); i++ ) {
			if ( mRoot.getDir(i).isOPDSRoot() )
				return mRoot.getDir(i);
		}
		L.w("OPDS root directory not found!");
		return null;
	}
	
	public Scanner( CoolReader coolReader, CRDB db, Engine engine )
	{
		this.engine = engine;
		this.db = db;
		this.coolReader = coolReader;
		mRoot = new FileInfo();
		mRoot.path = FileInfo.ROOT_DIR_TAG;	
		mRoot.filename = "File Manager";	
		mRoot.pathname = FileInfo.ROOT_DIR_TAG;
		mRoot.isListed = true;
		mRoot.isScanned = true;
		mRoot.isDirectory = true;
	}

	private final Engine engine;
	private final CRDB db;
	private final CoolReader coolReader;
}
