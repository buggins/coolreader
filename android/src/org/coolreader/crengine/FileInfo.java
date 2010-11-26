package org.coolreader.crengine;

import java.io.File;
import java.util.ArrayList;

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

	private void removeChild( FileInfo item )
	{
		int n = files.indexOf(item);
		if ( n>=0 && n<files.size() )
			files.remove(n);
	}
	
	public boolean deleteFile()
	{
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
}
