// CoolReader3 Engine JNI interface
// BASED on Android NDK Plasma example

#include <jni.h>
#include <time.h>
#include <android/log.h>
//#include <android/bitmap.h>

#include <stdio.h>
#include <stdlib.h>

#include "org_coolreader_crengine_Engine.h"
#include "org_coolreader_crengine_ReaderView.h"

#include "cr3java.h"
#include "crengine.h"


#define  LOG_TAG    "cr3eng"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  LOGV(...)  __android_log_print(ANDROID_LOG_VERBOSE,LOG_TAG,__VA_ARGS__)
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)

//====================================================================
// libjnigraphics replacement for pre-2.2 SDKs 
#define ANDROID_BITMAP_RESUT_SUCCESS            0
#define ANDROID_BITMAP_RESULT_BAD_PARAMETER     -1
#define ANDROID_BITMAP_RESULT_JNI_EXCEPTION     -2
#define ANDROID_BITMAP_RESULT_ALLOCATION_FAILED -3

enum AndroidBitmapFormat {
    ANDROID_BITMAP_FORMAT_NONE      = 0,
    ANDROID_BITMAP_FORMAT_RGBA_8888 = 1,
    ANDROID_BITMAP_FORMAT_RGB_565   = 4,
    ANDROID_BITMAP_FORMAT_RGBA_4444 = 7,
    ANDROID_BITMAP_FORMAT_A_8       = 8,
};

typedef struct {
    uint32_t    width;
    uint32_t    height;
    uint32_t    stride;
    int32_t     format;
//    uint32_t    flags;      // 0 for now
} myAndroidBitmapInfo;

/**
 * Given a java bitmap object, fill out the AndroidBitmap struct for it.
 * If the call fails, the info parameter will be ignored
 */
int myAndroidBitmap_getInfo(JNIEnv* env, jobject jbitmap,
                          myAndroidBitmapInfo* info)
{
	jclass cls = env->GetObjectClass(jbitmap);
	jmethodID mid;
	mid = env->GetMethodID(cls,	"getHeight", "()I");
	info->height = env->CallIntMethod(jbitmap, mid);	
	mid = env->GetMethodID(cls,	"getWidth", "()I");
	info->width = env->CallIntMethod(jbitmap, mid);	
	mid = env->GetMethodID(cls,	"getRowBytes", "()I");
	info->stride = env->CallIntMethod(jbitmap, mid);	
	mid = env->GetMethodID(cls,	"getConfig", "()Landroid/graphics/Bitmap$Config;");
	jobject configObj = env->CallObjectMethod(jbitmap, mid);	
	jclass configCls = env->GetObjectClass(configObj);
	mid = env->GetMethodID(configCls, "ordinal", "()I");
	info->width = env->CallIntMethod(configObj, mid);
	return ANDROID_BITMAP_RESUT_SUCCESS;	
}

/**
 * Given a java bitmap object, attempt to lock the pixel address.
 * Locking will ensure that the memory for the pixels will not move
 * until the unlockPixels call, and ensure that, if the pixels had been
 * previously purged, they will have been restored.
 *
 * If this call succeeds, it must be balanced by a call to
 * AndroidBitmap_unlockPixels, after which time the address of the pixels should
 * no longer be used.
 *
 * If this succeeds, *addrPtr will be set to the pixel address. If the call
 * fails, addrPtr will be ignored.
 */
int myAndroidBitmap_lockPixels(JNIEnv* env, jobject jbitmap, void** addrPtr)
{
	jclass cls = env->GetObjectClass(jbitmap);
	jmethodID mid;
	mid = env->GetMethodID(cls,	"getHeight", "()I");
	return ANDROID_BITMAP_RESULT_BAD_PARAMETER;
}

/**
 * Call this to balanace a successful call to AndroidBitmap_lockPixels
 */
int myAndroidBitmap_unlockPixels(JNIEnv* env, jobject jbitmap)
{
	return ANDROID_BITMAP_RESULT_BAD_PARAMETER;
}
//====================================================================

class JNICDRLogger : public CRLog
{
public:
    JNICDRLogger()
    {
    	curr_level = CRLog::LL_DEBUG;
    }
protected:
  
	virtual void log( const char * lvl, const char * msg, va_list args)
	{
	    #define MAX_LOG_MSG_SIZE 1024
		static char buffer[MAX_LOG_MSG_SIZE+1];
		vsnprintf(buffer, MAX_LOG_MSG_SIZE, msg, args);
		int level = ANDROID_LOG_DEBUG;
		//LOGD("CRLog::log is called with LEVEL %s, pattern %s", lvl, msg);
		if ( !strcmp(lvl, "FATAL") )
			level = ANDROID_LOG_FATAL;
		else if ( !strcmp(lvl, "ERROR") )
			level = ANDROID_LOG_ERROR;
		else if ( !strcmp(lvl, "WARN") )
			level = ANDROID_LOG_WARN;
		else if ( !strcmp(lvl, "INFO") )
			level = ANDROID_LOG_INFO;
		else if ( !strcmp(lvl, "DEBUG") )
			level = ANDROID_LOG_DEBUG;
		else if ( !strcmp(lvl, "TRACE") )
			level = ANDROID_LOG_VERBOSE;
		__android_log_write(level, LOG_TAG, buffer);
	}
};

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    initInternal
 * Signature: ([Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_Engine_initInternal
  (JNIEnv * penv, jobject obj, jobjectArray fontArray)
{
	LOGI("initInternal called");
	CRJNIEnv env(penv);
	LOGD("Redirecting CDRLog to Android");
	CRLog::setLogger( new JNICDRLogger() );
	CRLog::setLogLevel( CRLog::LL_TRACE );
	CRLog::info("CREngine log redirected");
	LOGI("creating font manager");
	InitFontManager(lString8());
	CRLog::debug("converting fonts array: %d items", (int)env->GetArrayLength(fontArray));
	lString16Collection fonts;
	env.fromJavaStringArray(fontArray, fonts);
	int len = fonts.length();
	CRLog::debug("registering fonts: %d fonts in list", len);
	for ( int i=0; i<len; i++ ) {
		lString8 fontName = UnicodeToUtf8(fonts[i]);
		CRLog::debug("registering font %s", fontName.c_str());
		if ( !fontMan->RegisterFont( fontName ) )
			LOGE("cannot load font %s", fontName.c_str());
	}
	LOGI("%d fonts registered", (int)fontMan->GetFontCount());
	return fontMan->GetFontCount() ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    uninitInternal
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_Engine_uninitInternal
  (JNIEnv *, jobject)
{
	LOGI("uninitInternal called");
	HyphMan::uninit();
	ShutdownFontManager();
	CRLog::setLogger(NULL);
}

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    getFontFaceListInternal
 * Signature: ()[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_org_coolreader_crengine_Engine_getFontFaceListInternal
  (JNIEnv * penv, jobject obj)
{
	LOGI("getFontFaceListInternal called");
	CRJNIEnv env(penv);
	lString16Collection list;
	fontMan->getFaceList(list);
	return env.toJavaStringArray(list);
}

/*
 * Class:     org_coolreader_crengine_ReaderView
 * Method:    getPageImage
 * Signature: (Landroid/graphics/Bitmap;)V
 */
JNIEXPORT void JNICALL Java_org_coolreader_crengine_ReaderView_getPageImage
  (JNIEnv * env, jclass cls, jobject bitmap)
{
    myAndroidBitmapInfo  info;
    void*              pixels;
    int                ret;
    static int         init;

    if (!init) {
        init = 1;
    }

    if ((ret = myAndroidBitmap_getInfo(env, bitmap, &info)) < 0) {
        LOGE("AndroidBitmap_getInfo() failed ! error=%d", ret);
        return;
    }

    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
        LOGE("Bitmap format is not RGBA_8888, it's %d !", info.format);
        return;
    }

//    if ((ret = myAndroidBitmap_lockPixels(env, bitmap, &pixels)) < 0) {
//        LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
//    }

//    myAndroidBitmap_unlockPixels(env, bitmap);
}



//=====================================================================

static JNINativeMethod sEngineMethods[] = {
  {"initInternal", "([Ljava/lang/String;)Z", (void*)Java_org_coolreader_crengine_Engine_initInternal},
  {"uninitInternal", "()V", (void*)Java_org_coolreader_crengine_Engine_uninitInternal},
  {"getFontFaceListInternal", "()[Ljava/lang/String", (void*)Java_org_coolreader_crengine_Engine_getFontFaceListInternal},
};


static JNINativeMethod sReaderViewMethods[] = {
  /* name, signature, funcPtr */
  {"getPageImage", "(Landroid/graphics/Bitmap;)V", (void*)Java_org_coolreader_crengine_ReaderView_getPageImage},
};
 
/*
 * Register native JNI-callable methods.
 *
 * "className" looks like "java/lang/String".
 */
static int jniRegisterNativeMethods(JNIEnv* env, const char* className,
    const JNINativeMethod* gMethods, int numMethods)
{
    jclass clazz;

    LOGV("Registering %s natives\n", className);
    clazz = env->FindClass(className);
    if (clazz == NULL) {
        LOGE("Native registration unable to find class '%s'\n", className);
        return -1;
    }
    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
        LOGE("RegisterNatives failed for '%s'\n", className);
        return -1;
    }
    return 0;
}


jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
   JNIEnv* env = NULL;
   jint result = -1;
 
   if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
      return result;
   }
 
   jniRegisterNativeMethods(env, "org/coolreader/crengine/Engine", sEngineMethods, 1);
   jniRegisterNativeMethods(env, "org/coolreader/crengine/ReaderView", sReaderViewMethods, 1);
   
   return JNI_VERSION_1_4;
}
