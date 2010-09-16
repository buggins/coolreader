#include "readerview.h"
#include "lvdocview.h"

static jfieldID gNativeObjectID = 0;

class DocViewCallback : public LVDocViewCallback {
	CRJNIEnv _env;
	LVDocView * _docview;
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
public:
	DocViewCallback( JNIEnv * env, LVDocView * docview, jobject obj )
	: _env(env), _docview(docview)
	{
		CRLog::info("DocViewCallback() getting object class");
		jclass objclass = _env->GetObjectClass(obj);
		//CRLog::trace("DocViewCallback() getting object readerCallback field");
		jfieldID fid = _env->GetFieldID(objclass, "readerCallback", "Lorg/coolreader/crengine/ReaderCallback;");
		//CRLog::trace("DocViewCallback() getting readerCallback field value");
		_obj = _env->GetObjectField(obj, fid); 
		//_class = _env->FindClass("org/coolreader/engine/ReaderCallback");
		//CRLog::trace("DocViewCallback() getting readerCallback field class");
		_class = _env->GetObjectClass(_obj);
		#define GET_METHOD(n,sign) \
		     _ ## n = _env->GetMethodID(_class, # n, sign)   
		//CRLog::trace("DocViewCallback() getting interface methods");
		GET_METHOD(OnLoadFileStart,"(Ljava/lang/String;)V");
	    GET_METHOD(OnLoadFileFormatDetected,"(Lorg/coolreader/crengine/DocumentFormat;)Ljava/lang/String;");
	    GET_METHOD(OnLoadFileEnd,"()V");
	    GET_METHOD(OnLoadFileFirstPagesReady,"()V");
	    GET_METHOD(OnLoadFileProgress,"(I)Z");
	    GET_METHOD(OnFormatStart,"()V");
	    GET_METHOD(OnFormatEnd,"()V");
	    GET_METHOD(OnFormatProgress,"(I)Z");
	    GET_METHOD(OnExportProgress,"(I)Z");
	    GET_METHOD(OnLoadFileError,"(Ljava/lang/String;)V");
	    GET_METHOD(OnExternalLink,"(Ljava/lang/String;Ljava/lang/String;)V");
		//CRLog::info("DocViewCallback() setting callback");
		_docview->setCallback( this );
	}
	virtual ~DocViewCallback()
	{
		_docview->setCallback( NULL );
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
};


ReaderViewNative::ReaderViewNative()
{
	_docview = new LVDocView(32); //32bpp
	_docview->setFontSize(24);
	_docview->createDefaultDocument(lString16("Welcome to CoolReader"), lString16("Please select file to open"));
}

static ReaderViewNative * getNative(JNIEnv * env, jobject _this)
{
    return (ReaderViewNative *)env->GetIntField(_this, gNativeObjectID); 
}

/*
 * Class:     org_coolreader_crengine_ReaderView
 * Method:    createInternal
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_ReaderView_createInternal
  (JNIEnv * env, jobject _this)
{
	CRLog::info("Creating new RenderView");
    jclass rvClass = env->FindClass("org/coolreader/crengine/ReaderView");
    gNativeObjectID = env->GetFieldID(rvClass, "mNativeObject", "I");
    ReaderViewNative * obj = new ReaderViewNative();
    env->SetIntField(_this, gNativeObjectID, (jint)obj);
    obj->_docview->setFontSize(24); 
}

/*
 * Class:     org_coolreader_crengine_ReaderView
 * Method:    destroyInternal
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_ReaderView_destroyInternal
  (JNIEnv * env, jobject view)
{
    ReaderViewNative * p = getNative(env, view);
    if ( p!=NULL ) {
		CRLog::info("Destroying RenderView");
    	delete p;
	    jclass rvClass = env->FindClass("org/coolreader/crengine/ReaderView");
	    gNativeObjectID = env->GetFieldID(rvClass, "mNativeObject", "I");
	    env->SetIntField(view, gNativeObjectID, 0);
	} else {
		CRLog::error("RenderView is already destroyed");
	}
}

/*
 * Class:     org_coolreader_crengine_ReaderView
 * Method:    getPageImage
 * Signature: (Landroid/graphics/Bitmap;)V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_ReaderView_getPageImage
  (JNIEnv * env, jobject view, jobject bitmap)
{
    ReaderViewNative * p = getNative(env, view);
    //CRLog::info("Initialize callback");
	DocViewCallback callback( env, p->_docview, view );	
    //CRLog::info("Initialized callback");
	BitmapAccessor bmp(env,bitmap);
	if ( bmp.isOk() ) {
	    LVDocImageRef img = p->_docview->getPageImage(0);
	    LVDrawBuf * drawbuf = img->getDrawBuf();
	    bmp.draw( drawbuf, 0, 0 );
	    
//		int dx = bmp.getWidth();
		//int dy = bmp.getHeight();
		//LVColorDrawBuf buf( dx, dy );
		//buf.FillRect(0, 0, dx, dy, 0xFF8000);
		//buf.FillRect(10, 10, dx-20, dy-20, 0xC0C0FF);
		//buf.FillRect(20, 20, 30, 30, 0x0000FF);
		//LVFontRef fnt = fontMan->GetFont(30, 400, false, css_ff_sans_serif, lString8("Droid Sans"));
		//fnt->DrawTextString(&buf, 40, 40, L"Text 1", 6, '?', NULL, false, 0, 0);
		//fnt->DrawTextString(&buf, 40, 90, L"Text 2", 6, '?', NULL, false, 0, 0);
		//bmp.draw( &buf, 0, 0 );		
//		lUInt32 row[500];
//		for ( int y=0; y<300; y++ ) {
//			for ( int x=0; x<500; x++ ) {
//				row[x] = x*5 + y*30;
//			}
//			bmp.setRowRGB( 0, y, row, 500 );
//		}
	} else {
		CRLog::error("bitmap accessor is invalid");
	}
}

/*
 * Class:     org_coolreader_crengine_ReaderView
 * Method:    loadDocument
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_ReaderView_loadDocumentInternal
  (JNIEnv * _env, jobject _this, jstring s)
{
	CRJNIEnv env(_env);
    ReaderViewNative * p = getNative(_env, _this);
    bool res;
    {
        //CRLog::debug("loadDocumentInternal: Before callback instantiate");
		DocViewCallback callback( _env, p->_docview, _this );
		//CRLog::debug("loadDocumentInternal: After callback instantiate");
		lString16 str = env.fromJavaString(s);
		CRLog::info("Loading document %s", LCSTR(str));
		res = p->_docview->LoadDocument(str.c_str());
		CRLog::info("Document %s is loaded %s", LCSTR(str), (res?"successfully":"with error"));
    }
    if ( p->_docview->isDocumentOpened() )
    	p->_docview->restorePosition();
    return res ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     org_coolreader_crengine_ReaderView
 * Method:    getSettings
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_coolreader_crengine_ReaderView_getSettingsInternal
  (JNIEnv *, jobject)
{
    return NULL;
}

/*
 * Class:     org_coolreader_crengine_ReaderView
 * Method:    applySettings
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_ReaderView_applySettingsInternal
  (JNIEnv *, jobject, jstring)
{
    return false;
}

/*
 * Class:     org_coolreader_crengine_ReaderView
 * Method:    readHistory
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_ReaderView_readHistoryInternal
  (JNIEnv * _env, jobject _this, jstring jFilename)
{
	CRJNIEnv env(_env);
    ReaderViewNative * p = getNative(_env, _this);
    CRFileHist * hist = p->_docview->getHistory();
    lString16 filename = env.fromJavaString(jFilename);
    LVStreamRef stream = LVOpenFileStream(filename.c_str(), LVOM_READ);
    if ( stream.isNull() )
    	return JNI_FALSE;
    bool res = hist->loadFromStream( stream );
    return res?JNI_TRUE:JNI_FALSE;
}

/*
 * Class:     org_coolreader_crengine_ReaderView
 * Method:    writeHistory
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_ReaderView_writeHistoryInternal
  (JNIEnv * _env, jobject _this, jstring jFilename)
{
	CRJNIEnv env(_env);
    ReaderViewNative * p = getNative(_env, _this);
    CRFileHist * hist = p->_docview->getHistory();
    lString16 filename = env.fromJavaString(jFilename);
    LVStreamRef stream = LVOpenFileStream(filename.c_str(), LVOM_WRITE);
    if ( stream.isNull() )
    	return JNI_FALSE;
    if ( p->_docview->isDocumentOpened() )
    	p->_docview->savePosition();
    bool res = hist->saveToStream( stream.get() );
    return res?JNI_TRUE:JNI_FALSE;
}

/*
 * Class:     org_coolreader_crengine_ReaderView
 * Method:    setStylesheet
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_ReaderView_setStylesheetInternal
  (JNIEnv * _env, jobject _view, jstring jcss)
{
	CRJNIEnv env(_env);
    ReaderViewNative * p = getNative(_env, _view);
    lString8 css8 = UnicodeToUtf8(env.fromJavaString(jcss));
    p->_docview->setStyleSheet(css8);
}

/*
 * Class:     org_coolreader_crengine_ReaderView
 * Method:    resize
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_ReaderView_resizeInternal
  (JNIEnv * _env, jobject _this, jint dx, jint dy)
{
    ReaderViewNative * p = getNative(_env, _this);
    p->_docview->Resize(dx, dy);
}  
  

/*
 * Class:     org_coolreader_crengine_ReaderView
 * Method:    doCommand
 * Signature: (II)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_ReaderView_doCommandInternal
  (JNIEnv * _env, jobject _this, jint cmd, jint param)
{
    ReaderViewNative * p = getNative(_env, _this);
    p->_docview->doCommand((LVDocCmd)cmd, param);
    return true;
}

/*
 * Class:     org_coolreader_crengine_ReaderView
 * Method:    getState
 * Signature: ()Lorg/coolreader/crengine/ReaderView/DocumentInfo;
 */
JNIEXPORT jobject JNICALL Java_org_coolreader_crengine_ReaderView_getStateInternal
  (JNIEnv *, jobject)
{
    return NULL;
}
  
