using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System;
using System.Runtime.InteropServices;
using System.Threading;

/* Mutex protected class for accessing cool reader native code. */
public class PopplerEngine {

  public static PopplerEngine instance = null;

  private Mutex m;
  
  [DllImport ("PopplerInterface")]
  private static extern IntPtr popDocViewCreate (int bitsPerPixel);

  [DllImport ("PopplerInterface")]
  private static extern bool popLoadDocument (IntPtr handle, string fname, int width, int height);
  
  [DllImport ("PopplerInterface")]
  private static extern IntPtr popGetTitle (IntPtr handle);

//   [DllImport ("PopplerInterface")]
//   private static extern IntPtr popGetLanguage (IntPtr handle);
// 
  [DllImport ("PopplerInterface")]
  private static extern IntPtr popGetAuthors (IntPtr handle);

  [DllImport ("PopplerInterface")]
  private static extern int popRenderPage (IntPtr handle, int page, IntPtr texture);

  [DllImport ("PopplerInterface")]
  private static extern void popMoveByPage (IntPtr handle, int d);

  [DllImport ("PopplerInterface")]
  private static extern void popGoToPage (IntPtr handle, int page);

  [DllImport ("PopplerInterface")]
  private static extern int popGetPageCount (IntPtr handle);

  [DllImport ("PopplerInterface")]
  private static extern int popPrepareCover (IntPtr handle, int width, int height);
  
  [DllImport ("PopplerInterface")]
  private static extern int popRenderCover (IntPtr handle, IntPtr texture, int width, int height);
  
  [DllImport ("PopplerInterface")]
  private static extern int popSetFontSize (IntPtr handle, int fontsize);
  
  [DllImport ("PopplerInterface")]
  private static extern int popGetFontSize (IntPtr handle);
  
  public PopplerEngine ()
  {
    // force a singleton.
    Debug.Log ("Creating pop");
    if (instance == null)
    {
      instance = this;
      
      m = new Mutex ();
    }
  }
  
  public IntPtr PopplerDocViewCreate (int bitsPerPixel)
  {
    instance.m.WaitOne ();
    IntPtr result = popDocViewCreate (bitsPerPixel);
    instance.m.ReleaseMutex ();
    return result;
  }



  public bool PopplerLoadDocument(IntPtr handle, string fname, int width, int height)
  {
    instance.m.WaitOne ();
    bool result = popLoadDocument (handle, fname, width, height);
    instance.m.ReleaseMutex ();
    return result;
  }

  public IntPtr PopplerGetTitle(IntPtr handle)
  {
    instance.m.WaitOne ();
    IntPtr result = popGetTitle (handle);
    instance.m.ReleaseMutex ();
    return result;
  }

//   public IntPtr PopplerGetLanguage(IntPtr handle)
//   {
//     instance.m.WaitOne ();
//     IntPtr result = popGetLanguage (handle);
//     instance.m.ReleaseMutex ();
//     return result;
//   }
// 
  public IntPtr PopplerGetAuthors(IntPtr handle)
  {
    instance.m.WaitOne ();
    IntPtr result = popGetAuthors (handle);
    instance.m.ReleaseMutex ();
    return result;
  }

  public int PopplerRenderPage(IntPtr handle, int page, IntPtr texture)
  {
    instance.m.WaitOne ();
    int result = popRenderPage (handle, page, texture);
    instance.m.ReleaseMutex ();
    return result;
  }

  public void PopplerMoveByPage(IntPtr handle, int d)
  {
    instance.m.WaitOne ();
    popMoveByPage (handle, d);
    instance.m.ReleaseMutex ();
  }

  public void PopplerGoToPage(IntPtr handle, int page)
  {
    instance.m.WaitOne ();
    popGoToPage (handle, page);
    instance.m.ReleaseMutex ();
  }

  public int PopplerGetPageCount (IntPtr handle)
  {
    int p = -1;
    instance.m.WaitOne ();
    p = popGetPageCount (handle);
    instance.m.ReleaseMutex ();
    return p;
  }

  public int PopplerPrepareCover (IntPtr handle, int width, int height)
  {
    instance.m.WaitOne ();
    int result = popPrepareCover (handle, width, height);
    instance.m.ReleaseMutex ();
    return result;
  }

  public int PopplerRenderCover (IntPtr handle, IntPtr texture, int width, int height)
  {
    instance.m.WaitOne ();
    int result = popRenderCover (handle, texture, width, height);
    instance.m.ReleaseMutex ();
    return result;
  }

  public int PopplerSetFontSize (IntPtr handle, int fontsize)
  {
    instance.m.WaitOne ();
    int result = popSetFontSize (handle, fontsize);
    instance.m.ReleaseMutex ();
    return result;
  }

  public int PopplerGetFontSize (IntPtr handle)
  {
    instance.m.WaitOne ();
    int result = popGetFontSize (handle);
    instance.m.ReleaseMutex ();
    return result;
  }
}
