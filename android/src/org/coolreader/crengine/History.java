package org.coolreader.crengine;

import java.io.ByteArrayInputStream;
import java.lang.reflect.Method;
import java.util.ArrayList;

import org.coolreader.CoolReader;

import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.util.Log;

public class History {
	private ArrayList<BookInfo> mBooks = new ArrayList<BookInfo>();
	private final CRDB mDB;
	private final CoolReader mCoolReader;
	private FileInfo mRecentBooksFolder;
	
	public History(CoolReader cr, CRDB db)
	{
		this.mCoolReader = cr;
		this.mDB = db;
	}
	
	public BookInfo getLastBook()
	{
		if ( mBooks.size()==0 )
			return null;
		return mBooks.get(0);
	}

	public BookInfo getPreviousBook()
	{
		if ( mBooks.size()<2 )
			return null;
		return mBooks.get(1);
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
		BitmapDrawable drawable = null;
	}
	class ImageDataCache {
		private final int maxSize;
		private int dataSize = 0;
		private int maxCount = 15;
		private ArrayList<ImageData> list = new ArrayList<ImageData>();
		public ImageDataCache( int maxSize, int maxCount ) {
			this.maxSize = maxSize;
			this.maxCount = maxCount;
		}
		synchronized public void clear() {
			list.clear();
		}
		synchronized public byte[] get( long bookId ) {
			for ( int i=0; i<list.size(); i++ )
				if ( list.get(i).bookId==bookId )
					return list.get(i).data;
			return null;
		}
		synchronized public void put( long bookId, byte[] data ) {
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
				if ( dataSize>maxSize || list.size()>maxCount ) {
					ImageData item = list.remove(i);
					dataSize -= item.data.length;
				} else
					break;
			}
		}
		synchronized public BitmapDrawable getImage( long bookId )
		{
			ImageData item = null;
			for ( int i=0; i<list.size(); i++ )
				if ( list.get(i).bookId==bookId ) {
					item = list.get(i);
					break;
				}
			if ( item==null )
				return null;
			byte[] data = get(bookId);
			if ( data==null || data.length==0 )
				return null;
			if ( item.drawable!=null )
				return item.drawable;
			// decode & resize
			BitmapDrawable res = decodeCoverPage( data );
			if ( res!=null ) {
	    		item.drawable = res;
			} else {
	    		item.data = new byte[] {};
			}
			return res;
		}
		synchronized void invalidateImages()
		{
			for ( int i=0; i<list.size(); i++ )
				list.get(i).drawable = null;
		}
	}
	
	private static Method bitmapSetDensityMethod;
	private static Method canvasSetDensityMethod;
	private static boolean isNewApiChecked;
	public BitmapDrawable decodeCoverPage( byte[] data )
	{
		try {
			ByteArrayInputStream is = new ByteArrayInputStream(data);
			Bitmap srcbmp = BitmapFactory.decodeStream(is);
			//BitmapDrawable drawable = new BitmapDrawable(mCoolReader.getResources(), is);
			//BitmapDrawable drawable = new BitmapDrawable(null, is);
			
			if ( !isNewApiChecked ) {
				isNewApiChecked = true;
				try {
					bitmapSetDensityMethod = Bitmap.class.getMethod("setDensity", new Class[] {int.class});
					canvasSetDensityMethod = Canvas.class.getMethod("setDensity", new Class[] {int.class});
				} catch ( Exception e ) {
					L.w("No Bitmap.setDensity() method found");
				}
			}
			
			Bitmap bmp = Bitmap.createBitmap(coverPageWidth, coverPageHeight, Bitmap.Config.ARGB_8888);
			if ( bitmapSetDensityMethod!=null )
				bitmapSetDensityMethod.invoke(bmp, Bitmap.DENSITY_NONE);
			//bmp.setDensity(Bitmap.DENSITY_NONE); // mCoolReader.getResources().getDisplayMetrics().densityDpi
			Canvas canvas = new Canvas(bmp);
			if ( canvasSetDensityMethod!=null )
				canvasSetDensityMethod.invoke(canvas, Bitmap.DENSITY_NONE);
			//canvas.setDensity(Bitmap.DENSITY_NONE); // mCoolReader.getResources().getDisplayMetrics().densityDpi
			canvas.drawBitmap(srcbmp, new Rect(0, 0, srcbmp.getWidth(), srcbmp.getHeight()),
					new Rect(0, 0, coverPageWidth, coverPageHeight), null);
    		Log.d("cr3", "cover page format: " + srcbmp.getWidth() + "x" + srcbmp.getHeight());
    		BitmapDrawable res = new BitmapDrawable(bmp);

    		
//			BitmapDrawable drawable = new BitmapDrawable(mCoolReader.getResources(), is);
//			//BitmapDrawable drawable = new BitmapDrawable(null, is);
//			Bitmap bmp = Bitmap.createBitmap(coverPageWidth, coverPageHeight, Bitmap.Config.ARGB_8888);
//			bmp.setDensity(mCoolReader.getResources().getDisplayMetrics().densityDpi); //Bitmap.DENSITY_NONE
//			Canvas canvas = new Canvas(bmp);
//			canvas.setDensity(mCoolReader.getResources().getDisplayMetrics().densityDpi); //Bitmap.DENSITY_NONE
//			canvas.drawBitmap(drawable.getBitmap(), new Rect(0, 0, drawable.getIntrinsicWidth(), drawable.getIntrinsicHeight()),
//					new Rect(0, 0, coverPageWidth, coverPageHeight), null);
//    		Log.d("cr3", "cover page format: " + drawable.getIntrinsicWidth() + "x" + drawable.getIntrinsicHeight());
//    		BitmapDrawable res = new BitmapDrawable(bmp);
    		
    		
    		return res;
		} catch ( Exception e ) {
    		Log.e("cr3", "exception while decoding coverpage " + e.getMessage());
    		return null;
		}
	}
	
	private boolean coverPagesEnabled = true;
	public boolean getCoverPagesEnabled()
	{
		return coverPagesEnabled;
	}
	
	public void setCoverPagesEnabled(boolean coverPagesEnabled)
	{
		this.coverPagesEnabled = coverPagesEnabled;
		if ( !coverPagesEnabled ) {
			coverPageCache.clear();
		}
	}
	
	public final static int COVERPAGE_IMAGE_CACHE_DATA_SIZE = 500000;
	public final static int COVERPAGE_IMAGE_CACHE_MAX_COUNT = 20;
	private ImageDataCache coverPageCache = new ImageDataCache(COVERPAGE_IMAGE_CACHE_DATA_SIZE, COVERPAGE_IMAGE_CACHE_MAX_COUNT);
	private int coverPageWidth = 60;
	private int coverPageHeight = 80;
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
	public void updateCoverPageSize( int screenDX, int screenDY )
	{
		int min = screenDX<screenDY ? screenDX : screenDY;
		coverPageHeight = min / 4;
		coverPageWidth = coverPageHeight * 3 / 4;
		coverPageCache.invalidateImages();
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
		byte[] data = getBookCoverpageData(bookId);
		if ( data==null )
			return null;
		return coverPageCache.getImage( bookId );
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
