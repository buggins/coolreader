package org.coolreader.crengine;

import android.graphics.Bitmap;

public class DocView {

	public static final Logger log = L.create("dv");
	
    // Native functions
    /* implementend by libcr3engine.so */
    // get current page image
    /*private*/ native void getPageImageInternal(Bitmap bitmap);
    // constructor's native part
    /*private*/ native void createInternal();
    /*private*/ native void destroyInternal();
    /*private*/ native boolean loadDocumentInternal( String fileName );
    /*private*/ native java.util.Properties getSettingsInternal();
    /*private*/ native boolean applySettingsInternal( java.util.Properties settings );
    /*private*/ native void setStylesheetInternal( String stylesheet );
    /*private*/ native void resizeInternal( int dx, int dy );
    /*private*/ native boolean doCommandInternal( int command, int param );
    /*private*/ native Bookmark getCurrentPageBookmarkInternal();
    /*private*/ native boolean goToPositionInternal(String xPath);
    /*private*/ native PositionProperties getPositionPropsInternal(String xPath);
    /*private*/ native void updateBookInfoInternal( BookInfo info );
    /*private*/ native TOCItem getTOCInternal();
    /*private*/ native void clearSelectionInternal();
    /*private*/ native boolean findTextInternal( String pattern, int origin, int reverse, int caseInsensitive );
    /*private*/ native void setBatteryStateInternal( int state );
    /*private*/ native byte[] getCoverPageDataInternal();
    /*private*/ native void setPageBackgroundTextureInternal( byte[] imageBytes, int tileFlags );
    /*private*/ native void updateSelectionInternal( Selection sel );
    /*private*/ native boolean moveSelectionInternal( Selection sel, int moveCmd, int params );
    /*private*/ native String checkLinkInternal( int x, int y, int delta );
    /*private*/ native int goLinkInternal( String link );
    /// returns either SWAP_DONE, SWAP_TIMEOUT or SWAP_ERROR 
    /*private*/ native int swapToCacheInternal();
    public static final int SWAP_DONE=0;
    public static final int SWAP_TIMEOUT=1;
    public static final int SWAP_ERROR=2;
    protected int mNativeObject; // used from JNI
    private ReaderCallback readerCallback;
    
    public void setReaderCallback(ReaderCallback readerCallback) {
    	this.readerCallback = readerCallback;
    }
}
