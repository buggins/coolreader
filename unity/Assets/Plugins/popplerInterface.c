#include <stdio.h>
#include <string.h>

#include <poppler-document.h>
#include <poppler-image.h>
#include <poppler-page.h>
#include <poppler-page-renderer.h>

#ifdef ANDROID
#include <GLES3/gl31.h>
#else
#include <GL/gl.h>
#endif

// The interface between unity and the cr3engine is provided through the functions listed.
extern "C" {
  void * popDocViewCreate (int bitsPerPixel);
  
  void popDocDestroy (void * handle);

  bool popLoadDocument(void * handle, const char * fname, int width, int height);
  
  char * popGetTitle(void * handle);
  
  const char * popGetAuthors(void * handle);
  
  int popRenderPage (void * handle, int page, int texture);
  
  void popMoveByPage (void * handle, int d);
  
  void popGoToPage (void * handle, int page);

  int popGetPageCount (void * handle);
  
  int popPrepareCover (void * handle, int width, int height);
  
  int popRenderCover (void * handle, int texture, int width, int height);
  
  int popSetFontSize (void * handle, int fontsize);
  
  int popGetFontSize (void * handle);
  
}

class popDoc
{
  public:
    poppler::document * doc;
    int bitsPerPixel;
    int width;
    int height;
    int page;
    
    char * coverBuf;
};

// Create a document. Unity expects the bitsperpixel to be 32.
void * popDocViewCreate (int bitsPerPixel)
{
  popDoc * phandle = new popDoc ();
  phandle->doc = NULL;
  phandle->bitsPerPixel = bitsPerPixel;
  phandle->width = 0;
  phandle->height = 0;
  phandle->page = 0;
  phandle->coverBuf = NULL;
  
  return phandle;
}

void popDocDestroy (void * handle)
{
  popDoc * phandle = (popDoc *) handle;
  
  if (phandle->coverBuf != NULL)
  {
    delete [] phandle->coverBuf;
    phandle->coverBuf = NULL;
  }
  
  if (phandle->doc != NULL)
  {
    delete phandle->doc;
    phandle->doc = NULL;
  }
  
  delete phandle;
}

// unity and 64 bit C alignment for strings seems a bit dodgy. Manually convert.
char * remap (std::string & a)
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

bool popLoadDocument(void * handle, const char * fname, int width, int height)
{
  popDoc * phandle = (popDoc *) handle;
  
  phandle->width = width;
  phandle->height = height;

  const std::string file_name (fname);

  phandle->doc = poppler::document::load_from_file (file_name);
  if (!phandle->doc) {
      printf("loading error");
      return false;
  }
  if (phandle->doc->is_locked()) {
      printf("encrypted document");
      return false;
  }
  return true;
}

char * popGetTitle(void * handle) 
{ 
  popDoc * phandle = (popDoc *) handle;

  poppler::document * doc = phandle->doc;
  std::string title = doc->get_title ().to_latin1 ();
  return (char *) remap (title);
}


const char * popGetAuthors(void * handle) 
{ 
  popDoc * phandle = (popDoc *) handle;

  poppler::document * doc = phandle->doc;
  std::string author = doc->get_author ().to_latin1 ();
  return (char *) remap (author);
}

// Render a page to a texture image. The texture handle is provided (from Unity).
int popRenderPage (void * handle, int page, int texture)
{
  popDoc * phandle = (popDoc *) handle;
  poppler::document * doc = phandle->doc;

  int doc_page = page;
  if (doc_page < 0 || doc_page >= doc->pages()) {
      printf("specified page number out of page count");
      return 1;
  }
  std::unique_ptr<poppler::page> p(doc->create_page(doc_page));
  if (!p.get()) {
      printf("NULL page");
      return 2;
  }

  poppler::page_renderer pr;
  pr.set_render_hint(poppler::page_renderer::antialiasing, true);
  pr.set_render_hint(poppler::page_renderer::text_antialiasing, true);
  
  double pg_w = 72.0;
  double pg_h = 72.0;
  
  poppler::image img = pr.render_page(p.get(), pg_w, pg_h);
  if (!img.is_valid()) {
      printf("rendering failed");
      return 3;
  }
  img = pr.render_page(p.get(), (phandle->width / img.width ()) * pg_w, (phandle->height / img.height ()) * pg_h, -1, -1, phandle->width, phandle->height);
  if (!img.is_valid()) {
      printf("second rendering failed");
      return 4;
  }

  int width = img.width ();
  int height = img.height();
  const char * data = img.const_data ();
  
  glEnable (GL_TEXTURE_2D);
  glBindTexture (GL_TEXTURE_2D, texture);

#ifdef ANDROID
  unsigned char * dd = new unsigned char [width * height * 4];
  unsigned char * s = (unsigned char *) data;
  unsigned char * d = dd;
  
  for (int i = 0; i < height * width; i++) 
  {
    *d++ = *(s+2);
    *d++ = *(s+1);
    *d++ = *(s+0);
    *d++ = *(s+3);
    s += 4;
  }
  glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, dd);
#else
  glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, data);
#endif

  glBindTexture (GL_TEXTURE_2D, 0);
  glDisable (GL_TEXTURE_2D);
//   delete [] data;
#ifdef ANDROID
  delete [] dd;
#endif
  
  int r = 0;
  r = glGetError ();
//   r = height;
  return r;
}

void popMoveByPage (void * handle, int d)
{
}

void popGoToPage (void * handle, int page)
{
}

int popGetPageCount (void * handle)
{
  popDoc * phandle = (popDoc *) handle;

  poppler::document * doc = phandle->doc;
  return doc->pages ();
}

// Cover rendering is quite slow, and is not thread safe if updating texture content at the same time.
// Render in the background to a static buffer, which can be retrieved later with renderCover.
int popPrepareCover (void * handle, int width, int height)
{
  popDoc * phandle = (popDoc *) handle;
  poppler::document * doc = phandle->doc;

  int doc_page = 0;
  if (doc_page < 0 || doc_page >= doc->pages()) {
      printf("specified page number out of page count");
      return 1;
  }
  std::unique_ptr<poppler::page> p(doc->create_page(doc_page));
  if (!p.get()) {
      printf("NULL page");
      return 2;
  }

  poppler::page_renderer pr;
  pr.set_render_hint(poppler::page_renderer::antialiasing, true);
  pr.set_render_hint(poppler::page_renderer::text_antialiasing, true);
  
  double pg_w = 72.0;
  double pg_h = 72.0;
  
  poppler::image img = pr.render_page(p.get(), pg_w, pg_h);
  if (!img.is_valid()) {
      printf("rendering failed");
      return 3;
  }
  img = pr.render_page(p.get(), (phandle->width / img.width ()) * pg_w, (phandle->height / img.height ()) * pg_h, -1, -1, phandle->width, phandle->height);

  if (phandle->coverBuf != NULL)
  {
    delete [] phandle->coverBuf;
  }
  phandle->coverBuf = new char [width * height * 4];
  if (phandle->coverBuf != NULL)
  {
    memcpy (phandle->coverBuf, img.const_data (), width * height * 4);
  }
  else
  {
    return 4;
  }

#ifdef ANDROID
  unsigned char * s = (unsigned char *) phandle->coverBuf;

  for ( int i = 0; i < height * width; i++) 
  {
    unsigned char t = *(s+2);
    *(s+2) = *(s+0);
    *(s+0) = t;
    s += 4;
  }
#endif  
  
  return 0;
  
}

// Requires a prepareCover first.
int popRenderCover (void * handle, int texture, int width, int height)
{
  popDoc * phandle = (popDoc *) handle;
  
  if (phandle->coverBuf == NULL)
  {
    return 1;
  }
  glEnable (GL_TEXTURE_2D);
  glBindTexture (GL_TEXTURE_2D, texture);


#ifdef ANDROID
  glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, phandle->coverBuf);
#else
  glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, phandle->coverBuf);
#endif
  glBindTexture (GL_TEXTURE_2D, 0);
  glDisable (GL_TEXTURE_2D);
  
  return 0;
}

int popSetFontSize (void * handle, int fontsize)
{
  return 0;
}

int popGetFontSize (void * handle)
{
  int fontsize = 10;
  return fontsize;
}

