using UnityEngine;
using System;
using System.Collections.Generic;

// A common interface to the various engines used to access different types of books.
interface BookEngineInterface
{
  IntPtr BEIDocViewCreate (string fname);
  bool BEILoadDocument(IntPtr handle, string fname, int width, int height);
  IntPtr BEIGetTitle(IntPtr handle);
  IntPtr BEIGetAuthors(IntPtr handle);
  int BEIRenderPage(IntPtr handle, int page, IntPtr texture);
  void BEIMoveByPage(IntPtr handle, int d);
  void BEIGoToPage(IntPtr handle, int page);
  int BEIGetPageCount (IntPtr handle);
  int BEIPrepareCover (IntPtr handle, int width, int height);
  int BEIRenderCover (IntPtr handle, IntPtr texture, int width, int height);
  int BEISetFontSize (IntPtr handle, int fontsize);
  int BEIGetFontSize (IntPtr handle);
}

// The class through which books are accessed. This class
// then relays calls to the appropriate book engine for
// the individual book format.
public class BookEngine : BookEngineInterface {

  public enum BookFormat
  {
    unknown,
    epub,
    pdf,
  };
  
  private BookFormat format = BookFormat.unknown;
 
  private static Dictionary <string, BookFormat> formatExtensions = new Dictionary <string, BookFormat> ()
  {
    { ".epub", BookFormat.epub },
    { ".pdf", BookFormat.pdf },
  };
  
  public static BookFormat getFormatFromName (string filename)
  {
    string path = filename.ToLower ();
    foreach (KeyValuePair <string, BookFormat> entry in formatExtensions)
    {
      if (path.EndsWith (entry.Key))
      {
        return entry.Value;
      }
    }
    return BookFormat.unknown;
  }
  
  // The handle to the coolreader engine interface.
  private BookEngineInterface cri;
  
  // The handle to the poppler engine interface.
  private BookEngineInterface pop;
    
  public BookEngine ()
  {
    cri = new CoolReaderEngine ();
    pop = new PopplerEngine ();
  }

  IntPtr BookEngineInterface.BEIDocViewCreate (string fname)
  {
    IntPtr bookHandle = IntPtr.Zero;
    format = getFormatFromName (fname);
    switch (format)
    {
      case BookFormat.epub:
        // Create a book with a default font size.
        bookHandle = cri.BEIDocViewCreate (fname);
        Debug.Log ("CRI Handle" + bookHandle);
        break;
      case BookFormat.pdf:
        // Create a book with a default font size.
        bookHandle = pop.BEIDocViewCreate (fname);
        Debug.Log ("Poppler Handle" + bookHandle);
        break;
    }
    return bookHandle;
  }

  bool BookEngineInterface.BEILoadDocument(IntPtr handle, string fname, int width, int height)
  {
    bool result = false;
    
    switch (format)
    {
      case BookFormat.epub:
        result = cri.BEILoadDocument (handle, fname, width, height);
        break;
      case BookFormat.pdf:
        result = pop.BEILoadDocument (handle, fname, width, height);
        Debug.Log ("Load pop " + result + "  " + handle + " " + fname);
        break;
    }
        Debug.Log ("Loaded " + result + "  " + handle + " " + fname + " " + format);
    return result;
  }

  IntPtr BookEngineInterface.BEIGetTitle(IntPtr handle)
  {
    IntPtr result = IntPtr.Zero;
    switch (format)
    {
      case BookFormat.epub:
        result = cri.BEIGetTitle (handle);
        break;
      case BookFormat.pdf:
        result = pop.BEIGetTitle (handle);
        break;
    }
    return result;
  }

  IntPtr BookEngineInterface.BEIGetAuthors(IntPtr handle)
  {
    IntPtr result = IntPtr.Zero;
    switch (format)
    {
      case BookFormat.epub:
        result = cri.BEIGetAuthors (handle);
        break;
      case BookFormat.pdf:
        result = pop.BEIGetAuthors (handle);
        break;
    }
    return result;
  }

  int BookEngineInterface.BEIRenderPage(IntPtr handle, int page, IntPtr texture)
  {
    int result = 0;
    switch (format)
    {
      case BookFormat.epub:
        result = cri.BEIRenderPage (handle, page, texture);
        break;
      case BookFormat.pdf:
        result = pop.BEIRenderPage (handle, page, texture);
        break;
    }
    return result;
  }

  // Deprecate?
  void BookEngineInterface.BEIMoveByPage(IntPtr handle, int d)
  {
    switch (format)
    {
      case BookFormat.epub:
        cri.BEIMoveByPage (handle, d);
        break;
      case BookFormat.pdf:
        pop.BEIMoveByPage (handle, d);
        break;
    }
  }

  void BookEngineInterface.BEIGoToPage(IntPtr handle, int page)
  {
    switch (format)
    {
      case BookFormat.epub:
        cri.BEIGoToPage (handle, page);
        break;
      case BookFormat.pdf:
        pop.BEIGoToPage (handle, page);
        break;
    }
  }

  int BookEngineInterface.BEIGetPageCount (IntPtr handle)
  {
    int pagecount = 0;
    switch (format)
    {
      case BookFormat.epub:
        pagecount = cri.BEIGetPageCount (handle);
        break;
      case BookFormat.pdf:
        pagecount = pop.BEIGetPageCount (handle);
        break;
    }
    return pagecount;
  }

  int BookEngineInterface.BEIPrepareCover (IntPtr handle, int width, int height)
  {
    int result = 0;
    switch (format)
    {
      case BookFormat.epub:
        result = cri.BEIPrepareCover (handle, width, height);
        break;
      case BookFormat.pdf:
        result = pop.BEIPrepareCover (handle, width, height);
        break;
    }
    return result;
  }

  int BookEngineInterface.BEIRenderCover (IntPtr handle, IntPtr texture, int width, int height)
  {
    int result = 0;
    switch (format)
    {
      case BookFormat.epub:
        result = cri.BEIRenderCover (handle, texture, width, height);
        break;
      case BookFormat.pdf:
        result = pop.BEIRenderCover (handle, texture, width, height);
        break;
    }
    return result;
  }

  int BookEngineInterface.BEISetFontSize (IntPtr handle, int fontsize)
  {
    int result = 0;
    switch (format)
    {
      case BookFormat.epub:
        result = cri.BEISetFontSize (handle, fontsize);
        break;
      case BookFormat.pdf:
        result = pop.BEISetFontSize (handle, fontsize);
        break;
    }
    return result;
  }

  int BookEngineInterface.BEIGetFontSize (IntPtr handle)
  {
    int result = 0;
    switch (format)
    {
      case BookFormat.epub:
        result = cri.BEIGetFontSize (handle);
        break;
      case BookFormat.pdf:
        result = pop.BEIGetFontSize (handle);
        break;
    }
    return result;
  }
}
