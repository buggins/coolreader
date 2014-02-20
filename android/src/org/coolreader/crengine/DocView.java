package org.coolreader.crengine;


import android.graphics.Bitmap;

public class DocView {

	public static final Logger log = L.create("dv");

	public static final int SWAP_DONE = 0;
	public static final int SWAP_TIMEOUT = 1;
	public static final int SWAP_ERROR = 2;

	private Object mutex;

	public DocView(Object mutex) {
		log.i("DocView()");
		this.mutex = mutex;
	}
	
	/**
	 * Create native object.
	 */
	public void create() {
		synchronized(mutex) {
			createInternal();
		}
	}

	/**
	 * Destroy native object.
	 */
	public void destroy() {
		synchronized(mutex) {
			destroyInternal();
		}
	}

	/**
	 * Set document callback.
	 * @param readerCallback is callback to set
	 */
	public void setReaderCallback(ReaderCallback readerCallback) {
		this.readerCallback = readerCallback;
	}

	/**
	 * If document uses cache file, swap all unsaved data to it.
	 * @return either SWAP_DONE, SWAP_TIMEOUT, SWAP_ERROR
	 */
	public int swapToCache() {
		
		synchronized(mutex) {
			return swapToCacheInternal();
		}
	}

	/**
	 * Follow link.
	 * @param link
	 * @return
	 */
	public int goLink(String link) {
		synchronized(mutex) {
			return goLinkInternal(link);
		}
	}
	
	/**
	 * Find a link near to specified window coordinates.
	 * @param x
	 * @param y
	 * @param delta
	 * @return
	 */
	public String checkLink(int x, int y, int delta) {
		synchronized(mutex) {
			return checkLinkInternal(x, y, delta);
		}
	}
	
	/**
	 * Set selection range.
	 * @param sel
	 */
	public void updateSelection(Selection sel) {
		synchronized(mutex) {
			updateSelectionInternal(sel);
		}
	}

	/**
	 * Move selection.
	 * @param sel
	 * @param moveCmd
	 * @param params
	 * @return
	 */
	public boolean moveSelection(Selection sel,
			int moveCmd, int params) {
		synchronized(mutex) {
			return moveSelectionInternal(sel, moveCmd, params);
		}
	}

	/**
	 * Send battery state to native object.
	 * @param state
	 */
	public void setBatteryState(int state) {
		synchronized(mutex) {
			setBatteryStateInternal(state);
		}
	}

	/**
	 * Get current book coverpage data bytes.
	 * @return
	 */
	public byte[] getCoverPageData() {
		synchronized(mutex) {
			return getCoverPageDataInternal();
		}
	}

	/**
	 * Set texture for page background.
	 * @param imageBytes
	 * @param tileFlags
	 */
	public void setPageBackgroundTexture(
			byte[] imageBytes, int tileFlags) {
		synchronized(mutex) {
			setPageBackgroundTextureInternal(imageBytes, tileFlags);
		}
	}

	/**
	 * Load document from file.
	 * @param fileName
	 * @return
	 */
	public boolean loadDocument(String fileName) {
		synchronized(mutex) {
			return loadDocumentInternal(fileName);
		}
	}

	/**
	 * Get settings from native object.
	 * @return
	 */
	public java.util.Properties getSettings() {
		synchronized(mutex) {
			return getSettingsInternal();
		}
	}

	/**
	 * Apply settings.
	 * @param settings
	 * @return
	 */
	public boolean applySettings(java.util.Properties settings) {
		synchronized(mutex) {
			return applySettingsInternal(settings);
		}
	}

	/**
	 * Set stylesheet for document.
	 * @param stylesheet
	 */
	public void setStylesheet(String stylesheet) {
		synchronized(mutex) {
			setStylesheetInternal(stylesheet);
		}
	}
	
	public void requestRender() {
		doCommand(ReaderCommand.DCMD_REQUEST_RENDER.getNativeId(), 0);
	}

	/**
	 * Change window size.
	 * @param dx
	 * @param dy
	 */
	public void resize(int dx, int dy) {
		synchronized(mutex) {
			log.d("DocView.resize(" + dx + ", "+ dy + ")");
			resizeInternal(dx, dy);
		}
	}

	/**
	 * Execute command by native object.
	 * @param command
	 * @param param
	 * @return
	 */
	public boolean doCommand(int command, int param) {
		synchronized(mutex) {
			return doCommandInternal(command, param);
		}
	}

	/**
	 * Get current page bookmark info.
	 * @return
	 */
	public Bookmark getCurrentPageBookmark() {
		synchronized(mutex) {
			return getCurrentPageBookmarkInternal();
		}
	}
	
	/**
	 * Get current page bookmark info, returning null if document is not yet rendered (to avoid long call).
	 * @return bookmark for current page, null if cannot be determined fast
	 */
	public Bookmark getCurrentPageBookmarkNoRender() {
		if (!isRenderedInternal())
			return null;
		synchronized(mutex) {
			return getCurrentPageBookmarkInternal();
		}
	}
	
	/**
	 * Check whether document is formatted/rendered.
	 * @return true if document is rendered, and e.g. retrieving of page image will not cause long activity (formatting etc.)
	 */
	public boolean isRendered() {
		// thread safe
		return isRenderedInternal();
	}

	/**
	 * Move reading position to specified xPath.
	 * @param xPath
	 * @return
	 */
	public boolean goToPosition(String xPath, boolean saveToHistory) {
		synchronized(mutex) {
			return goToPositionInternal(xPath, saveToHistory);
		}
	}

	/**
	 * Get position properties by xPath.
	 * @param xPath
	 * @return
	 */
	public PositionProperties getPositionProps(String xPath) {
		synchronized(mutex) {
			return getPositionPropsInternal(xPath);
		}
	}

	/**
	 * Fill book info fields using metadata from current book. 
	 * @param info
	 */
	public void updateBookInfo(BookInfo info) {
		synchronized(mutex) {
			updateBookInfoInternal(info);
		}
	}

	/**
	 * Get TOC tree from current book.
	 * @return
	 */
	public TOCItem getTOC() {
		synchronized(mutex) {
			return getTOCInternal();
		}
	}

	/**
	 * Clear selection.
	 */
	public void clearSelection() {
		synchronized(mutex) {
			clearSelectionInternal();
		}
	}

	/**
	 * Find text in book.
	 * @param pattern
	 * @param origin
	 * @param reverse
	 * @param caseInsensitive
	 * @return
	 */
	public boolean findText(String pattern, int origin,
			int reverse, int caseInsensitive) {
		synchronized(mutex) {
			return findTextInternal(pattern, origin, reverse, caseInsensitive);
		}
	}

	/**
	 * Get current page image.
	 * @param bitmap is buffer to put data to.
	 */
	public void getPageImage(Bitmap bitmap) {
		synchronized(mutex) {
			getPageImageInternal(bitmap, DeviceInfo.EINK_SCREEN ? 4 : 32);
		}
	}

	private void getPageImageTexture(int[] buf, int width, int height) {
		synchronized(mutex) {
			getPageImageTextureInternal(buf, width, height, DeviceInfo.EINK_SCREEN ? 4 : 32);
		}
	}

	/**
	 * Check whether point of current document contains image.
	 * If image is found, image becomes current image to be drawn by drawImage(), dstImage fields are set to image dimension.
	 *  
	 * @param x is X coordinate in document window
	 * @param y is Y coordinate in document window
	 * @param dstImage is to place found image dimensions to
	 * @return true if point belongs to image
	 */
	public boolean checkImage(int x, int y, ImageInfo dstImage) {
		synchronized(mutex) {
			return checkImageInternal(x, y, dstImage);
		}
	}

	/**
	 * Check whether point of current document belongs to bookmark.
	 *  
	 * @param x is X coordinate in document window
	 * @param y is Y coordinate in document window
	 * @return bookmark if point belongs to bookmark, null otherwise
	 */
	public Bookmark checkBookmark(int x, int y) {
		synchronized(mutex) {
			Bookmark dstBookmark = new Bookmark();
			if (checkBookmarkInternal(x, y, dstBookmark)) {
				return dstBookmark;
			}
			return null;
		}
	}
	
	
	/**
	 * Draws currently opened image to bitmap.
	 * @param bitmap is destination bitmap
	 * @param imageInfo contains image position and scaling parameters.
	 * @return true if current image is drawn successfully.
	 */
	public boolean drawImage(Bitmap bitmap, ImageInfo imageInfo) {
		synchronized(mutex) {
			return drawImageInternal(bitmap, DeviceInfo.EINK_SCREEN ? 4 : 32, imageInfo);
		}
	}

	/**
	 * Close currently opened image, free resources.
	 * @return true if there was opened current image, and it's now closed 
	 */
	public boolean closeImage() {
		synchronized(mutex) {
			return closeImageInternal();
		}
	}
	
	/**
	 * Highlight bookmarks.
	 * Remove highlight using clearSelection().
	 * @params bookmarks is array of bookmarks to highlight 
	 */
	public void hilightBookmarks(Bookmark[] bookmarks) {
		synchronized(mutex) {
			hilightBookmarksInternal(bookmarks);
		}
	}
	
	//========================================================================================
	// Native functions
	/* implementend by libcr3engine.so */
	//========================================================================================
	private native void getPageImageInternal(Bitmap bitmap, int bpp);

	private native void createInternal();

	private native void destroyInternal();

	private native boolean loadDocumentInternal(String fileName);

	private native java.util.Properties getSettingsInternal();

	private native boolean applySettingsInternal(
			java.util.Properties settings);

	private native void setStylesheetInternal(String stylesheet);

	private native void resizeInternal(int dx, int dy);

	private native boolean doCommandInternal(int command, int param);

	private native Bookmark getCurrentPageBookmarkInternal();

	private native boolean goToPositionInternal(String xPath, boolean saveToHistory);

	private native PositionProperties getPositionPropsInternal(String xPath);
	
	private native void updateBookInfoInternal(BookInfo info);

	private native TOCItem getTOCInternal();

	private native void clearSelectionInternal();

	private native boolean findTextInternal(String pattern, int origin,
			int reverse, int caseInsensitive);

	private native void setBatteryStateInternal(int state);

	private native byte[] getCoverPageDataInternal();

	private native void setPageBackgroundTextureInternal(
			byte[] imageBytes, int tileFlags);

	private native void updateSelectionInternal(Selection sel);

	private native boolean moveSelectionInternal(Selection sel,
			int moveCmd, int params);

	private native String checkLinkInternal(int x, int y, int delta);

	private native boolean checkImageInternal(int x, int y, ImageInfo dstImage);

	private native boolean checkBookmarkInternal(int x, int y, Bookmark dstBookmark);

	private native boolean drawImageInternal(Bitmap bitmap, int bpp, ImageInfo dstImage);

	private native boolean closeImageInternal();

	private native boolean isRenderedInternal();

	private native int goLinkInternal(String link);

	private native void hilightBookmarksInternal(Bookmark[] bookmarks);

	// / returns either SWAP_DONE, SWAP_TIMEOUT or SWAP_ERROR
	private native int swapToCacheInternal();

	private native void getPageImageTextureInternal(int[] buf, int width, int height, int bpp);
	
	private int mNativeObject; // used from JNI

	private ReaderCallback readerCallback;
	

}
