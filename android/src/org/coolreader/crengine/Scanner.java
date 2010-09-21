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
					DocumentFormat fmt = DocumentFormat.byExtension(f.getName());
					if ( fmt!=null ) {
						FileInfo item = new FileInfo();
						item.filename = f.getName();
						item.path = f.getPath();
						item.pathname = f.getAbsolutePath();
						item.parent = baseDir;
						item.format = fmt;
						
						if ( item.format==DocumentFormat.FB2 ) {
							engine.scanBookProperties(item);
						}
						
						baseDir.addFile(item);
						fileList.add(item);
					}
				}
			}
			// process directories 
			for ( File f : items ) {
				if ( f.isDirectory() ) {
					FileInfo item = new FileInfo();
					item.filename = f.getName();
					item.path = f.getPath();
					item.pathname = f.getAbsolutePath();
					item.isDirectory = true;
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
	
	public Scanner( Engine engine, File rootDir, String description )
	{
		this.engine = engine;
		root = new FileInfo();
		root.path = rootDir.getPath();	
		root.filename = rootDir.getName();	
		root.pathname = rootDir.getAbsolutePath();
	}

	private final Engine engine;
}
