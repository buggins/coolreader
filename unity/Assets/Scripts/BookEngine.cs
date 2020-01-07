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
    coolreader,
    poppler,
  };
  
  private BookFormat format = BookFormat.unknown;
 
  private static Dictionary <string, BookFormat> formatExtensions = new Dictionary <string, BookFormat> ()
  {
    { ".epub", BookFormat.coolreader },
    { ".fb2", BookFormat.coolreader },
    { ".doc", BookFormat.coolreader },
//    { ".txt", BookFormat.coolreader },
    { ".rtf", BookFormat.coolreader },
    { ".html", BookFormat.coolreader },
    { ".chm", BookFormat.coolreader },
    { ".tcr", BookFormat.coolreader },
    { ".pdb", BookFormat.coolreader },
    { ".prc", BookFormat.coolreader },
    { ".mobi", BookFormat.coolreader },
    { ".pml", BookFormat.coolreader },
    { ".pdf", BookFormat.poppler },
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
  
  public static List <string> getAcceptedFormats ()
  {
    List <string> result = new List <string> ();
    foreach (string key in formatExtensions.Keys)
    {
      result.Add (key.ToLower ());
      result.Add (key.ToUpper ());
    }
    return result;
  }
  
  // The handle to the coolreader engine interface.
  private BookEngineInterface cri;
  
  // The handle to the poppler engine interface.
  private BookEngineInterface pop;
  
  private IntPtr handle = IntPtr.Zero;
    
  public BookEngine ()
  {
    cri = new CoolReaderEngine ();
    pop = new PopplerEngine ();
  }

  IntPtr BookEngineInterface.BEIDocViewCreate (string fname)
  {
    BookFormat newFormat = getFormatFromName (fname);
    
    if ((newFormat != format) || (handle == IntPtr.Zero))
    {
      if (handle != IntPtr.Zero)
      {
        switch (format)
        {
// TODO          
          case BookFormat.coolreader:
//            cri.BEIReleaseHandle (handle);
            break;
          case BookFormat.poppler:
//            pop.BEIReleaseHandle (handle);
            break;
        }
      }
      
      switch (newFormat)
      {
        case BookFormat.coolreader:
          // Create a book with a default font size.
          handle = cri.BEIDocViewCreate (fname);
          Debug.Log ("CRI Handle " + handle);
          break;
        case BookFormat.poppler:
          // Create a book with a default font size.
          handle = pop.BEIDocViewCreate (fname);
          Debug.Log ("Poppler Handle " + handle);
          break;
      }
      format = newFormat;
    }
    return handle;
  }

  bool BookEngineInterface.BEILoadDocument(IntPtr handle, string fname, int width, int height)
  {
    bool result = false;
    
    switch (format)
    {
      case BookFormat.coolreader:
        result = cri.BEILoadDocument (handle, fname, width, height);
        break;
      case BookFormat.poppler:
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
      case BookFormat.coolreader:
        result = cri.BEIGetTitle (handle);
        break;
      case BookFormat.poppler:
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
      case BookFormat.coolreader:
        result = cri.BEIGetAuthors (handle);
        break;
      case BookFormat.poppler:
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
      case BookFormat.coolreader:
        result = cri.BEIRenderPage (handle, page, texture);
        break;
      case BookFormat.poppler:
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
      case BookFormat.coolreader:
        cri.BEIMoveByPage (handle, d);
        break;
      case BookFormat.poppler:
        pop.BEIMoveByPage (handle, d);
        break;
    }
  }

  void BookEngineInterface.BEIGoToPage(IntPtr handle, int page)
  {
    switch (format)
    {
      case BookFormat.coolreader:
        cri.BEIGoToPage (handle, page);
        break;
      case BookFormat.poppler:
        pop.BEIGoToPage (handle, page);
        break;
    }
  }

  int BookEngineInterface.BEIGetPageCount (IntPtr handle)
  {
    int pagecount = 0;
    switch (format)
    {
      case BookFormat.coolreader:
        pagecount = cri.BEIGetPageCount (handle);
        break;
      case BookFormat.poppler:
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
      case BookFormat.coolreader:
        result = cri.BEIPrepareCover (handle, width, height);
        break;
      case BookFormat.poppler:
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
      case BookFormat.coolreader:
        result = cri.BEIRenderCover (handle, texture, width, height);
        break;
      case BookFormat.poppler:
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
      case BookFormat.coolreader:
        result = cri.BEISetFontSize (handle, fontsize);
        break;
      case BookFormat.poppler:
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
      case BookFormat.coolreader:
        result = cri.BEIGetFontSize (handle);
        break;
      case BookFormat.poppler:
        result = pop.BEIGetFontSize (handle);
        break;
    }
    return result;
  }
}
