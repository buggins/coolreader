#include "cr3java.h"

lString16 CRJNIEnv::fromJavaString( jstring str )
{
	if ( !str )
		return lString16();
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

CRPropRef CRJNIEnv::fromJavaProperties( jobject jprops )
{
	CRPropRef props = LVCreatePropsContainer();
    CRObjectAccessor jp(env, jprops);
    CRMethodAccessor p_getProperty(jp, "getProperty", "(Ljava/lang/String;)Ljava/lang/String;");
    jobject en = CRMethodAccessor( jp, "propertyNames", "()Ljava/util/Enumeration;").callObj();
    CRObjectAccessor jen(env, en);
    CRMethodAccessor jen_hasMoreElements(jen, "hasMoreElements", "()Z");
    CRMethodAccessor jen_nextElement(jen, "nextElement", "()Ljava/lang/Object;");
    while ( jen_hasMoreElements.callBool() ) {
    	jstring key = (jstring)jen_nextElement.callObj();
    	jstring value = (jstring)p_getProperty.callObj(key);
    	props->setString(LCSTR(fromJavaString(key)),LCSTR(fromJavaString(value)));  
    }
	return props;
}

jobject CRJNIEnv::toJavaProperties( CRPropRef props )
{
    jclass cls = env->FindClass("java/util/Properties");
    jmethodID mid = env->GetMethodID(cls, "<init>", "()V");
    jobject obj = env->NewObject(cls, mid);
    CRObjectAccessor jp(env, obj);
    CRMethodAccessor p_setProperty(jp, "setProperty", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/Object;");
    for ( int i=0; i<props->getCount(); i++ ) {
    	jstring key = toJavaString(lString16(props->getName(i)));
    	jstring value = toJavaString(lString16(props->getValue(i)));
    	p_setProperty.callObj(key, value);
    }
	return obj;
}

class TOCItemAccessor {
	CRJNIEnv & _env;
	jclass _cls;
	jmethodID _constructor;
	jmethodID _addChild;
	jfieldID _level;
	jfieldID _page;
	jfieldID _percent;
	jfieldID _name;
	jfieldID _path;
public:
	TOCItemAccessor( CRJNIEnv & env )
	: _env(env), _cls(env->FindClass("org/coolreader/crengine/TOCItem"))
	{
		_constructor = _env->GetMethodID(_cls, "<init>", "()V");
		_addChild = _env->GetMethodID(_cls, "addChild", "()Lorg/coolreader/crengine/TOCItem;");
		_level = _env->GetFieldID(_cls, "mLevel", "I");
		_page = _env->GetFieldID(_cls, "mPage", "I");
		_percent = _env->GetFieldID(_cls, "mPercent", "I");
		_name = _env->GetFieldID(_cls, "mName", "Ljava/lang/String;");
		_path = _env->GetFieldID(_cls, "mPath", "Ljava/lang/String;");
	}
	void set( jobject obj, LVTocItem * item )
	{
		_env->SetIntField(obj, _level, item->getLevel() );
		_env->SetIntField(obj, _page, item->getPage() );
		_env->SetIntField(obj, _percent, item->getPercent() );
		_env->SetObjectField(obj, _name, _env.toJavaString( item->getName() ) );
		_env->SetObjectField(obj, _path, _env.toJavaString( item->getPath() ) );
	}
	void add( jobject obj, LVTocItem * child )
	{
		jobject jchild = _env->CallObjectMethod(obj, _addChild);
		set( jchild, child );
		for ( int i=0; i<child->getChildCount(); i++ ) {
			add( jchild, child->getChild(i) );
		}
	}
	jobject toJava(LVTocItem * root)
	{
		jobject jroot = _env->NewObject(_cls, _constructor);
		for ( int i=0; i<root->getChildCount(); i++ ) {
			add( jroot, root->getChild(i) );
		}
	} 
};

jobject CRJNIEnv::toJavaTOCItem( LVTocItem * toc )
{
	TOCItemAccessor acc(*this);
	return acc.toJava( toc );
}

jobject CRJNIEnv::enumByNativeId( const char * classname, int id )
{
	jclass cl = env->FindClass(classname);
	if ( cl ) {
		jmethodID method = env->GetStaticMethodID(cl, "byId", "(I)Lorg/coolreader/crengine/DocumentFormat;");
		if ( method ) {
			return env->CallStaticObjectMethod(cl, method, (jint)id);
		}
	}
	return NULL;
} 

LVStreamRef CRJNIEnv::jbyteArrayToStream( jbyteArray array )
{
	if ( !array )
		return LVStreamRef();
	int len = env->GetArrayLength(array);
	if ( !len )
		return LVStreamRef();
    lUInt8 * data = (lUInt8 *)env->GetByteArrayElements(array, 0);
    LVStreamRef res = LVCreateMemoryStream(data, len, true, LVOM_READ);
    env->ReleaseByteArrayElements(array, (jbyte*)data, 0);
    return res;
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
	mid = env->GetMethodID(configCls, "ordinal", "()I");
	int ord = env->CallIntMethod(configObj, mid);
	switch ( ord ) {
	case 1:
		info->format = ANDROID_BITMAP_FORMAT_A_8;
		break; 
	case 2:
		info->format = ANDROID_BITMAP_FORMAT_RGBA_4444;
		break; 
	case 3:
		info->format = ANDROID_BITMAP_FORMAT_RGBA_8888;
		break; 
	case 4:
		info->format = ANDROID_BITMAP_FORMAT_RGB_565;
		break;
	default:
		info->format = ANDROID_BITMAP_FORMAT_NONE;
		break; 
	}
	jfieldID fid;
	fid = env->GetFieldID(configCls, "nativeInt", "I");
	//info->format 
	int ni = env->GetIntField(configObj, fid);
	//CRLog::debug("Bitmap format: %d (ord=%d, nativeInt=%d)", info->format, ord, ni);
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
	if ( format!=ANDROID_BITMAP_FORMAT_RGBA_8888 ) {
		CRLog::error("BitmapAccessor : bitmap format %d is not yet supported", format);
		return;
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

void BitmapAccessor::draw(LVDrawBuf * buf, int x, int y)
{
	int h = height - y;
	if ( h > buf->GetHeight() )
	    h = buf->GetHeight();
	int w = width - x;
	if ( w > buf->GetWidth() )
	    w = buf->GetWidth();
	//CRLog::debug("copy drawbuf image %d x %d", w, h);
	for ( int yy=0; yy<h; yy++ ) {
		switch ( buf->GetBitsPerPixel() ) {
		case 32:
			{
				lUInt32 * row = (lUInt32 *)buf->GetScanLine(yy);
				setRowRGB( x, y+yy, row, w );
			}
		    break;
		default:
			// only color buffer supported now
			// TODO:
			break;
		} 
	}
} 

void BitmapAccessor::setRowRGB( int x, int y, lUInt32 * rgb, int dx )
{
	if ( !pixels )
		return;
	switch ( format ) {
	case ANDROID_BITMAP_FORMAT_RGBA_8888:
	    {
			if ( x + dx > width )
				dx = width - x;
			lUInt32 * row = (lUInt32 *)(pixels + stride * y);
			for ( int x=0; x<dx; x++ ) {
				lUInt32 cl = rgb[x];
				cl ^= 0xFF000000; 
				cl = (cl&0xFF000000)|((cl&0x00FF0000)>>16)|((cl&0x0000FF00))|((cl&0x000000FF)<<16);
				row[x] = cl;
			}
		}
		break;
	default:
		CRLog::error("BitmapAccessor::setRowRGB : bitmap format %d is not yet supported", format);
		break;
	} 
}
 
void BitmapAccessor::setRowGray( int x, int y, lUInt8 * gray, int dx, int bpp )
{
	CRLog::error("TODO: BitmapAccessor::setRowGray() is not yet implemented");
} 
