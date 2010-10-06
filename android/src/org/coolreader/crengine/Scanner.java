package org.coolreader.crengine;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;

import org.coolreader.CoolReader;
import org.coolreader.R;
import org.coolreader.crengine.Engine.EngineTask;

import android.os.Environment;
import android.util.Log;

public class Scanner {
	
	HashMap<String, FileInfo> mFileList = new HashMap<String, FileInfo>();
	ArrayList<FileInfo> mFilesForParsing = new ArrayList<FileInfo>();
	FileInfo mRoot;

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
//			Log.e("cr3", "Exception while scanning directory " + baseDir.pathname, e);
//			return false;
//		}
//	}
	
	/**
	 * Adds dir and file children to directory FileInfo item.
	 * @param baseDir is directory to list files and dirs for
	 * @return true if successful.
	 */
	public boolean listDirectory( FileInfo baseDir )
	{
		if ( baseDir.isListed )
			return true;
		try {
			File dir = new File(baseDir.pathname);
			File[] items = dir.listFiles();
			// process normal files
			if ( items!=null ) {
				for ( File f : items ) {
					if ( !f.isDirectory() ) {
						String pathName = f.getAbsolutePath();
						FileInfo item = mFileList.get(pathName);
						boolean isNew = false;
						if ( item==null ) {
							item = new FileInfo( f );
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
						FileInfo item = new FileInfo( f );
						item.parent = baseDir;
						baseDir.addDir(item);					
					}
				}
			}
			baseDir.isListed = true;
			return !baseDir.isEmpty();
		} catch ( Exception e ) {
			Log.e("cr3", "Exception while listing directory " + baseDir.pathname, e);
			baseDir.isListed = true;
			return false;
		}
	}
	
	/**
	 * Scan single directory for dir and file properties in background thread.
	 * @param baseDir is directory to scan
	 * @param readyCallback is called on completion
	 */
	public void scanDirectory( final FileInfo baseDir, final Runnable readyCallback )
	{
		if ( baseDir.isScanned ) {
			readyCallback.run();
			return;
		}
		final long startTime = System.currentTimeMillis(); 
		listDirectory(baseDir);
		engine.execute(new EngineTask() {
			long nextProgressTime = startTime + 2000;
			boolean progressShown = false;
			void progress( int percent )
			{
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
				Log.e("cr3", "Exception while scanning directory " + baseDir.pathname, e);
				baseDir.isScanned = true;
				if ( progressShown )
					engine.hideProgress();
				readyCallback.run();
			}

			public void work() throws Exception {
				// scan (list) directories
				progress(1000);
				for ( int i=0; i<baseDir.dirCount(); i++ ) {
					listDirectory(baseDir.getDir(i));
				}
				progress(2000);
				ArrayList<FileInfo> filesForParsing = new ArrayList<FileInfo>();
				int count = baseDir.fileCount();
				for ( int i=0; i<count; i++ ) {
					FileInfo item = baseDir.getFile(i);
					boolean found = db.findByPathname(item);
					if ( found )
						Log.v("cr3db", "File " + item.pathname + " is found in DB (id="+item.id+", title=" + item.title + ", authors=" + item.authors +")");

					boolean saveToDB = true;
					if ( !found && item.format==DocumentFormat.FB2 ) {
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
					FileInfo item = filesForParsing.get(i);
					engine.scanBookProperties(item);
					db.save(item);
					Log.v("cr3db", "File " + item.pathname + " is added to DB (id="+item.id+", title=" + item.title + ", authors=" + item.authors +")");
					progress( 5000 + 5000 * i / count );
				}
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
	
	private boolean addRoot( String pathname, String filename, boolean listIt)
	{
		FileInfo dir = new FileInfo();
		dir.isDirectory = true;
		dir.pathname = pathname;
		dir.filename = filename;
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
	
	public void initRoots()
	{
		mRoot.clear();
		// create recent books dir
		addRoot( "@recent", "Recent Books", false);
		addRoot( Environment.getExternalStorageDirectory().getAbsolutePath(), "SD card", true);
		// internal SD card on Nook
		addRoot( "/system/media/sdcard", "Internal SD card", true);
	}
	
//	public boolean scan()
//	{
//		Log.i("cr3", "Started scanning");
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
//		Log.i("cr3", "Finished scanning (" + (System.currentTimeMillis()-start)+ " ms)");
//		return res;
//	}
	
	
	public FileInfo getRoot() 
	{
		return mRoot;
	}
	public Scanner( CoolReader coolReader, CRDB db, Engine engine )
	{
		this.engine = engine;
		this.db = db;
		this.coolReader = coolReader;
		mRoot = new FileInfo();
		mRoot.path = "@root";	
		mRoot.filename = "File Manager";	
		mRoot.pathname = "@root";
		mRoot.isListed = true;
		mRoot.isScanned = true;
	}

	private final Engine engine;
	private final CRDB db;
	private final CoolReader coolReader;
}
