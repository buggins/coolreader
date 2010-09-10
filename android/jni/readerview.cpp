#include "readerview.h"
#include "lvdocview.h"

static jfieldID gNativeObjectID = 0;

ReaderViewNative::ReaderViewNative()
{
	_docview = new LVDocView(32); //32bpp
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
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_ReaderView_loadDocument
  (JNIEnv * _env, jobject _this, jstring s)
{
	CRJNIEnv env(_env);
    ReaderViewNative * p = getNative(_env, _this);
    lString16 str = env.fromJavaString(s);
	CRLog::info("Loading document %s", LCSTR(str));
    return p->_docview->LoadDocument(str.c_str());
}

/*
 * Class:     org_coolreader_crengine_ReaderView
 * Method:    getSettings
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_coolreader_crengine_ReaderView_getSettings
  (JNIEnv *, jobject)
{
    return NULL;
}

/*
 * Class:     org_coolreader_crengine_ReaderView
 * Method:    applySettings
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_ReaderView_applySettings
  (JNIEnv *, jobject, jstring)
{
    return false;
}

/*
 * Class:     org_coolreader_crengine_ReaderView
 * Method:    readHistory
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_ReaderView_readHistory
  (JNIEnv *, jobject, jstring)
{
    return false;
}

/*
 * Class:     org_coolreader_crengine_ReaderView
 * Method:    writeHistory
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_ReaderView_writeHistory
  (JNIEnv *, jobject, jstring)
{
    return false;
}

/*
 * Class:     org_coolreader_crengine_ReaderView
 * Method:    setStylesheet
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_ReaderView_setStylesheet
  (JNIEnv *, jobject, jstring)
{
}

/*
 * Class:     org_coolreader_crengine_ReaderView
 * Method:    resize
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_ReaderView_resize
  (JNIEnv *, jobject, jint, jint)
{
}  
  

/*
 * Class:     org_coolreader_crengine_ReaderView
 * Method:    doCommand
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_ReaderView_doCommand
  (JNIEnv *, jobject, jint)
{
    return false;
}

/*
 * Class:     org_coolreader_crengine_ReaderView
 * Method:    getState
 * Signature: ()Lorg/coolreader/crengine/ReaderView/DocumentInfo;
 */
JNIEXPORT jobject JNICALL Java_org_coolreader_crengine_ReaderView_getState
  (JNIEnv *, jobject)
{
    return NULL;
}
  