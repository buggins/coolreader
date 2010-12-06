package org.coolreader.crengine;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;

import org.coolreader.R;

import android.util.Log;

public class FileInfo {
	Long id; // db id
	String title; // book title
	String authors; // authors, delimited with '|'
	String series; // series name w/o number
	int seriesNumber; // number of book inside series
	String path; // path to directory where file or archive is located
	String filename; // file name w/o path for normal file, with optional path for file inside archive 
	String pathname; // full path+arcname+filename
	String arcname; // archive file name w/o path
	DocumentFormat format;
	int size;
	int arcsize;
	long createTime;
	long lastAccessTime;
	boolean isArchive;
	boolean isDirectory;
	boolean isModified;
	boolean isListed;
	boolean isScanned;
	private ArrayList<FileInfo> files;// files
	private ArrayList<FileInfo> dirs; // directories
	FileInfo parent; // parent item

	public FileInfo( String path )
	{
		this(new File(path));
	}
	
	public FileInfo( File f )
	{
		if ( !f.isDirectory() ) {
			DocumentFormat fmt = DocumentFormat.byExtension(f.getName());
			filename = f.getName();
			path = f.getParent();
			pathname = f.getAbsolutePath();
			format = fmt;
			createTime = f.lastModified();
			size = (int)f.length();
		} else {
			filename = f.getName();
			path = f.getParent();
			pathname = f.getAbsolutePath();
			isDirectory = true;
		}
	}
	
	public FileInfo()
	{
	}

	/// doesn't copy parent and children
	public FileInfo(FileInfo v)
	{
		title = v.title;
		authors = v.authors;
		series = v.series;
		seriesNumber = v.seriesNumber;
		path = v.path;
		filename = v.filename;
		pathname = v.pathname;
		arcname = v.arcname;
		format = v.format;
		size = v.size;
		arcsize = v.arcsize;
		isArchive = v.isArchive;
		isDirectory = v.isDirectory;
		createTime = v.createTime;
		lastAccessTime = v.lastAccessTime;
	}
	
	public final static String RECENT_DIR_TAG = "@recent";
	public final static String ROOT_DIR_TAG = "@root";
	
	public boolean isRecentDir()
	{
		return RECENT_DIR_TAG.equals(pathname);
	}
	
	public boolean isRootDir()
	{
		return ROOT_DIR_TAG.equals(pathname);
	}
	
	public boolean isSpecialDir()
	{
		return pathname.startsWith("@");
	}
	
	public boolean isHidden()
	{
		return pathname.startsWith(".");
	}
	
	public String getPathName()
	{
		return pathname;
	}
	public int dirCount()
	{
		return dirs!=null ? dirs.size() : 0;
	}
	public int fileCount()
	{
		return files!=null ? files.size() : 0;
	}
	public int size()
	{
		return dirCount() + fileCount();
	}
	public void addDir( FileInfo dir )
	{
		if ( dirs==null )
			dirs = new ArrayList<FileInfo>();
		dirs.add(dir);
	}
	public void addFile( FileInfo file )
	{
		if ( files==null )
			files = new ArrayList<FileInfo>();
		files.add(file);
	}
	public boolean isEmpty()
	{
		return fileCount()==0 && dirCount()==0;
	}
	public FileInfo getItem( int index )
	{
		if ( index<0 )
			throw new IndexOutOfBoundsException();
		if ( index<dirCount())
			return dirs.get(index);
		index -= dirCount();
		if ( index<fileCount())
			return files.get(index);
		Log.e("cr3", "Index out of bounds " + index + " at FileInfo.getItem() : returning 0");
		//throw new IndexOutOfBoundsException();
		return null;
	}
	public FileInfo findItemByPathName( String pathName )
	{
		if ( dirs!=null )
			for ( FileInfo dir : dirs )
				if ( pathName.equals(dir.getPathName() ))
					return dir;
		if ( files!=null )
			for ( FileInfo file : files )
				if ( pathName.equals(file.getPathName() ))
					return file;
		return null;
	}
	public int getItemIndex( FileInfo item )
	{
		if ( item==null )
			return -1;
		for ( int i=0; i<dirCount(); i++ ) {
			if ( item.pathname.equals(getDir(i).pathname) )
				return i;
		}
		for ( int i=0; i<fileCount(); i++ ) {
			if ( item.pathname.equals(getFile(i).pathname) )
				return i + dirCount();
		}
		return -1;
	}

	public FileInfo getDir( int index )
	{
		if ( index<0 )
			throw new IndexOutOfBoundsException();
		if ( index<dirCount())
			return dirs.get(index);
		throw new IndexOutOfBoundsException();
	}
	public FileInfo getFile( int index )
	{
		if ( index<0 )
			throw new IndexOutOfBoundsException();
		if ( index<fileCount())
			return files.get(index);
		throw new IndexOutOfBoundsException();
	}

	public void removeEmptyDirs()
	{
		if ( parent==null || pathname.startsWith("@") )
			return;
		for ( int i=dirCount()-1; i>=0; i-- )
			if ( getDir(i).dirCount()==0 && getDir(i).fileCount()==0 )
				dirs.remove(i);
	}
	
	private void removeChild( FileInfo item )
	{
		int n = files.indexOf(item);
		if ( n>=0 && n<files.size() )
			files.remove(n);
	}
	
	public boolean deleteFile()
	{
		if ( isArchive ) {
			File f = new File(arcname);
			if ( f.exists() && !f.isDirectory() ) {
				if ( !f.delete() )
					return false;
				if ( parent!=null ) {
					if ( parent.isArchive ) {
						// remove all files belonging to this archive
					} else {
						parent.removeChild(this);
					}
				}
				return true;
			}
		}
		if ( isDirectory )
			return false;
		if ( !fileExists() )
			return false;
		File f = new File(getPathName());
		if ( f.delete() ) {
			if ( parent!=null ) {
				parent.removeChild(this);
			}
			return true;
		}
		return false;
	}
	
	public boolean fileExists()
	{
		if (isDirectory)
			return false;
		if ( isArchive && arcname!=null )
			return new File(arcname).exists();
		return new File(pathname).exists();
	}
	
	public boolean isModified() {
		return isModified || id==null;
	}

	public void setModified(boolean isModified) {
		this.isModified = isModified;
	}

	public void clear()
	{
		dirs = null;
		files = null;
	}
	
	public static enum SortOrder {
		FILENAME(R.string.mi_book_sort_order_filename, new Comparator<FileInfo>() {
			public int compare( FileInfo f1, FileInfo f2 )
			{
				if ( f1==null || f2==null )
					return 0;
				return cmp(f1.filename, f2.filename);
			}
		}),
		FILENAME_DESC(R.string.mi_book_sort_order_filename_desc, FILENAME),
		TIMESTAMP(R.string.mi_book_sort_order_timestamp, new Comparator<FileInfo>() {
			public int compare( FileInfo f1, FileInfo f2 )
			{
				if ( f1==null || f2==null )
					return 0;
				return firstNz( cmp(f1.createTime, f2.createTime), cmp(f1.filename, f2.filename) );
			}
		}),
		TIMESTAMP_DESC(R.string.mi_book_sort_order_timestamp_desc, TIMESTAMP),
		AUTHOR_TITLE(R.string.mi_book_sort_order_author, new Comparator<FileInfo>() {
			public int compare( FileInfo f1, FileInfo f2 )
			{
				if ( f1==null || f2==null )
					return 0;
				return firstNz(
						cmpNotNullFirst(f1.authors, f2.authors)
						,cmpNotNullFirst(f1.series, f2.series)
						,cmp(f1.seriesNumber, f2.seriesNumber)
						,cmpNotNullFirst(f1.title, f2.title)
						,cmp(f1.filename, f2.filename) 
						);
			}
		}),
		AUTHOR_TITLE_DESC(R.string.mi_book_sort_order_author_desc, AUTHOR_TITLE),
		TITLE_AUTHOR(R.string.mi_book_sort_order_title, new Comparator<FileInfo>() {
			public int compare( FileInfo f1, FileInfo f2 )
			{
				if ( f1==null || f2==null )
					return 0;
				return firstNz(
						cmpNotNullFirst(f1.series, f2.series)
						,cmp(f1.seriesNumber, f2.seriesNumber)
						,cmpNotNullFirst(f1.title, f2.title)
						,cmpNotNullFirst(f1.authors, f2.authors)
						,cmp(f1.filename, f2.filename) 
						);
			}
		}),
		TITLE_AUTHOR_DESC(R.string.mi_book_sort_order_title_desc, TITLE_AUTHOR);
		//================================================
		private final Comparator<FileInfo> comparator;
		public final int resourceId;
		private SortOrder( int resourceId, Comparator<FileInfo> comparator )
		{
			this.resourceId = resourceId;
			this.comparator = comparator;
		}
		private SortOrder( int resourceId, final SortOrder base )
		{
			this.resourceId = resourceId;
			this.comparator = new Comparator<FileInfo>() {
				public int compare( FileInfo f1, FileInfo f2 )
				{
					return -base.comparator.compare(f1, f2);
				}
			};
		}
		
		public final Comparator<FileInfo> getComparator()
		{
			return comparator;
		}
		
		/**
		 * Compares two strings
		 * @param str1
		 * @param str2
		 * @return
		 */
		private static int cmp( String str1, String str2 )
		{
			if ( str1==null && str2==null )
				return 0;
			if ( str1==null )
				return -1;
			if ( str2==null )
				return 1;
			return str1.compareTo(str2);
		}
		
		/**
		 * Same as cmp, but not-null comes first
		 * @param str1
		 * @param str2
		 * @return
		 */
		private static int cmpNotNullFirst( String str1, String str2 )
		{
			if ( str1==null && str2==null )
				return 0;
			if ( str1==null )
				return 1;
			if ( str2==null )
				return -1;
			return str1.compareTo(str2);
		}
		
		private static int cmp( long n1, long n2 )
		{
			if ( n1<n2 )
				return -1;
			if ( n1>n2 )
				return 1;
			return 0;
		}
		
		private static int firstNz( int... v)
		{
			for ( int i=0; i<v.length; i++ ) {
				if ( v[i]!=0 )
					return v[i];
			}
			return 0;
		}
	}
	public final static SortOrder DEF_SORT_ORDER = SortOrder.AUTHOR_TITLE;
		
	public void sort( SortOrder SortOrder )
	{
		if ( dirs!=null ) {
			ArrayList<FileInfo> newDirs = new ArrayList<FileInfo>(dirs);
			Collections.sort( newDirs, SortOrder.getComparator() );
			dirs = newDirs;
		}
		if ( files!=null ) {
			ArrayList<FileInfo> newFiles = new ArrayList<FileInfo>(files);
			Collections.sort( newFiles, SortOrder.getComparator() );
			files = newFiles;
		}
	}
	
	@Override
	public String toString()
	{
		return pathname;
	}
}
