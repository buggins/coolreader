package org.coolreader.crengine;

import java.util.ArrayList;

import android.os.Parcel;
import android.os.Parcelable;

public class BookInfo implements Parcelable {
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



	public final static Parcelable.Creator<BookInfo> CREATOR = new Parcelable.Creator<BookInfo>() {

		public BookInfo createFromParcel(Parcel source) {
			try {
				BookInfo res = new BookInfo(source);
				return res;
			} catch ( Exception e ) {
				return null;
			}
		}

		public BookInfo[] newArray(int size) {
			return new BookInfo[size];
		}
	};
	
	public int describeContents() {
		return 0;
	}

	private static final int FORMAT_VERSION = 1;
	private BookInfo(Parcel source) throws Exception
	{
		if (source.readInt()!=FORMAT_VERSION)
			throw new Exception("Invalid FileInfo format");
		int count = source.readInt();
		fileInfo = FileInfo.CREATOR.createFromParcel(source);
		if ( count!=0 )
			lastPosition = Bookmark.CREATOR.createFromParcel(source);
		for ( int i=1; i<count; i++ ) {
			bookmarks.add(Bookmark.CREATOR.createFromParcel(source));
		}
	}
	
	public void writeToParcel(Parcel dest, int flags) {
		int count = bookmarks.size();
		if ( lastPosition!=null )
			count++;
		dest.writeInt(count);
		if ( lastPosition!=null )
			dest.writeParcelable(lastPosition, 0);
		for ( int i=1; i<count; i++ ) {
			dest.writeParcelable(bookmarks.get(i), 0);
		}
	}

}
