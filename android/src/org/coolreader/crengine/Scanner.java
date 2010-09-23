package org.coolreader.crengine;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import android.util.Log;

public class Scanner {
	final static String[] supportedExtensions = new String[] {
		".fb2",
		".txt",
		".tcr",
		".htm",
		".html",
		".rtf",
		".epub",
		".zip",
	};
	
	List<FileInfo> fileList = new ArrayList<FileInfo>();
	FileInfo root;

	private boolean scanDirectories( FileInfo baseDir )
	{
		try {
			File dir = new File(baseDir.pathname);
			File[] items = dir.listFiles();
			// process normal files
			for ( File f : items ) {
				if ( !f.isDirectory() ) {
					FileInfo item = new FileInfo( f );
					boolean found = db.findByPathname(item);
					if ( found )
						Log.v("cr3db", "File " + item.pathname + " is found in DB (id="+item.id+", title=" + item.title + ", authors=" + item.authors +")");
					if ( item.format!=null ) {
						item.parent = baseDir;
						
						if ( !found && item.format==DocumentFormat.FB2 ) {
							engine.scanBookProperties(item);
						}

						if ( !found ) {
							db.save(item);
							Log.v("cr3db", "File " + item.pathname + " is added to DB (id="+item.id+", title=" + item.title + ", authors=" + item.authors +")");
						}
						
						baseDir.addFile(item);
						fileList.add(item);
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
	
	public boolean scan()
	{
		Log.i("cr3", "Started scanning");
		long start = System.currentTimeMillis();
		fileList.clear();
		root.clear();
		boolean res = scanDirectories( root );
		Log.i("cr3", "Finished scanning (" + (System.currentTimeMillis()-start)+ " ms)");
		return res;
	}
	
	public Scanner( CRDB db, Engine engine, File rootDir, String description )
	{
		this.engine = engine;
		this.db = db;
		root = new FileInfo();
		root.path = rootDir.getPath();	
		root.filename = rootDir.getName();	
		root.pathname = rootDir.getAbsolutePath();
	}

	private final Engine engine;
	private final CRDB db;
}
