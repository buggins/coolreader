package org.coolreader.crengine;

import android.graphics.Bitmap;

public class DocView {

	public static final Logger log = L.create("dv");

	public static final int SWAP_DONE = 0;
	public static final int SWAP_TIMEOUT = 1;
	public static final int SWAP_ERROR = 2;

	/**
	 * Create native object.
	 */
	public void create() {
		createInternal();
	}

	/**
	 * Destroy native object.
	 */
	public void destroy() {
		destroyInternal();
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
		return swapToCacheInternal();
	}

	/**
	 * Follow link.
	 * @param link
	 * @return
	 */
	public int goLink(String link) {
		return goLinkInternal(link);
	}
	
	/**
	 * Find a link near to specified window coordinates.
	 * @param x
	 * @param y
	 * @param delta
	 * @return
	 */
	public String checkLink(int x, int y, int delta) {
		return checkLinkInternal(x, y, delta);
	}
	
	/**
	 * Set selection range.
	 * @param sel
	 */
	public void updateSelection(Selection sel) {
		updateSelectionInternal(sel);
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
		return moveSelectionInternal(sel, moveCmd, params);
	}

	/**
	 * Send battery state to native object.
	 * @param state
	 */
	public void setBatteryState(int state) {
		setBatteryStateInternal(state);
	}

	/**
	 * Get current book coverpage data bytes.
	 * @return
	 */
	public byte[] getCoverPageData() {
		return getCoverPageDataInternal();
	}

	/**
	 * Set texture for page background.
	 * @param imageBytes
	 * @param tileFlags
	 */
	public void setPageBackgroundTexture(
			byte[] imageBytes, int tileFlags) {
		setPageBackgroundTextureInternal(imageBytes, tileFlags);
	}

	/**
	 * Load document from file.
	 * @param fileName
	 * @return
	 */
	public boolean loadDocument(String fileName) {
		return loadDocumentInternal(fileName);
	}

	/**
	 * Get settings from native object.
	 * @return
	 */
	public java.util.Properties getSettings() {
		return getSettingsInternal();
	}

	/**
	 * Apply settings.
	 * @param settings
	 * @return
	 */
	public boolean applySettings(java.util.Properties settings) {
		return applySettingsInternal(settings);
	}

	/**
	 * Set stylesheet for document.
	 * @param stylesheet
	 */
	public void setStylesheet(String stylesheet) {
		setStylesheetInternal(stylesheet);
	}

	/**
	 * Change window size.
	 * @param dx
	 * @param dy
	 */
	public void resize(int dx, int dy) {
		resizeInternal(dx, dy);
	}

	/**
	 * Execute command by native object.
	 * @param command
	 * @param param
	 * @return
	 */
	public boolean doCommand(int command, int param) {
		return doCommandInternal(command, param);
	}

	/**
	 * Get current page bookmark info.
	 * @return
	 */
	public Bookmark getCurrentPageBookmark() {
		return getCurrentPageBookmarkInternal();
	}

	/**
	 * Move reading position to specified xPath.
	 * @param xPath
	 * @return
	 */
	public boolean goToPosition(String xPath) {
		return goToPositionInternal(xPath);
	}

	/**
	 * Get position properties by xPath.
	 * @param xPath
	 * @return
	 */
	public PositionProperties getPositionProps(String xPath) {
		return getPositionPropsInternal(xPath);
	}

	/**
	 * Fill book info fields using metadata from current book. 
	 * @param info
	 */
	public void updateBookInfo(BookInfo info) {
		updateBookInfoInternal(info);
	}

	/**
	 * Get TOC tree from current book.
	 * @return
	 */
	public TOCItem getTOC() {
		return getTOCInternal();
	}

	/**
	 * Clear selection.
	 */
	public void clearSelection() {
		clearSelectionInternal();
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
		return findTextInternal(pattern, origin, reverse, caseInsensitive);
	}

	/**
	 * Get current page image.
	 * @param bitmap is buffer to put data to.
	 */
	public void getPageImage(Bitmap bitmap) {
		getPageImageInternal(bitmap);
	}
	
	//========================================================================================
	// Native functions
	/* implementend by libcr3engine.so */
	//========================================================================================
	private native void getPageImageInternal(Bitmap bitmap);

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

	private native boolean goToPositionInternal(String xPath);

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

	private native int goLinkInternal(String link);

	// / returns either SWAP_DONE, SWAP_TIMEOUT or SWAP_ERROR
	private native int swapToCacheInternal();

	private int mNativeObject; // used from JNI

	private ReaderCallback readerCallback;

}
