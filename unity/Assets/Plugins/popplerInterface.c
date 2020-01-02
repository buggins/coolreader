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

  bool popLoadDocument(void * handle, const char * fname, int width, int height);
  
  char * popGetTitle(void * handle);
  
//   const char * popGetLanguage(void * handle);
//   
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

// int main(int argc, char *argv[])
// {
//     const std::string file_name("algorithms-05-00588.pdf");
// 
//     std::unique_ptr<poppler::document> doc(poppler::document::load_from_file(file_name));
//     if (!doc.get()) {
//         printf("loading error");
//     }
//     if (doc->is_locked()) {
//         printf("encrypted document");
//     }
// 
//     int doc_page = 2;
//     if (doc_page < 0 || doc_page >= doc->pages()) {
//         printf("specified page number out of page count");
//     }
//     std::unique_ptr<poppler::page> p(doc->create_page(doc_page));
//     if (!p.get()) {
//         printf("NULL page");
//     }
// 
//     poppler::page_renderer pr;
//     pr.set_render_hint(poppler::page_renderer::antialiasing, true);
//     pr.set_render_hint(poppler::page_renderer::text_antialiasing, true);
// 
//     poppler::image img = pr.render_page(p.get(), 200.0, 200.0);
//     if (!img.is_valid()) {
//         printf("rendering failed");
//     }
// 
//     if (!img.save("data.png", "png")) {
//         printf("saving to file failed");
//     }
// 
//     return 0;
// }

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

/*
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
  
  int getFontSize (void * handle);
  
}
*/

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
/*  
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
  
  return result;*/
}

char * popGetTitle(void * handle) 
{ 
  popDoc * phandle = (popDoc *) handle;

  poppler::document * doc = phandle->doc;
  std::string title = doc->get_title ().to_latin1 ();
  return (char *) remap (title);
/*  
  LVDocView * a = (LVDocView *) handle;
  lString16 b = a->getTitle ();
  char * c = remap (b);
  return c; */
}


// const char * popGetLanguage(void * handle) 
// { 
//   popDoc * phandle = (popDoc *) handle;
//   
//   poppler::document * doc = phandle->doc;
//   std::string title = doc->get_subject ().to_latin1 ();
//   return (char *) title.c_str ();
// //   LVDocView * a = (LVDocView *) handle;
// //   lString16 b = a->getLanguage(); 
// //   char * c = remap (b);
// //   return c;
// }
// 
const char * popGetAuthors(void * handle) 
{ 
  popDoc * phandle = (popDoc *) handle;

  poppler::document * doc = phandle->doc;
  std::string author = doc->get_author ().to_latin1 ();
  return (char *) remap (author);
//   LVDocView * a = (LVDocView *) handle;
//   lString16 b = a->getAuthors(); 
//   char * c = remap (b);
//   return c; 
}

// Render a page to a texture image. The texture handle is provided (from Unity).
int popRenderPage (void * handle, int page, int texture)
{
  popDoc * phandle = (popDoc *) handle;
  poppler::document * doc = phandle->doc;

  int doc_page = page;
  if (doc_page < 0 || doc_page >= doc->pages()) {
      printf("specified page number out of page count");
  }
  std::unique_ptr<poppler::page> p(doc->create_page(doc_page));
  if (!p.get()) {
      printf("NULL page");
  }

  poppler::page_renderer pr;
  pr.set_render_hint(poppler::page_renderer::antialiasing, true);
  pr.set_render_hint(poppler::page_renderer::text_antialiasing, true);
  
  double pg_w = 72.0;
  double pg_h = 72.0;
  
  poppler::image img = pr.render_page(p.get(), pg_w, pg_h);
  if (!img.is_valid()) {
      printf("rendering failed");
  }
  img = pr.render_page(p.get(), (phandle->width / img.width ()) * pg_w, (phandle->height / img.height ()) * pg_h, -1, -1, phandle->width, phandle->height);

//   if (!img.save("data.png", "png")) {
//       printf("saving to file failed");
//   }
  
//   a->goToPage (page);
//   // Get the image.
//   LVDocImageRef p = a->getPageImage (0);

  // Copy it from the buffer to another contiguous buffer.
//   LVDrawBuf * buf = p->getDrawBuf();
  int width = img.width ();
  int height = img.height();
  //int bpp = buf->GetBitsPerPixel();
  
//  CRLog::info("AAA %d %d %d\n", width, height, img.format ());
  const char * data = img.const_data ();
//  CRLog::info("drawing tex: %d %d %d   %d", width, height, bpp, texture);
//   for ( int i=0; i<height; i++ ) {
//     unsigned char * dst = data + 4 * i * width;
//     unsigned char * src = buf->GetScanLine(i);
//     
//     for ( int x=0; x<width; x++ ) {
//       *dst++ = *(src+2);
//       *dst++ = *(src+1);
//       *dst++ = *(src+0);
//       *dst++ = *(src+3);
//       src += 4;
//     }
//   }
  
//   glGetError ();
//   width = 256;
//   height = 256;
//   char * data = new char [4*width*height];
//   data[0] = 128;
//   data[1] = 12;
//   data[2] = 250;
//   data[3] = 255;
  
  glEnable (GL_TEXTURE_2D);
  glBindTexture (GL_TEXTURE_2D, texture);
  glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, data);
  glBindTexture (GL_TEXTURE_2D, 0);
  glDisable (GL_TEXTURE_2D);
//   delete [] data;
  
  int r = glGetError ();
  r = height;
  return r;
}

void popMoveByPage (void * handle, int d)
{
//   LVDocView * a = (LVDocView *) handle;
//   a->moveByPage (d);
}

void popGoToPage (void * handle, int page)
{
//   LVDocView * a = (LVDocView *) handle;
//   a->goToPage (page);
}

int popGetPageCount (void * handle)
{
  popDoc * phandle = (popDoc *) handle;

  poppler::document * doc = phandle->doc;
  return doc->pages ();
//   LVDocView * a = (LVDocView *) handle;
//   return a->getPageCount ();
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
  }
  std::unique_ptr<poppler::page> p(doc->create_page(doc_page));
  if (!p.get()) {
      printf("NULL page");
  }

  poppler::page_renderer pr;
  pr.set_render_hint(poppler::page_renderer::antialiasing, true);
  pr.set_render_hint(poppler::page_renderer::text_antialiasing, true);
  
  double pg_w = 72.0;
  double pg_h = 72.0;
  
  poppler::image img = pr.render_page(p.get(), pg_w, pg_h);
  if (!img.is_valid()) {
      printf("rendering failed");
  }
  img = pr.render_page(p.get(), (phandle->width / img.width ()) * pg_w, (phandle->height / img.height ()) * pg_h, -1, -1, phandle->width, phandle->height);

  if (phandle->coverBuf != NULL)
  {
    delete [] phandle->coverBuf;
  }
  phandle->coverBuf = new char [width * height * 4];
  memcpy (phandle->coverBuf, img.const_data (), width * height * 4);

  return 0;
  
//   int r = glGetError ();
//   r = height;
//   return r;
//   LVDocView * a = (LVDocView *) handle;
//   
//   if (buf != NULL)
//   {
//     delete buf;
//     buf = NULL;
//   }
//   if (buf == NULL)
//   {
//     buf = new LVColorDrawBuf (width, height);
//   }
//   
//   lvRect rc (0, 0, width, height);
//   
//   (*buf).Clear (0xFFFFFF);  
//   a->drawCoverTo (buf, rc);
//   
//   return 0;
}

// Requires a prepareCover first.
int popRenderCover (void * handle, int texture, int width, int height)
{
  popDoc * phandle = (popDoc *) handle;
  
  glEnable (GL_TEXTURE_2D);
  glBindTexture (GL_TEXTURE_2D, texture);
  glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, phandle->coverBuf);
  glBindTexture (GL_TEXTURE_2D, 0);
  glDisable (GL_TEXTURE_2D);
// //   delete [] data;
//   
// //  LVDocView * a = (LVDocView *) handle;
//   
// //   LVColorDrawBuf buf (width, height);
// //   lvRect rc (0, 0, width, height);
//   
// //   buf.Clear (0xFFFFFF);  
// //   a->drawCoverTo (&buf, rc);
//   
//   unsigned char * data = new unsigned char [width * height * 4];
//   CRLog::info("drawing cover: %d %d   %d", width, height, texture);
//   for (int i = 0; i < height; i++) 
//   {
//     unsigned char * dst = data + 4 * i * width;
//     unsigned char * src = (*buf).GetScanLine(i);
//     
//     for (int x = 0; x < width; x++ ) 
//     {
//       *dst++ = *(src+2);
//       *dst++ = *(src+1);
//       *dst++ = *(src+0);
//       *dst++ = *(src+3);
//       src += 4;
//     }
//   }
//   
//   glEnable (GL_TEXTURE_2D);
//   glBindTexture (GL_TEXTURE_2D, texture);
//   glTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
//   glBindTexture (GL_TEXTURE_2D, 0);
//   glDisable (GL_TEXTURE_2D);
//   delete [] data;
  
  return 0;
}

int popSetFontSize (void * handle, int fontsize)
{
//   LVDocView * a = (LVDocView *) handle;
// //  CRLog::info("setting font: %p %d", a, fontsize);
//   a->setFontSize (fontsize);
// //  CRLog::info("done setting font: %p %d", a, fontsize);
  return 0;
}

int popGetFontSize (void * handle)
{
//   LVDocView * a = (LVDocView *) handle;
// //  CRLog::info("setting font: %p %d", a, fontsize);
//   int fontsize = a->getFontSize ();
// //  CRLog::info("done setting font: %p %d", a, fontsize);
  int fontsize = 10;
  return fontsize;
}

