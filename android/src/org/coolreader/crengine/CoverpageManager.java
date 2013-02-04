package org.coolreader.crengine;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;

import org.coolreader.db.CRDBService;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;

public class CoverpageManager {

	public static final Logger log = L.create("cp");
	
	public static class ImageItem {
		public FileInfo file;
		public int maxWidth;
		public int maxHeight;
		public ImageItem(FileInfo file, int maxWidth, int maxHeight) {
			this.file = file;
			this.maxWidth = maxWidth;
			this.maxHeight = maxHeight;
		}
		public boolean fileMatches(ImageItem item) {
			return file.pathNameEquals(item.file);
		}
		public boolean sizeMatches(ImageItem item) {
			return (maxWidth == item.maxWidth && maxHeight == item.maxHeight)
					|| (item.maxHeight <= -1 && item.maxWidth <= -1)
					|| (maxHeight <= -1 && maxWidth <= -1);
		}
		public boolean matches(ImageItem item) {
			return fileMatches(item) && sizeMatches(item);
		}
		@Override
		public String toString() {
			return "[" + file + " " + maxWidth
					+ "x" + maxHeight + "]";
		}
		
	}
	
	/**
	 * Callback on coverpage decoding finish.
	 */
	public interface CoverpageReadyListener {
		void onCoverpagesReady(ArrayList<ImageItem> file);
	}

	public interface CoverpageBitmapReadyListener {
		void onCoverpageReady(ImageItem file, Bitmap bitmap);
	}

	/**
	 * Cancel queued tasks for specified files.
	 */
	public void unqueue(Collection<ImageItem> filesToUnqueue) {
		synchronized(LOCK) {
			for (ImageItem file : filesToUnqueue) {
				mCheckFileCacheQueue.remove(file);
				mScanFileQueue.remove(file);
				mReadyQueue.remove(file);
				mCache.unqueue(file);
			}
		}
	}
	
	/**
	 * Set listener for cover page load completion.
	 */
	public void addCoverpageReadyListener(CoverpageReadyListener listener) {
		this.listeners.add(listener);
	}
	
	/**
	 * Set listener for cover page load completion.
	 */
	public void removeCoverpageReadyListener(CoverpageReadyListener listener) {
		this.listeners.remove(listener);
	}
	
	public boolean setCoverpageSize(int width, int height) {
		synchronized(LOCK) {
			if (maxWidth == width && maxHeight == height)
				return false;
			//clear();
			maxWidth = width;
			maxHeight = height;
			return true;
		}
	}
	
	public boolean setFontFace(String face) {
		synchronized(LOCK) {
			clear();
			if (fontFace.equals(face))
				return false;
			fontFace = face;
			return true;
		}
	}
	
	public void setCoverpageData(final CRDBService.LocalBinder db, FileInfo fileInfo, byte[] data) {
		synchronized(LOCK) {
			ImageItem item = new ImageItem(fileInfo, -1, -1);
			unqueue(Collections.singleton(item));
			mCache.remove(item);
			db.saveBookCoverpage(item.file, data);
			coverpageLoaded(item, data);
		}
	}
	
	public void clear() {
		log.d("CoverpageManager.clear()");
		synchronized(LOCK) {
			mCache.clear();
			mCheckFileCacheQueue.clear();
			mScanFileQueue.clear();
			mReadyQueue.clear();
		}
	}
	
	/**
	 * Constructor.
	 * @param activity is CoolReader main activity.
	 */
	public CoverpageManager () {
	}
	
	/**
	 * Returns coverpage drawable for book.
	 * Internally it will load coverpage in background.
	 * @param book is file to get coverpage for.
	 * @return Drawable which can be used to draw coverpage.
	 */
	public Drawable getCoverpageDrawableFor(final CRDBService.LocalBinder db, FileInfo book) {
		return new CoverImage(db, new ImageItem(new FileInfo(book), maxWidth, maxHeight));
	}
	
	/**
	 * Returns coverpage drawable for book.
	 * Internally it will load coverpage in background.
	 * @param book is file to get coverpage for.
	 * @param maxWidth is width in pixel of destination image size.
	 * @param maxHeight is height in pixel of destination image size.
	 * @return Drawable which can be used to draw coverpage.
	 */
	public Drawable getCoverpageDrawableFor(final CRDBService.LocalBinder db, FileInfo book, int maxWidth, int maxHeight) {
		return new CoverImage(db, new ImageItem(new FileInfo(book), maxWidth, maxHeight));
	}
	
	private int maxWidth = 110;
	private int maxHeight = 140;
	private String fontFace = "Droid Sans";

	private enum State {
		UNINITIALIZED,
		LOAD_SCHEDULED,
		FILE_CACHE_LOOKUP,
		IMAGE_DRAW_SCHEDULED,
		DRAWING,
		READY,
	}
	
	// hack for heap size limit
	private static final VMRuntimeHack runtime = new VMRuntimeHack();

	private class BitmapCacheItem {
		private final ImageItem file;
		private Bitmap bitmap;
		private State state = State.UNINITIALIZED;
		public BitmapCacheItem(ImageItem file) {
			this.file = file;
		}
		private boolean canUnqueue() {
			switch (state) {
			case FILE_CACHE_LOOKUP:
			case LOAD_SCHEDULED:
			case UNINITIALIZED:
				return true;
			default:
				return false;
			}
		}
		private void setBitmap(Bitmap bmp) {
			if (bitmap != null)
				removed();
			bitmap = bmp;
			if (bitmap != null) {
				int bytes = bitmap.getRowBytes() * bitmap.getHeight();
				runtime.trackFree(bytes); // hack for heap size limit
			}
		}
		private void removed() {
			if (bitmap != null) {
				int bytes = bitmap.getRowBytes() * bitmap.getHeight();
				runtime.trackAlloc(bytes); // hack for heap size limit
				bitmap.recycle();
				bitmap = null;
			}
		}
		@Override
		protected void finalize() throws Throwable {
			// don't forget to free resource
			removed();
			super.finalize();
		}
		
	}

	private class BitmapCache {
		public BitmapCache(int maxSize) {
			this.maxSize = maxSize;
		}
		private ArrayList<BitmapCacheItem> list = new ArrayList<BitmapCacheItem>();
		private int maxSize;
		private int find(ImageItem file) {
			for (int i = 0; i < list.size(); i++) {
				BitmapCacheItem item = list.get(i); 
				if (item.file.matches(file))
					return i;
			}
			return -1;
		}
		private void moveOnTop(int index) {
			if (index >= list.size() - 1)
				return;
			BitmapCacheItem item = list.get(index);
			list.remove(index);
			list.add(item);
		}
		private void checkMaxSize() {
			int itemsToRemove = list.size() - maxSize;
			for (int i = itemsToRemove - 1; i >= 0; i--) {
				BitmapCacheItem item = list.get(i);
				list.remove(i);
				item.removed();
			}
		}
		public void clear() {
			for (BitmapCacheItem item : list) {
				if (item.bitmap != null)
					item.removed();
			}
			list.clear();
		}
		public BitmapCacheItem getItem(ImageItem file) {
			int index = find(file);
			if (index < 0)
				return null;
			BitmapCacheItem item = list.get(index);
			moveOnTop(index);
			return item;
		}
		public BitmapCacheItem addItem(ImageItem file) {
			BitmapCacheItem item = new BitmapCacheItem(file);
			list.add(item);
			checkMaxSize();
			return item;
		}
		public void unqueue(ImageItem file) {
			int index = find(file);
			if (index < 0)
				return;
			BitmapCacheItem item = list.get(index);
			if (item.canUnqueue()) {
				list.remove(index);
				item.removed();
			}
		}
		public void remove(ImageItem file) {
			int index = find(file);
			if (index < 0)
				return;
			BitmapCacheItem item = list.get(index);
			list.remove(index);
			item.removed();
		}
		public Bitmap getBitmap(ImageItem file) {
			synchronized (LOCK) {
				BitmapCacheItem item = getItem(file);
				if (item == null || item.bitmap == null || item.bitmap.isRecycled())
					return null;
				return item.bitmap;
			}
		}
	}
	private BitmapCache mCache = new BitmapCache(32);
	
	private FileInfoQueue mCheckFileCacheQueue = new FileInfoQueue(); 
	private FileInfoQueue mScanFileQueue = new FileInfoQueue();
	private FileInfoQueue mReadyQueue = new FileInfoQueue();
	
	private static class FileInfoQueue {
		ArrayList<ImageItem> list = new ArrayList<ImageItem>();
		public int indexOf(ImageItem file) {
			for (int i = list.size() - 1; i >= 0; i--) {
				if (file.matches(list.get(i))) {
					return i;
				}
			}
			return -1;
		}
		public void remove(ImageItem file) {
			int index = indexOf(file);
			if (index >= 0)
				list.remove(index);
		}
		public void moveOnTop(ImageItem file) {
			int index = indexOf(file);
			if (index == 0)
				return;
			moveOnTop(index);
		}
		public void moveOnTop(int index) {
			ImageItem item = list.get(index);
			list.remove(index);
			list.add(0, item);
		}
		public boolean empty() {
			return list.size() == 0;
		}
		public void add(ImageItem file) {
			int index = indexOf(file);
			if (index >= 0)
				return;
			list.add(file);
		}
		public void clear() {
			list.clear();
		}
		public boolean addOnTop(ImageItem file) {
			int index = indexOf(file);
			if (index >= 0) {
				if (index > 0)
					moveOnTop(index);
				return false;
			}
			list.add(0, file);
			return true;
		}
		public ImageItem next() {
			if (list.size() == 0)
				return null;
			ImageItem item = list.get(0);
			list.remove(0);
			return item;
		}
	}
	
	private Object LOCK = new Object();

	private Runnable lastCheckCacheTask = null;
	private Runnable lastScanFileTask = null;
	private BitmapCacheItem setItemState(ImageItem file, State state) {
		synchronized(LOCK) {
			BitmapCacheItem item = mCache.getItem(file);
			if (item == null)
				item = mCache.addItem(file);
			item.state = state;
			return item;
		}
	}

	private final static int COVERPAGE_UPDATE_DELAY = DeviceInfo.EINK_SCREEN ? 1000 : 100;
	private final static int COVERPAGE_MAX_UPDATE_DELAY = DeviceInfo.EINK_SCREEN ? 3000 : 300;
	private Runnable lastReadyNotifyTask;
	private long firstReadyTimestamp;
	private void notifyBitmapIsReady(final ImageItem file) {
		synchronized(LOCK) {
			if (mReadyQueue.empty())
				firstReadyTimestamp = Utils.timeStamp();
			mReadyQueue.add(file);
		}
		Runnable task = new Runnable() {
			@Override
			public void run() {
//				if (lastReadyNotifyTask != this && Utils.timeInterval(firstReadyTimestamp) < COVERPAGE_MAX_UPDATE_DELAY) {
//					log.v("skipping update, " + Utils.timeInterval(firstReadyTimestamp));
//					return;
//				}
				ArrayList<ImageItem> list = new ArrayList<ImageItem>();
				synchronized(LOCK) {
					for (;;) {
						ImageItem f = mReadyQueue.next();
						if (f == null)
							break;
						list.add(f);
					}
					mReadyQueue.clear();
					if (list.size() > 0)
						log.v("ready coverpages: " + list.size());
				}
				if (list.size() > 0) {
					for (CoverpageReadyListener listener : listeners)
						listener.onCoverpagesReady(list);
					firstReadyTimestamp = Utils.timeStamp();
				}
			}
		};
		lastReadyNotifyTask = task;
		BackgroundThread.instance().postGUI(task, COVERPAGE_UPDATE_DELAY);
	}

	private void draw(ImageItem file, byte[] data) {
		BitmapCacheItem item = null;
		synchronized(LOCK) {
			item = mCache.getItem(file);
			if (item == null)
				return;
			if (item.state == State.DRAWING || item.state == State.READY)
				return;
			item.state = State.DRAWING;
		}
		Bitmap bmp = drawCoverpage(data, file);
		if (bmp != null) {
			// successfully decoded
			log.v("coverpage is decoded for " + file);
			item.setBitmap(bmp);
			item.state = State.READY;
			notifyBitmapIsReady(file);
		}
	}

	private void coverpageLoaded(final ImageItem file, final byte[] data) {
		log.v("coverpage data is loaded for " + file);
		setItemState(file, State.IMAGE_DRAW_SCHEDULED);
		BackgroundThread.instance().postBackground(new Runnable() {
			@Override
			public void run() {
				draw(file, data);
			}
		});
	}
	private void scheduleCheckCache(final CRDBService.LocalBinder db) {
		// cache lookup
		lastCheckCacheTask = new Runnable() {
			@Override
			public void run() {
				ImageItem file = null;
				synchronized(LOCK) {
					if (lastCheckCacheTask == this) {
						file = mCheckFileCacheQueue.next();
					}
				}
				if (file != null) {
					final ImageItem request = file;
					db.loadBookCoverpage(file.file, new CRDBService.CoverpageLoadingCallback() {
						@Override
						public void onCoverpageLoaded(FileInfo fileInfo, byte[] data) {
							if (data == null) {
								log.v("cover not found in DB for " + fileInfo + ", scheduling scan");
								mScanFileQueue.addOnTop(request);
								scheduleScanFile(db);
							} else {
								coverpageLoaded(request, data);
							}
						}
					});
					scheduleCheckCache(db);
				}
			}
		};
		BackgroundThread.instance().postGUI(lastCheckCacheTask);
	}
	private void scheduleScanFile(final CRDBService.LocalBinder db) {
		// file scan
		lastScanFileTask = new Runnable() {
			@Override
			public void run() {
				ImageItem file = null;
				synchronized(LOCK) {
					if (lastScanFileTask == this) {
						file = mScanFileQueue.next();
					}
				}
				if (file != null) {
					final ImageItem fileInfo = file;
					if (fileInfo.file.format.canParseCoverpages) {
						BackgroundThread.instance().postBackground(new Runnable() {
							@Override
							public void run() {
								byte[] data = Services.getEngine().scanBookCover(fileInfo.file.getPathName());
								if (data == null)
									data = new byte[] {};
								if (fileInfo.file.format.needCoverPageCaching())
									db.saveBookCoverpage(fileInfo.file, data);
								coverpageLoaded(fileInfo, data);
							}
						});
					} else {
						coverpageLoaded(fileInfo, new byte[] {});
					}
					scheduleScanFile(db);
				}
			}
		};
		BackgroundThread.instance().postGUI(lastScanFileTask);
	}

	private void queueForDrawing(final CRDBService.LocalBinder db, ImageItem file) {
		synchronized (LOCK) {
			BitmapCacheItem item = mCache.getItem(file);
			if (item != null && (item.state == State.READY || item.state == State.DRAWING))
				return;
			if (file.file.format.needCoverPageCaching()) {
				if (mCheckFileCacheQueue.addOnTop(file)) {
					log.v("Scheduled coverpage DB lookup for " + file);
					scheduleCheckCache(db);
				}
			} else {
				if (mScanFileQueue.addOnTop(file)) {
					log.v("Scheduled coverpage filescan for " + file);
					scheduleScanFile(db);
				}
			}
		}
	}

	public static abstract class CoverImageBase extends Drawable {
		protected ImageItem book;
		public CoverImageBase(ImageItem book) {
			this.book = book;
		}
	}
	private class CoverImage extends CoverImageBase {
		
		Paint defPaint;
		final CRDBService.LocalBinder db;
		final static int alphaLevels = 16;
		final static int shadowSizePercent = 6;
		final static int minAlpha = 40;
		final static int maxAlpha = 180;
		final Paint[] shadowPaints = new Paint[alphaLevels + 1];
		
		public CoverImage(final CRDBService.LocalBinder db, ImageItem book) {
			super(book);
			this.db = db;
			defPaint = new Paint();
			defPaint.setColor(0xFF000000);
			defPaint.setFilterBitmap(true);
			for (int i=0; i <= alphaLevels; i++) {
				int alpha = (maxAlpha - minAlpha) * i / alphaLevels + minAlpha;
				shadowPaints[i] = new Paint();
				shadowPaints[i].setColor((alpha << 24) | 0x101010);
			}
		}

		public void drawShadow(Canvas canvas, Rect bookRect, Rect shadowRect) {
			int d = shadowRect.bottom - bookRect.bottom;
			if (d <= 0)
				return;
			Rect l = new Rect(shadowRect);
			Rect r = new Rect(shadowRect);
			Rect t = new Rect(shadowRect);
			Rect b = new Rect(shadowRect);
			for (int i = 0; i < d; i++) {
				shadowRect.left++;
				shadowRect.right--;
				shadowRect.top++;
				shadowRect.bottom--;
				if (shadowRect.bottom < bookRect.bottom || shadowRect.right < bookRect.right)
					break;
				l.set(shadowRect);
				l.top = bookRect.bottom;
				l.right = l.left + 1;
				t.set(shadowRect);
				t.left = bookRect.right;
				t.right--;
				t.bottom = t.top + 1;
				r.set(shadowRect);
				r.left = r.right - 1;
				b.set(shadowRect);
				b.top = b.bottom - 1;
				b.left++;
				b.right--;
				int index = i * alphaLevels / d;
				Paint paint = shadowPaints[index];
				if (!l.isEmpty())
					canvas.drawRect(l, paint);
				if (!r.isEmpty())
					canvas.drawRect(r, paint);
				if (!t.isEmpty())
					canvas.drawRect(t, paint);
				if (!b.isEmpty())
					canvas.drawRect(b, paint);
			}
		}
		boolean checkShadowSize(int bookSize, int shadowSize) {
			if (bookSize < 10)
				return false;
			int p = 100 * shadowSize / bookSize;
			if (p >= 0 && p >= shadowSizePercent - 2 && p <= shadowSizePercent + 2)
				return true;
			return false;
		}
		@Override
		public void draw(Canvas canvas) {
			try {
				Rect fullrc = getBounds();
				if (fullrc.width() < 5 || fullrc.height() < 5)
					return;
				int w = book.maxWidth;
				int h = book.maxHeight;
				int shadowW = fullrc.width() - w;
				int shadowH = fullrc.height() - h;
				if (!checkShadowSize(w, shadowW) || !checkShadowSize(h, shadowH)) {
					w = fullrc.width() * 100 / (100 + shadowSizePercent);
					h = fullrc.height() * 100 / (100 + shadowSizePercent);
					shadowW = fullrc.width() - w;
					shadowH = fullrc.height() - h;
				}
				Rect rc = new Rect(fullrc.left, fullrc.top, fullrc.right - shadowW, fullrc.bottom - shadowH);
				synchronized (mCache) {
					Bitmap bitmap = mCache.getBitmap(book);
					if (bitmap != null) {
						log.d("Image for " + book + " is found in cache, drawing...");
						Rect dst = getBestCoverSize(rc, bitmap.getWidth(), bitmap.getHeight());
						canvas.drawBitmap(bitmap, null, dst, defPaint);
						if (shadowSizePercent > 0) {
							Rect shadowRect = new Rect(rc.left + shadowW, rc.top + shadowH, rc.right + shadowW, rc.bottom + shadowW);
							drawShadow(canvas, rc, shadowRect);
						}
						return;
					}
				}
				log.d("Image for " + book + " is not found in cache, scheduling generation...");
				queueForDrawing(db, book);
				//if (h * bestWidth / bestHeight > w)
				//canvas.drawRect(rc, defPaint);
			} catch (Exception e) {
				log.e("exception in draw", e);
			}
		}
		
		@Override
		public int getIntrinsicHeight() {
			return book.maxHeight * (100 + shadowSizePercent) / 100;
		}

		@Override
		public int getIntrinsicWidth() {
			return book.maxWidth * (100 + shadowSizePercent) / 100;
		}

		@Override
		public int getOpacity() {
			return PixelFormat.TRANSPARENT; // part of pixels are transparent
		}

		@Override
		public void setAlpha(int alpha) {
			// ignore, not supported
		}

		@Override
		public void setColorFilter(ColorFilter cf) {
			// ignore, not supported
		}
	}

	public void drawCoverpageFor(final CRDBService.LocalBinder db, final FileInfo file, final Bitmap buffer, final CoverpageBitmapReadyListener callback) {
		db.loadBookCoverpage(file, new CRDBService.CoverpageLoadingCallback() {
			@Override
			public void onCoverpageLoaded(FileInfo fileInfo, final byte[] data) {
				BackgroundThread.instance().postBackground(new Runnable() {
					@Override
					public void run() {
						byte[] imageData = data;
						if (data == null && file.format.canParseCoverpages) {
							imageData = Services.getEngine().scanBookCover(file.getPathName());
							if (imageData == null)
								imageData = new byte[] {};
							if (file.format.needCoverPageCaching())
								db.saveBookCoverpage(file, imageData);
						}
						Services.getEngine().drawBookCover(buffer, imageData, fontFace, file.getTitleOrFileName(), file.authors, file.series, file.seriesNumber, DeviceInfo.EINK_SCREEN ? 4 : 16);
						BackgroundThread.instance().postGUI(new Runnable() {
							@Override
							public void run() {
								ImageItem item = new ImageItem(file, buffer.getWidth(), buffer.getHeight());
								callback.onCoverpageReady(item, buffer);
							}
						});
					}
				});
			}
		});
	}
	
	private Rect getBestCoverSize(Rect dst, int srcWidth, int srcHeight) {
		int w = dst.width();
		int h = dst.height();
		if (srcWidth < 20 || srcHeight < 20) {
			return dst;
		}
		int sw = srcHeight * w / h;
		int sh = srcWidth * h / w;
		if (sw <= w)
			sh = h;
		else
			sw = w;
		int dx = (w - sw) / 2;
		int dy = (h - sh) / 2;
		return new Rect(dst.left + dx, dst.top + dy, dst.left + sw + dx, dst.top + sh + dy); 
	}
	
	private Bitmap drawCoverpage(byte[] data, ImageItem file)
	{
		try {
			Bitmap bmp = Bitmap.createBitmap(file.maxWidth, file.maxHeight, DeviceInfo.BUFFER_COLOR_FORMAT);
			Services.getEngine().drawBookCover(bmp, data, fontFace, file.file.getTitleOrFileName(), file.file.authors, file.file.series, file.file.seriesNumber, DeviceInfo.EINK_SCREEN ? 4 : 16);
			return bmp;
		} catch ( Exception e ) {
    		Log.e("cr3", "exception while decoding coverpage " + e.getMessage());
    		return null;
		}
	}

	private ArrayList<CoverpageReadyListener> listeners = new ArrayList<CoverpageReadyListener>();


	public static void invalidateChildImages(View view, ArrayList<CoverpageManager.ImageItem> files) {
		if (view instanceof ViewGroup) {
			ViewGroup vg = (ViewGroup)view;
			for (int i=0; i<vg.getChildCount(); i++) {
				invalidateChildImages(vg.getChildAt(i), files);
			}
		} else if (view instanceof ImageView) {
			if (view.getTag() instanceof CoverpageManager.ImageItem) {
				CoverpageManager.ImageItem item = (CoverpageManager.ImageItem)view.getTag();
				for (CoverpageManager.ImageItem v : files)
					if (v.matches(item)) {
						log.v("invalidating view for " + item);
						view.invalidate();
					}
			}
			
		}
	}

}
