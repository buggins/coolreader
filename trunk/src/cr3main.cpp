/*
    First version of CR3 for EWL, based on etimetool example by Lunohod
*/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
//#include <Ewl.h>
#include <crengine.h>
#include <crgui.h>
#include <crtrace.h>

#include "cr3main.h"

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
    HyphMan::Close();
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
    const char * fontDir8s = fontDir8.c_str();
    //InitFontManager( fontDir8 );
    InitFontManager( lString8() );

    // Load font definitions into font manager
    // fonts are in files font1.lbf, font2.lbf, ... font32.lbf
    if (!fontMan->GetFontCount()) {


    #if (USE_FREETYPE==1)
        lString16 fontExt = L".ttf";
    #else
        lString16 fontExt = L".lbf";
    #endif
    #if (USE_FREETYPE==1)
        lString16Collection fonts;
        fontDirs.add( fontDir );
        static const char * msfonts[] = {
            "arial.ttf", "arialbd.ttf", "ariali.ttf", "arialbi.ttf",
            "cour.ttf", "courbd.ttf", "couri.ttf", "courbi.ttf",
            "times.ttf", "timesbd.ttf", "timesi.ttf", "timesbi.ttf",
            NULL
        };
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
    }

    // init hyphenation manager
    char hyphfn[1024];
    sprintf(hyphfn, "Russian_EnUS_hyphen_(Alan).pdb" );
    if ( !initHyph( (UnicodeToLocal(appPath) + hyphfn).c_str() ) ) {
#ifdef _LINUX
        initHyph( "/usr/share/crengine/hyph/Russian_EnUS_hyphen_(Alan).pdb" );
#endif
    }

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
    CRPropRef logprops = LVCreatePropsContainer();
    {
        LVStreamRef cfg = LVOpenFileStream( cfgfile, LVOM_READ );
        if ( !cfg.isNull() ) {
            logprops->loadFromStream( cfg.get() );
            logfname = logprops->getStringDef( PROP_LOG_FILENAME, "stdout" );
            loglevelstr = logprops->getStringDef( PROP_LOG_LEVEL, "TRACE" );
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
            CRLog::setFileLogger( UnicodeToUtf8( logfname ).c_str(), true );
    }
    CRLog::setLogLevel( level );
    CRLog::trace("Log initialization done.");
}

