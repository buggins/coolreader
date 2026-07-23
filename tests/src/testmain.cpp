#include <stdio.h>

#include "../crengine/include/crengine.h"
#include "../crengine/include/cr3version.h"
#include "../crengine/include/lvstring8collection.h"
#include "../crengine/include/lvstring32collection.h"
#include "stringtest.h"
#include <memory>
#include <chrono>
#include <vector>

//using unique_ptr = std::unique_ptr;

bool getDirectoryFiles(lString32 path, lString32Collection & files)
{
    int foundCount = 0;
	LVContainerRef dir = LVOpenDirectory(path.c_str());
	if ( !dir.isNull() ) {
		CRLog::trace("Checking directory %s", path.c_str() );
		for ( int i=0; i < dir->GetObjectCount(); i++ ) {
			const LVContainerItemInfo * item = dir->GetObjectInfo(i);
			lString32 fileName = item->GetName();
			//printf("file in directory: %s\n", UnicodeToLocal(fileName).c_str());
				//printf(" test(%s) ", fn.c_str() );
			if ( !item->IsContainer() ) {
				//bool found = false;
				lString32 lc = fileName;
				lc.lowercase();
				if (lc.startsWith(".")) {
					continue;
				}
				lString32 fn;
				fn << path;
				LVAppendPathDelimiter(fn);
				fn << fileName;
				foundCount++;
				files.add( fn );
			}
		}
	} else {
		printf("Cannot open directory %s\n", UnicodeToLocal(path).c_str());
	}
    return foundCount > 0;
}

#if (USE_FREETYPE==1)
bool getDirectoryFonts( lString32Collection & pathList, lString32Collection & ext, lString32Collection & fonts, bool absPath )
{
    int foundCount = 0;
    lString32 path;
    for ( int di=0; di<pathList.length();di++ ) {
        path = pathList[di];
        LVContainerRef dir = LVOpenDirectory(path.c_str());
        if ( !dir.isNull() ) {
            CRLog::trace("Checking directory %s", UnicodeToUtf8(path).c_str() );
            for ( int i=0; i < dir->GetObjectCount(); i++ ) {
                const LVContainerItemInfo * item = dir->GetObjectInfo(i);
                lString32 fileName = item->GetName();
                lString8 fn = UnicodeToLocal(fileName);
                    //printf(" test(%s) ", fn.c_str() );
                if ( !item->IsContainer() ) {
                    bool found = false;
                    lString32 lc = fileName;
                    lc.lowercase();
                    for ( int j=0; j<ext.length(); j++ ) {
                        if ( lc.endsWith(ext[j]) ) {
                            found = true;
                            break;
                        }
                    }
                    if ( !found )
                        continue;
                    lString32 fn;
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

lUInt64 currentTimeMillis() {
    // Get the current time from the system clock
    auto now = std::chrono::system_clock::now();

           // Convert the current time to time since epoch
    auto duration = now.time_since_epoch();

           // Convert duration to milliseconds
    auto milliseconds
        = std::chrono::duration_cast<std::chrono::milliseconds>(
              duration)
              .count();
    return milliseconds;
}

struct PerfCounts {
    lString32 fname;
    int loadCount;
    lUInt64 loadTime;
    int renderCount;
    lUInt64 renderTime;
    int pageCount;
    PerfCounts(lString32 fn = lString32::empty_str)
        : fname(fn), loadCount(0), loadTime(0), renderCount(0), renderTime(0), pageCount(0)
    {
    }
    ~PerfCounts() = default;
    void reset() {
        loadCount = 0;
        loadTime = 0;
        renderCount = 0;
        renderTime = 0;
        pageCount = 0;
    }
    void dump() {
        printf("%s\t%lld\t%lld\t%d\n", UnicodeToLocal(fname).c_str(), loadTime, renderTime, pageCount);
    }
};

using perf_vector = std::vector<std::unique_ptr<PerfCounts>>;

bool benchmarkFile(PerfCounts& perf) {
    lString32 fname = perf.fname;
    std::unique_ptr<LVDocView> _docview { new LVDocView() };
    _docview->setViewMode(LVDocViewMode::DVM_PAGES, 2);
    _docview->setRenderProps(2000, 1000);
    _docview->setFontSize(30);
//    _docview->up

    lUInt64 loadStart = currentTimeMillis();
    if (!_docview->LoadDocument(fname.c_str(), false)) {
        return false;
    }
    perf.loadTime = currentTimeMillis() - loadStart;
    perf.loadCount++;
    lUInt64 renderStart = currentTimeMillis();
    _docview->Render(2000, 1000);
    perf.renderTime = currentTimeMillis() - renderStart;
    perf.pageCount += _docview->getPageCount();
    return true;
}

void executeBenchmark(perf_vector& perfStats) {
    for (auto &p : perfStats) {
        printf("Opening file %s\n", UnicodeToLocal(p->fname).c_str());
        bool res = benchmarkFile(*p);

        if (!res) {
            printf("Failed to open %s\n", UnicodeToLocal(p->fname).c_str());
        }
    }
}

void executeBenchmark(lString32Collection & testFiles) {
    perf_vector perfStats;
    for (int i = 0; i < testFiles.length(); i++) {
        auto p = std::make_unique<PerfCounts>(testFiles[i]);
        perfStats.push_back(std::move(p));
    }

    printf("Starting benchmark\n");
    executeBenchmark(perfStats);

    printf("Performance stats:\n");
    for (auto &p : perfStats) {
        p->dump();
    }
}

bool InitCREngine( const char * exename, lString32Collection & fontDirs )
{
	CRLog::trace("InitCREngine(%s)", exename);
#ifdef _WIN32
    lString32 appname( exename );
    int lastSlash=-1;
    lChar32 slashChar = '/';
    for ( int p=0; p<(int)appname.length(); p++ ) {
        if ( appname[p]=='\\' ) {
            slashChar = '\\';
            lastSlash = p;
        } else if ( appname[p]=='/' ) {
            slashChar = '/';
            lastSlash=p;
        }
    }

    lString32 appPath;
    if ( lastSlash>=0 )
        appPath = appname.substr( 0, lastSlash+1 );
	InitCREngineLog(UnicodeToUtf8(appPath).c_str());
    lString32 datadir = appPath;
#else
    lString32 datadir = lString32(CR3_DATA_DIR);
#endif
    lString32 fontDir = datadir + "fonts";
	lString8 fontDir8_ = UnicodeToUtf8(fontDir);

    fontDirs.add( fontDir );

    LVAppendPathDelimiter( fontDir );

    lString8 fontDir8 = UnicodeToLocal(fontDir);
    //const char * fontDir8s = fontDir8.c_str();
    //InitFontManager( fontDir8 );
    InitFontManager(lString8::empty_str);

#if defined(_WIN32) && USE_FONTCONFIG!=1
    wchar_t sysdir_w[MAX_PATH+1];
    GetWindowsDirectoryW(sysdir_w, MAX_PATH);
    lString32 fontdir = Utf16ToUnicode( sysdir_w );
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
#endif  // defined(_WIN32) && USE_FONTCONFIG!=1
    // Load font definitions into font manager
    // fonts are in files font1.lbf, font2.lbf, ... font32.lbf
    // use fontconfig

#if USE_FREETYPE==1
    lString32Collection fontExt;
    fontExt.add(cs32(".ttf"));
    fontExt.add(cs32(".otf"));
    fontExt.add(cs32(".pfa"));
    fontExt.add(cs32(".pfb"));
    lString32Collection fonts;

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
#endif  // USE_FREETYPE==1

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
    lString32 logfname;
    lString32 loglevelstr = 
#ifdef _DEBUG
		U"TRACE";
#else
                U"TRACE";
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

void ShutdownCREngine()
{
    CRLog::info("Shutting down CREngine...");
    HyphMan::uninit();
    ShutdownFontManager();
    CRLog::setLogger( NULL );
#if LDOM_USE_OWN_MEM_MAN == 1
//    ldomFreeStorage();
#endif
}

bool testsOnly = true;

void runLibraryTestSuite() {
    printf("CR3 Library Tests\n");
    testStrings();
    testStringCollections();
    printf("CR3 Library Tests\n");
}

int main(int argc, const char ** argv) {

#if (LDOM_USE_OWN_MEM_MAN == 1)
    printf("LDOM_USE_OWN_MEM_MAN is turned ON\n");
#endif
    runLibraryTestSuite();
    if (testsOnly) {
#if (LDOM_USE_OWN_MEM_MAN == 1)
        free_ls_storage();
#endif
        printf("CR3 Library Tests done. Exiting.\n");
        return 0;
    }

    printf("CR3 Performance Tests\n");

    lString32Collection files;
    lString8 bookDir("../tests/testdata");
    for (int i = 1; i < argc; i++) {
        if (argv[i][0]=='-') {
            printf("option: %s\n", argv[i]);
        } else {
            bookDir = argv[i];
            if (!getDirectoryFiles(LocalToUnicode(bookDir), files)) {
                printf("No book files found in directory %s\n", bookDir.c_str());
                return 3;
            }
        }
    }
    //LVAppendPathDelimiter(bookDir);
    printf("Book dir: %s\n", bookDir.c_str());
#ifdef DEBUG
    lString8 loglevel("TRACE");
    lString8 logfile("stdout");
#else
    lString8 loglevel("ERROR");
    lString8 logfile("stderr");
#endif

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

    lString32 exename = LocalToUnicode( lString8(argv[0]) );
    lString32 exedir = LVExtractPath(exename);
    lString32 datadir = lString32(CR3_DATA_DIR);
    LVAppendPathDelimiter(exedir);
    LVAppendPathDelimiter(datadir);
    lString32 exefontpath = exedir + "fonts";
    CRLog::info("main()");
    lString32Collection fontDirs;

    lString32 home = Utf8ToUnicode(lString8(( getenv("HOME") ) ));
    lString32 homecr3 = home;
    LVAppendPathDelimiter(homecr3);
    homecr3 << ".cr3";
    LVAppendPathDelimiter(homecr3);
    //~/.cr3/
    lString32 homefonts = homecr3;
    homefonts << "fonts";

    //fontDirs.add( cs32("/usr/local/share/crengine/fonts") );
    //fontDirs.add( cs32("/usr/local/share/fonts/truetype/freefont") );
    //fontDirs.add( cs32("/mnt/fonts") );
    fontDirs.add(homefonts);

    if ( !InitCREngine( argv[0], fontDirs ) ) {
            printf("Cannot init CREngine - exiting\n");
            return 2;
    }

    // use cache in current directory
    if (!ldomDocCache::init( lString32(".cr3/cache"), 0x100000 * 256 )) {
        printf("Cannot init doc cache!\n");
    } else {
        ldomDocCache::clear();
    }


    for (int i = 0; i < files.length(); i++) {
        printf("test file [%d]: %s\n", i, UnicodeToLocal(files[i]).c_str());
    }

    printf("Starting benchmark...\n");
    executeBenchmark(files);

    ShutdownCREngine();
    printf("Exiting...\n");
    return 0;
}
