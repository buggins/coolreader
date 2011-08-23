// CoolReader3 Engine JNI interface
// BASED on Android NDK Plasma example

#include <jni.h>
#include <time.h>


#include <stdio.h>
#include <stdlib.h>

#include "org_coolreader_crengine_Engine.h"
#include "org_coolreader_crengine_ReaderView.h"

#include "cr3java.h"
#include "readerview.h"
#include "crengine.h"
#include "epubfmt.h"
#include "lvstream.h"


#include <fb2def.h>

#define XS_IMPLEMENT_SCHEME 1
#include <fb2def.h>
#include <sys/stat.h>


/// returns current time representation string
static lString16 getDateTimeString( time_t t )
{
    tm * bt = localtime(&t);
    char str[32];
#ifdef _LINUX
    snprintf(str, 32,
#else
    sprintf(str, 
#endif
        "%04d/%02d/%02d %02d:%02d", bt->tm_year+1900, bt->tm_mon+1, bt->tm_mday, bt->tm_hour, bt->tm_min);
    str[31] = 0;
    return Utf8ToUnicode( lString8( str ) );
}

#if 0
static lString16 extractDocSeriesReverse( ldomDocument * doc, int & seriesNumber )
{
	seriesNumber = 0;
    lString16 res;
    ldomXPointer p = doc->createXPointer(L"/FictionBook/description/title-info/sequence");
    if ( p.isNull() )
        return res;
    ldomNode * series = p.getNode();
    if ( series ) {
        lString16 sname = series->getAttributeValue( attr_name );
        lString16 snumber = series->getAttributeValue( attr_number );
        if ( !sname.empty() ) {
            res << L"(";
            if ( !snumber.empty() ) {
                res << L"#" << snumber << L" ";
                seriesNumber = snumber.atoi();
            }
            res << sname;
            res << L")";
        }
    }
    return res;
}
#endif

class BookProperties
{
public:
    lString16 filename;
    lString16 title;
    lString16 author;
    lString16 series;
    int filesize;
    lString16 filedate;
    int seriesNumber;
};

static bool GetEPUBBookProperties(const char *name, LVStreamRef stream, BookProperties * pBookProps)
{
    LVContainerRef m_arc = LVOpenArchieve( stream );
    if ( m_arc.isNull() )
        return false; // not a ZIP archive

    // check root media type
    lString16 rootfilePath = EpubGetRootFilePath(m_arc);
    if ( rootfilePath.empty() )
    	return false;

    lString16 codeBase;
    codeBase=LVExtractPath(rootfilePath, false);

    LVStreamRef content_stream = m_arc->OpenStream(rootfilePath.c_str(), LVOM_READ);
    if ( content_stream.isNull() )
        return false;

    ldomDocument * doc = LVParseXMLStream( content_stream );
    if ( !doc )
        return false;

    time_t t = (time_t)time(0);
    struct stat fs;
    if ( !stat( name, &fs ) ) {
        t = fs.st_mtime;
    }

    lString16 author = doc->textFromXPath( lString16(L"package/metadata/creator"));
    lString16 title = doc->textFromXPath( lString16(L"package/metadata/title"));

    pBookProps->author = author;
    pBookProps->title = title;

    for ( int i=1; i<20; i++ ) {
        ldomNode * item = doc->nodeFromXPath( lString16(L"package/metadata/meta[") + lString16::itoa(i) + L"]" );
        if ( !item )
            break;
        lString16 name = item->getAttributeValue(L"name");
        lString16 content = item->getAttributeValue(L"content");
        if ( name==L"calibre:series" )
        	pBookProps->series = content;
        else if ( name==L"calibre:series_index" )
        	pBookProps->seriesNumber = content.atoi();
    }

    pBookProps->filesize = (long)stream->GetSize();
    pBookProps->filename = lString16(name);
    pBookProps->filedate = getDateTimeString( t );

    delete doc;

    return true;
}

static bool GetBookProperties(const char *name,  BookProperties * pBookProps)
{
    CRLog::trace("GetBookProperties( %s )", name);

    // check archieve
    lString16 arcPathName;
    lString16 arcItemPathName;
    bool isArchiveFile = LVSplitArcName( lString16(name), arcPathName, arcItemPathName );

    // open stream
    LVStreamRef stream = LVOpenFileStream( (isArchiveFile ? arcPathName : Utf8ToUnicode(lString8(name))).c_str() , LVOM_READ);
    if (!stream) {
        CRLog::error("cannot open file %s", name);
        return false;
    }


    if ( DetectEpubFormat( stream ) ) {
        CRLog::trace("GetBookProperties() : epub format detected");
    	return GetEPUBBookProperties( name, stream, pBookProps );
    }

    time_t t = (time_t)time(0);

    if ( isArchiveFile ) {
        int arcsize = (int)stream->GetSize();
        LVContainerRef container = LVOpenArchieve(stream);
        if ( container.isNull() ) {
            CRLog::error( "Cannot read archive contents from %s", LCSTR(arcPathName) );
            return false;
        }
        stream = container->OpenStream(arcItemPathName.c_str(), LVOM_READ);
        if ( stream.isNull() ) {
            CRLog::error( "Cannot open archive file item stream %s", LCSTR(lString16(name)) );
            return false;
        }
    }
    struct stat fs;
    if ( !stat( name, &fs ) ) {
        t = fs.st_mtime;
    }

    // read document
#if COMPACT_DOM==1
    ldomDocument doc(stream, 0);
#else
    ldomDocument doc;
#endif
    ldomDocumentWriter writer(&doc, true);
    doc.setNodeTypes( fb2_elem_table );
    doc.setAttributeTypes( fb2_attr_table );
    doc.setNameSpaceTypes( fb2_ns_table );
    LVXMLParser parser( stream, &writer );
    CRLog::trace( "checking format..." );
    if ( !parser.CheckFormat() ) {
        return false;
    }
    CRLog::trace( "parsing..." );
    if ( !parser.Parse() ) {
        return false;
    }
    CRLog::trace( "parsed" );
    #if 0
        char ofname[512];
        sprintf(ofname, "%s.xml", name);
        CRLog::trace("    writing to file %s", ofname);
        LVStreamRef out = LVOpenFileStream(ofname, LVOM_WRITE);
        doc.saveToStream(out, "utf16");
    #endif
    lString16 authors = extractDocAuthors( &doc, lString16(L"|"), false );
    lString16 title = extractDocTitle( &doc );
    lString16 series = extractDocSeries( &doc, &pBookProps->seriesNumber );
#if SERIES_IN_AUTHORS==1
    if ( !series.empty() )
    	authors << L"    " << series;
#endif
    pBookProps->title = title;
    pBookProps->author = authors;
    pBookProps->series = series;
    pBookProps->filesize = (long)stream->GetSize();
    pBookProps->filename = lString16(name);
    pBookProps->filedate = getDateTimeString( t );
    return true;
}


/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    scanBookPropertiesInternal
 * Signature: (Lorg/coolreader/crengine/FileInfo;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_Engine_scanBookPropertiesInternal
  (JNIEnv * _env, jobject _engine, jobject _fileInfo)
{
	CRJNIEnv env(_env);
	jclass objclass = env->GetObjectClass(_fileInfo);
	jfieldID fid = env->GetFieldID(objclass, "pathname", "Ljava/lang/String;");
	lString16 filename = env.fromJavaString( (jstring)env->GetObjectField(_fileInfo, fid) );
    fid = env->GetFieldID(objclass, "arcname", "Ljava/lang/String;");
    lString16 arcname = env.fromJavaString( (jstring)env->GetObjectField(_fileInfo, fid) );
	if ( filename.empty() )
		return JNI_FALSE;
	if ( !arcname.empty() )
	   filename = arcname + L"@/" + filename;

	BookProperties props;
	CRLog::debug("Looking for properties of file %s", LCSTR(filename));
	bool res = GetBookProperties(LCSTR(filename),  &props);
	if ( !res )
		return JNI_FALSE;
	#define SET_STR_FLD(fldname,src) \
	{ \
	    jfieldID fid = env->GetFieldID(objclass, fldname, "Ljava/lang/String;"); \
	    env->SetObjectField(_fileInfo,fid,env.toJavaString(src)); \
	}
	#define SET_INT_FLD(fldname,src) \
	{ \
	    jfieldID fid = env->GetFieldID(objclass, fldname, "I"); \
	    env->SetIntField(_fileInfo,fid,src); \
	}
	SET_STR_FLD("title",props.title);
	SET_STR_FLD("authors",props.author);
	SET_STR_FLD("series",props.series);
	SET_INT_FLD("seriesNumber",props.seriesNumber);
	
	return JNI_TRUE;
}

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    getArchiveItemsInternal
 * Signature: (Ljava/lang/String;)[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_org_coolreader_crengine_Engine_getArchiveItemsInternal
  (JNIEnv * _env, jobject, jstring jarcName)
{
    CRJNIEnv env(_env);
    lString16 arcName = env.fromJavaString(jarcName);
    lString16Collection list;
    
    //fontMan->getFaceList(list);
    LVStreamRef stream = LVOpenFileStream( arcName.c_str(), LVOM_READ );
    if ( !stream.isNull() ) {
        LVContainerRef arc = LVOpenArchieve(stream);
        if ( !arc.isNull() ) {
            // convert
            for ( int i=0; i<arc->GetObjectCount(); i++ ) {
                const LVContainerItemInfo * item = arc->GetObjectInfo(i);
                if ( item->IsContainer())
                    continue;
                list.add( item->GetName() );
                list.add( lString16::itoa(item->GetSize()) );
            }
        }
    }
    return env.toJavaStringArray(list);
}


/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    setHyphenationMethod
 * Signature: (I[B)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_Engine_setHyphenationMethod
  (JNIEnv * _env, jobject _engine, jint method, jbyteArray data)
{
	CRJNIEnv env(_env);
	if ( method==0 ) {
		CRLog::info("Selecting hyphenation method: Disabled");
		return HyphMan::activateDictionary(lString16(HYPH_DICT_ID_NONE));
	} else if ( method==1 ) {
		CRLog::info("Selecting hyphenation method: Algoryphmic");
		return HyphMan::activateDictionary(lString16(HYPH_DICT_ID_ALGORITHM));
	} else {
		CRLog::info("Selecting hyphenation method: Dictionary");
		LVStreamRef stream = env.jbyteArrayToStream( data );
		CRLog::debug("Stream is created from byte array, length=%d", (int)(stream.isNull()?0:stream->GetSize()));
		bool res = HyphMan::activateDictionaryFromStream(stream);
		if ( !res ) {
			CRLog::error("Dictionary activation is failed: disabling hyphenation");
			HyphMan::activateDictionary(lString16(HYPH_DICT_ID_NONE));
			return false;
		}
		return true;
	}
}


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

//typedef void (lv_FatalErrorHandler_t)(int errorCode, const char * errorText );

void cr3androidFatalErrorHandler(int errorCode, const char * errorText )
{
	static char str[1001];
	snprintf(str, 1000, "CoolReader Fatal Error #%d: %s", errorCode, errorText);
	LOGE(str);
	LOGASSERTFAILED(errorText, str);
}

/// set fatal error handler
void crSetFatalErrorHandler( lv_FatalErrorHandler_t * handler );

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    initInternal
 * Signature: ([Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_Engine_initInternal
  (JNIEnv * penv, jobject obj, jobjectArray fontArray)
{
	CRJNIEnv env(penv);
	
	LOGI("initInternal called");
	// set fatal error handler
	crSetFatalErrorHandler( &cr3androidFatalErrorHandler );
	LOGD("Redirecting CDRLog to Android");
	CRLog::setLogger( new JNICDRLogger() );
	CRLog::setLogLevel( CRLog::LL_TRACE );
	CRLog::info("CREngine log redirected");
	
	CRLog::info("initializing hyphenation manager");
	HyphMan::initDictionaries(lString16()); //don't look for dictionaries
	HyphMan::activateDictionary(lString16(HYPH_DICT_ID_NONE));
	CRLog::info("creating font manager");
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
			CRLog::error("cannot load font %s", fontName.c_str());
	}
	CRLog::info("%d fonts registered", (int)fontMan->GetFontCount());
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
 * Class:     org_coolreader_crengine_Engine
 * Method:    setCacheDirectoryInternal
 * Signature: (Ljava/lang/String;I)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_Engine_setCacheDirectoryInternal
  (JNIEnv * penv, jobject obj, jstring dir, jint size)
{
	CRJNIEnv env(penv);
	bool res = ldomDocCache::init(env.fromJavaString(dir), size ); 
	return res ? JNI_TRUE : JNI_FALSE;
}

/*
 * Class:     org_coolreader_crengine_Engine
 * Method:    isLink
 * Signature: (Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_org_coolreader_crengine_Engine_isLink
  (JNIEnv * env, jobject obj, jstring pathname)
{
	if ( !pathname )
		return JNI_FALSE;
	int res = JNI_FALSE;
	jboolean iscopy;
	const char * s = env->GetStringUTFChars( pathname, &iscopy );
	struct stat st;
	if ( !lstat( s, &st) ) {
		if ( S_ISLNK(st.st_mode) )
			res = JNI_TRUE;
	}
	env->ReleaseStringUTFChars(pathname, s);
	return res;
}


//=====================================================================

static JNINativeMethod sEngineMethods[] = {
  /* name, signature, funcPtr */
  {"initInternal", "([Ljava/lang/String;)Z", (void*)Java_org_coolreader_crengine_Engine_initInternal},
  {"uninitInternal", "()V", (void*)Java_org_coolreader_crengine_Engine_uninitInternal},
  {"getFontFaceListInternal", "()[Ljava/lang/String;", (void*)Java_org_coolreader_crengine_Engine_getFontFaceListInternal},
  {"setCacheDirectoryInternal", "(Ljava/lang/String;I)Z", (void*)Java_org_coolreader_crengine_Engine_setCacheDirectoryInternal},
  {"scanBookPropertiesInternal", "(Lorg/coolreader/crengine/FileInfo;)Z", (void*)Java_org_coolreader_crengine_Engine_scanBookPropertiesInternal},
  {"setHyphenationMethod", "(I[B)Z", (void*)Java_org_coolreader_crengine_Engine_setHyphenationMethod},
  {"getArchiveItemsInternal", "(Ljava/lang/String;)[Ljava/lang/String;", (void*)Java_org_coolreader_crengine_Engine_getArchiveItemsInternal},
  {"isLink", "(Ljava/lang/String;)Z", (void*)JNICALL Java_org_coolreader_crengine_Engine_isLink},
};


static JNINativeMethod sReaderViewMethods[] = {
  /* name, signature, funcPtr */
  {"createInternal", "()V", (void*)Java_org_coolreader_crengine_ReaderView_createInternal},
  {"destroyInternal", "()V", (void*)Java_org_coolreader_crengine_ReaderView_destroyInternal},
  {"getPageImageInternal", "(Landroid/graphics/Bitmap;)V", (void*)Java_org_coolreader_crengine_ReaderView_getPageImageInternal},
  {"loadDocumentInternal", "(Ljava/lang/String;)Z", (void*)Java_org_coolreader_crengine_ReaderView_loadDocumentInternal},
  {"getSettingsInternal", "()Ljava/util/Properties;", (void*)Java_org_coolreader_crengine_ReaderView_getSettingsInternal},
  {"applySettingsInternal", "(Ljava/util/Properties;)Z", (void*)Java_org_coolreader_crengine_ReaderView_applySettingsInternal},
  {"setStylesheetInternal", "(Ljava/lang/String;)V", (void*)Java_org_coolreader_crengine_ReaderView_setStylesheetInternal},
  {"resizeInternal", "(II)V", (void*)Java_org_coolreader_crengine_ReaderView_resizeInternal},
  {"doCommandInternal", "(II)Z", (void*)Java_org_coolreader_crengine_ReaderView_doCommandInternal},
  {"getCurrentPageBookmarkInternal", "()Lorg/coolreader/crengine/Bookmark;", (void*)Java_org_coolreader_crengine_ReaderView_getCurrentPageBookmarkInternal},
  {"goToPositionInternal", "(Ljava/lang/String;)Z", (void*)Java_org_coolreader_crengine_ReaderView_goToPositionInternal},
  {"getPositionPropsInternal", "(Ljava/lang/String;)Lorg/coolreader/crengine/PositionProperties;", (void*)Java_org_coolreader_crengine_ReaderView_getPositionPropsInternal},
  {"updateBookInfoInternal", "(Lorg/coolreader/crengine/BookInfo;)V", (void*)Java_org_coolreader_crengine_ReaderView_updateBookInfoInternal},
  {"getTOCInternal", "()Lorg/coolreader/crengine/TOCItem;", (void*)Java_org_coolreader_crengine_ReaderView_getTOCInternal},
  {"clearSelectionInternal", "()V", (void*)Java_org_coolreader_crengine_ReaderView_clearSelectionInternal},
  {"findTextInternal", "(Ljava/lang/String;III)Z", (void*)Java_org_coolreader_crengine_ReaderView_findTextInternal},
  {"setBatteryStateInternal", "(I)V", (void*)Java_org_coolreader_crengine_ReaderView_setBatteryStateInternal},
  {"getCoverPageDataInternal", "()[B", (void*)Java_org_coolreader_crengine_ReaderView_getCoverPageDataInternal},
  {"setPageBackgroundTextureInternal", "([BI)V", (void*)Java_org_coolreader_crengine_ReaderView_setPageBackgroundTextureInternal},
  {"updateSelectionInternal", "(Lorg/coolreader/crengine/Selection;)V", (void*)Java_org_coolreader_crengine_ReaderView_updateSelectionInternal},
  {"checkLinkInternal", "(III)Ljava/lang/String;", (void*)Java_org_coolreader_crengine_ReaderView_checkLinkInternal},
  {"goLinkInternal", "(Ljava/lang/String;)I", (void*)Java_org_coolreader_crengine_ReaderView_goLinkInternal},
  {"moveSelectionInternal", "(Lorg/coolreader/crengine/Selection;II)Z", (void*)Java_org_coolreader_crengine_ReaderView_moveSelectionInternal},
  {"swapToCacheInternal", "()I", (void*)JNICALL Java_org_coolreader_crengine_ReaderView_swapToCacheInternal},
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
   jint res = -1;
 
#ifdef JNI_VERSION_1_6
    if (res==-1 && vm->GetEnv((void**) &env, JNI_VERSION_1_6) == JNI_OK) {
        LOGI("JNI_OnLoad: JNI_VERSION_1_6\n");
   	    res = JNI_VERSION_1_6;
    }
#endif
#ifdef JNI_VERSION_1_4
    if (res==-1 && vm->GetEnv((void**) &env, JNI_VERSION_1_4) == JNI_OK) {
        LOGI("JNI_OnLoad: JNI_VERSION_1_4\n");
   	    res = JNI_VERSION_1_4;
    }
#endif
#ifdef JNI_VERSION_1_2
    if (res==-1 && vm->GetEnv((void**) &env, JNI_VERSION_1_2) == JNI_OK) {
        LOGI("JNI_OnLoad: JNI_VERSION_1_2\n");
   	    res = JNI_VERSION_1_2;
    }
#endif
	if ( res==-1 )
		return res;
 
    jniRegisterNativeMethods(env, "org/coolreader/crengine/Engine", sEngineMethods, sizeof(sEngineMethods)/sizeof(JNINativeMethod));
    jniRegisterNativeMethods(env, "org/coolreader/crengine/ReaderView", sReaderViewMethods, sizeof(sReaderViewMethods)/sizeof(JNINativeMethod));
    LOGI("JNI_OnLoad: native methods are registered!\n");
    return res;
}
