package org.coolreader.crengine;

import java.util.ArrayList;

import android.util.Log;

public class History {
	private ArrayList<BookInfo> books = new ArrayList<BookInfo>();
	private final CRDB db;
	
	public History(CRDB db)
	{
		this.db = db;
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
		Log.v("cr3", "History.updateBookAccess() for " + bookInfo.getFileInfo().getPathName());
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
	
	public boolean loadFromDB( ArrayList<FileInfo> fileList, int maxItems )
	{
		books = db.loadRecentBooks(fileList, maxItems);
		return true;
	}

	public boolean saveToDB( )
	{
		Log.v("cr3", "History.saveToDB()");
		try {
			for ( BookInfo book : books )
				db.save(book);
			return true;
		} catch ( Exception e ) {
			Log.e("cr3", "error while saving file history " + e.getMessage(), e);
			return false;
		}
	}

}
