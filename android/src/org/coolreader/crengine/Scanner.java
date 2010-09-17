package org.coolreader.crengine;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import android.util.Log;

public class Scanner {
	static class FileInfo {
		String path;
		String filename;
		String pathname;
		String extension;
		int size;
		boolean isArchive;
		ArrayList<FileInfo> subitems; // ZIP archive items
	}
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
	static void scanDirectories( File dir, List<FileInfo> list )
	{
		try { 
			File[] items = dir.listFiles();
			// process normal files
			for ( File f : items ) {
				if ( !f.isDirectory() ) {
					String fname = f.getName().toLowerCase();
					for ( String ext : supportedExtensions ) {
						if ( fname.endsWith(ext) ) {
							FileInfo item = new FileInfo();
							item.filename = f.getName();
							item.path = f.getPath();
							item.pathname = f.getAbsolutePath();
							list.add(item);
						}
					}
				}
			}
			// process directories files
			for ( File f : items ) {
				if ( f.isDirectory() )
					scanDirectories(f, list);
			}
		} catch ( Exception e ) {
			Log.e("cr3", "Exception while scanning directory " + dir.getAbsolutePath(), e);
		}
	}
}
