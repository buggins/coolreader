#include "cr3java.h"

lString16 CRJNIEnv::fromJavaString( jstring str )
{
	jboolean iscopy;
	const char * s = env->GetStringUTFChars( str, &iscopy );
	lString16 res(s);
	env->ReleaseStringUTFChars(str, s);
	return res;
}

jstring CRJNIEnv::toJavaString( const lString16 & str )
{
	return env->NewStringUTF(UnicodeToUtf8(str).c_str());
}

void CRJNIEnv::fromJavaStringArray( jobjectArray array, lString16Collection & dst )
{
	dst.clear();
	int len = env->GetArrayLength(array);
	for ( int i=0; i<len; i++ ) {
		jstring str = (jstring)env->GetObjectArrayElement(array, i);
		dst.add(fromJavaString(str));
	}
}

jobjectArray CRJNIEnv::toJavaStringArray( lString16Collection & src )
{
    int len = src.length();
	jobjectArray array = env->NewObjectArray(len, env->FindClass("java/lang/String"), env->NewStringUTF(""));
	for ( int i=0; i<len; i++ ) {
		env->SetObjectArrayElement(array, i, toJavaString(src[i]));
	}
	return array;
}



#ifndef USE_JNIGRAPHICS
//====================================================================
// libjnigraphics replacement for pre-2.2 SDKs 
#define ANDROID_BITMAP_RESUT_SUCCESS            0
#define ANDROID_BITMAP_RESULT_BAD_PARAMETER     -1
#define ANDROID_BITMAP_RESULT_JNI_EXCEPTION     -2
#define ANDROID_BITMAP_RESULT_ALLOCATION_FAILED -3

typedef struct {
    uint32_t    width;
    uint32_t    height;
    uint32_t    stride;
    int32_t     format;
//    uint32_t    flags;      // 0 for now
} AndroidBitmapInfo;

/**
 * Given a java bitmap object, fill out the AndroidBitmap struct for it.
 * If the call fails, the info parameter will be ignored
 */
int AndroidBitmap_getInfo(JNIEnv* env, jobject jbitmap,
                          AndroidBitmapInfo* info)
{
	jclass cls = env->GetObjectClass(jbitmap);
	jmethodID mid;
	mid = env->GetMethodID(cls,	"getHeight", "()I");
	info->height = env->CallIntMethod(jbitmap, mid);	
	//CRLog::debug("Bitmap height: %d", info->height);
	mid = env->GetMethodID(cls,	"getWidth", "()I");
	info->width = env->CallIntMethod(jbitmap, mid);	
	//CRLog::debug("Bitmap width: %d", info->width);
	mid = env->GetMethodID(cls,	"getRowBytes", "()I");
	info->stride = env->CallIntMethod(jbitmap, mid);	
	//CRLog::debug("Bitmap stride: %d", info->stride);
	mid = env->GetMethodID(cls,	"getConfig", "()Landroid/graphics/Bitmap$Config;");
	jobject configObj = env->CallObjectMethod(jbitmap, mid);	
	jclass configCls = env->GetObjectClass(configObj);
	//mid = env->GetMethodID(configCls, "ordinal", "()I");
	//info->format = env->CallIntMethod(configObj, mid);
	jfieldID fid;
	fid = env->GetFieldID(configCls, "nativeInt", "I");
	info->format = env->GetIntField(configObj, fid);
	//CRLog::debug("Bitmap format: %d", info->format);
	return ANDROID_BITMAP_RESUT_SUCCESS;	
}
#endif

bool BitmapAccessor::isOk()
{
	return pixels!=NULL;
}

BitmapAccessor::BitmapAccessor( JNIEnv * pEnv, jobject bmp )
: CRJNIEnv(pEnv), bitmap(bmp), width(0), height(0), format(0), stride(0), pixels(NULL)
{
	AndroidBitmapInfo info;
	if ( ANDROID_BITMAP_RESUT_SUCCESS==AndroidBitmap_getInfo(env, bitmap, &info) ) {
		width = info.width;
		height = info.height;
		stride = info.stride;
		format = info.format;
	}
#ifdef USE_JNIGRAPHICS
	if ( ANDROID_BITMAP_RESUT_SUCCESS!=AndroidBitmap_lockPixels(env, bitmap, &pixels) ) {
        CRLog::error("AndroidBitmap_lockPixels failed");
	    pixels = NULL;
	}
#else
	int size = stride * height;
    array = env->NewIntArray((size+3)/4); 
    pixels = (lUInt8 *)env->GetIntArrayElements(array, 0);
#endif
}

BitmapAccessor::~BitmapAccessor()
{
#ifdef USE_JNIGRAPHICS
	if ( pixels!=NULL ) {
		if ( ANDROID_BITMAP_RESUT_SUCCESS!=AndroidBitmap_unlockPixels(env, bitmap) ) {
		    CRLog::error("AndroidBitmap_unlockPixels failed");
		}
        pixels = NULL;
	}
#else
    if ( pixels!=NULL ) {
    	env->ReleaseIntArrayElements(array, (jint*)pixels, 0);
        // IntBuffer testBuf = IntBuffer.wrap(pixels);
        jclass cls = env->FindClass("java/nio/IntBuffer");
		jmethodID mid = env->GetStaticMethodID(cls, "wrap", "([I)Ljava/nio/IntBuffer;");
        jobject jbuf = env->CallStaticObjectMethod(cls, mid, array);
        // mBitmap.copyPixelsFromBuffer(testBuf);
		cls = env->GetObjectClass(bitmap);
		mid = env->GetMethodID(cls,	"copyPixelsFromBuffer", "(Ljava/nio/Buffer;)V");
		env->CallVoidMethod(bitmap, mid, jbuf);
		env->DeleteLocalRef(jbuf);
		env->DeleteLocalRef(array);
    	pixels = NULL;
    }
#endif
}

void BitmapAccessor::setRowRGB( int y, lUInt32 * rgb, int dx )
{
	if ( dx > width )
		dx = width;
	lUInt32 * row = (lUInt32 *)(pixels + stride * y);
	for ( int x=0; x<dx; x++ ) {
		lUInt32 cl = rgb[x];
		cl ^= 0xFF000000; 
		row[x] = cl;
	} 
}
 
void BitmapAccessor::setRowGray( int y, lUInt8 * gray, int dx, int bpp )
{
	CRLog::error("TODO: BitmapAccessor::setRowGray() is not yet implemented");
} 
