package org.coolreader.crengine;

import java.util.ArrayList;

import android.util.Log;

public class History {
	private ArrayList<BookInfo> mBooks = new ArrayList<BookInfo>();
	private final CRDB mDB;
	private FileInfo mRecentBooksFolder;
	
	public History(CRDB db)
	{
		this.mDB = db;
	}
	
	public BookInfo getLastBook()
	{
		if ( mBooks.size()==0 )
			return null;
		return mBooks.get(0);
	}

	public BookInfo getOrCreateBookInfo( FileInfo file )
	{
		BookInfo res = getBookInfo(file);
		if ( res==null ) {
			res = new BookInfo( file );
			mBooks.add(0, res);
		}
		return res;
	}
	
	public BookInfo getBookInfo( FileInfo file )
	{
		int index = findBookInfo( file );
		if ( index>=0 )
			return mBooks.get(index);
		return null;
	}

	public BookInfo getBookInfo( String pathname )
	{
		int index = findBookInfo( pathname );
		if ( index>=0 )
			return mBooks.get(index);
		return null;
	}
	
	public void removeBookInfo( BookInfo bookInfo )
	{
		int index = findBookInfo(bookInfo.getFileInfo());
		if ( index>=0 )
			mBooks.remove(index);
	}
	
	public void updateBookAccess( BookInfo bookInfo )
	{
		Log.v("cr3", "History.updateBookAccess() for " + bookInfo.getFileInfo().getPathName());
		int index = findBookInfo(bookInfo.getFileInfo());
		if ( index>=0 ) {
			BookInfo info = mBooks.get(index);
			if ( index>0 ) {
				mBooks.remove(index);
				mBooks.add(0, info);
			}
			info.updateAccess();
			updateRecentDir();
		}
	}
	
	public int findBookInfo( String pathname )
	{
		for ( int i=0; i<mBooks.size(); i++ )
			if ( pathname.equals(mBooks.get(i).getFileInfo().getPathName()) )
				return i;
		return -1;
	}
	
	public int findBookInfo( FileInfo file )
	{
		return findBookInfo( file.getPathName() );
	}
	
	public Bookmark getLastPos( FileInfo file )
	{
		int index = findBookInfo(file);
		if ( index<0 )
			return null;
		return mBooks.get(index).getLastPosition();
	}
	protected void updateRecentDir()
	{
		mRecentBooksFolder.clear();
		for ( BookInfo book : mBooks )
			mRecentBooksFolder.addFile(book.getFileInfo());
	}
	public boolean loadFromDB( Scanner scanner, int maxItems )
	{
		mBooks = mDB.loadRecentBooks(scanner.mFileList, maxItems);
		mRecentBooksFolder = scanner.mRoot.getDir(0);
		updateRecentDir();
		return true;
	}

	public boolean saveToDB( )
	{
		Log.v("cr3", "History.saveToDB()");
		try {
			for ( BookInfo book : mBooks )
				mDB.save(book);
			return true;
		} catch ( Exception e ) {
			Log.e("cr3", "error while saving file history " + e.getMessage(), e);
			return false;
		}
	}

}
