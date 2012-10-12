#include "cr3java.h"

#include <dlfcn.h>

lString16 CRJNIEnv::fromJavaString( jstring str )
{
	if ( !str )
        return lString16::empty_str;
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
		env->DeleteLocalRef(str);
	}
}

jobjectArray CRJNIEnv::toJavaStringArray( lString16Collection & src )
{
    int len = src.length();
	jobjectArray array = env->NewObjectArray(len, env->FindClass("java/lang/String"), env->NewStringUTF(""));
	for ( int i=0; i<len; i++ ) {
		jstring local = toJavaString(src[i]); 
		env->SetObjectArrayElement(array, i, local);
		env->DeleteLocalRef(local);
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
    	env->DeleteLocalRef(key); 
    	env->DeleteLocalRef(value); 
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
		env->DeleteLocalRef(key);
		env->DeleteLocalRef(value);
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
		jstring str1 = _env.toJavaString( item->getName() );
		_env->SetObjectField(obj, _name, str1 );
		_env->DeleteLocalRef(str1);
		jstring str2 = _env.toJavaString( item->getPath() );
		_env->SetObjectField(obj, _path, str2 );
		_env->DeleteLocalRef(str2);
	}
	void add( jobject obj, LVTocItem * child )
	{
		jobject jchild = _env->CallObjectMethod(obj, _addChild);
		set( jchild, child );
		for ( int i=0; i<child->getChildCount(); i++ ) {
			add( jchild, child->getChild(i) );
		}
		_env->DeleteLocalRef(jchild);
	}
	jobject toJava(LVTocItem * root)
	{
		jobject jroot = _env->NewObject(_cls, _constructor);
		for ( int i=0; i<root->getChildCount(); i++ ) {
			add( jroot, root->getChild(i) );
		}
		return jroot;
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

jbyteArray CRJNIEnv::streamToJByteArray( LVStreamRef stream )
{
	if ( stream.isNull() )
		return NULL;
	unsigned sz = stream->GetSize();
	if ( sz<10 || sz>2000000 )
		return NULL;
    jbyteArray array = env->NewByteArray(sz); 
    lUInt8 * array_data = (lUInt8 *)env->GetByteArrayElements(array, 0);
    lvsize_t bytesRead = 0;
    stream->Read(array_data, sz, &bytesRead);
   	env->ReleaseByteArrayElements(array, (jbyte*)array_data, 0);
    if (bytesRead != sz)
    	return NULL;
    return array; 
}

static void ConvertCRColorsToAndroid( lUInt8 * buf, int dx, int dy )
{
	int sz = dx * dy;
	for ( lUInt8 * p = buf; --sz>=0; p+=4 ) {
		// invert A
		p[3] ^= 0xFF; 
		// swap R and B
		lUInt8 c = p[0];
		p[0] = p[2];
		p[2] = c;
	}
} 

class LVColorDrawBufEx : public LVColorDrawBuf {
public:
    lUInt8 * getData() { return _data; }
    void convert() {
    	if ( GetBitsPerPixel()==32 )
    		ConvertCRColorsToAndroid( _data, GetWidth(), GetHeight() );
    }
    
	LVColorDrawBufEx(int dx, int dy, lUInt8 * pixels, int bpp)
	: LVColorDrawBuf( dx, dy, pixels, bpp ) {
	}
};

class JNIGraphicsLib : public BitmapAccessorInterface
{
    void * _lib;

	int (*AndroidBitmap_getInfo)(JNIEnv* env, jobject jbitmap, AndroidBitmapInfo* info);
	int (*AndroidBitmap_lockPixels)(JNIEnv* env, jobject jbitmap, void** addrPtr);
	int (*AndroidBitmap_unlockPixels)(JNIEnv* env, jobject jbitmap);
public:
    virtual LVDrawBuf * lock(JNIEnv* env, jobject jbitmap) {
	    //CRLog::trace("JNIGraphicsLib::lock entered");
		AndroidBitmapInfo info;
		if ( ANDROID_BITMAP_RESUT_SUCCESS!=AndroidBitmap_getInfo(env, jbitmap, &info) ) {
			CRLog::error("BitmapAccessor : cannot get bitmap info");
			return NULL;
		}
		int width = info.width;
		int height = info.height;
		int stride = info.stride;
		int format = info.format;
		if ( format!=ANDROID_BITMAP_FORMAT_RGBA_8888 && format!=ANDROID_BITMAP_FORMAT_RGB_565  && format!=8 ) {
			CRLog::error("BitmapAccessor : bitmap format %d is not yet supported", format);
			return NULL;
		}
		int bpp = (format==ANDROID_BITMAP_FORMAT_RGBA_8888) ? 32 : 16;
	    //CRLog::trace("JNIGraphicsLib::lock info: %d (%d) x %d", width, stride, height);
		lUInt8 * pixels = NULL; 
		if ( ANDROID_BITMAP_RESUT_SUCCESS!=AndroidBitmap_lockPixels(env, jbitmap, (void**)&pixels) ) {
	        CRLog::error("AndroidBitmap_lockPixels failed");
		    pixels = NULL;
		}
	    //CRLog::trace("JNIGraphicsLib::lock pixels locked!" );
		return new LVColorDrawBufEx( width, height, pixels, bpp );
    } 
    virtual void unlock(JNIEnv* env, jobject jbitmap, LVDrawBuf * buf ) {
    	LVColorDrawBufEx * bmp = (LVColorDrawBufEx*)buf;
    	bmp->convert();
    	AndroidBitmap_unlockPixels(env, jbitmap);
    	delete buf;
    } 

    void * getProc( const char * procName )
    {
        return dlsym( _lib, procName );
    }
    bool load( const char * libName )
    {
        if ( !_lib ) {
            _lib = dlopen( libName, RTLD_NOW | RTLD_LOCAL );
        }
        if ( _lib ) {
        	AndroidBitmap_getInfo = (int (*)(JNIEnv* env, jobject jbitmap, AndroidBitmapInfo* info))
                 getProc( "AndroidBitmap_getInfo" );
            AndroidBitmap_lockPixels = (int (*)(JNIEnv* env, jobject jbitmap, void** addrPtr))
                 getProc( "AndroidBitmap_lockPixels" );
            AndroidBitmap_unlockPixels = (int (*)(JNIEnv* env, jobject jbitmap))
	             getProc( "AndroidBitmap_unlockPixels");
            if ( !AndroidBitmap_getInfo || !AndroidBitmap_lockPixels || !AndroidBitmap_unlockPixels )
                unload(); // not all functions found in library, fail
        }
        return ( _lib!=NULL );
    }
    bool unload()
    {
        bool res = false;
        if ( _lib ) {
            dlclose( _lib );
            res = true;
        }
        _lib = NULL;
        return res;
    }
    JNIGraphicsLib()
    : _lib(NULL) {
    }
    ~JNIGraphicsLib() {
        unload();
    }
};

class JNIGraphicsReplacement : public BitmapAccessorInterface
{
    jintArray _array;
public:
	virtual int getInfo(JNIEnv* env, jobject jbitmap, AndroidBitmapInfo* info) {
	    //CRLog::trace("JNIGraphicsReplacement::getInfo entered");
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
		case 8:
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
    virtual LVDrawBuf * lock(JNIEnv* env, jobject jbitmap) {
	    //CRLog::trace("JNIGraphicsReplacement::lock entered");
		AndroidBitmapInfo info;
		if ( ANDROID_BITMAP_RESUT_SUCCESS!=getInfo(env, jbitmap, &info) )
			return NULL;
		int width = info.width;
		int height = info.height;
		int stride = info.stride;
		int format = info.format;
	    //CRLog::trace("JNIGraphicsReplacement::lock info: %d (%d) x %d", width, stride, height);
		if ( format!=ANDROID_BITMAP_FORMAT_RGBA_8888 && format!=ANDROID_BITMAP_FORMAT_RGB_565  && format!=8  ) {
			CRLog::error("BitmapAccessor : bitmap format %d is not yet supported", format);
			return NULL;
		}
		int bpp = (format==ANDROID_BITMAP_FORMAT_RGBA_8888) ? 32 : 16;
		//int size = stride * height;
		//CRLog::trace("lock: %d x %d stride = %d, width*4 = %d", width, height, stride, width*4 );
		int size = width * height;
		if ( bpp==16 )
			size = (size + 1) >> 1;
		reallocArray( env, size );
	    //CRLog::trace("JNIGraphicsReplacement::lock getting pixels");
	    lUInt8 * pixels = (lUInt8 *)env->GetIntArrayElements(_array, 0);
	    //CRLog::trace("Pixels address %08x", (int)(pixels));
	    //CRLog::trace("JNIGraphicsReplacement::lock exiting");
		LVDrawBuf * buf = new LVColorDrawBufEx(width, height, pixels, bpp);
	    //CRLog::trace("Last row address %08x", (int)buf->GetScanLine(height-1));
	    //pixels[0] = 0x12;
	    //pixels[width*height*4-1] = 0x34;
	    //CRLog::trace("Write access ok");
		return buf;
    }
    void reallocArray(JNIEnv* env, int len )
    {
    	if ( _array==NULL || env->GetArrayLength(_array)<len ) {
    		//CRLog::trace("JNIGraphicsReplacement::reallocArray( %d )", len);
	    	freeArray(env);
	    	jobject lref = env->NewIntArray(len);
		    _array = (jintArray)env->NewGlobalRef( lref );
		    env->DeleteLocalRef(lref);
		}
    }
    void freeArray(JNIEnv* env)
    {
	    if ( _array!=NULL ) {
			env->DeleteGlobalRef(_array);
			_array = NULL;
		}
    }
    virtual void unlock(JNIEnv* env, jobject jbitmap, LVDrawBuf * buf ) {
	    //CRLog::trace("JNIGraphicsReplacement::unlock entering");
    	if ( !buf)
    		return;
    	LVColorDrawBufEx * bmp = (LVColorDrawBufEx*)buf;
    	bmp->convert();
    	lUInt8 * pixels = bmp->getData();
    	env->ReleaseIntArrayElements(_array, (jint*)pixels, 0);
        // IntBuffer testBuf = IntBuffer.wrap(pixels);
        jclass cls = env->FindClass("java/nio/IntBuffer");
		jmethodID mid = env->GetStaticMethodID(cls, "wrap", "([I)Ljava/nio/IntBuffer;");
        jobject jbuf = env->CallStaticObjectMethod(cls, mid, _array);
        // mBitmap.copyPixelsFromBuffer(testBuf);
		cls = env->GetObjectClass(jbitmap);
		mid = env->GetMethodID(cls,	"copyPixelsFromBuffer", "(Ljava/nio/Buffer;)V");
		env->CallVoidMethod(jbitmap, mid, jbuf);
		env->DeleteLocalRef(jbuf);
	    //CRLog::trace("JNIGraphicsReplacement::unlock exiting");
		delete buf;
    }
    JNIGraphicsReplacement() : _array(NULL) {
    }
    ~JNIGraphicsReplacement() {
    }
};

static BitmapAccessorInterface * _bitmapAccessorInstance = NULL; 
BitmapAccessorInterface * BitmapAccessorInterface::getInstance()
{
	if ( _bitmapAccessorInstance==NULL ) {
		JNIGraphicsLib * lib = new JNIGraphicsLib();
		if ( !lib->load("libjnigraphics.so") ) {
			delete lib;
			CRLog::error("Cannot load libjnigraphics.so : will use slower replacement instead");
			_bitmapAccessorInstance = new JNIGraphicsReplacement(); 
		} else {
			_bitmapAccessorInstance = lib;
		}
	}
	return _bitmapAccessorInstance;
} 
