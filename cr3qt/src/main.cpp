// CoolReader3 / Qt
// main.cpp - entry point

#if (USE_FONTCONFIG==1)
    #include <fontconfig/fontconfig.h>
#endif


#include <QtGui/QApplication>
#include "../crengine/include/crengine.h"
#include "../crengine/include/cr3version.h"
#include "mainwindow.h"
#include <QTranslator>
#include <QLibraryInfo>
#include <QDir>

// prototypes
void InitCREngineLog( const char * cfgfile );
bool InitCREngine( const char * exename, lString16Collection & fontDirs );
void ShutdownCREngine();
lString8 readFileToString( const char * fname );
#if (USE_FREETYPE==1)
bool getDirectoryFonts( lString16Collection & pathList, lString16Collection & ext, lString16Collection & fonts, bool absPath );
#endif


static void printHelp() {
    printf("usage: cr3 [options] [filename]\n"
           "Options:\n"
           "  -h or --help: this message\n"
           "  -v or --version: print program version\n"
           "  --loglevel=ERROR|WARN|INFO|DEBUG|TRACE: set logging level\n"
           "  --logfile=<filename>|stdout|stderr: set log file\n"
           );
}

static void printVersion() {
    printf("Cool Reader " CR_ENGINE_VERSION " " CR_ENGINE_BUILD_DATE "\n");
}

int main(int argc, char *argv[])
{
    int res = 0;
    {
#ifdef DEBUG
        lString8 loglevel("TRACE");
        lString8 logfile("stdout");
#else
        lString8 loglevel("ERROR");
        lString8 logfile("stderr");
#endif
        for ( int i=1; i<argc; i++ ) {
            if ( !strcmp("-h", argv[i]) || !strcmp("-?", argv[i]) || !strcmp("/?", argv[i]) || !strcmp("--help", argv[i]) ) {
                printHelp();
                return 0;
            }
            if ( !strcmp("-v", argv[i]) || !strcmp("/v", argv[i]) || !strcmp("--version", argv[i]) ) {
                printVersion();
                return 0;
            }
            if ( !strcmp("--stats", argv[i]) && i<argc-4 ) {
                if ( i!=argc-5 ) {
                    printf("To calculate character encoding statistics, use cr3 <infile.txt> <outfile.cpp> <codepagename> <langname>\n");
                    return 1;
                }
                lString8 list;
                FILE * out = fopen(argv[i+2], "wb");
                if ( !out ) {
                    printf("Cannot create file %s", argv[i+2]);
                    return 1;
                }
                MakeStatsForFile( argv[i+1], argv[i+3], argv[i+4], 0, out, list );
                fclose(out);
                return 0;
            }
            lString8 s(argv[i]);
            if ( s.startsWith(cs8("--loglevel=")) ) {
                loglevel = s.substr(11, s.length()-11);
            } else if ( s.startsWith(cs8("--logfile=")) ) {
                logfile = s.substr(10, s.length()-10);
            }
        }

        // set logger
        if ( logfile=="stdout" )
            CRLog::setStdoutLogger();
        else if ( logfile=="stderr" )
                CRLog::setStderrLogger();
        else if ( !logfile.empty() )
                CRLog::setFileLogger(logfile.c_str());
        if ( loglevel=="TRACE" )
            CRLog::setLogLevel(CRLog::LL_TRACE);
        else if ( loglevel=="DEBUG" )
            CRLog::setLogLevel(CRLog::LL_DEBUG);
        else if ( loglevel=="INFO" )
            CRLog::setLogLevel(CRLog::LL_INFO);
        else if ( loglevel=="WARN" )
            CRLog::setLogLevel(CRLog::LL_WARN);
        else if ( loglevel=="ERROR" )
            CRLog::setLogLevel(CRLog::LL_ERROR);
        else
            CRLog::setLogLevel(CRLog::LL_FATAL);

        lString16 exename = LocalToUnicode( lString8(argv[0]) );
        lString16 exedir = LVExtractPath(exename);
        lString16 datadir = lString16(CR3_DATA_DIR);
        LVAppendPathDelimiter(exedir);
        LVAppendPathDelimiter(datadir);
        lString16 exefontpath = exedir + "fonts";
        CRLog::info("main()");
        lString16Collection fontDirs;

        lString16 home = Utf8ToUnicode(lString8(( getenv("HOME") ) ));
        lString16 homecr3 = home;
        LVAppendPathDelimiter(homecr3);
        homecr3 << ".cr3";
        LVAppendPathDelimiter(homecr3);
        //~/.cr3/
        lString16 homefonts = homecr3;
        homefonts << "fonts";

        //fontDirs.add( cs16("/usr/local/share/crengine/fonts") );
        //fontDirs.add( cs16("/usr/local/share/fonts/truetype/freefont") );
        //fontDirs.add( cs16("/mnt/fonts") );
        fontDirs.add(homefonts);
#if MAC==1
        fontDirs.add( cs16("/Library/Fonts") );
#endif
#if 0
        fontDirs.add( exefontpath );
        fontDirs.add( cs16("/usr/share/fonts/truetype") );
        fontDirs.add( cs16("/usr/share/fonts/truetype/liberation") );
        fontDirs.add( cs16("/usr/share/fonts/truetype/freefont") );
#endif
        // TODO: use fontconfig instead
        //fontDirs.add( cs16("/root/fonts/truetype") );
        if ( !InitCREngine( argv[0], fontDirs ) ) {
            printf("Cannot init CREngine - exiting\n");
            return 2;
        }

		if ( argc>=2 && !strcmp(argv[1], "unittest") ) {
#if 0 && defined(_DEBUG)
			runTinyDomUnitTests();
#endif
			CRLog::info("UnitTests finished: exiting");
			return 0;
		}
        //if ( argc!=2 ) {
        //    printf("Usage: cr3 <filename_to_open>\n");
        //    return 3;
        //}
        {
            QApplication a(argc, argv);
#if MAC == 1
            QString exeDir = QDir::toNativeSeparators(qApp->applicationDirPath() + "/Contents/Resources/"); //QDir::separator();
            QString translations = exeDir + "i18n";
#else
#if defined(_WIN32)
            QString exeDir = QDir::toNativeSeparators(qApp->applicationDirPath() + "/"); //QDir::separator();
            QString translations = exeDir + "i18n";
#else
            QString exeDir = cr2qt(datadir);
            QString translations = exeDir + "i18n/";
#endif
#endif
             QTranslator qtTranslator;
             if (qtTranslator.load("qt_" + QLocale::system().name(),
                     QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
                QApplication::installTranslator(&qtTranslator);

             QTranslator myappTranslator;
             QString trname = "cr3_" + QLocale::system().name();
             CRLog::info("Using translation file %s from dir %s", UnicodeToUtf8(qt2cr(trname)).c_str(), UnicodeToUtf8(qt2cr(translations)).c_str() );
             if ( myappTranslator.load(trname, translations) )
                 QApplication::installTranslator(&myappTranslator);
             else
                CRLog::error("Canot load translation file %s from dir %s", UnicodeToUtf8(qt2cr(trname)).c_str(), UnicodeToUtf8(qt2cr(translations)).c_str() );
            MainWindow w;
            w.show();
            res = a.exec();
        }
    }
    ShutdownCREngine();
    return res;
}

#ifdef _WIN32
int WINAPI WinMain( HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
	)
{
	wchar_t buf0[MAX_PATH];
	GetModuleFileNameW(NULL, buf0, MAX_PATH-1);
	lString16 str016(buf0);
#ifdef _UNICODE
	lString16 str116(lpCmdLine);
#else
	lString8 str18(lpCmdLine);
	lString16 str116 = LocalToUnicode(str18);
#endif
	lString8 str0 = UnicodeToUtf8(str016);
	lString8 str1 = UnicodeToUtf8(str116);
	if ( !str1.empty() && str1[0]=='\"' ) {
		// quoted filename support
		str1.erase(0, 1);
        int pos = str1.pos(cs8("\""));
		if ( pos>=0 )
			str1 = str1.substr(0, pos);
	}
	char * argv[2];
	argv[0] = str0.modify();
	argv[1] = str1.modify();
	int argc = str1.empty() ? 1 : 2;
	return main(argc, argv);
}
#endif


/*
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
*/


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
//    ldomFreeStorage();
#endif
}

#if (USE_FREETYPE==1)
bool getDirectoryFonts( lString16Collection & pathList, lString16Collection & ext, lString16Collection & fonts, bool absPath )
{
    int foundCount = 0;
    lString16 path;
    for ( int di=0; di<pathList.length();di++ ) {
        path = pathList[di];
        LVContainerRef dir = LVOpenDirectory(path.c_str());
        if ( !dir.isNull() ) {
            CRLog::trace("Checking directory %s", UnicodeToUtf8(path).c_str() );
            for ( int i=0; i < dir->GetObjectCount(); i++ ) {
                const LVContainerItemInfo * item = dir->GetObjectInfo(i);
                lString16 fileName = item->GetName();
                lString8 fn = UnicodeToLocal(fileName);
                    //printf(" test(%s) ", fn.c_str() );
                if ( !item->IsContainer() ) {
                    bool found = false;
                    lString16 lc = fileName;
                    lc.lowercase();
                    for ( int j=0; j<ext.length(); j++ ) {
                        if ( lc.endsWith(ext[j]) ) {
                            found = true;
                            break;
                        }
                    }
                    if ( !found )
                        continue;
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
#ifdef _WIN32
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
	InitCREngineLog(UnicodeToUtf8(appPath).c_str());
    lString16 datadir = appPath;
#else
    lString16 datadir = lString16(CR3_DATA_DIR);
#endif
    lString16 fontDir = datadir + "fonts";
	lString8 fontDir8_ = UnicodeToUtf8(fontDir);

    fontDirs.add( fontDir );

    LVAppendPathDelimiter( fontDir );

    lString8 fontDir8 = UnicodeToLocal(fontDir);
    //const char * fontDir8s = fontDir8.c_str();
    //InitFontManager( fontDir8 );
    InitFontManager(lString8::empty_str);

#ifdef _WIN32
    lChar16 sysdir[MAX_PATH+1];
    GetWindowsDirectoryW(sysdir, MAX_PATH);
    lString16 fontdir( sysdir );
    fontdir << "\\Fonts\\";
    lString8 fontdir8( UnicodeToUtf8(fontdir) );
    const char * fontnames[] = {
        "arial.ttf",
        "ariali.ttf",
        "arialb.ttf",
        "arialbi.ttf",
        "arialn.ttf",
        "arialni.ttf",
        "arialnb.ttf",
        "arialnbi.ttf",
        "cour.ttf",
        "couri.ttf",
        "courbd.ttf",
        "courbi.ttf",
        "times.ttf",
        "timesi.ttf",
        "timesb.ttf",
        "timesbi.ttf",
        "comic.ttf",
        "comicbd.ttf",
        "verdana.ttf",
        "verdanai.ttf",
        "verdanab.ttf",
        "verdanaz.ttf",
        "bookos.ttf",
        "bookosi.ttf",
        "bookosb.ttf",
        "bookosbi.ttf",
       "calibri.ttf",
        "calibrii.ttf",
        "calibrib.ttf",
        "calibriz.ttf",
        "cambria.ttf",
        "cambriai.ttf",
        "cambriab.ttf",
        "cambriaz.ttf",
        "georgia.ttf",
        "georgiai.ttf",
        "georgiab.ttf",
        "georgiaz.ttf",
        NULL
    };
    for ( int fi = 0; fontnames[fi]; fi++ ) {
        fontMan->RegisterFont( fontdir8 + fontnames[fi] );
    }
#endif
    // Load font definitions into font manager
    // fonts are in files font1.lbf, font2.lbf, ... font32.lbf
    // use fontconfig

    lString16Collection fontExt;
    fontExt.add(cs16(".ttf"));
    fontExt.add(cs16(".otf"));
    fontExt.add(cs16(".pfa"));
    fontExt.add(cs16(".pfb"));
    lString16Collection fonts;

    getDirectoryFonts( fontDirs, fontExt, fonts, true );

    // load fonts from file
    CRLog::debug("%d font files found", fonts.length());
    //if (!fontMan->GetFontCount()) {
    for ( int fi=0; fi<fonts.length(); fi++ ) {
	    lString8 fn = UnicodeToLocal(fonts[fi]);
	    CRLog::trace("loading font: %s", fn.c_str());
	    if ( !fontMan->RegisterFont(fn) ) {
		CRLog::trace("    failed\n");
	    }
	}
    //}

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
    lString16 loglevelstr = 
#ifdef _DEBUG
		L"TRACE";
#else
		L"INFO";
#endif
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
    if (loglevelstr == "OFF") {
        level = CRLog::LL_FATAL;
        logfname.clear();
    } else if (loglevelstr == "FATAL") {
        level = CRLog::LL_FATAL;
    } else if (loglevelstr == "ERROR") {
        level = CRLog::LL_ERROR;
    } else if (loglevelstr == "WARN") {
        level = CRLog::LL_WARN;
    } else if (loglevelstr == "INFO") {
        level = CRLog::LL_INFO;
    } else if (loglevelstr == "DEBUG") {
        level = CRLog::LL_DEBUG;
    } else if (loglevelstr == "TRACE") {
        level = CRLog::LL_TRACE;
    }
    if ( !logfname.empty() ) {
        if (logfname == "stdout")
            CRLog::setStdoutLogger();
        else if (logfname == "stderr")
            CRLog::setStderrLogger();
        else
            CRLog::setFileLogger( UnicodeToUtf8( logfname ).c_str(), autoFlush );
    }
    CRLog::setLogLevel( level );
    CRLog::trace("Log initialization done.");
}

