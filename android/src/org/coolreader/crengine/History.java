package org.coolreader.crengine;

import java.util.ArrayList;

import android.os.Parcel;
import android.os.Parcelable;

public class History implements Parcelable {
	private ArrayList<BookInfo> books = new ArrayList<BookInfo>();
	
	public History()
	{
	}
	
	public BookInfo getLastBook()
	{
		if ( books.size()==0 )
			return null;
		return books.get(0);
	}

	public BookInfo getOrCreateBookInfo( FileInfo file )
	{
		BookInfo res = getBookInfo(file);
		if ( res==null ) {
			res = new BookInfo( file );
			books.add(0, res);
		}
		return res;
	}
	
	public BookInfo getBookInfo( FileInfo file )
	{
		int index = findBookInfo( file );
		if ( index>=0 )
			return books.get(index);
		return null;
	}

	public BookInfo getBookInfo( String pathname )
	{
		int index = findBookInfo( pathname );
		if ( index>=0 )
			return books.get(index);
		return null;
	}
	
	public void removeBookInfo( BookInfo bookInfo )
	{
		int index = findBookInfo(bookInfo.getFileInfo());
		if ( index>=0 )
			books.remove(index);
	}
	
	public void updateBookAccess( BookInfo bookInfo )
	{
		int index = findBookInfo(bookInfo.getFileInfo());
		if ( index>=0 ) {
			BookInfo info = books.get(index);
			if ( index>0 ) {
				books.remove(index);
				books.add(0, info);
			}
			info.updateAccess();
		}
	}
	
	public int findBookInfo( String pathname )
	{
		for ( int i=0; i<books.size(); i++ )
			if ( pathname.equals(books.get(i).getFileInfo().getPathName()) )
				return i;
		return -1;
	}
	
	public int findBookInfo( FileInfo file )
	{
		return findBookInfo( file.getPathName() );
	}
	
	public final static Parcelable.Creator<History> CREATOR = new Parcelable.Creator<History>() {

		public History createFromParcel(Parcel source) {
			try {
				History res = new History(source);
				return res;
			} catch ( Exception e ) {
				return null;
			}
		}

		public History[] newArray(int size) {
			return new History[size];
		}
	};
	
	public int describeContents() {
		return 0;
	}

	private static final int FORMAT_VERSION = 1;
	private History(Parcel source) throws Exception
	{
		if (source.readInt()!=FORMAT_VERSION)
			throw new Exception("Invalid FileInfo format");
		int count = source.readInt();
		for ( int i=0; i<count; i++ ) {
			books.add(BookInfo.CREATOR.createFromParcel(source));
		}
	}
	
	public void writeToParcel(Parcel dest, int flags) {
		int count = books.size();
		dest.writeInt(count);
		for ( int i=0; i<count; i++ ) {
			dest.writeParcelable(books.get(i), 0);
		}
	}
}
