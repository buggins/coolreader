using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System;
using System.Runtime.InteropServices;
using System.Threading;

/*
 * The book manager handles the operation of an individual book. This provides the interface
 * between the affordances expected of a book (page turning, page rendering, etc.) and the
 * coolreader library that provides access to the content and settings.
 */
public class BookManager : MonoBehaviour {
  
  public enum BookFormat
  {
    unknown,
    epub,
    pdf,
  };
  
  // The handle to the coolreader engine interface.
  private CoolReaderInterface cri;
  
  // The handle to the poppler engine interface.
  private PopplerEngine pop;
  
  private BookFormat format = BookFormat.unknown;
  
  // The handle to the current book, as used by the engine.
  private IntPtr bookHandle;
  // Book state: current page open.
  private int currentPage;

  // True when the book has finished loading from file.
  private bool bookLoaded = false;
  
  // The current font size
  private int fontSize = 32;
  
  // Materials used for the book.
  // Left page, currently in view.
  private Material bookLeftPage;
  // Right page, currently in view.
  private Material bookRightPage;
  // Non visible left page, but can be seen while page turning.
  private Material bookLeftPageTurn;
  // The non visible right page seen when page turning.
  private Material bookRightPageTurn;
  // The front cover image.
  private Material bookFrontCover;
  // The back cover image.
  private Material bookBackCover;
  // The spine material.
  private Material bookSpine;
  // A material for any remaining part of the book.
  private Material bookMaterial;
  
  // The texture for the visible left page, containing contents of that page.
  private Texture2D leftPage;
  // The texture with the contents of the visible right page.
  private Texture2D rightPage;
  
  // The texture written with the contents of the new left page during turning.
  private Texture2D leftPageTurn;
  // The texture written with the contents of the new right page during turning.
  private Texture2D rightPageTurn;
  
  // The texture that the front cover image is rendered to.
  private Texture2D frontCover;
  
  // A scratch texture used for rendering during calls to the cool reader engine.
  private Texture directRenderTexture;
  
  // A link to the informational message.
  private TextMesh informationMessage;
  
  // The properties of the book.
  private BookPropertySet bookProperties;
  
  public delegate void StateChangeEvent ();
  
  // Any event to subscribe to for anyone wanting notifications of book state updates.
  public event StateChangeEvent onStateChange;
  
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
  
  // Use this for initialization
  void Awake () {
    cri = new CoolReaderInterface ();
    pop = new PopplerEngine ();
    
  }
  
  // Used to signal to any interested parties that the book
  // state has updated as a result of any long drawn out processes.
  // (such as re-rendering the book).
  private void stateChanged ()
  {
    onStateChange ();
  }
  
  private void setInformation (string message)
  {
    if (informationMessage == null)
    {
      Transform m = gameObject.transform.Find ("AdviceMessage");
      if (m != null)
      {
        informationMessage = m.gameObject.GetComponent <TextMesh> ();
      }
    }
    if (informationMessage != null)
    {
      informationMessage.text = message;
    }
  }
  
  // Set up the materials and textures required.
  private void initializeTextures ()
  {
    // Access materials for various types of pages.
    bookLeftPage = null;
    bookRightPage = null;
    bookLeftPageTurn = null;
    bookRightPageTurn = null;
    bookFrontCover = null;
    bookBackCover = null;
    bookSpine = null;
    
    // Assign materials to book. This will need to adapt if a 
    // different book model is used in future.
    SkinnedMeshRenderer smr = transform.GetComponentInChildren <SkinnedMeshRenderer> ();
    if (smr != null)
    {
      bookRightPage = smr.materials[0];
      bookMaterial = smr.materials[1];
      bookFrontCover = smr.materials[2];
      bookSpine = smr.materials[3];
      bookBackCover = smr.materials[4];
      bookRightPageTurn = smr.materials[5];
      bookLeftPageTurn = smr.materials[6];
      bookLeftPage = smr.materials[7];            
    }
    else
    {
      Debug.Log ("Could not assign materials to book");
    }
    
    // Create the textures used to hold book content.
    int rwidth = 2048;
    int rheight = 2048;

    // Create a texture for rendering to.    
    directRenderTexture = new Texture2D (rwidth, rheight, TextureFormat.ARGB32,false);
    
    // Create materials and textures for display. Anti-aliasing and filtering
    // is crucial for getting best readability.
    leftPage = new Texture2D (rwidth, rheight, TextureFormat.ARGB32,true);
    leftPage.filterMode = FilterMode.Bilinear;
    leftPage.anisoLevel = 9;
    leftPage.mipMapBias = -0.5f;
    bookLeftPage.mainTexture = leftPage;
    rightPage = new Texture2D (rwidth, rheight, TextureFormat.ARGB32,true);
    rightPage.filterMode = FilterMode.Bilinear;
    rightPage.anisoLevel = 9;
    rightPage.mipMapBias = -0.5f;
    bookRightPage.mainTexture = rightPage;
    
    leftPageTurn = new Texture2D (rwidth, rheight, TextureFormat.ARGB32,true);
    leftPageTurn.filterMode = FilterMode.Bilinear;
    leftPageTurn.anisoLevel = 9;
    leftPageTurn.mipMapBias = -0.5f;
    bookLeftPageTurn.mainTexture = leftPageTurn;
    rightPageTurn = new Texture2D (rwidth, rheight, TextureFormat.ARGB32,true);
    rightPageTurn.filterMode = FilterMode.Bilinear;
    rightPageTurn.anisoLevel = 9;
    rightPageTurn.mipMapBias = -0.5f;
    bookRightPageTurn.mainTexture = rightPageTurn;
    
    frontCover = new Texture2D (rwidth, rheight, TextureFormat.ARGB32,true);
    frontCover.filterMode = FilterMode.Bilinear;
    frontCover.anisoLevel = 9;
    frontCover.mipMapBias = -0.5f;
    bookFrontCover.mainTexture = frontCover;
  }
  
  // Loading a new book is time consuming. Ideally run this in a separate
  // thread so the rendering cycle does not get interupted. Needs to be
  // done with care, since sensitive to thread interference if both unity 
  // and the coolreader engine are playing with textures (rendering) at the same time.
  private string loadingName;
  private int rx;
  private int ry;
  private bool loading;
  private void doLoading ()
  {
    switch (format)
    {
      case BookFormat.epub:
        // Create a book with a default font size.
        bookHandle = cri.CRDocViewCreate (fontSize);
        Debug.Log ("CRI Handle" + bookHandle);
        cri.CRLoadDocument (bookHandle, loadingName, rx, ry);
    
        cri.CRPrepareCover (bookHandle, rx, ry);
        break;
      case BookFormat.pdf:
        // Create a book with a default font size.
        bookHandle = pop.PopplerDocViewCreate (fontSize);
        Debug.Log ("Poppler Handle" + bookHandle);
        pop.PopplerLoadDocument (bookHandle, loadingName, rx, ry);
    
        pop.PopplerPrepareCover (bookHandle, rx, ry);
        break;
    }
    
    loading = false;
  }
  
  public bool hasLoaded ()
  {
    return bookLoaded;
  }
  
  // Create a book from the filename to the ebook.
  public IEnumerator loadBook (string bookFileName, BookPropertySet props)
  {
    format = getFormatFromName (bookFileName);
    
    setInformation ("Loading");
    yield return null;
    
    initializeTextures ();

    setInformation ("Loading.");
    yield return null;
    
    // Try to load the book in a separate thread.
    loadingName = bookFileName;
    rx = directRenderTexture.width;
    ry = directRenderTexture.height;
    loading = true;
    Thread t = new Thread (new ThreadStart (doLoading));
    t.Start ();
    while (loading)
    {
      yield return new WaitForSeconds (0.1f);
      print ("Waiting");
    }
    
    setInformation ("Loading..");
    yield return null;
    
    // Retrieve the title and author settings.
    string title = "No title found";
    string author = "No author found";
    switch (format)
    {
      case BookFormat.epub:
        title = Marshal.PtrToStringAnsi (cri.CRGetTitle (bookHandle));
        author = Marshal.PtrToStringAnsi (cri.CRGetAuthors (bookHandle));
        break;
      case BookFormat.pdf:
        title = Marshal.PtrToStringAnsi (pop.PopplerGetTitle (bookHandle));
        author = Marshal.PtrToStringAnsi (pop.PopplerGetAuthors (bookHandle));
        break;
    }
    Debug.Log ("Author " + author + " title: " + title);
    
    // Retrieve the cover image. This involves render operations so
    // cannot take place in its own thread.
    retrieveCoverMaterial (frontCover);
    
    setInformation ("Loading...");
    yield return null;
    
    bookProperties = props;
    // Change page to force initial page drawing.
    currentPage = props.currentPage;
//    changePage (currentPage);
//    pageTurnComplete ();

    fontSize = props.fontSize;
    yield return updateFont ();

    fontSize = 10;
    
    switch (format)
    {
      case BookFormat.epub:
        fontSize = cri.CRGetFontSize (bookHandle);
        break;
      case BookFormat.pdf:
        fontSize = 10; // not relevant.
        break;
    }
    
    
    bookLoaded = true;
    
    setInformation ("");
    
    stateChanged ();
  }
  
  // Cover rendering must run in main thread.
  public void retrieveCoverMaterial (Texture2D texture)
  {
    switch (format)
    {
      case BookFormat.epub:
        cri.CRRenderCover (bookHandle, directRenderTexture.GetNativeTexturePtr (), directRenderTexture.width, directRenderTexture.height);
        break;
      case BookFormat.pdf:
        pop.PopplerRenderCover (bookHandle, directRenderTexture.GetNativeTexturePtr (), directRenderTexture.width, directRenderTexture.height);
        break;
    }
    
    // Create a mip-mapped texture version. This is extra work but produces much more readable texture.
    RenderTexture texcopy = RenderTexture.GetTemporary (directRenderTexture.width, directRenderTexture.height, 0, RenderTextureFormat.Default, RenderTextureReadWrite.Linear);
    Graphics.Blit (directRenderTexture, texcopy);
    
    RenderTexture previous = RenderTexture.active;
    RenderTexture.active = texcopy;
    texture.ReadPixels (new Rect(0, 0, directRenderTexture.width, directRenderTexture.height), 0, 0);
    texture.Apply ();
    RenderTexture.active = previous;
    RenderTexture.ReleaseTemporary (texcopy);  
  }

  // Render and copy a given page to the specified texture.
  public void retrievePageToTexture (int page, Texture2D texture)
  {
    switch (format)
    {
      case BookFormat.epub:
        cri.CRGoToPage (bookHandle, page);
        cri.CRRenderPage (bookHandle, page, directRenderTexture.GetNativeTexturePtr ());
        break;
      case BookFormat.pdf:
        pop.PopplerGoToPage (bookHandle, page);
        int r = pop.PopplerRenderPage (bookHandle, page, directRenderTexture.GetNativeTexturePtr ());
        Debug.Log ("Loaded " + r);
        break;
    }
    
    // Create a mip-mapped texture version. This is extra work but produces much more readable texture.
    RenderTexture texcopy = RenderTexture.GetTemporary (directRenderTexture.width, directRenderTexture.height, 0, RenderTextureFormat.Default, RenderTextureReadWrite.Linear);
    Graphics.Blit (directRenderTexture, texcopy);
    
    RenderTexture previous = RenderTexture.active;
    RenderTexture.active = texcopy;
    texture.ReadPixels (new Rect(0, 0, directRenderTexture.width, directRenderTexture.height), 0, 0);
    texture.Apply ();
    RenderTexture.active = previous;
    RenderTexture.ReleaseTemporary (texcopy);
  }
  
  // Page turn animation complete, so swap textures.
  public void pageTurnComplete ()
  {
    // Swap textures rather than re-render all of them.
    Texture2D tmp; 
    tmp = leftPageTurn;
    leftPageTurn = leftPage;
    leftPage = tmp;
    tmp = rightPageTurn;
    rightPageTurn = rightPage;
    rightPage = tmp;
    
    bookLeftPage.mainTexture = leftPage;
    bookRightPage.mainTexture = rightPage;
    bookLeftPageTurn.mainTexture = leftPageTurn;
    bookRightPageTurn.mainTexture = rightPageTurn;
  }
  
  // Attempt to change page. If goes out of limits, then clamps
  // the page at the limit, and returns false.
  public bool changePage (int d)
  {
    return setCurrentPage (currentPage + d);
  }
  
  public int getCurrentPage ()
  {
    return currentPage;
  }
  
  public int getMaxPages ()
  {
    int pagecount = 0;
    switch (format)
    {
      case BookFormat.epub:
        pagecount = cri.CRGetPageCount (bookHandle);
        break;
      case BookFormat.pdf:
        pagecount = pop.PopplerGetPageCount (bookHandle);
        break;
    }
    return pagecount;
  }
  
  // Returns false if target page is out of range. Still restricts
  // visible page to an acceptable value anyway.
  public bool setCurrentPage (int page)
  {
    bool result = true;
    currentPage = page;
    if (currentPage < 0)
    {
      currentPage = 0;
      result = false;
    }

    int maxPages = 1;
    switch (format)
    {
      case BookFormat.epub:
        maxPages = cri.CRGetPageCount (bookHandle);
        break;
      case BookFormat.pdf:
        maxPages = pop.PopplerGetPageCount (bookHandle);
        Debug.Log ("Max page " + maxPages);
        break;
    }
    
    if ((maxPages > 0) && (currentPage >= maxPages))
    {
      currentPage = maxPages - 2;
      result = false;
    }
//     print (currentPage + " " + maxPages + " " + cri.CRGetPageCount (bookHandle));
    retrievePageToTexture (currentPage, leftPageTurn);
    retrievePageToTexture (currentPage + 1, rightPageTurn);
    
    bookProperties.currentPage = currentPage;
    bookProperties.Save ();
    
    return result;
  }
  
  public int getFontSize ()
  {
    return fontSize;
  }

  
  public void setFontSize (int d)
  {
    fontSize = d;
    if (fontSize < 1)
    {
      fontSize = 1;
    }

    bookProperties.fontSize = fontSize;
    bookProperties.Save ();
    
    StartCoroutine (updateFont ());
  }
  
  private IEnumerator updateFont ()
  {
    setInformation ("Updating");
    yield return null;

    int oldMaxPages = 1;
    switch (format)
    {
      case BookFormat.epub:
        oldMaxPages = cri.CRGetPageCount (bookHandle);
        cri.CRSetFontSize (bookHandle, fontSize);
        break;
      case BookFormat.pdf:
        oldMaxPages = pop.PopplerGetPageCount (bookHandle);
        pop.PopplerSetFontSize (bookHandle, fontSize);
        break;
    }
    
    // render a page to force page count update.
    retrievePageToTexture (currentPage, leftPageTurn);
    
    setInformation ("Updating.");
    yield return null;
    
    int newMaxPages = 1;
    switch (format)
    {
      case BookFormat.epub:
        newMaxPages = cri.CRGetPageCount (bookHandle);
        break;
      case BookFormat.pdf:
        newMaxPages = pop.PopplerGetPageCount (bookHandle);
        break;
    }
//     Debug.Log ("setting fonh" + fontSize + " " + oldMaxPages + " " + newMaxPages);

    setInformation ("Updating..");
    yield return null;
    
    // Try to stay close to the same page.
    if (oldMaxPages > 0)
    {
      currentPage = newMaxPages * currentPage / oldMaxPages;
    }
    // Redraw the current page.
    changePage (0);    
    pageTurnComplete ();

    setInformation ("");    
    
    fontSize = 10;
    switch (format)
    {
      case BookFormat.epub:
        fontSize = cri.CRGetFontSize (bookHandle);    
        break;
      case BookFormat.pdf:
        fontSize = pop.PopplerGetFontSize (bookHandle);    
        break;
    }
    
    stateChanged ();
  }
}
