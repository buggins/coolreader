package org.coolreader.crengine;

import java.io.ByteArrayInputStream;
import java.util.ArrayList;

import android.content.res.Resources;
import android.graphics.drawable.BitmapDrawable;
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
	
	public void removeBookInfo( FileInfo fileInfo, boolean removeRecentAccessFromDB, boolean removeBookFromDB )
	{
		int index = findBookInfo(fileInfo);
		if ( index>=0 )
			mBooks.remove(index);
		if ( mDB.findByPathname(fileInfo) ) {
			if ( removeBookFromDB )
				mDB.deleteBook(fileInfo);
			else if ( removeRecentAccessFromDB )
				mDB.deleteRecentPosition(fileInfo);
		}
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
		Log.v("cr3", "History.updateRecentDir()");
		if ( mRecentBooksFolder!=null ) { 
			mRecentBooksFolder.clear();
			for ( BookInfo book : mBooks )
				mRecentBooksFolder.addFile(book.getFileInfo());
		} else {
			Log.v("cr3", "History.updateRecentDir() : mRecentBooksFolder is null");
		}
	}
	static class ImageData {
		long bookId;
		byte[] data;
	}
	static class ImageDataCache {
		private final int maxSize;
		private int dataSize = 0;
		private ArrayList<ImageData> list = new ArrayList<ImageData>();
		public ImageDataCache( int maxSize ) {
			this.maxSize = maxSize;
		}
		public byte[] get( long bookId ) {
			for ( int i=0; i<list.size(); i++ )
				if ( list.get(i).bookId==bookId )
					return list.get(i).data;
			return null;
		}
		public void put( long bookId, byte[] data ) {
			boolean found = false;
			for ( int i=0; i<list.size(); i++ )
				if ( list.get(i).bookId==bookId ) {
					dataSize -= list.get(i).data.length;  
					dataSize += data.length;  
					list.get(i).data = data;
					if ( i>0 ) {
						ImageData item = list.remove(i);
						list.add(0, item);
					}
					found = true;
					break;
				}
			if ( !found ) {
				ImageData item = new ImageData();
				item.bookId = bookId;
				item.data = data;
				list.add(0, item);
				dataSize += data.length;
			}
			for ( int i=list.size()-1; i>0; i-- ) {
				if ( dataSize>maxSize ) {
					ImageData item = list.remove(i);
					dataSize -= item.data.length;
				} else
					break;
			}
		}
	}
	public final static int COVERPAGE_IMAGE_CACHE_SIZE = 500000;
	ImageDataCache coverPageCache = new ImageDataCache(COVERPAGE_IMAGE_CACHE_SIZE);
	public void setBookCoverpageData(long bookId, byte[] coverpageData )
	{
		if ( bookId==0 )
			return;
		byte[] oldData = coverPageCache.get(bookId);
		if ( coverpageData==null )
			coverpageData = new byte[] {};
		if ( oldData==null || oldData.length!=coverpageData.length ) { 
			coverPageCache.put(bookId, coverpageData);
			mDB.saveBookCoverpage(bookId, coverpageData);
		}
	}
	public byte[] getBookCoverpageData(long bookId)
	{
		if ( bookId==0 )
			return null;
		byte[] data = coverPageCache.get(bookId);
		if ( data==null ) {
			data = mDB.loadBookCoverpage(bookId);
			if ( data==null )
				data = new byte[] {};
			coverPageCache.put(bookId, data);
		}
		return data.length>0 ? data : null;
	}
	public BitmapDrawable getBookCoverpageImage(Resources resources, long bookId)
	{
		// TODO: caching 
		byte[] data = getBookCoverpageData(bookId);
		if ( data==null )
			return null;
		try {
			ByteArrayInputStream is = new ByteArrayInputStream(data);
			BitmapDrawable drawable = new BitmapDrawable(resources, is);
    		Log.d("cr3", "cover page format: " + drawable.getIntrinsicWidth() + "x" + drawable.getIntrinsicHeight());
    		return drawable;
		} catch ( Exception e ) {
    		Log.e("cr3", "exception while decoding coverpage " + e.getMessage());
    		return null;
		}
	}
	public boolean loadFromDB( Scanner scanner, int maxItems )
	{
		Log.v("cr3", "History.loadFromDB()");
		mBooks = mDB.loadRecentBooks(scanner.mFileList, maxItems);
		mRecentBooksFolder = scanner.mRoot.getDir(0);
		if ( mRecentBooksFolder==null )
			Log.v("cr3", "History.loadFromDB() : mRecentBooksFolder is null");
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
