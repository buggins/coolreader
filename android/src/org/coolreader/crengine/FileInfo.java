package org.coolreader.crengine;

import java.io.File;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Comparator;
import java.util.Enumeration;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import org.coolreader.R;

import android.util.Log;

public class FileInfo {

	public final static String RECENT_DIR_TAG = "@recent";
	public final static String SEARCH_RESULT_DIR_TAG = "@search";
	public final static String ROOT_DIR_TAG = "@root";
	public final static String OPDS_LIST_TAG = "@opds";
	public final static String OPDS_DIR_PREFIX = "@opds:";
	public final static String AUTHORS_TAG = "@authors";
	public final static String AUTHOR_PREFIX = "@author:";
	
	
	
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
	int flags;
	boolean isArchive;
	boolean isDirectory;
	boolean isModified;
	boolean isListed;
	boolean isScanned;
	private ArrayList<FileInfo> files;// files
	private ArrayList<FileInfo> dirs; // directories
	FileInfo parent; // parent item
	
	Object tag; // some additional information
	
	public static final int DONT_USE_DOCUMENT_STYLES_FLAG = 1;

	/**
	 * To separate archive name from file name inside archive.
	 */
	public static final String ARC_SEPARATOR = "@/";
	
	
	public void setFlag( int flag, boolean value ) {
		flags = flags & (~flag) | (value? flag : 0);
	}
	
	public boolean getFlag( int flag ) {
		return (flags & flag)!=0;
	}
	
	/**
	 * Split archive + file path name by ARC_SEPARATOR
	 * @param pathName is pathname like /arc_file_path@/filepath_inside_arc or /file_path 
	 * @return item[0] is pathname, item[1] is archive name (null if no archive)
	 */
	public static String[] splitArcName( String pathName )
	{
		String[] res = new String[2];
		int arcSeparatorPos = pathName.indexOf(ARC_SEPARATOR);
		if ( arcSeparatorPos>=0 ) {
			// from archive
			res[1] = pathName.substring(0, arcSeparatorPos);
			res[0] = pathName.substring(arcSeparatorPos + ARC_SEPARATOR.length());
		} else {
			res[0] = pathName;
		}
		return res;
	}
	
	public FileInfo( String pathName )
	{
		String[] parts = splitArcName( pathName );
		if ( parts[1]!=null ) {
			// from archive
			isArchive = true;
			arcname = parts[1];
			pathname = parts[0];
			File f = new File(pathname);
			filename = f.getName();
			path = f.getPath();
			File arc = new File(arcname);
			if ( arc.isFile() && arc.exists() ) {
				arcsize = (int)arc.length();
				try {
					ZipFile zip = new ZipFile(new File(arcname));
					for ( Enumeration<?> e = zip.entries(); e.hasMoreElements(); ) {
						ZipEntry entry = (ZipEntry)e.nextElement();
						
						String name = entry.getName();
						if ( !entry.isDirectory() && !pathname.equals(name) ) {
							format = DocumentFormat.byExtension(name);
							size = (int)entry.getSize();
							createTime = entry.getTime();
							break;
						}
					}
				} catch ( Exception e ) {
					Log.e("cr3", "error while reading contents of " + arcname);
				}
			}
		} else {
			fromFile(new File(pathName));
		}
	}
	
	public String getFileNameToDisplay() {
		boolean isSingleFileArchive = (isArchive && parent!=null && !parent.isArchive && arcname!=null);
		return isSingleFileArchive
			? new File(arcname).getName() : filename;
	}
	
	private void fromFile( File f )
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
	
	public FileInfo( File f )
	{
		fromFile(f);
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
	
	/**
	 * @return archive file path and name, null if this object is neither archive nor a file inside archive
	 */
	public String getArchiveName()
	{
		return arcname;
	}
	
	/**
	 * @return file name inside archive, null if this object is not a file inside archive
	 */
	public String getArchiveItemName()
	{
		if ( isArchive && !isDirectory && pathname!=null )
			return pathname;
		return null;
	}
	
	public boolean isRecentDir()
	{
		return RECENT_DIR_TAG.equals(pathname);
	}
	
	public boolean isSearchDir()
	{
		return SEARCH_RESULT_DIR_TAG.equals(pathname);
	}
	
	public boolean isRootDir()
	{
		return ROOT_DIR_TAG.equals(pathname);
	}
	
	public boolean isSpecialDir()
	{
		return pathname!=null && pathname.startsWith("@");
	}
	
	public boolean isOPDSDir()
	{
		return pathname!=null && pathname.startsWith(OPDS_DIR_PREFIX);
	}
	
	public boolean isOPDSRoot()
	{
		return OPDS_LIST_TAG.equals(pathname);
	}
	
	public boolean isHidden()
	{
		return pathname.startsWith(".");
	}
	
	public String getOPDSUrl()
	{
		if ( !pathname.startsWith(OPDS_DIR_PREFIX) )
			return null;
		return pathname.substring(OPDS_DIR_PREFIX.length());
	}
	
	/**
	 * Get absolute path to file.
	 * For plain files, returns /abs_path_to_file/filename.ext
	 * For archives, returns /abs_path_to_archive/arc_file_name.zip@/filename_inside_archive.ext
	 * @return full path + filename
	 */
	public String getPathName()
	{
		if ( arcname!=null )
			return arcname + ARC_SEPARATOR + pathname;
		return pathname;
	}

	public String getBasePath()
	{
		if ( arcname!=null )
			return arcname;
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

	public int itemCount()
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
	public void addItems( Collection<FileInfo> items )
	{
		for ( FileInfo item : items ) {
			if ( item.isDirectory )
				addDir(item);
			else
				addFile(item);
			item.parent = this;
		}
	}
	public void replaceItems( Collection<FileInfo> items )
	{
		files = null;
		dirs = null;
		addItems( items );
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
			for ( FileInfo file : files ) {
				if ( pathName.equals(file.getPathName() ))
					return file;
				if ( file.getPathName().startsWith(pathName+"@/" ))
					return file;
			}
		return null;
	}
	public int getItemIndex( FileInfo item )
	{
		if ( item==null )
			return -1;
		for ( int i=0; i<dirCount(); i++ ) {
			if ( item.getPathName().equals(getDir(i).getPathName()) )
				return i;
		}
		for ( int i=0; i<fileCount(); i++ ) {
			if ( item.getPathName().equals(getFile(i).getPathName()) )
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
	
	public void removeChild( FileInfo item )
	{
		if ( item.isSpecialDir() )
			return;
		if ( files!=null ) {
			int n = files.indexOf(item);
			if ( n>=0 && n<files.size() ) {
				files.remove(n);
				return;
			}
		}
		if ( dirs!=null ) {
			int n = dirs.indexOf(item);
			if ( n>=0 && n<dirs.size() ) {
				dirs.remove(n);
			}
		}
	}
	
	public boolean deleteFile()
	{
		if ( isArchive ) {
			if ( isDirectory )
				return false;
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
		File f = new File(pathname);
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
		if ( isArchive ) {
			if ( arcname!=null )
				return new File(arcname).exists();
			return false;
		}
		return new File(pathname).exists();
	}
	
	/**
	 * @return true if item (file, directory, or archive) exists
	 */
	public boolean exists()
	{
		if ( isArchive ) {
			if ( arcname==null )
				return false;
			File f = new File(arcname);
			return f.exists();
		}
		File f = new File(pathname);
		return f.exists();
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
				return cmp(f1.getFileNameToDisplay(), f2.getFileNameToDisplay());
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
		 * Compares two strings - with numbers sorted by value.
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
			
			int p1 = 0;
			int p2 = 0;
			for ( ;; ) {
				if ( p1>=str1.length() ) {
					if ( p2>=str2.length() )
						return 0;
					return 1;
				}
				if ( p2>=str2.length() )
					return -1;
				char ch1 = str1.charAt(p1);
				char ch2 = str2.charAt(p2);
				if ( ch1>='0' && ch1<='9' && ch2>='0' && ch2<='9' ) {
					int n1 = 0;
					int n2 = 0;
					while ( ch1>='0' && ch1<='9' ) {
						p1++;
						n1 = n1 * 10 + (ch1-'0');
						if ( p1>=str1.length() )
							break;
						ch1 = str1.charAt(p1);
					}
					while ( ch2>='0' && ch2<='9' ) {
						p2++;
						n2 = n2 * 10 + (ch2-'0');
						if ( p2>=str2.length() )
							break;
						ch2 = str2.charAt(p2);
					}
					int c = cmp(n1, n2);
					if ( c!=0 )
						return c;
				} else {
					if ( ch1<ch2 )
						return -1;
					if ( ch1>ch2 )
						return 1;
					p1++;
					p2++;
				}
			}
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
			return cmp(str1, str2);
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
		public static SortOrder fromName( String name ) {
			if ( name!=null )
				for ( SortOrder order : values() )
					if ( order.name().equals(name) )
						return order;
			return DEF_SORT_ORDER;
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
	
	public boolean allowSorting() {
		return isDirectory && !isRootDir() && !isRecentDir() && !isOPDSDir();
	}
}
