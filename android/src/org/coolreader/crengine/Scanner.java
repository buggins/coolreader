package org.coolreader.crengine;

import java.io.File;
import java.util.ArrayList;

import android.util.Log;

public class Scanner {
	
	ArrayList<FileInfo> mFileList = new ArrayList<FileInfo>();
	ArrayList<FileInfo> mFilesForParsing = new ArrayList<FileInfo>();
	FileInfo mRoot;

	private boolean scanDirectories( FileInfo baseDir )
	{
		try {
			File dir = new File(baseDir.pathname);
			File[] items = dir.listFiles();
			// process normal files
			for ( File f : items ) {
				if ( !f.isDirectory() ) {
					FileInfo item = new FileInfo( f );
					if ( item.format!=null ) {
						item.parent = baseDir;
						baseDir.addFile(item);
						mFileList.add(item);
					}
				}
			}
			// process directories 
			for ( File f : items ) {
				if ( f.isDirectory() ) {
					FileInfo item = new FileInfo( f );
					item.parent = baseDir;
					scanDirectories(item);
					if ( !item.isEmpty() ) {
						baseDir.addDir(item);					
					}
				}
			}
			return !baseDir.isEmpty();
		} catch ( Exception e ) {
			Log.e("cr3", "Exception while scanning directory " + baseDir.pathname, e);
			return false;
		}
	}

	private int lastPercent = 0;
	private long lastProgressUpdate = 0;
	private final int PROGRESS_UPDATE_INTERVAL = 2000; // 2 seconds
	private void updateProgress( int percent )
	{
		long ts = System.currentTimeMillis();
		if ( percent!=lastPercent && ts>lastProgressUpdate+PROGRESS_UPDATE_INTERVAL ) {
			engine.showProgress(percent, "Scanning directories...");
			lastPercent = percent;
			lastProgressUpdate = ts;
		}
	}
	
	private void lookupDB()
	{
		int count = mFileList.size();
		for ( int i=0; i<count; i++ ) {
			FileInfo item = mFileList.get(i);
			boolean found = db.findByPathname(item);
			if ( found )
				Log.v("cr3db", "File " + item.pathname + " is found in DB (id="+item.id+", title=" + item.title + ", authors=" + item.authors +")");

			boolean saveToDB = true;
			if ( !found && item.format==DocumentFormat.FB2 ) {
				mFilesForParsing.add(item);
				saveToDB = false;
			}

			if ( !found && saveToDB ) {
				db.save(item);
				Log.v("cr3db", "File " + item.pathname + " is added to DB (id="+item.id+", title=" + item.title + ", authors=" + item.authors +")");
			}
			updateProgress( 1000 + 4000 * i / count );
		}
	}
	
	private void parseBookProperties()
	{
		int count = mFilesForParsing.size();
		for ( int i=0; i<count; i++ ) {
			FileInfo item = mFilesForParsing.get(i);
			engine.scanBookProperties(item);
			db.save(item);
			Log.v("cr3db", "File " + item.pathname + " is added to DB (id="+item.id+", title=" + item.title + ", authors=" + item.authors +")");
			updateProgress( 5000 + 5000 * i / count );
		}
	}
	
	public boolean scan()
	{
		Log.i("cr3", "Started scanning");
		long start = System.currentTimeMillis();
		mFileList.clear();
		mFilesForParsing.clear();
		mRoot.clear();
		// create recent books dir
		FileInfo recentDir = new FileInfo();
		recentDir.isDirectory = true;
		recentDir.pathname = "@recent";
		recentDir.filename = "Recent Books";
		mRoot.addDir(recentDir);
		recentDir.parent = mRoot;
		// scan directories
		lastPercent = -1;
		lastProgressUpdate = System.currentTimeMillis() - 500;
		boolean res = scanDirectories( mRoot );
		// process found files
		lookupDB();
		parseBookProperties();
		updateProgress(9999);
		Log.i("cr3", "Finished scanning (" + (System.currentTimeMillis()-start)+ " ms)");
		return res;
	}
	
	public Scanner( CRDB db, Engine engine, File rootDir, String description )
	{
		this.engine = engine;
		this.db = db;
		mRoot = new FileInfo();
		mRoot.path = rootDir.getPath();	
		mRoot.filename = rootDir.getName();	
		mRoot.pathname = rootDir.getAbsolutePath();
	}

	private final Engine engine;
	private final CRDB db;
}
