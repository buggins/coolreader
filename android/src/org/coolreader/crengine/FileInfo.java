package org.coolreader.crengine;

import java.io.File;
import java.util.ArrayList;

import android.os.Parcel;
import android.os.Parcelable;
import android.util.Log;

public class FileInfo implements Parcelable {
	String title;
	String authors;
	String series;
	int seriesNumber;
	String path;
	String filename;
	String pathname;
	String arcname;
	DocumentFormat format;
	int size;
	boolean isArchive;
	boolean isDirectory;
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
			path = f.getPath();
			pathname = f.getAbsolutePath();
			format = fmt;
		} else {
			filename = f.getName();
			path = f.getPath();
			pathname = f.getAbsolutePath();
			isDirectory = true;
		}
	}
	
	public final static Parcelable.Creator<FileInfo> CREATOR = new Parcelable.Creator<FileInfo>() {

		public FileInfo createFromParcel(Parcel source) {
			try {
				FileInfo res = new FileInfo(source);
				return res;
			} catch ( Exception e ) {
				return null;
			}
		}

		public FileInfo[] newArray(int size) {
			return new FileInfo[size];
		}
	};
	
	public int describeContents() {
		return 0;
	}

	private static final int FORMAT_VERSION = 1;
	private FileInfo(Parcel source) throws Exception
	{
		if (source.readInt()!=FORMAT_VERSION)
			throw new Exception("Invalid FileInfo format");
		title = source.readString();
		authors = source.readString();
		series = source.readString();
		seriesNumber = source.readInt();
		path = source.readString();
		filename = source.readString();
		pathname = source.readString();
		arcname = source.readString();
		format = (DocumentFormat)source.readSerializable();
		size = source.readInt();
		isArchive = source.readInt()!=0;
		isDirectory = source.readInt()!=0;
	}
	
	public void writeToParcel(Parcel dest, int flags) {
		dest.writeInt(FORMAT_VERSION);
		dest.writeString(title);
		dest.writeString(authors);
		dest.writeString(series);
		dest.writeInt(seriesNumber);
		dest.writeString(path);
		dest.writeString(filename);
		dest.writeString(pathname);
		dest.writeString(arcname);
		dest.writeSerializable(format);
		dest.writeInt(size);
		dest.writeInt(isArchive?1:0);
		dest.writeInt(isDirectory?1:0);
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
		isArchive = v.isArchive;
		isDirectory = v.isDirectory;
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
	public void clear()
	{
		dirs = null;
		files = null;
	}
}
