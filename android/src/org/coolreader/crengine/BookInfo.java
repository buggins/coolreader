package org.coolreader.crengine;

import java.util.ArrayList;

import android.os.Parcel;
import android.os.Parcelable;

public class BookInfo {
	private FileInfo fileInfo;
	private Bookmark lastPosition;
	private ArrayList<Bookmark> bookmarks = new ArrayList<Bookmark>();

	public void updateAccess()
	{
		// TODO:
	}
	
	public BookInfo( FileInfo fileInfo )
	{
		this.fileInfo = new FileInfo(fileInfo);
	}
	
	public Bookmark getLastPosition()
	{
		return lastPosition;
	}
	
	public void setLastPosition( Bookmark position )
	{
		if ( lastPosition!=null )
			position.setId(lastPosition.getId());
		lastPosition = position;
		lastPosition.setModified(true);
		fileInfo.lastAccessTime = lastPosition.getTimeStamp();
		fileInfo.setModified(true);
	}
	
	public FileInfo getFileInfo()
	{
		return fileInfo;
	}
	
	public void addBookmark( Bookmark bm )
	{
		bookmarks.add(bm);
	}

	public int getBookmarkCount()
	{
		return bookmarks.size();
	}

	public Bookmark getBookmark( int index )
	{
		return bookmarks.get(index);
	}

	public void removeBookmark( int index )
	{
		bookmarks.remove(index);
	}
	
	void setBookmarks(ArrayList<Bookmark> list)
	{
		if ( list.size()>0 ) {
			if ( list.get(0).getType()==0 ) {
				lastPosition = list.remove(0); 
			}
		}
		if ( list.size()>0 ) {
			bookmarks = list;
		}
	}

}
