#include <lvdocview.h>
#include <stdio.h>

#include <unistd.h>
#include <limits.h>

#ifdef ANDROID
#include <android/log.h>
#define  LOG_TAG    "cr3eng"
#endif

#ifdef ANDROID
#include <GLES3/gl31.h>
#else
#include <GL/gl.h>
#endif

// The interface between unity and the cr3engine is provided through the functions listed.
extern "C" {
  void * LVDocViewCreate (int bitsPerPixel);
  
  bool LoadDocument(void * handle, const char * fname, int width, int height );
  
  char * getTitle(void * handle);
  
  const char * getLanguage(void * handle);
  
  const char * getAuthors(void * handle);
  
  int renderPage (void * handle, int page, int texture);
  
  void moveByPage (void * handle, int d);
  
  void goToPage (void * handle, int page);

  int getPageCount (void * handle);
  
  int prepareCover (void * handle, int width, int height);
  
  int renderCover (void * handle, int texture, int width, int height);
  
  int setFontSize (void * handle, int fontsize);
  
}

// Taken from the cool reader code, find and add fonts for each system.
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

// Also taken from coolreader code, initialized required font elements.
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
  lString16 datadir = lString16("/usr/share/cr3/");
#endif
  lString16 fontDir = datadir + "fonts";
  lString8 fontDir8_ = UnicodeToUtf8(fontDir);
  
  fontDirs.add( fontDir );
  
  LVAppendPathDelimiter( fontDir );
  
  lString8 fontDir8 = UnicodeToLocal(fontDir);
  InitFontManager(lString8::empty_str);
  
  {
    lString16 font ("/system/fonts/Roboto-Regular.ttf");
    lString8 fontName = UnicodeToUtf8(font);
    CRLog::debug("registering font %s", fontName.c_str());
    if ( !fontMan->RegisterFont( fontName ) )
      CRLog::error("cannot load font %s", fontName.c_str());
    
    CRLog::info("%d fonts registered", fontMan->GetFontCount());    
  }
  
  #if defined(_WIN32) && USE_FONTCONFIG!=1
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

// Include some debugging tools required for working on android devices.
#ifdef ANDROID

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
#endif

// Create a document. Unity expects the bitsperpixel to be 32.
void * LVDocViewCreate (int bitsPerPixel)
{
  
  CRLog::setStdoutLogger();
  #ifdef ANDROID    
  CRLog::setLogger( new JNICDRLogger() );
  #endif
  
  CRLog::setLogLevel(CRLog::LL_INFO);
  CRLog::setLogLevel(CRLog::LL_DEBUG);
  
  
  printf ("Opening\n");
  lString16Collection fontDirs;
  
  lString16 home = Utf8ToUnicode(lString8(( getenv("HOME") ) ));
  lString16 homecr3 = home;
  LVAppendPathDelimiter(homecr3);
  homecr3 << ".cr3";
  LVAppendPathDelimiter(homecr3);
  //~/.cr3/
  lString16 homefonts = homecr3;
  homefonts << "fonts";

  fontDirs.add(homefonts);
  InitCREngine( "vrcoolreader", fontDirs );

  LVDocView * a = new LVDocView (bitsPerPixel);
  
  return (void *) a;
}

// unity and 64 bit C alignment for strings seems a bit dodgy. Manually convert.
char * remap (lString16 & a)
{
  static char * buffer = NULL;
  if (buffer != NULL)
  {
    delete [] buffer;
  }
  buffer = new char [a.length () + 1];
  for (int i = 0; i < a.length (); i++)
  {
    buffer[i] = a[i];
  }
  buffer[a.length ()] = 0;
  return buffer;
}

// Replace the current document with one loaded from file.
bool LoadDocument(void * handle, const char * fname, int width, int height )
{
  LVDocView * a = (LVDocView *) handle;
  
  bool result = false;
  result =  a->LoadDocument (fname);
  
//  printf ("Au: %s Ti: %s  %ld\n", LCSTR (a->getAuthors ()), LCSTR (a->getTitle ()), a);
  
  // Some tweaks that may need to be exposed via parameters at some point in the future.
  a->Resize (width, height);
  a->setFontSize (120);
  lvRect rc = a->getPageMargins();
  rc.left = 40;
  rc.top = 40;
  rc.right = 40;
  rc.bottom = 40;
  a->setPageMargins(rc);
  
  return result;
}

char * getTitle(void * handle) 
{ 
  LVDocView * a = (LVDocView *) handle;
  lString16 b = a->getTitle ();
  char * c = remap (b);
  return c; 
}

const char * getLanguage(void * handle) 
{ 
  LVDocView * a = (LVDocView *) handle;
  lString16 b = a->getLanguage(); 
  char * c = remap (b);
  return c;
}

const char * getAuthors(void * handle) 
{ 
  LVDocView * a = (LVDocView *) handle;
  lString16 b = a->getAuthors(); 
  char * c = remap (b);
  return c; 
}

// Render a page to a texture image. The texture handle is provided (from Unity).
int renderPage (void * handle, int page, int texture)
{
  LVDocView * a = (LVDocView *) handle;
  a->goToPage (page);
  // Get the image.
  LVDocImageRef p = a->getPageImage (0);

  // Copy it from the buffer to another contiguous buffer.
  LVDrawBuf * buf = p->getDrawBuf();
  int width = buf->GetWidth();
  int height = buf->GetHeight();
  //int bpp = buf->GetBitsPerPixel();
  
  unsigned char * data = new unsigned char [width * height * 4];
//  CRLog::info("drawing tex: %d %d %d   %d", width, height, bpp, texture);
  for ( int i=0; i<height; i++ ) {
    unsigned char * dst = data + 4 * i * width;
    unsigned char * src = buf->GetScanLine(i);
    
    for ( int x=0; x<width; x++ ) {
      *dst++ = *(src+2);
      *dst++ = *(src+1);
      *dst++ = *(src+0);
      *dst++ = *(src+3);
      src += 4;
    }
  }
  
  glEnable (GL_TEXTURE_2D);
  glBindTexture (GL_TEXTURE_2D, texture);
  glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
  glBindTexture (GL_TEXTURE_2D, 0);
  glDisable (GL_TEXTURE_2D);
  delete [] data;
  
  int r = glGetError ();
  return r;
}

void moveByPage (void * handle, int d)
{
  LVDocView * a = (LVDocView *) handle;
  a->moveByPage (d);
}

void goToPage (void * handle, int page)
{
  LVDocView * a = (LVDocView *) handle;
  a->goToPage (page);
}

int getPageCount (void * handle)
{
  LVDocView * a = (LVDocView *) handle;
  return a->getPageCount ();
}

LVColorDrawBuf * buf = NULL;

// Cover rendering is quite slow, and is not thread safe if updating texture content at the same time.
// Render in the background to a static buffer, which can be retrieved later with renderCover.
int prepareCover (void * handle, int width, int height)
{
  LVDocView * a = (LVDocView *) handle;
  
  if (buf != NULL)
  {
    delete buf;
    buf = NULL;
  }
  if (buf == NULL)
  {
    buf = new LVColorDrawBuf (width, height);
  }
  
  lvRect rc (0, 0, width, height);
  
  (*buf).Clear (0xFFFFFF);  
  a->drawCoverTo (buf, rc);
  
  return 0;
}

// Requires a prepareCover first.
int renderCover (void * handle, int texture, int width, int height)
{
//  LVDocView * a = (LVDocView *) handle;
  
//   LVColorDrawBuf buf (width, height);
//   lvRect rc (0, 0, width, height);
  
//   buf.Clear (0xFFFFFF);  
//   a->drawCoverTo (&buf, rc);
  
  unsigned char * data = new unsigned char [width * height * 4];
  CRLog::info("drawing cover: %d %d   %d", width, height, texture);
  for (int i = 0; i < height; i++) 
  {
    unsigned char * dst = data + 4 * i * width;
    unsigned char * src = (*buf).GetScanLine(i);
    
    for (int x = 0; x < width; x++ ) 
    {
      *dst++ = *(src+2);
      *dst++ = *(src+1);
      *dst++ = *(src+0);
      *dst++ = *(src+3);
      src += 4;
    }
  }
  
  glEnable (GL_TEXTURE_2D);
  glBindTexture (GL_TEXTURE_2D, texture);
  glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
  glBindTexture (GL_TEXTURE_2D, 0);
  glDisable (GL_TEXTURE_2D);
  delete [] data;
  
  return 0;
}

int setFontSize (void * handle, int fontsize)
{
  LVDocView * a = (LVDocView *) handle;
//  CRLog::info("setting font: %p %d", a, fontsize);
  a->setFontSize (fontsize);
//  CRLog::info("done setting font: %p %d", a, fontsize);
  return 0;
}
