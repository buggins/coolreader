/*
 * CoolReader 3 Java Port helpers.
 */

#ifndef CR3_JAVA_H
#define CR3_JAVA_H

#include <jni.h>
#include <android/log.h>

#define  LOG_TAG    "cr3eng"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  LOGV(...)  __android_log_print(ANDROID_LOG_VERBOSE,LOG_TAG,__VA_ARGS__)
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGASSERTFAILED(cond,...)  __android_log_assert(cond,LOG_TAG,__VA_ARGS__)

#include "lvstring.h"
#include "lvdrawbuf.h"

//====================================================================
#ifdef USE_JNIGRAPHICS
#include <android/bitmap.h>
#else
// libjnigraphics replacement for pre-2.2 SDKs 
enum AndroidBitmapFormat {
    ANDROID_BITMAP_FORMAT_NONE      = 0,
    ANDROID_BITMAP_FORMAT_RGBA_8888 = 1,
    ANDROID_BITMAP_FORMAT_RGB_565   = 4,
    ANDROID_BITMAP_FORMAT_RGBA_4444 = 7,
    ANDROID_BITMAP_FORMAT_A_8       = 8,
};
#endif
//====================================================================

class CRJNIEnv {
protected:
	JNIEnv * env;
public:
    CRJNIEnv(JNIEnv * pEnv) : env(pEnv) { }
    JNIEnv * operator -> () { return env; }
	lString16 fromJavaString( jstring str );
	jstring toJavaString( const lString16 & str );
	void fromJavaStringArray( jobjectArray array, lString16Collection & dst );
	jobjectArray toJavaStringArray( lString16Collection & dst );
	jobject enumByNativeId( const char * classname, int id ); 
};

class BitmapAccessor : public CRJNIEnv {
private:
    jobject bitmap;
	int width;
	int height;
	int format;
	int stride;
	lUInt8 * pixels;
#ifndef USE_JNIGRAPHICS
    jintArray array; 
#endif
public:
	int getWidth() { return width; }
	int getHeight() { return height; }
	int getFormat() { return format; }
    void draw(LVDrawBuf * buf, int x, int y); 
    void setRowRGB( int x, int y, lUInt32 * rgb, int dx ); 
    void setRowGray( int x, int y, lUInt8 * gray, int dx, int bpp ); 
	bool isOk();
	BitmapAccessor( JNIEnv * pEnv, jobject bmp );
	~BitmapAccessor();
};

#endif
