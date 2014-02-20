#include "docview.h"
#include "lvdocview.h"
//#include "crgl.h"



static jfieldID gNativeObjectID = 0;

class DocViewCallback : public LVDocViewCallback {
	CRJNIEnv _env;
	LVDocView * _docview;
	LVDocViewCallback * _oldcallback;
	jclass _class;
	jobject _obj;
	jmethodID _OnLoadFileStart;
    jmethodID _OnLoadFileFormatDetected;
    jmethodID _OnLoadFileEnd;
    jmethodID _OnLoadFileFirstPagesReady;
    jmethodID _OnLoadFileProgress;
    jmethodID _OnFormatStart;
    jmethodID _OnFormatEnd;
    jmethodID _OnFormatProgress;
    jmethodID _OnExportProgress;
    jmethodID _OnLoadFileError;
    jmethodID _OnExternalLink;
    jmethodID _OnImageCacheClear;
    jmethodID _OnRequestReload;
public:
	DocViewCallback( JNIEnv * env, LVDocView * docview, jobject obj )
	: _env(env), _docview(docview)
	{
		jclass objclass = _env->GetObjectClass(obj);
		jfieldID fid = _env->GetFieldID(objclass, "readerCallback", "Lorg/coolreader/crengine/ReaderCallback;");
		_obj = _env->GetObjectField(obj, fid); 
		_class = _env->GetObjectClass(_obj);
		#define GET_METHOD(n,sign) \
		     _ ## n = _env->GetMethodID(_class, # n, sign)   
		GET_METHOD(OnLoadFileStart,"(Ljava/lang/String;)V");
	    GET_METHOD(OnLoadFileFormatDetected,"(Lorg/coolreader/crengine/DocumentFormat;)Ljava/lang/String;");
	    GET_METHOD(OnLoadFileEnd,"()V");
	    GET_METHOD(OnLoadFileFirstPagesReady,"()V");
	    GET_METHOD(OnLoadFileProgress,"(I)Z");
	    GET_METHOD(OnFormatStart,"()V");
	    GET_METHOD(OnFormatEnd,"()V");
	    GET_METHOD(OnFormatProgress,"(I)Z");
	    GET_METHOD(OnExportProgress,"(I)Z");
	    GET_METHOD(OnRequestReload,"()Z");
	    GET_METHOD(OnLoadFileError,"(Ljava/lang/String;)V");
	    GET_METHOD(OnExternalLink,"(Ljava/lang/String;Ljava/lang/String;)V");
	    GET_METHOD(OnImageCacheClear,"()V");
	    _oldcallback = _docview->setCallback( this );
	}
	virtual ~DocViewCallback()
	{
		_docview->setCallback( _oldcallback );
	}
    /// on starting file loading
    virtual void OnLoadFileStart( lString16 filename )
    {
		CRLog::info("DocViewCallback::OnLoadFileStart() called");
    	_env->CallVoidMethod(_obj, _OnLoadFileStart, _env.toJavaString(filename));
    }
    /// format detection finished
    virtual void OnLoadFileFormatDetected( doc_format_t fileFormat )
    {
		CRLog::info("DocViewCallback::OnLoadFileFormatDetected() called");
    	jobject e = _env.enumByNativeId("org/coolreader/crengine/DocumentFormat", (int)fileFormat);
    	jstring css = (jstring)_env->CallObjectMethod(_obj, _OnLoadFileFormatDetected, e);
    	if ( css ) {
    		lString16 s = _env.fromJavaString(css);
    		CRLog::info("OnLoadFileFormatDetected: setting CSS for format %d", (int)fileFormat);
    		_docview->setStyleSheet( UnicodeToUtf8(s) );
    	}
    }
    /// file loading is finished successfully - drawCoveTo() may be called there
    virtual void OnLoadFileEnd()
    {
		CRLog::info("DocViewCallback::OnLoadFileEnd() called");
    	_env->CallVoidMethod(_obj, _OnLoadFileEnd);
    }
    /// first page is loaded from file an can be formatted for preview
    virtual void OnLoadFileFirstPagesReady()
    {
		CRLog::info("DocViewCallback::OnLoadFileFirstPagesReady() called");
    	_env->CallVoidMethod(_obj, _OnLoadFileFirstPagesReady);
    }
    /// file progress indicator, called with values 0..100
    virtual void OnLoadFileProgress( int percent )
    {
		CRLog::info("DocViewCallback::OnLoadFileProgress() called");
    	jboolean res = _env->CallBooleanMethod(_obj, _OnLoadFileProgress, (jint)(percent*100));
    }
    /// document formatting started
    virtual void OnFormatStart()
    {
		CRLog::info("DocViewCallback::OnFormatStart() called");
    	_env->CallVoidMethod(_obj, _OnFormatStart);
    }
    /// document formatting finished
    virtual void OnFormatEnd()
    {
		CRLog::info("DocViewCallback::OnFormatEnd() called");
    	_env->CallVoidMethod(_obj, _OnFormatEnd);
    }
    /// format progress, called with values 0..100
    virtual void OnFormatProgress( int percent )
    {
		CRLog::info("DocViewCallback::OnFormatProgress() called");
    	jboolean res = _env->CallBooleanMethod(_obj, _OnFormatProgress, (jint)(percent*100));
    }
    /// format progress, called with values 0..100
    virtual void OnExportProgress( int percent )
    {
		CRLog::info("DocViewCallback::OnExportProgress() called");
    	jboolean res = _env->CallBooleanMethod(_obj, _OnExportProgress, (jint)(percent*100));
    }
    /// file load finiished with error
    virtual void OnLoadFileError( lString16 message )
    {
		CRLog::info("DocViewCallback::OnLoadFileError() called");
    	_env->CallVoidMethod(_obj, _OnLoadFileError, _env.toJavaString(message));
    }
    /// Override to handle external links
    virtual void OnExternalLink( lString16 url, ldomNode * node )
    {
		CRLog::info("DocViewCallback::OnExternalLink() called");
    	lString16 path = ldomXPointer(node,0).toString();
    	_env->CallVoidMethod(_obj, _OnExternalLink, _env.toJavaString(url), _env.toJavaString(path));
    }
    virtual void OnImageCacheClear()
    {
		//CRLog::info("DocViewCallback::OnImageCacheClear() called");
    	_env->CallVoidMethod(_obj, _OnImageCacheClear);
    }

    /// return true if reload will be processed by external code, false to let internal code process it
    virtual bool OnRequestReload() {
    	_env->CallBooleanMethod(_obj, _OnRequestReload);
    	return true;
    }

};

CRTimerUtil _timeoutControl;

#define DECL_DEF_CR_FONT_SIZES static int cr_font_sizes[] = \
 { 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, \
   31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 42, 44, 48, 52, 56, 60, 64, 68, 72 }

DECL_DEF_CR_FONT_SIZES;

#define NO_BATTERY_GAUGE 1

static void replaceColor( char * str, lUInt32 color )
{
	// in line like "0 c #80000000",
	// replace value of color
	for ( int i=0; i<8; i++ ) {
		str[i+5] = toHexDigit((color>>28) & 0xF);
		color <<= 4;
	}
}

static LVRefVec<LVImageSource> getBatteryIcons( lUInt32 color )
{
	CRLog::debug("Making list of Battery icon bitmats");

	#include "battery_icons.h"
    lUInt32 cl1 = 0x00000000|(color&0xFFFFFF);
    lUInt32 cl2 = 0x40000000|(color&0xFFFFFF);
    lUInt32 cl3 = 0x80000000|(color&0xFFFFFF);
    lUInt32 cl4 = 0xF0000000|(color&0xFFFFFF);

    static char color1[] = "0 c #80000000";
    static char color2[] = "X c #80000000";
    static char color3[] = "o c #80AAAAAA";
    static char color4[] = ". c #80FFFFFF";
	#define BATTERY_HEADER \
			"28 15 5 1", \
			color1, \
			color2, \
			color3, \
			color4, \
			"  c None",

    static const char * battery8[] = {
        BATTERY_HEADER
        "   .........................",
        "   .00000000000000000000000.",
        "   .0.....................0.",
        "....0.XXXX.XXXX.XXXX.XXXX.0.",
        ".0000.XXXX.XXXX.XXXX.XXXX.0.",
        ".0..0.XXXX.XXXX.XXXX.XXXX.0.",
        ".0..0.XXXX.XXXX.XXXX.XXXX.0.",
        ".0..0.XXXX.XXXX.XXXX.XXXX.0.",
        ".0..0.XXXX.XXXX.XXXX.XXXX.0.",
        ".0..0.XXXX.XXXX.XXXX.XXXX.0.",
        ".0000.XXXX.XXXX.XXXX.XXXX.0.",
        "....0.XXXX.XXXX.XXXX.XXXX.0.",
        "   .0.....................0.",
        "   .00000000000000000000000.",
        "   .........................",
    };
    static const char * battery7[] = {
        BATTERY_HEADER
        "   .........................",
        "   .00000000000000000000000.",
        "   .0.....................0.",
        "....0.oooo.XXXX.XXXX.XXXX.0.",
        ".0000.oooo.XXXX.XXXX.XXXX.0.",
        ".0..0.oooo.XXXX.XXXX.XXXX.0.",
        ".0..0.oooo.XXXX.XXXX.XXXX.0.",
        ".0..0.oooo.XXXX.XXXX.XXXX.0.",
        ".0..0.oooo.XXXX.XXXX.XXXX.0.",
        ".0..0.oooo.XXXX.XXXX.XXXX.0.",
        ".0000.oooo.XXXX.XXXX.XXXX.0.",
        "....0.oooo.XXXX.XXXX.XXXX.0.",
        "   .0.....................0.",
        "   .00000000000000000000000.",
        "   .........................",
    };
    static const char * battery6[] = {
        BATTERY_HEADER
        "   .........................",
        "   .00000000000000000000000.",
        "   .0.....................0.",
        "....0......XXXX.XXXX.XXXX.0.",
        ".0000......XXXX.XXXX.XXXX.0.",
        ".0..0......XXXX.XXXX.XXXX.0.",
        ".0..0......XXXX.XXXX.XXXX.0.",
        ".0..0......XXXX.XXXX.XXXX.0.",
        ".0..0......XXXX.XXXX.XXXX.0.",
        ".0..0......XXXX.XXXX.XXXX.0.",
        ".0000......XXXX.XXXX.XXXX.0.",
        "....0......XXXX.XXXX.XXXX.0.",
        "   .0.....................0.",
        "   .00000000000000000000000.",
        "   .........................",
    };
    static const char * battery5[] = {
        BATTERY_HEADER
        "   .........................",
        "   .00000000000000000000000.",
        "   .0.....................0.",
        "....0......oooo.XXXX.XXXX.0.",
        ".0000......oooo.XXXX.XXXX.0.",
        ".0..0......oooo.XXXX.XXXX.0.",
        ".0..0......oooo.XXXX.XXXX.0.",
        ".0..0......oooo.XXXX.XXXX.0.",
        ".0..0......oooo.XXXX.XXXX.0.",
        ".0..0......oooo.XXXX.XXXX.0.",
        ".0000......oooo.XXXX.XXXX.0.",
        "....0......oooo.XXXX.XXXX.0.",
        "   .0.....................0.",
        "   .00000000000000000000000.",
        "   .........................",
    };
    static const char * battery4[] = {
        BATTERY_HEADER
        "   .........................",
        "   .00000000000000000000000.",
        "   .0.....................0.",
        "....0...........XXXX.XXXX.0.",
        ".0000...........XXXX.XXXX.0.",
        ".0..0...........XXXX.XXXX.0.",
        ".0..0...........XXXX.XXXX.0.",
        ".0..0...........XXXX.XXXX.0.",
        ".0..0...........XXXX.XXXX.0.",
        ".0..0...........XXXX.XXXX.0.",
        ".0000...........XXXX.XXXX.0.",
        "....0...........XXXX.XXXX.0.",
        "   .0.....................0.",
        "   .00000000000000000000000.",
        "   .........................",
    };
    static const char * battery3[] = {
        BATTERY_HEADER
        "   .........................",
        "   .00000000000000000000000.",
        "   .0.....................0.",
        "....0...........oooo.XXXX.0.",
        ".0000...........oooo.XXXX.0.",
        ".0..0...........oooo.XXXX.0.",
        ".0..0...........oooo.XXXX.0.",
        ".0..0...........oooo.XXXX.0.",
        ".0..0...........oooo.XXXX.0.",
        ".0..0...........oooo.XXXX.0.",
        ".0000...........oooo.XXXX.0.",
        "....0...........oooo.XXXX.0.",
        "   .0.....................0.",
        "   .00000000000000000000000.",
        "   .........................",
    };
    static const char * battery2[] = {
        BATTERY_HEADER
        "   .........................",
        "   .00000000000000000000000.",
        "   .0.....................0.",
        "....0................XXXX.0.",
        ".0000................XXXX.0.",
        ".0..0................XXXX.0.",
        ".0..0................XXXX.0.",
        ".0..0................XXXX.0.",
        ".0..0................XXXX.0.",
        ".0..0................XXXX.0.",
        ".0000................XXXX.0.",
        "....0................XXXX.0.",
        "   .0.....................0.",
        "   .00000000000000000000000.",
        "   .........................",
    };
    static const char * battery1[] = {
        BATTERY_HEADER
        "   .........................",
        "   .00000000000000000000000.",
        "   .0.....................0.",
        "   .0................oooo.0.",
        ".0000................oooo.0.",
        ".0..0................oooo.0.",
        ".0..0................oooo.0.",
        ".0..0................oooo.0.",
        ".0..0................oooo.0.",
        ".0..0................oooo.0.",
        ".0000................oooo.0.",
        "   .0................oooo.0.",
        "   .0.....................0.",
        "   .00000000000000000000000.",
        "   .........................",
    };
    static const char * battery0[] = {
        BATTERY_HEADER
        "   .........................",
        "   .00000000000000000000000.",
        "   .0.....................0.",
        "   .0.....................0.",
        ".0000.....................0.",
        ".0..0.....................0.",
        ".0..0.....................0.",
        ".0..0.....................0.",
        ".0..0.....................0.",
        ".0..0.....................0.",
        ".0000.....................0.",
        "....0.....................0.",
        "   .0.....................0.",
        "   .00000000000000000000000.",
        "   .........................",
    };
//#endif

    static const char * battery_charge[] = {
        BATTERY_HEADER
        "   .........................",
        "   .00000000000000000000000.",
        "   .0.....................0.",
        "....0.....................0.",
        ".0000............XX.......0.",
        ".0..0...........XXXX......0.",
        ".0..0..XX......XXXXXX.....0.",
        ".0..0...XXX...XXXX..XX....0.",
        ".0..0....XXX..XXXX...XX...0.",
        ".0..0.....XXXXXXX.....XX..0.",
        ".0000.......XXXX..........0.",
        "....0........XX...........0.",
        "   .0.....................0.",
        "   .00000000000000000000000.",
        "   .........................",
    };
    static const char * battery_frame[] = {
        BATTERY_HEADER
        "   .........................",
        "   .00000000000000000000000.",
        "   .0.....................0.",
        "....0.....................0.",
        ".0000.....................0.",
        ".0..0.....................0.",
        ".0..0.....................0.",
        ".0..0.....................0.",
        ".0..0.....................0.",
        ".0..0.....................0.",
        ".0000.....................0.",
        "....0.....................0.",
        "   .0.....................0.",
        "   .00000000000000000000000.",
        "   .........................",
    };

    const char * * icon_bpm[] = {
		battery_charge,
		battery0,
		battery1,
		battery2,
		battery3,
		battery4,
		battery5,
		battery6,
		battery7,
		battery8,
		battery_frame,
		NULL
    };


	replaceColor( color1, cl1 );
	replaceColor( color2, cl2 );
	replaceColor( color3, cl3 );
	replaceColor( color4, cl4 );

    LVRefVec<LVImageSource> icons;
    for ( int i=0; icon_bpm[i]; i++ )
        icons.add( LVCreateXPMImageSource( icon_bpm[i] ) );

    return icons;
}

DocViewNative::DocViewNative()
{
	_docview = new LVDocView(16); //16bpp

    LVRefVec<LVImageSource> icons = getBatteryIcons( 0x000000 );
    _docview->setBatteryIcons( icons );

    LVArray<int> sizes( cr_font_sizes, sizeof(cr_font_sizes)/sizeof(int) );
    _docview->setShowCover( true );
    _docview->setFontSizes( sizes, true );
	_docview->setFontSize(24);
	_docview->setBatteryFont( fontMan->GetFont( 16, 600, false, css_ff_sans_serif, lString8("Droid Sans") ));
	
	_docview->createDefaultDocument(lString16("Welcome to CoolReader"), lString16("Please select file to open"));
}

static DocViewNative * getNative(JNIEnv * env, jobject _this)
{
	if (!gNativeObjectID) {
		CRLog::error("gNativeObjectID is not defined");
	    jclass rvClass = env->FindClass("org/coolreader/crengine/DocView");
	    gNativeObjectID = env->GetFieldID(rvClass, "mNativeObject", "I");
	    if (!gNativeObjectID)
	    	return NULL;
	}
	DocViewNative * res = (DocViewNative *)env->GetIntField(_this, gNativeObjectID);
	if (res == NULL)
		CRLog::error("Native DocView is NULL");
	return res;
}

bool DocViewNative::loadDocument( lString16 filename )
{
	CRLog::info("Loading document %s", LCSTR(filename));
	bool res = _docview->LoadDocument(filename.c_str());
	CRLog::info("Document %s is loaded %s", LCSTR(filename), (res?"successfully":"with error"));
    return res;
}

bool DocViewNative::openRecentBook()
{
	CRLog::debug("DocViewNative::openRecentBook()");
	int index = 0;
	if ( _docview->isDocumentOpened() ) {
		CRLog::debug("DocViewNative::openRecentBook() : saving previous document state");
		_docview->swapToCache();
        _docview->getDocument()->updateMap();
	    _docview->savePosition();
		closeBook();
	    index = 1;
	}
    LVPtrVector<CRFileHistRecord> & files = _docview->getHistory()->getRecords();
    CRLog::info("DocViewNative::openRecentBook() : %d files found in history, startIndex=%d", files.length(), index);
    if ( index < files.length() ) {
        CRFileHistRecord * file = files.get( index );
        lString16 fn = file->getFilePathName();
        CRLog::info("DocViewNative::openRecentBook() : checking file %s", LCSTR(fn));
        // TODO: check error
        if ( LVFileExists(fn) ) {
            return loadDocument( fn );
        } else {
        	CRLog::error("file %s doesn't exist", LCSTR(fn));
        	return false;
        }
        //_docview->swapToCache();
    } else {
        CRLog::info("DocViewNative::openRecentBook() : no recent book found in history");
    }
    return false;
}

bool DocViewNative::closeBook()
{
	closeImage();
	if ( _docview->isDocumentOpened() ) {
	    _docview->savePosition();
        _docview->getDocument()->updateMap();
        saveHistory(lString16::empty_str);
	    _docview->close();
	    return true;
	}
	return false;
}

void DocViewNative::clearSelection()
{
    _docview->clearSelection();
}

bool DocViewNative::findText( lString16 pattern, int origin, bool reverse, bool caseInsensitive )
{
    if ( pattern.empty() )
        return false;
    if ( pattern!=_lastPattern && origin==1 )
        origin = 0;
    _lastPattern = pattern;
    LVArray<ldomWord> words;
    lvRect rc;
    _docview->GetPos( rc );
    int pageHeight = rc.height();
    int start = -1;
    int end = -1;
    if ( reverse ) {
        // reverse
        if ( origin == 0 ) {
            // from end current page to first page
            end = rc.bottom;
        } else if ( origin == -1 ) {
            // from last page to end of current page
            start = rc.bottom;
        } else { // origin == 1
            // from prev page to first page
            end = rc.top;
        }
    } else {
        // forward
        if ( origin == 0 ) {
            // from current page to last page
            start = rc.top;
        } else if ( origin == -1 ) {
            // from first page to current page
            end = rc.top;
        } else { // origin == 1
            // from next page to last
            start = rc.bottom;
        }
    }
    CRLog::debug("CRViewDialog::findText: Current page: %d .. %d", rc.top, rc.bottom);
    CRLog::debug("CRViewDialog::findText: searching for text '%s' from %d to %d origin %d", LCSTR(pattern), start, end, origin );
    if ( _docview->getDocument()->findText( pattern, caseInsensitive, reverse, start, end, words, 200, pageHeight ) ) {
        CRLog::debug("CRViewDialog::findText: pattern found");
        _docview->clearSelection();
        _docview->selectWords( words );
        ldomMarkedRangeList * ranges = _docview->getMarkedRanges();
        if ( ranges ) {
            if ( ranges->length()>0 ) {
                int pos = ranges->get(0)->start.y;
                _docview->SetPos(pos);
            }
        }
        return true;
    }
    CRLog::debug("CRViewDialog::findText: pattern not found");
    return false;
}



bool DocViewNative::loadHistory( lString16 filename )
{
    CRFileHist * hist = _docview->getHistory();
	if ( !filename.empty() )
		historyFileName = filename;
    historyFileName = filename;
    if ( historyFileName.empty() ) {
    	CRLog::error("No history file name specified");
    	return false;
    }
	CRLog::info("Trying to load history from file %s", LCSTR(historyFileName));
    LVStreamRef stream = LVOpenFileStream(historyFileName.c_str(), LVOM_READ);
    if ( stream.isNull() ) {
    	CRLog::error("Cannot open file %s", LCSTR(historyFileName));
    	return false;
    }
    bool res = hist->loadFromStream( stream );
    if ( res )
    	CRLog::info("%d items found", hist->getRecords().length());
    else
    	CRLog::error("Cannot read history file content");
    return res;
}

bool DocViewNative::saveHistory( lString16 filename )
{
	if ( !filename.empty() )
		historyFileName = filename;
    if ( historyFileName.empty() )
    	return false;
	if ( _docview->isDocumentOpened() ) {
		CRLog::debug("DocViewNative::saveHistory() : saving position");
	    _docview->savePosition();
	}
	CRLog::info("Trying to save history to file %s", LCSTR(historyFileName));
    CRFileHist * hist = _docview->getHistory();
    LVStreamRef stream = LVOpenFileStream(historyFileName.c_str(), LVOM_WRITE);
    if ( stream.isNull() ) {
    	CRLog::error("Cannot create file %s for writing", LCSTR(historyFileName));
    	return false;
    }
    if ( _docview->isDocumentOpened() )
    	_docview->savePosition();
    return hist->saveToStream( stream.get() );
}

int DocViewNative::doCommand( int cmd, int param )
{
	switch (cmd) {
    case DCMD_OPEN_RECENT_BOOK:
    	{
    		return openRecentBook();
    	}
    	break;
    case DCMD_CLOSE_BOOK:
    	{
    		return closeBook();
    	}
    	break;
    case DCMD_RESTORE_POSITION:
	    {
    		if ( _docview->isDocumentOpened() ) {
		        _docview->restorePosition();
    		}
	    }
	    break;
    default:
    	return 0;
   	}
   	return 1;
}


/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    createInternal
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_DocView_createInternal
  (JNIEnv * env, jobject _this)
{
	CRLog::info("******************************************************************");
	CRLog::info("createInternal: Creating new RenderView");
	CRLog::info("******************************************************************");
    jclass rvClass = env->FindClass("org/coolreader/crengine/DocView");
    gNativeObjectID = env->GetFieldID(rvClass, "mNativeObject", "I");
    DocViewNative * obj = new DocViewNative();
    env->SetIntField(_this, gNativeObjectID, (jint)obj);
    obj->_docview->setFontSize(24); 
}

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    destroyInternal
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_DocView_destroyInternal
  (JNIEnv * env, jobject view)
{
    DocViewNative * p = getNative(env, view);
    if ( p!=NULL ) {
    	CRLog::info("******************************************************************");
		CRLog::info("Destroying RenderView");
		CRLog::info("******************************************************************");
    	delete p;
	    jclass rvClass = env->FindClass("org/coolreader/crengine/DocView");
	    gNativeObjectID = env->GetFieldID(rvClass, "mNativeObject", "I");
	    env->SetIntField(view, gNativeObjectID, 0);
	    gNativeObjectID = 0;
	} else {
		CRLog::error("RenderView is already destroyed");
	}
}

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    getPageImageInternal
 * Signature: (Landroid/graphics/Bitmap;I)V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_DocView_getPageImageInternal
  (JNIEnv * env, jobject view, jobject bitmap, jint bpp)
{
    CRLog::trace("getPageImageInternal entered : bpp=%d", bpp);
    DocViewNative * p = getNative(env, view);
    if (!p) {
    	CRLog::error("Cannot get native view");
    	return;
    }
    //CRLog::info("Initialize callback");
	DocViewCallback callback( env, p->_docview, view );	
    //CRLog::info("Initialized callback");
    //CRLog::trace("getPageImageInternal calling bitmap->lock");
	LVDrawBuf * drawbuf = BitmapAccessorInterface::getInstance()->lock(env, bitmap);
	if ( drawbuf!=NULL ) {
		if (bpp >= 16) {
			// native resolution
			p->_docview->Draw( *drawbuf );
		} else {
			LVGrayDrawBuf grayBuf(drawbuf->GetWidth(), drawbuf->GetHeight(), bpp);
			p->_docview->Draw(grayBuf);
			grayBuf.DrawTo(drawbuf, 0, 0, 0, NULL);
		}
	    //CRLog::trace("getPageImageInternal calling bitmap->unlock");
		BitmapAccessorInterface::getInstance()->unlock(env, bitmap, drawbuf);
	} else {
		CRLog::error("bitmap accessor is invalid");
	}
    //CRLog::trace("getPageImageInternal exiting");
}

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    checkImageInternal
 * Signature: (IILorg/coolreader/crengine/ImageInfo;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_DocView_checkImageInternal
  (JNIEnv * _env, jobject view, jint x, jint y, jobject imageInfo)
{
    //CRLog::trace("checkImageInternal entered");
	CRJNIEnv env(_env);
    DocViewNative * p = getNative(_env, view);
    if (!p) {
    	CRLog::error("Cannot get native view");
    	return false;
    }
    int dx, dy;
    bool needRotate = false;
    CRObjectAccessor acc(_env, imageInfo);
    int width = CRIntField(acc,"bufWidth").get();
    int height = CRIntField(acc,"bufHeight").get();
	if (!p->checkImage(x, y, width, height, dx, dy, needRotate))
		return JNI_FALSE;
    CRIntField(acc,"rotation").set(needRotate ? 1 : 0);
    CRIntField(acc,"width").set(dx);
    CRIntField(acc,"height").set(dy);
    CRIntField(acc,"scaledWidth").set(dx);
    CRIntField(acc,"scaledHeight").set(dy);
    CRIntField(acc,"x").set(0);
    CRIntField(acc,"y").set(0);
	return JNI_TRUE;
}

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    checkBookmarkInternal
 * Signature: (IILorg/coolreader/crengine/Bookmark;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_DocView_checkBookmarkInternal
  (JNIEnv * _env, jobject view, jint x, jint y, jobject bmk) {
	CRJNIEnv env(_env);
    DocViewNative * p = getNative(_env, view);
    if (!p) {
    	CRLog::error("Cannot get native view");
    	return false;
    }
    CRObjectAccessor acc(_env, bmk);
    //CRLog::trace("checkBookmarkInternal(%d, %d)", x, y);
    CRBookmark * found = p->_docview->findBookmarkByPoint(lvPoint(x, y));
    if (!found)
		return JNI_FALSE;
    //CRLog::trace("checkBookmarkInternal - found bookmark of type %d", found->getType());
    CRIntField(acc,"type").set(found->getType());
    CRStringField(acc,"startPos").set(found->getStartPos());
    CRStringField(acc,"endPos").set(found->getEndPos());
	return JNI_TRUE;
}


/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    drawImageInternal
 * Signature: (Landroid/graphics/Bitmap;ILorg/coolreader/crengine/ImageInfo;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_DocView_drawImageInternal
  (JNIEnv * _env, jobject view, jobject bitmap, jint bpp, jobject imageInfo)
{
    CRLog::trace("checkImageInternal entered");
	CRJNIEnv env(_env);
    DocViewNative * p = getNative(_env, view);
    if (!p) {
    	CRLog::error("Cannot get native view");
    	return false;
    }
    CRObjectAccessor acc(_env, imageInfo);
    int dx = CRIntField(acc,"scaledWidth").get();
    int dy = CRIntField(acc,"scaledHeight").get();
    int x = CRIntField(acc,"x").get();
    int y = CRIntField(acc,"y").get();
    int rotation = CRIntField(acc,"rotation").get();
    int dpi = CRIntField(acc,"bufDpi").get();
	LVDrawBuf * drawbuf = BitmapAccessorInterface::getInstance()->lock(_env, bitmap);
	bool res = false;
	if ( drawbuf!=NULL ) {

	    lvRect full(0, 0, drawbuf->GetWidth(), drawbuf->GetHeight());
	    lvRect zoomInRect(full);
	    lvRect zoomOutRect(full);
	    int zoomSize = dpi * 4 / 10;
	    if (rotation == 0) {
	    	zoomInRect.right = zoomInRect.left + zoomSize;
	    	zoomInRect.top = zoomInRect.bottom - zoomSize;
	    	zoomOutRect.left = zoomOutRect.right - zoomSize;
	    	zoomOutRect.top = zoomOutRect.bottom - zoomSize;
	    } else {
	    	zoomInRect.right = zoomInRect.left + zoomSize;
	    	zoomInRect.bottom = zoomInRect.top + zoomSize;
	    	zoomOutRect.right = zoomOutRect.left + zoomSize;
	    	zoomOutRect.top = zoomOutRect.bottom - zoomSize;
	    }

		if (bpp >= 16) {
			// native resolution
			res = p->drawImage(drawbuf, x, y, dx, dy);
			p->drawIcon(drawbuf, zoomInRect, 0);
			p->drawIcon(drawbuf, zoomOutRect, rotation ? 2 : 1);
		} else {
			LVGrayDrawBuf grayBuf(drawbuf->GetWidth(), drawbuf->GetHeight(), bpp);
			res = p->drawImage(&grayBuf, x, y, dx, dy);
			p->drawIcon(&grayBuf, zoomInRect, 0);
			p->drawIcon(&grayBuf, zoomOutRect, rotation ? 2 : 1);
			grayBuf.DrawTo(drawbuf, 0, 0, 0, NULL);
		}
	    //CRLog::trace("getPageImageInternal calling bitmap->unlock");
		BitmapAccessorInterface::getInstance()->unlock(_env, bitmap, drawbuf);
	} else {
		CRLog::error("bitmap accessor is invalid");
	}
	return res ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    closeImageInternal
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_DocView_closeImageInternal
  (JNIEnv * env, jobject view)
{
    CRLog::trace("checkImageInternal entered");
    DocViewNative * p = getNative(env, view);
    if (!p) {
    	CRLog::error("Cannot get native view");
    	return false;
    }
	return p->closeImage() ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    loadDocument
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_DocView_loadDocumentInternal
  (JNIEnv * _env, jobject _this, jstring s)
{
	CRJNIEnv env(_env);
    DocViewNative * p = getNative(_env, _this);
    if (!p) {
    	CRLog::error("Cannot get native view");
    	return false;
    }
	DocViewCallback callback( _env, p->_docview, _this );
	lString16 str = env.fromJavaString(s);
    bool res = p->loadDocument(str);
    return res ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    getSettings
 * Signature: ()Ljava/util/Properties;
 */
JNIEXPORT jobject JNICALL Java_org_coolreader_crengine_DocView_getSettingsInternal
  (JNIEnv * _env, jobject _this)
{
	CRLog::trace("DocView_getSettingsInternal");
	CRJNIEnv env(_env);
    DocViewNative * p = getNative(_env, _this);
    if (!p) {
    	CRLog::error("Cannot get native view");
    	return NULL;
    }
	CRPropRef props = p->_docview->propsGetCurrent();
    return env.toJavaProperties(props);
}

#define PROP_NIGHT_MODE "crengine.night.mode"

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    applySettingsInternal
 * Signature: (Ljava/util/Properties;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_DocView_applySettingsInternal
  (JNIEnv * _env, jobject _this, jobject _props)
{
	CRLog::trace("DocView_applySettingsInternal");
	CRJNIEnv env(_env);
    DocViewNative * p = getNative(_env, _this);
    if (!p) {
    	CRLog::error("Cannot get native view");
    	return false;
    }
	DocViewCallback callback( _env, p->_docview, _this );
	CRPropRef props = env.fromJavaProperties(_props);
	CRPropRef oldProps = p->_docview->propsGetCurrent();
	p->_docview->propsUpdateDefaults( props );
	//bool oldNightMode = oldProps->getBoolDef(PROP_NIGHT_MODE, false);
	//bool newNightMode = props->getBoolDef(PROP_NIGHT_MODE, false);
	lUInt32 oldTextColor = oldProps->getColorDef(PROP_FONT_COLOR, 0x000000);
	lUInt32 newTextColor = props->getColorDef(PROP_FONT_COLOR, 0x000000);
	lUInt32 oldStatusColor = oldProps->getColorDef(PROP_STATUS_FONT_COLOR, 0xFF000000);
	lUInt32 newStatusColor = props->getColorDef(PROP_STATUS_FONT_COLOR, 0xFF000000);
	//CRLog::debug("Text colors: %x->%x, %x->%x", oldTextColor, newTextColor, oldStatusColor, newStatusColor);
	CRPropRef diff = oldProps ^ props;
	CRPropRef unknown = p->_docview->propsApply(props); //diff
	lUInt32 batteryColor = newStatusColor;
	if ( batteryColor==0xFF000000 )
		batteryColor = newTextColor;
	if ( 1 || oldTextColor!=newTextColor || oldStatusColor!=newStatusColor ) { //oldNightMode!=newNightMode
		//CRLog::debug("%x->%x, %x->%x: Setting Battery icon color = #%06x", oldTextColor, newTextColor, oldStatusColor, newStatusColor, batteryColor);
	    LVRefVec<LVImageSource> icons = getBatteryIcons( batteryColor );
		//CRLog::debug("Setting list of Battery icon bitmats");
	    p->_docview->setBatteryIcons( icons );
		//CRLog::debug("Setting list of Battery icon bitmats - done");
	}
	CRLog::trace("DocView_applySettingsInternal - done");
    return JNI_TRUE;
}
#if 0
/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    readHistory
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_DocView_readHistoryInternal
  (JNIEnv * _env, jobject _this, jstring jFilename)
{
	CRJNIEnv env(_env);
    DocViewNative * p = getNative(_env, _this);
    bool res = p->loadHistory( env.fromJavaString(jFilename) );
    return res?JNI_TRUE:JNI_FALSE;
}

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    writeHistory
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_DocView_writeHistoryInternal
  (JNIEnv * _env, jobject _this, jstring jFilename)
{
	CRJNIEnv env(_env);
    DocViewNative * p = getNative(_env, _this);
    bool res = p->saveHistory( env.fromJavaString(jFilename) );
    return res?JNI_TRUE:JNI_FALSE;
}
#endif

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    setStylesheet
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_DocView_setStylesheetInternal
  (JNIEnv * _env, jobject _view, jstring jcss)
{
	CRJNIEnv env(_env);
    DocViewNative * p = getNative(_env, _view);
    if (!p) {
    	CRLog::error("Cannot get native view");
    	return;
    }
	DocViewCallback callback( _env, p->_docview, _view );
    lString8 css8 = UnicodeToUtf8(env.fromJavaString(jcss));
    p->_docview->setStyleSheet(css8);
}

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    resize
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_DocView_resizeInternal
  (JNIEnv * _env, jobject _this, jint dx, jint dy)
{
	CRJNIEnv env(_env);
	CRLog::debug("resizeInternal(%d, %d) is called", dx, dy);
    DocViewNative * p = getNative(_env, _this);
    if (!p) {
    	CRLog::error("Cannot get native view");
    	return;
    }
	DocViewCallback callback( _env, p->_docview, _this );
    p->_docview->Resize(dx, dy);
    //p->_docview->checkRender();
    CRLog::trace("resizeInternal() is finished");
}  
  

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    doCommandInternal
 * Signature: (II)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_DocView_doCommandInternal
  (JNIEnv * _env, jobject _this, jint cmd, jint param)
{
    DocViewNative * p = getNative(_env, _this);
    if (!p) {
    	CRLog::error("Cannot get native view");
    	return false;
    }
	DocViewCallback callback( _env, p->_docview, _this );	
    if ( cmd>=READERVIEW_DCMD_START && cmd<=READERVIEW_DCMD_END) {
    	return p->doCommand(cmd, param)?JNI_TRUE:JNI_FALSE;
    }
    //CRLog::trace("doCommandInternal(%d, %d) -- passing to LVDocView", cmd, param);
    return p->_docview->doCommand((LVDocCmd)cmd, param) ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    isRenderedInternal
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_DocView_isRenderedInternal
(JNIEnv *_env, jobject _this)
{
	CRJNIEnv env(_env);
    DocViewNative * p = getNative(_env, _this);
    if (!p) {
    	CRLog::error("Cannot get native view");
    	return false;
    }
	if (!p->_docview->isDocumentOpened())
		return JNI_FALSE;
	return p->_docview->IsRendered() ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    getCurrentPageBookmarkInternal
 * Signature: ()Lorg/coolreader/crengine/Bookmark;
 */
JNIEXPORT jobject JNICALL Java_org_coolreader_crengine_DocView_getCurrentPageBookmarkInternal
  (JNIEnv *_env, jobject _this)
{
	CRJNIEnv env(_env);
    DocViewNative * p = getNative(_env, _this);
	if (p == NULL || !p->_docview->isDocumentOpened())
		return NULL;
	DocViewCallback callback( _env, p->_docview, _this );
	
	CRLog::trace("getCurrentPageBookmarkInternal: calling getBookmark()");
	ldomXPointer ptr = p->_docview->getBookmark();
	if ( ptr.isNull() )
		return JNI_FALSE;
	CRBookmark bm(ptr);
	lString16 comment;
    lString16 titleText;
    lString16 posText;
    bm.setType( bmkt_pos );
    if ( p->_docview->getBookmarkPosText( ptr, titleText, posText ) ) {
         bm.setTitleText( titleText );
         bm.setPosText( posText );
    }
    bm.setStartPos( ptr.toString() );
    int pos = ptr.toPoint().y;
    int fh = p->_docview->getDocument()->getFullHeight();
    int percent = fh > 0 ? (int)(pos * (lInt64)10000 / fh) : 0;
    if ( percent<0 )
        percent = 0;
    if ( percent>10000 )
        percent = 10000;
    bm.setPercent( percent );
    bm.setCommentText( comment );
    jclass cls = _env->FindClass("org/coolreader/crengine/Bookmark");
    jmethodID mid = _env->GetMethodID(cls, "<init>", "()V");
    jobject obj = _env->NewObject(cls, mid);
    CRObjectAccessor acc(_env, obj);
    CRStringField(acc,"startPos").set(bm.getStartPos());
    CRStringField(acc,"endPos").set(bm.getEndPos());
    CRStringField(acc,"titleText").set(bm.getTitleText());
    CRStringField(acc,"posText").set(bm.getPosText());
    CRStringField(acc,"commentText").set(bm.getCommentText());
    CRIntField(acc,"percent").set(bm.getPercent());
    //CRIntField(acc,"page").set(bm.getPageNum());
    CRIntField(acc,"type").set(bm.getType());
    CRLongField(acc,"timeStamp").set((lInt64)bm.getTimestamp()*1000);
	return obj;
}

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    updateBookInfoInternal
 * Signature: (Lorg/coolreader/crengine/BookInfo;)V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_DocView_updateBookInfoInternal
  (JNIEnv * _env, jobject _this, jobject _info)
{
	CRJNIEnv env(_env);
    DocViewNative * p = getNative(_env, _this);
    if (!p) {
    	CRLog::error("Cannot get native view");
    	return;
    }
	if ( !p->_docview->isDocumentOpened() )
		return;
	DocViewCallback callback( _env, p->_docview, _this );
    CRObjectAccessor bookinfo(_env, _info);
    CRObjectAccessor fileinfo(_env, CRFieldAccessor(bookinfo, "fileInfo", "Lorg/coolreader/crengine/FileInfo;").getObject() );
    CRStringField titleField(fileinfo,"title");
    if (titleField.get().empty())
    	titleField.set(p->_docview->getTitle());
    CRStringField authorsField(fileinfo,"authors");
    if (authorsField.get().empty())
    	authorsField.set(p->_docview->getAuthors());
    CRStringField seriesField(fileinfo,"series");
    if (seriesField.get().empty()) {
    	seriesField.set(p->_docview->getSeriesName());
    	CRIntField seriesNumberField(fileinfo,"seriesNumber");
    	seriesNumberField.set(p->_docview->getSeriesNumber());
    }
    CRStringField languageField(fileinfo,"language");
    if (languageField.get().empty())
    	languageField.set(p->_docview->getLanguage());
}

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    goToPositionInternal
 * Signature: (Ljava/lang/String;Z)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_DocView_goToPositionInternal
  (JNIEnv * _env, jobject _this, jstring jstr, jboolean saveToHistory)
{
	CRJNIEnv env(_env);
    DocViewNative * p = getNative(_env, _this);
    if (!p) {
    	CRLog::error("Cannot get native view");
    	return false;
    }
	if ( !p->_docview->isDocumentOpened() )
		return JNI_FALSE;
	DocViewCallback callback( _env, p->_docview, _this );
	lString16 str = env.fromJavaString(jstr);
    ldomXPointer bm = p->_docview->getDocument()->createXPointer(str);
	if ( bm.isNull() )
		return JNI_FALSE;
	if (saveToHistory)
		p->_docview->savePosToNavigationHistory();
	p->_docview->goToBookmark(bm);
	return JNI_TRUE;
}

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    getPositionPropsInternal
 * Signature: (Ljava/lang/String;)Lorg/coolreader/crengine/DocView/PositionProperties;
 */
JNIEXPORT jobject JNICALL Java_org_coolreader_crengine_DocView_getPositionPropsInternal
    (JNIEnv * _env, jobject _this, jstring _path)
{
    CRJNIEnv env(_env);
    DocViewNative * p = getNative(_env, _this);
    if (!p) {
    	CRLog::error("Cannot get native view");
    	return false;
    }

    jclass cls = _env->FindClass("org/coolreader/crengine/PositionProperties");
    jmethodID mid = _env->GetMethodID(cls, "<init>", "()V");
    jobject obj = _env->NewObject(cls, mid);

    if (!p->_docview->isDocumentOpened()) {
		CRLog::debug("getPositionPropsInternal: document is not opened");
		return obj;
	}
	DocViewCallback callback( _env, p->_docview, _this );
    lString16 str = env.fromJavaString(_path);
    ldomXPointer bm;
    bool useCurPos = false; // use current Y position for scroll view mode
    p->_docview->checkPos();
    if ( !str.empty() ) {
        bm = p->_docview->getDocument()->createXPointer(str);
    } else {
        useCurPos = p->_docview->getViewMode()==DVM_SCROLL;
        if ( !useCurPos ) {
            bm = p->_docview->getBookmark();
            if ( bm.isNull() ) {
                CRLog::error("getPositionPropsInternal: Cannot get current position bookmark");
            }
        }
    }
    CRObjectAccessor v(_env, obj);
    lvPoint pt = !bm.isNull() ? bm.toPoint() : lvPoint(0, p->_docview->GetPos());
    CRIntField(v,"x").set(pt.x);
    CRIntField(v,"y").set(pt.y);
    CRIntField(v,"fullHeight").set(p->_docview->GetFullHeight());
    CRIntField(v,"pageHeight").set(p->_docview->GetHeight());
    CRIntField(v,"pageWidth").set(p->_docview->GetWidth());
    CRIntField(v,"pageNumber").set(p->_docview->getCurPage());
    CRIntField(v,"pageCount").set(p->_docview->getPageCount());
    CRIntField(v,"pageMode").set(p->_docview->getViewMode()==DVM_PAGES ? p->_docview->getVisiblePageCount() : 0);
    CRIntField(v,"charCount").set(p->_docview->getCurrentPageCharCount());
    CRIntField(v,"imageCount").set(p->_docview->getCurrentPageImageCount());
    CRStringField(v,"pageText").set(p->_docview->getPageText(false, -1));
	return obj;
}

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    getTOCInternal
 * Signature: ()Lorg/coolreader/crengine/TOCItem;
 */
JNIEXPORT jobject JNICALL Java_org_coolreader_crengine_DocView_getTOCInternal
  (JNIEnv * _env, jobject _this)
{
    CRJNIEnv env(_env);
    DocViewNative * p = getNative(_env, _this);
    if (!p) {
    	CRLog::error("Cannot get native view");
    	return NULL;
    }
	if ( !p->_docview->isDocumentOpened() )
		return NULL;
	DocViewCallback callback( _env, p->_docview, _this );
	LVTocItem * toc = p->_docview->getToc();
	return env.toJavaTOCItem(toc);
}

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    clearSelectionInternal
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_DocView_clearSelectionInternal
  (JNIEnv * _env, jobject _this)
{
    CRJNIEnv env(_env);
    DocViewNative * p = getNative(_env, _this);
    if (!p) {
    	CRLog::error("Cannot get native view");
    	return;
    }
    if ( !p->_docview->isDocumentOpened() )
        return;
    p->clearSelection();
}

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    findTextInternal
 * Signature: (Ljava/lang/String;III)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_DocView_findTextInternal
  (JNIEnv * _env, jobject _this, jstring jpattern, jint origin, jint reverse, jint caseInsensitive)
{
    CRJNIEnv env(_env);
    DocViewNative * p = getNative(_env, _this);
    if (!p) {
    	CRLog::error("Cannot get native view");
    	return false;
    }
    if ( !p->_docview->isDocumentOpened() )
        return JNI_FALSE;
    return p->findText(env.fromJavaString(jpattern), origin, reverse, caseInsensitive);
}

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    setBatteryStateInternal
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_DocView_setBatteryStateInternal
  (JNIEnv * _env, jobject _this, jint state)
{
    CRJNIEnv env(_env);
    DocViewNative * p = getNative(_env, _this);
    if (!p) {
    	CRLog::error("Cannot get native view");
    	return;
    }
    p->_docview->setBatteryState(state);
}

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    swapToCacheInternal
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_coolreader_crengine_DocView_swapToCacheInternal
(JNIEnv * _env, jobject _this)
{
    CRJNIEnv env(_env);
    DocViewNative * p = getNative(_env, _this);
    if (!p) {
    	CRLog::error("Cannot get native view");
    	return 0;
    }
    CRTimerUtil timeout(60000); // 1 minute, can be cancelled by Engine.suspendContinuousOperationInternal()
    _timeoutControl = timeout;
    return p->_docview->updateCache(_timeoutControl);
}


/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    getCoverPageDataInternal
 * Signature: ()[B
 */
JNIEXPORT jbyteArray JNICALL Java_org_coolreader_crengine_DocView_getCoverPageDataInternal
  (JNIEnv * _env, jobject _this)
{
    CRJNIEnv env(_env);
    DocViewNative * p = getNative(_env, _this);
    if (!p) {
    	CRLog::error("Cannot get native view");
    	return NULL;
    }
//	CRLog::trace("getCoverPageDataInternal() : requesting cover image stream");
    LVStreamRef stream = p->_docview->getCoverPageImageStream();
//	CRLog::trace("getCoverPageDataInternal() : converting stream to byte array");
    jbyteArray array = env.streamToJByteArray(stream);
    if ( array!=NULL )
    	CRLog::debug("getCoverPageDataInternal() : returned cover page array");
    else
    	CRLog::debug("getCoverPageDataInternal() : cover page data not found");
    return array;
}

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    setPageBackgroundTextureInternal
 * Signature: ([BI)V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_DocView_setPageBackgroundTextureInternal
  (JNIEnv * _env, jobject _this, jbyteArray jdata, jint tileFlags )
{
    CRJNIEnv env(_env);
    DocViewNative * p = getNative(_env, _this);
    if (!p) {
    	CRLog::error("Cannot get native view");
    	return;
    }
    LVImageSourceRef img;
    if ( jdata!=NULL ) {
        LVStreamRef stream = env.jbyteArrayToStream( jdata );
        if ( !stream.isNull() ) {
            img = LVCreateStreamImageSource(stream);
        }
    }
    p->_docview->setBackgroundImage(img, tileFlags!=0);
}

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    updateSelectionInternal
 * Signature: (Lorg/coolreader/crengine/Selection;)V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_DocView_updateSelectionInternal
  (JNIEnv * _env, jobject _this, jobject _sel)
{
    CRJNIEnv env(_env);
    DocViewNative * p = getNative(_env, _this);
    if (!p) {
    	CRLog::error("Cannot get native view");
    	return;
    }
    CRObjectAccessor sel(_env, _sel);
    CRStringField sel_startPos(sel, "startPos");
    CRStringField sel_endPos(sel, "endPos");
    CRStringField sel_text(sel, "text");
    CRStringField sel_chapter(sel, "chapter");
    CRIntField sel_startX(sel, "startX");
    CRIntField sel_startY(sel, "startY");
    CRIntField sel_endX(sel, "endX");
    CRIntField sel_endY(sel, "endY");
    CRIntField sel_percent(sel, "percent");
    lvPoint startpt ( sel_startX.get(), sel_startY.get() );
    lvPoint endpt ( sel_endX.get(), sel_endY.get() );
	ldomXPointer startp = p->_docview->getNodeByPoint( startpt );
	ldomXPointer endp = p->_docview->getNodeByPoint( endpt );
    if ( !startp.isNull() && !endp.isNull() ) {
        ldomXRange r( startp, endp );
        if ( r.getStart().isNull() || r.getEnd().isNull() )
            return;
        r.sort();
		if ( !r.getStart().isVisibleWordStart() )
			r.getStart().prevVisibleWordStart();
		//lString16 start = r.getStart().toString();
		if ( !r.getEnd().isVisibleWordEnd() )
			r.getEnd().nextVisibleWordEnd();
        if ( r.isNull() )
            return;
        //lString16 end = r.getEnd().toString();
        //CRLog::debug("Range: %s - %s", UnicodeToUtf8(start).c_str(), UnicodeToUtf8(end).c_str());
        r.setFlags(1);
        p->_docview->selectRange( r );
        int page = p->_docview->getBookmarkPage(startp);
        int pages = p->_docview->getPageCount();
        lString16 titleText;
        lString16 posText;
        p->_docview->getBookmarkPosText(startp, titleText, posText);
        int percent = 0;
        if ( pages>1 )
        	percent = 10000 * page/(pages-1);
        lString16 selText = r.getRangeText( '\n', 8192 );
        sel_percent.set(percent);
    	sel_startPos.set( r.getStart().toString() );
    	sel_endPos.set( r.getEnd().toString() );
    	sel_text.set(selText);
    	sel_chapter.set(titleText);
    }

}

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    moveSelectionInternal
 * Signature: (Lorg/coolreader/crengine/Selection;II)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_DocView_moveSelectionInternal
  (JNIEnv * _env, jobject _this, jobject _sel, jint _cmd, jint _param)
{
    CRJNIEnv env(_env);
    DocViewNative * p = getNative(_env, _this);
    if (!p) {
    	CRLog::error("Cannot get native view");
    	return false;
    }
    CRObjectAccessor sel(_env, _sel);
    CRStringField sel_startPos(sel, "startPos");
    CRStringField sel_endPos(sel, "endPos");
    CRStringField sel_text(sel, "text");
    CRStringField sel_chapter(sel, "chapter");
    CRIntField sel_startX(sel, "startX");
    CRIntField sel_startY(sel, "startY");
    CRIntField sel_endX(sel, "endX");
    CRIntField sel_endY(sel, "endY");
    CRIntField sel_percent(sel, "percent");
    int res = p->_docview->doCommand( (LVDocCmd)_cmd, (int)_param );
    if ( res ) {
        ldomXRangeList & sel = p->_docview->getDocument()->getSelections();
        if ( sel.length()>0 ) {
            ldomXRange currSel;
            currSel = *sel[0];
            if ( !currSel.isNull() ) {
                sel_startPos.set( currSel.getStart().toString() );
                sel_endPos.set( currSel.getEnd().toString() );
                lvPoint startpt ( currSel.getStart().toPoint() );
                lvPoint endpt ( currSel.getEnd().toPoint() );
                sel_startX.set( startpt.x );
                sel_startY.set( startpt.y );
                sel_endX.set( endpt.x );
                sel_endY.set( endpt.y );

                int page = p->_docview->getBookmarkPage(currSel.getStart());
                int pages = p->_docview->getPageCount();
                lString16 titleText;
                lString16 posText;
                p->_docview->getBookmarkPosText(currSel.getStart(), titleText, posText);
                int percent = 0;
                if ( pages>1 )
                	percent = 10000 * page/(pages-1);
                lString16 selText = currSel.getRangeText( '\n', 8192 );
                sel_percent.set(percent);
            	sel_text.set(selText);
            	sel_chapter.set(titleText);

            	return JNI_TRUE;
            }
        }
    }
    return JNI_FALSE;
}


lString16 DocViewNative::getLink( int x, int y, int r )
{
	int step = 5;
	int n = r / step;
	r = n * step;
	if ( r==0 )
		return getLink(x, y);
	lString16 link;
	for ( int xx = -r; xx<=r; xx+=step ) {
		link = getLink( x+xx, y-r );
		if ( !link.empty() )
			return link;
		link = getLink( x+xx, y+r );
		if ( !link.empty() )
			return link;
	}
	for ( int yy = -r + step; yy<=r - step; yy+=step ) {
		link = getLink( x+r, y+yy );
		if ( !link.empty() )
			return link;
		link = getLink( x-r, y+yy );
		if ( !link.empty() )
			return link;
	}
	return lString16::empty_str;
}

lString16 DocViewNative::getLink( int x, int y )
{
	ldomXPointer p = _docview->getNodeByPoint( lvPoint(x, y) );
	if ( p.isNull() )
		return lString16::empty_str;
	lString16 href = p.getHRef();
	return href;
}

#define MAX_IMAGE_BUF_SIZE 1200000

// checks whether point belongs to image: if found, returns true, and _currentImage is set to image
bool DocViewNative::checkImage(int x, int y, int bufWidth, int bufHeight, int &dx, int &dy, bool & needRotate)
{
	_currentImage = _docview->getImageByPoint(lvPoint(x, y));
	if (_currentImage.isNull())
		return false;
	dx = _currentImage->GetWidth();
	dy = _currentImage->GetHeight();
	if (dx < 8 && dy < 8) {
		_currentImage.Clear();
		return JNI_FALSE;
	}
	needRotate = false;
	if (bufWidth <= bufHeight) {
		// screen == portrait
		needRotate = 8 * dx > 10 * dy;
	} else {
		// screen == landscape
		needRotate = 10 * dx < 8 * dy;
	}
	int scale = 1;
	if (dx * dy > MAX_IMAGE_BUF_SIZE) {
		scale = dx * dy /  MAX_IMAGE_BUF_SIZE;
		dx /= scale;
		dy /= scale;
	}
	LVColorDrawBuf * buf = new LVColorDrawBuf(dx, dy);
	buf->Clear(0xFF000000); // transparent
	buf->Draw(_currentImage, 0, 0, dx, dy, false);
	if (needRotate) {
		int c = dx;
		dx = dy;
		dy = c;
		buf->Rotate(CR_ROTATE_ANGLE_90);
	}
	_currentImage = LVCreateDrawBufImageSource(buf, true);
	return true;
}

// draws current image to buffer (scaled, panned)
bool DocViewNative::drawImage(LVDrawBuf * buf, int x, int y, int dx, int dy)
{
	if (_currentImage.isNull())
		return false;
	return _docview->drawImage(buf, _currentImage, x, y, dx, dy);
}

// draws icon to buffer
bool DocViewNative::drawIcon(LVDrawBuf * buf, lvRect & rc, int type) {
	rc.shrink(rc.width() / 7);
	lUInt32 light = 0x60C0C0C0;
	lUInt32 dark = 0x80606060;
	lUInt32 colors[2];
	colors[0] = dark;
	colors[1] = light;
	// x0  x1  x2  x3
	int x0 = rc.left;
	int x1 = rc.left + rc.width() * 4 / 10;
	int x2 = rc.right -  + rc.width() * 4 / 10;
	int x3 = rc.right;
	int y0 = rc.top;
	int y1 = rc.top + rc.width() * 4 / 10;
	int y2 = rc.bottom -  + rc.height() * 4 / 10;
	int y3 = rc.bottom;
	for (int w = 1; w>=0; w--) {
		if (type == 1) {
			// horizontal minus
			buf->FillRect(x0-w, y1-w, x3+w+1, y1+w+1, colors[w]);
			buf->FillRect(x0-w, y2-w, x3+w+1, y2+w+1, colors[w]);
			buf->FillRect(x0-w, y1-w, x0+w+1, y2+w+1, colors[w]);
			buf->FillRect(x3-w, y1-w, x3+w+1, y2+w+1, colors[w]);
		} else if (type == 2) {
			// vertical minus
			buf->FillRect(x1-w, y0-w, x1+w+1, y3+w+1, colors[w]);
			buf->FillRect(x2-w, y0-w, x2+w+1, y3+w+1, colors[w]);
			buf->FillRect(x1-w, y0-w, x2+w+1, y0+w+1, colors[w]);
			buf->FillRect(x1-w, y3-w, x2+w+1, y3+w+1, colors[w]);
		} else {
			// plus
			buf->FillRect(x0-w, y1-w, x1+w+1, y1+w+1, colors[w]);
			buf->FillRect(x1-w, y0-w, x1+w+1, y1+w+1, colors[w]);
			buf->FillRect(x0-w, y1-w, x0+w+1, y2+w+1, colors[w]);
			buf->FillRect(x1-w, y0-w, x2+w+1, y0+w+1, colors[w]);
			buf->FillRect(x2-w, y0-w, x2+w+1, y1+w+1, colors[w]);
			buf->FillRect(x2-w, y1-w, x3+w+1, y1+w+1, colors[w]);
			buf->FillRect(x3-w, y1-w, x3+w+1, y2+w+1, colors[w]);
			buf->FillRect(x2-w, y2-w, x3+w+1, y2+w+1, colors[w]);
			buf->FillRect(x2-w, y2-w, x2+w+1, y3+w+1, colors[w]);
			buf->FillRect(x1-w, y3-w, x2+w+1, y3+w+1, colors[w]);
			buf->FillRect(x1-w, y2-w, x1+w+1, y3+w+1, colors[w]);
			buf->FillRect(x0-w, y2-w, x1+w+1, y2+w+1, colors[w]);
		}
	}
	return true;
}

// sets current image to null
bool DocViewNative::closeImage() {
	if (_currentImage.isNull())
		return false;
	_currentImage.Clear();
	return true;
}

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    checkLinkInternal
 * Signature: (III)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_coolreader_crengine_DocView_checkLinkInternal
  (JNIEnv * _env, jobject _this, jint x, jint y, jint delta)
{
    CRJNIEnv env(_env);
    DocViewNative * p = getNative(_env, _this);
    if (!p) {
    	CRLog::error("Cannot get native view");
    	return false;
    }
    lString16 link;
    for ( int r=0; r<=delta; r+=5 ) {
    	link = p->getLink(x, y, r);
    	if ( !link.empty() )
    		return env.toJavaString(link);
    }
    return NULL;
}

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    goLinkInternal
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_coolreader_crengine_DocView_goLinkInternal
  (JNIEnv * _env, jobject _this, jstring _link)
{
    CRJNIEnv env(_env);
    DocViewNative * p = getNative(_env, _this);
    if (!p) {
    	CRLog::error("Cannot get native view");
    	return 0;
    }
    lString16 link = env.fromJavaString(_link);
    bool res = p->_docview->goLink( link, true );
    return res ? 1 : 0;
}

/*
 * Class:     org_coolreader_crengine_DocView
 * Method:    hilightBookmarksInternal
 * Signature: ([Lorg/coolreader/crengine/Bookmark;)V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_DocView_hilightBookmarksInternal
  (JNIEnv * _env, jobject _this, jobjectArray list) {
    CRJNIEnv env(_env);
    DocViewNative * p = getNative(_env, _this);
    if (!p) {
    	CRLog::error("Cannot get native view");
    	return;
    }
    LVPtrVector<CRBookmark> bookmarks;
    if (list) {
    	int len = _env->GetArrayLength(list);
    	for (int i=0; i<len; i++) {
    		jobject obj = _env->GetObjectArrayElement(list, i);
    	    CRObjectAccessor bmk(_env, obj);
    	    CRStringField startPos(bmk, "startPos");
    	    CRStringField endPos(bmk, "endPos");
    	    CRIntField type(bmk, "type");
    	    CRBookmark * bookmark = new CRBookmark(startPos.get(), endPos.get());
    	    bookmark->setType(type.get());
    	    bookmarks.add(bookmark);
    	    env->DeleteLocalRef(obj);
    	}
    }
    p->_docview->setBookmarkList(bookmarks);
}

