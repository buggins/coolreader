package org.coolreader.crengine;

import java.util.ArrayList;

import android.util.Log;

public class FileInfo {
	String path;
	String filename;
	String pathname;
	String extension;
	DocumentFormat format;
	int size;
	boolean isArchive;
	boolean isDirectory;
	private ArrayList<FileInfo> files;// files
	private ArrayList<FileInfo> dirs; // directories
	FileInfo parent; // parent item
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
	public void clear()
	{
		dirs = null;
		files = null;
	}
}
