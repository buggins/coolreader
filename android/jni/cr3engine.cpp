// CoolReader3 Engine JNI interface
// BASED on Android NDK Plasma example

#include <jni.h>
#include <time.h>
#include <android/log.h>
//#include <android/bitmap.h>

#include <stdio.h>
#include <stdlib.h>

#define  LOG_TAG    "libcr3engine"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

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



JNIEXPORT void JNICALL Java_org_coolreader_crengine_CRView_getPageImage(JNIEnv * env, jobject  obj, jobject bitmap)
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
        LOGE("Bitmap format is not RGBA_8888 !");
        return;
    }

    if ((ret = myAndroidBitmap_lockPixels(env, bitmap, &pixels)) < 0) {
        LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
    }

    myAndroidBitmap_unlockPixels(env, bitmap);
}
