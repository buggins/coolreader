using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System;
using System.Runtime.InteropServices;
using System.Threading;

/* Mutex protected class for accessing cool reader native code. */
public class CoolReaderInterface {

  public static CoolReaderInterface instance = null;

  private Mutex m;
  
//   [DllImport ("CREngine", EntryPoint="loadLibrary")]
//   private static extern bool loadLibraryCREngine ();
//   
  [DllImport ("CREngine")]
  private static extern IntPtr LVDocViewCreate (int bitsPerPixel);

  [DllImport ("CREngine")]
  private static extern bool LoadDocument (IntPtr handle, string fname, int width, int height);
  
  [DllImport ("CREngine")]
  private static extern IntPtr getTitle (IntPtr handle);

//   [DllImport ("CREngine")]
//   private static extern IntPtr getLanguage (IntPtr handle);
// 
  [DllImport ("CREngine")]
  private static extern IntPtr getAuthors (IntPtr handle);

  [DllImport ("CREngine")]
  private static extern int renderPage (IntPtr handle, int page, IntPtr texture);

  [DllImport ("CREngine")]
  private static extern void moveByPage (IntPtr handle, int d);

  [DllImport ("CREngine")]
  private static extern void goToPage (IntPtr handle, int page);

  [DllImport ("CREngine")]
  private static extern int getPageCount (IntPtr handle);

  [DllImport ("CREngine")]
  private static extern int prepareCover (IntPtr handle, int width, int height);
  
  [DllImport ("CREngine")]
  private static extern int renderCover (IntPtr handle, IntPtr texture, int width, int height);
  
  [DllImport ("CREngine")]
  private static extern int setFontSize (IntPtr handle, int fontsize);
  
  [DllImport ("CREngine")]
  private static extern int getFontSize (IntPtr handle);
  
  public CoolReaderInterface ()
  {
    // force a singleton.
    Debug.Log ("Creating cri");
    if (instance == null)
    {
      instance = this;
      
      m = new Mutex ();
    }
  }

  public IntPtr CRDocViewCreate (int bitsPerPixel)
  {
    instance.m.WaitOne ();
    IntPtr result = LVDocViewCreate (bitsPerPixel);
    instance.m.ReleaseMutex ();
    return result;
  }

  public bool CRLoadDocument(IntPtr handle, string fname, int width, int height)
  {
    instance.m.WaitOne ();
    bool result = LoadDocument (handle, fname, width, height);
    instance.m.ReleaseMutex ();
    return result;
  }

  public IntPtr CRGetTitle(IntPtr handle)
  {
    instance.m.WaitOne ();
    IntPtr result = getTitle (handle);
    instance.m.ReleaseMutex ();
    return result;
  }

//   public IntPtr CRGetLanguage(IntPtr handle)
//   {
//     instance.m.WaitOne ();
//     IntPtr result = getLanguage (handle);
//     instance.m.ReleaseMutex ();
//     return result;
//   }
// 
  public IntPtr CRGetAuthors(IntPtr handle)
  {
    instance.m.WaitOne ();
    IntPtr result = getAuthors (handle);
    instance.m.ReleaseMutex ();
    return result;
  }

  public int CRRenderPage(IntPtr handle, int page, IntPtr texture)
  {
    instance.m.WaitOne ();
    int result = renderPage (handle, page, texture);
    instance.m.ReleaseMutex ();
    return result;
  }

  public void CRMoveByPage(IntPtr handle, int d)
  {
    instance.m.WaitOne ();
    moveByPage (handle, d);
    instance.m.ReleaseMutex ();
  }

  public void CRGoToPage(IntPtr handle, int page)
  {
    instance.m.WaitOne ();
    goToPage (handle, page);
    instance.m.ReleaseMutex ();
  }

  public int CRGetPageCount (IntPtr handle)
  {
    int p = -1;
    instance.m.WaitOne ();
    p = getPageCount (handle);
    instance.m.ReleaseMutex ();
    return p;
  }

  public int CRPrepareCover (IntPtr handle, int width, int height)
  {
    instance.m.WaitOne ();
    int result = prepareCover (handle, width, height);
    instance.m.ReleaseMutex ();
    return result;
  }

  public int CRRenderCover (IntPtr handle, IntPtr texture, int width, int height)
  {
    instance.m.WaitOne ();
    int result = renderCover (handle, texture, width, height);
    instance.m.ReleaseMutex ();
    return result;
  }

  public int CRSetFontSize (IntPtr handle, int fontsize)
  {
    instance.m.WaitOne ();
    int result = setFontSize (handle, fontsize);
    instance.m.ReleaseMutex ();
    return result;
  }

  public int CRGetFontSize (IntPtr handle)
  {
    instance.m.WaitOne ();
    int result = getFontSize (handle);
    instance.m.ReleaseMutex ();
    return result;
  }
}
