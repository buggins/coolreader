package org.coolreader.crengine;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;

import org.coolreader.CoolReader;
import org.coolreader.db.CRDBService;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.util.Log;

public class CoverpageManager {

	public static final Logger log = L.create("cp");
	
	/**
	 * Callback on coverpage decoding finish.
	 */
	public interface CoverpageReadyListener {
		void onCoverpagesReady(ArrayList<FileInfo> file);
	}

	/**
	 * Cancel queued tasks for specified files.
	 */
	public void unqueue(Collection<FileInfo> filesToUnqueue) {
		synchronized(LOCK) {
			for (FileInfo file : filesToUnqueue) {
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
	public void setCoverpageReadyListener(CoverpageReadyListener listener) {
		this.listener = listener;
	}
	
	public boolean setCoverpageSize(int width, int height) {
		synchronized(LOCK) {
			if (maxWidth == width && maxHeight == height)
				return false;
			clear();
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
	
	public void setCoverpageData(FileInfo fileInfo, byte[] data) {
		synchronized(LOCK) {
			unqueue(Collections.singleton(fileInfo));
			mCache.remove(fileInfo);
			mActivity.getDB().saveBookCoverpage(fileInfo, data);
			coverpageLoaded(fileInfo, data);
		}
	}
	
	public void clear() {
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
	public CoverpageManager (CoolReader activity) {
		this.mActivity = activity;
	}
	
	/**
	 * Returns coverpage drawable for book.
	 * Internally it will load coverpage in background.
	 * @param book is file to get coverpage for.
	 * @return Drawable which can be used to draw coverpage.
	 */
	public Drawable getCoverpageDrawableFor(FileInfo book) {
		return new CoverImage(book);
	}
	
	private CoolReader mActivity;
	
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
		private final FileInfo file;
		private Bitmap bitmap;
		private State state = State.UNINITIALIZED;
		public BitmapCacheItem(FileInfo file) {
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
		private int find(FileInfo file) {
			for (int i = 0; i < list.size(); i++) {
				if (list.get(i).file.pathNameEquals(file))
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
		public BitmapCacheItem getItem(FileInfo file) {
			int index = find(file);
			if (index < 0)
				return null;
			BitmapCacheItem item = list.get(index);
			moveOnTop(index);
			return item;
		}
		public BitmapCacheItem addItem(FileInfo file) {
			BitmapCacheItem item = new BitmapCacheItem(file);
			list.add(item);
			checkMaxSize();
			return item;
		}
		public void unqueue(FileInfo file) {
			int index = find(file);
			if (index < 0)
				return;
			BitmapCacheItem item = list.get(index);
			if (item.canUnqueue()) {
				list.remove(index);
				item.removed();
			}
		}
		public void remove(FileInfo file) {
			int index = find(file);
			if (index < 0)
				return;
			BitmapCacheItem item = list.get(index);
			list.remove(index);
			item.removed();
		}
		public Bitmap getBitmap(FileInfo file) {
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
		ArrayList<FileInfo> list = new ArrayList<FileInfo>();
		public int indexOf(FileInfo file) {
			for (int i = list.size() - 1; i >= 0; i--) {
				if (file.pathNameEquals(list.get(i))) {
					return i;
				}
			}
			return -1;
		}
		public void remove(FileInfo file) {
			int index = indexOf(file);
			if (index >= 0)
				list.remove(index);
		}
		public void moveOnTop(FileInfo file) {
			int index = indexOf(file);
			if (index == 0)
				return;
			moveOnTop(index);
		}
		public void moveOnTop(int index) {
			FileInfo item = list.get(index);
			list.remove(index);
			list.add(0, item);
		}
		public boolean empty() {
			return list.size() == 0;
		}
		public void add(FileInfo file) {
			int index = indexOf(file);
			if (index >= 0)
				return;
			list.add(file);
		}
		public void clear() {
			list.clear();
		}
		public boolean addOnTop(FileInfo file) {
			int index = indexOf(file);
			if (index >= 0) {
				moveOnTop(index);
				return false;
			}
			list.add(0, file);
			return true;
		}
		public FileInfo next() {
			if (list.size() == 0)
				return null;
			FileInfo item = list.get(0);
			list.remove(0);
			return item;
		}
	}
	
	private Object LOCK = new Object();

	private Runnable lastCheckCacheTask = null;
	private Runnable lastScanFileTask = null;
	private BitmapCacheItem setItemState(FileInfo file, State state) {
		synchronized(LOCK) {
			BitmapCacheItem item = mCache.getItem(file);
			if (item == null)
				item = mCache.addItem(file);
			item.state = state;
			return item;
		}
	}

	private final static int COVERPAGE_UPDATE_DELAY = DeviceInfo.EINK_SCREEN ? 1000 : 100;
	private final static int COVERPAGE_MAX_UPDATE_DELAY = DeviceInfo.EINK_SCREEN ? 3000 : 400;
	private Runnable lastReadyNotifyTask;
	private long firstReadyTimestamp;
	private void notifyBitmapIsReady(final FileInfo file) {
		synchronized(LOCK) {
			if (mReadyQueue.empty())
				firstReadyTimestamp = Utils.timeStamp();
			mReadyQueue.add(file);
		}
		Runnable task = new Runnable() {
			@Override
			public void run() {
				if (lastReadyNotifyTask != this && Utils.timeInterval(firstReadyTimestamp) < COVERPAGE_MAX_UPDATE_DELAY)
					return;
				ArrayList<FileInfo> list = new ArrayList<FileInfo>();
				synchronized(LOCK) {
					for (;;) {
						FileInfo f = mReadyQueue.next();
						if (f == null)
							break;
						list.add(f);
					}
					mReadyQueue.clear();
				}
				if (list.size() > 0) {
					listener.onCoverpagesReady(list);
					firstReadyTimestamp = Utils.timeStamp();
				}
			}
		};
		lastReadyNotifyTask = task;
		BackgroundThread.instance().postGUI(task, 500);
	}

	private void draw(FileInfo file, byte[] data) {
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

	private void coverpageLoaded(final FileInfo file, final byte[] data) {
		log.v("coverpage data is loaded for " + file);
		setItemState(file, State.IMAGE_DRAW_SCHEDULED);
		BackgroundThread.instance().postBackground(new Runnable() {
			@Override
			public void run() {
				draw(file, data);
			}
		});
	}
	private void scheduleCheckCache() {
		// cache lookup
		lastCheckCacheTask = new Runnable() {
			@Override
			public void run() {
				FileInfo file = null;
				synchronized(LOCK) {
					if (lastCheckCacheTask == this) {
						file = mCheckFileCacheQueue.next();
					}
				}
				if (file != null) {
					mActivity.getDB().loadBookCoverpage(file, new CRDBService.CoverpageLoadingCallback() {
						@Override
						public void onCoverpageLoaded(FileInfo fileInfo, byte[] data) {
							if (data == null) {
								log.v("cover not found in DB for " + fileInfo + ", scheduling scan");
								mScanFileQueue.addOnTop(fileInfo);
								scheduleScanFile();
							} else {
								coverpageLoaded(fileInfo, data);
							}
						}
					});
					scheduleCheckCache();
				}
			}
		};
		BackgroundThread.instance().postGUI(lastCheckCacheTask);
	}
	private void scheduleScanFile() {
		// file scan
		lastScanFileTask = new Runnable() {
			@Override
			public void run() {
				FileInfo file = null;
				synchronized(LOCK) {
					if (lastScanFileTask == this) {
						file = mScanFileQueue.next();
					}
				}
				if (file != null) {
					final FileInfo fileInfo = file;
					if (fileInfo.format.canParseCoverpages) {
						BackgroundThread.instance().postBackground(new Runnable() {
							@Override
							public void run() {
								byte[] data = mActivity.getEngine().scanBookCover(fileInfo.getPathName());
								if (data == null)
									data = new byte[] {};
								if (fileInfo.format.needCoverPageCaching())
									mActivity.getDB().saveBookCoverpage(fileInfo, data);
								coverpageLoaded(fileInfo, data);
							}
						});
					} else {
						coverpageLoaded(fileInfo, new byte[] {});
					}
					scheduleScanFile();
				}
			}
		};
		BackgroundThread.instance().postGUI(lastScanFileTask);
	}

	private void queueForDrawing(FileInfo file) {
		synchronized (LOCK) {
			BitmapCacheItem item = mCache.getItem(file);
			if (item != null && (item.state == State.READY || item.state == State.DRAWING))
				return;
			if (file.format.needCoverPageCaching()) {
				if (mCheckFileCacheQueue.addOnTop(file)) {
					log.v("Scheduled coverpage DB lookup for " + file);
					scheduleCheckCache();
				}
			} else {
				if (mScanFileQueue.addOnTop(file)) {
					log.v("Scheduled coverpage filescan for " + file);
					scheduleScanFile();
				}
			}
		}
	}

	private class CoverImage extends Drawable {
		
		FileInfo book;
		Paint defPaint;
		final static int alphaLevels = 16;
		final static int shadowSizePercent = 6;
		final static int minAlpha = 40;
		final static int maxAlpha = 180;
		final Paint[] shadowPaints = new Paint[alphaLevels + 1];
		
		public CoverImage(FileInfo book) {
			this.book = new FileInfo(book);
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
				int w = maxWidth;
				int h = maxHeight;
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
						Rect dst = getBestCoverSize(rc, bitmap.getWidth(), bitmap.getHeight());
						canvas.drawBitmap(bitmap, null, dst, defPaint);
						if (shadowSizePercent > 0) {
							Rect shadowRect = new Rect(rc.left + shadowW, rc.top + shadowH, rc.right + shadowW, rc.bottom + shadowW);
							drawShadow(canvas, rc, shadowRect);
						}
						return;
					}
				}
				queueForDrawing(book);
				//if (h * bestWidth / bestHeight > w)
				//canvas.drawRect(rc, defPaint);
			} catch (Exception e) {
				log.e("exception in draw", e);
			}
		}
		
		@Override
		public int getIntrinsicHeight() {
			return maxHeight * (100 + shadowSizePercent) / 100;
		}

		@Override
		public int getIntrinsicWidth() {
			return maxWidth * (100 + shadowSizePercent) / 100;
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

	private Rect getBestCoverSize(int srcWidth, int srcHeight) {
		if (srcWidth < 20 || srcHeight < 20) {
			return new Rect(0, 0, maxWidth, maxHeight);
		}
		int sw = srcHeight * maxWidth / maxHeight;
		int sh = srcWidth * maxHeight / maxWidth;
		if (sw <= maxWidth)
			sh = maxHeight;
		else
			sw = maxWidth;
		int dx = (maxWidth - sw) / 2;
		int dy = (maxHeight - sh) / 2;
		return new Rect(dx, dy, sw + dx, sh + dy); 
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
	
	private Bitmap drawCoverpage(byte[] data, FileInfo file)
	{
		try {
			Bitmap bmp = Bitmap.createBitmap(maxWidth, maxHeight, DeviceInfo.BUFFER_COLOR_FORMAT);
			mActivity.getEngine().drawBookCover(bmp, data, fontFace, file.getTitleOrFileName(), file.authors, file.series, file.seriesNumber, DeviceInfo.EINK_SCREEN ? 4 : 16);
			return bmp;
		} catch ( Exception e ) {
    		Log.e("cr3", "exception while decoding coverpage " + e.getMessage());
    		return null;
		}
	}

	private CoverpageReadyListener listener = null;
}
