#include <QtGui/QApplication>
#include "../crengine/include/crengine.h"
#include "mainwindow.h"

// prototypes
void InitCREngineLog( const char * cfgfile );
bool InitCREngine( const char * exename, lString16Collection & fontDirs );
void ShutdownCREngine();
lString8 readFileToString( const char * fname );
#if (USE_FREETYPE==1)
bool getDirectoryFonts( lString16Collection & pathList, lString16 ext, lString16Collection & fonts, bool absPath );
#endif



int main(int argc, char *argv[])
{
    int res = 0;
    {
        CRLog::setStdoutLogger();
    #ifdef __i386__
        CRLog::setLogLevel( CRLog::LL_TRACE );
    #else
        CRLog::setLogLevel( CRLog::LL_ERROR );
    #endif
        CRLog::info("main()");
        lString16Collection fontDirs;
        //fontDirs.add( lString16(L"/usr/local/share/crengine/fonts") );
        //fontDirs.add( lString16(L"/usr/local/share/fonts/truetype/freefont") );
        //fontDirs.add( lString16(L"/mnt/fonts") );
        fontDirs.add( lString16(L"/usr/share/fonts/truetype") );
        fontDirs.add( lString16(L"/usr/share/fonts/truetype/liberation") );
        fontDirs.add( lString16(L"/usr/share/fonts/truetype/freefont") );
        // TODO: use fontconfig instead
        //fontDirs.add( lString16(L"/root/fonts/truetype") );
        if ( !InitCREngine( argv[0], fontDirs ) ) {
            printf("Cannot init CREngine - exiting\n");
            return 2;
        }

        //if ( argc!=2 ) {
        //    printf("Usage: cr3 <filename_to_open>\n");
        //    return 3;
        //}
        {
            QApplication a(argc, argv);
            MainWindow w;
            w.show();
            res = a.exec();
        }
    }
    ShutdownCREngine();
    return res;
}


bool initHyph(const char * fname)
{
    //HyphMan hyphman;
    //return;

    LVStreamRef stream = LVOpenFileStream( fname, LVOM_READ);
    if (!stream)
    {
        printf("Cannot load hyphenation file %s\n", fname);
        return false;
    }
    return HyphMan::Open( stream.get() );
}

lString8 readFileToString( const char * fname )
{
    lString8 buf;
    LVStreamRef stream = LVOpenFileStream(fname, LVOM_READ);
    if (!stream)
        return buf;
    int sz = stream->GetSize();
    if (sz>0)
    {
        buf.insert( 0, sz, ' ' );
        stream->Read( buf.modify(), sz, NULL );
    }
    return buf;
}

void ShutdownCREngine()
{
    HyphMan::uninit();
    ShutdownFontManager();
    CRLog::setLogger( NULL );
#if LDOM_USE_OWN_MEM_MAN == 1
    ldomFreeStorage();
#endif
}

#if (USE_FREETYPE==1)
bool getDirectoryFonts( lString16Collection & pathList, lString16 ext, lString16Collection & fonts, bool absPath )
{
    int foundCount = 0;
    lString16 path;
    for ( unsigned di=0; di<pathList.length();di++ ) {
        path = pathList[di];
        LVContainerRef dir = LVOpenDirectory(path.c_str());
        if ( !dir.isNull() ) {
            CRLog::trace("Checking directory %s", UnicodeToUtf8(path).c_str() );
            for ( int i=0; i < dir->GetObjectCount(); i++ ) {
                const LVContainerItemInfo * item = dir->GetObjectInfo(i);
                lString16 fileName = item->GetName();
                lString8 fn = UnicodeToLocal(fileName);
                    //printf(" test(%s) ", fn.c_str() );
                if ( !item->IsContainer() && fileName.length()>4 && lString16(fileName, fileName.length()-4, 4)==ext ) {
                    lString16 fn;
                    if ( absPath ) {
                        fn = path;
                        if ( !fn.empty() && fn[fn.length()-1]!=PATH_SEPARATOR_CHAR)
                            fn << PATH_SEPARATOR_CHAR;
                    }
                    fn << fileName;
                    foundCount++;
                    fonts.add( fn );
                }
            }
        }
    }
    return foundCount > 0;
}
#endif

bool InitCREngine( const char * exename, lString16Collection & fontDirs )
{
    CRLog::trace("InitCREngine(%s)", exename);
    lString16 appname( exename );
    int lastSlash=-1;
    lChar16 slashChar = '/';
    for ( int p=0; p<(int)appname.length(); p++ ) {
        if ( appname[p]=='\\' ) {
            slashChar = '\\';
            lastSlash = p;
        } else if ( appname[p]=='/' ) {
            slashChar = '/';
            lastSlash=p;
        }
    }

    lString16 appPath;
    if ( lastSlash>=0 )
        appPath = appname.substr( 0, lastSlash+1 );

    lString16 fontDir = appPath + L"fonts";
    fontDir << slashChar;
    lString8 fontDir8 = UnicodeToLocal(fontDir);
    //const char * fontDir8s = fontDir8.c_str();
    //InitFontManager( fontDir8 );
    InitFontManager( lString8() );

    // Load font definitions into font manager
    // fonts are in files font1.lbf, font2.lbf, ... font32.lbf
    if (!fontMan->GetFontCount()) {

        // TODO: use fontconfig
        #if 0 //(USE_FONTCONFIG==1)
                lString16Collection fonts;

                FcFontSet *fontset;

                FcObjectSet *os = FcObjectSetBuild(FC_FILE, NULL);
                FcPattern *pat = FcPatternCreate();
        //FcBool b = 1;
        FcPatternAddBool(pat, FC_SCALABLE, 1);

                fontset = FcFontList(NULL, pat, os);

                FcPatternDestroy(pat);
                FcObjectSetDestroy(os);

                // load fonts from file
                CRLog::debug("%d font files found", fonts.length());
                if (!fontMan->GetFontCount()) {
                        for(int i = 0; i < fontset->nfont; i++) {
                                FcChar8 *s;
                //FcBool b;
                                FcResult res;
                //FC_SCALABLE
                //res = FcPatternGetBool( fontset->fonts[i], FC_OUTLINE, 0, (FcBool*)&b);
                //if(res != FcResultMatch)
                //    continue;
                //if ( !b )
                //    continue; // skip non-scalable fonts
                                res = FcPatternGetString(fontset->fonts[i], FC_FILE, 0, (FcChar8 **)&s);
                                if(res != FcResultMatch)
                                        continue;

                                lString8 fn = UnicodeToLocal(lString16((lChar8 *)s));
                                //CRLog::trace("loading font: %s", fn.c_str());
                                if ( !fontMan->RegisterFont(fn) ) {
                    CRLog::trace("loading of font %s failed", fn.c_str());
                                }
                        }
                }

                FcFontSetDestroy(fontset);

        #else

    #if (USE_FREETYPE==1)
        lString16 fontExt = L".ttf";
    #else
        lString16 fontExt = L".lbf";
    #endif
    #if (USE_FREETYPE==1)
        CRLog::trace("USE_FREETYPE==1 -- msfonts");
        lString16Collection fonts;
        fontDirs.add( fontDir );
        static const char * msfonts[] = {
            "arial.ttf", "arialbd.ttf", "ariali.ttf", "arialbi.ttf",
            "cour.ttf", "courbd.ttf", "couri.ttf", "courbi.ttf",
            "times.ttf", "timesbd.ttf", "timesi.ttf", "timesbi.ttf",
            NULL
        };
    #ifdef _WIN32
        wchar_t sd_buf[MAX_PATH];
        sd_buf[0] = 0;
        ::GetSystemDirectoryW(sd_buf, MAX_PATH-1);
        lString16 sysFontDir = lString16(sd_buf) + L"\\..\\fonts\\";
        lString8 sfd = UnicodeToLocal( sysFontDir );
        //const char * s = sfd.c_str();
        //CRLog::debug(s);
        for ( int fi=0; msfonts[fi]; fi++ )
            fonts.add( sysFontDir + lString16(msfonts[fi]) );
    #endif
    #ifdef _LINUX
    #ifndef LBOOK
        fontDirs.add( lString16(L"/usr/local/share/crengine/fonts") );
        fontDirs.add( lString16(L"/usr/local/share/fonts/truetype/freefont") );
        fontDirs.add( lString16(L"/usr/share/crengine/fonts") );
        fontDirs.add( lString16(L"/usr/share/fonts/truetype/freefont") );
        fontDirs.add( lString16(L"/root/fonts/truetype") );
        //fontDirs.add( lString16(L"/usr/share/fonts/truetype/msttcorefonts") );
        for ( int fi=0; msfonts[fi]; fi++ )
            fonts.add( lString16(L"/usr/share/fonts/truetype/msttcorefonts/") + lString16(msfonts[fi]) );
    #endif
    #endif
        getDirectoryFonts( fontDirs, fontExt, fonts, true );

        // load fonts from file
        CRLog::debug("%d font files found", fonts.length());
        if (!fontMan->GetFontCount()) {
            for ( unsigned fi=0; fi<fonts.length(); fi++ ) {
                lString8 fn = UnicodeToLocal(fonts[fi]);
                CRLog::trace("loading font: %s", fn.c_str());
                if ( !fontMan->RegisterFont(fn) ) {
                    CRLog::trace("    failed\n");
                }
            }
        }
    #else
            #define MAX_FONT_FILE 128
            for (int i=0; i<MAX_FONT_FILE; i++)
            {
                char fn[1024];
                sprintf( fn, "font%d.lbf", i );
                printf("try load font: %s\n", fn);
                fontMan->RegisterFont( lString8(fn) );
            }
    #endif
        #endif
    }

    // init hyphenation manager
    //char hyphfn[1024];
    //sprintf(hyphfn, "Russian_EnUS_hyphen_(Alan).pdb" );
    //if ( !initHyph( (UnicodeToLocal(appPath) + hyphfn).c_str() ) ) {
#ifdef _LINUX
    //    initHyph( "/usr/share/crengine/hyph/Russian_EnUS_hyphen_(Alan).pdb" );
#endif
    //}

    if (!fontMan->GetFontCount())
    {
        //error
#if (USE_FREETYPE==1)
        printf("Fatal Error: Cannot open font file(s) .ttf \nCannot work without font\n" );
#else
        printf("Fatal Error: Cannot open font file(s) font#.lbf \nCannot work without font\nUse FontConv utility to generate .lbf fonts from TTF\n" );
#endif
        return false;
    }

    printf("%d fonts loaded.\n", fontMan->GetFontCount());

    return true;

}

void InitCREngineLog( const char * cfgfile )
{
    if ( !cfgfile ) {
        CRLog::setStdoutLogger();
        CRLog::setLogLevel( CRLog::LL_TRACE );
        return;
    }
    lString16 logfname;
    lString16 loglevelstr = L"INFO";
        bool autoFlush = false;
    CRPropRef logprops = LVCreatePropsContainer();
    {
        LVStreamRef cfg = LVOpenFileStream( cfgfile, LVOM_READ );
        if ( !cfg.isNull() ) {
            logprops->loadFromStream( cfg.get() );
            logfname = logprops->getStringDef( PROP_LOG_FILENAME, "stdout" );
            loglevelstr = logprops->getStringDef( PROP_LOG_LEVEL, "TRACE" );
                        autoFlush = logprops->getBoolDef( PROP_LOG_AUTOFLUSH, false );
        }
    }
    CRLog::log_level level = CRLog::LL_INFO;
    if ( loglevelstr==L"OFF" ) {
        level = CRLog::LL_FATAL;
        logfname.clear();
    } else if ( loglevelstr==L"FATAL" ) {
        level = CRLog::LL_FATAL;
    } else if ( loglevelstr==L"ERROR" ) {
        level = CRLog::LL_ERROR;
    } else if ( loglevelstr==L"WARN" ) {
        level = CRLog::LL_WARN;
    } else if ( loglevelstr==L"INFO" ) {
        level = CRLog::LL_INFO;
    } else if ( loglevelstr==L"DEBUG" ) {
        level = CRLog::LL_DEBUG;
    } else if ( loglevelstr==L"TRACE" ) {
        level = CRLog::LL_TRACE;
    }
    if ( !logfname.empty() ) {
        if ( logfname==L"stdout" )
            CRLog::setStdoutLogger();
        else if ( logfname==L"stderr" )
            CRLog::setStderrLogger();
        else
            CRLog::setFileLogger( UnicodeToUtf8( logfname ).c_str(), autoFlush );
    }
    CRLog::setLogLevel( level );
    CRLog::trace("Log initialization done.");
}

