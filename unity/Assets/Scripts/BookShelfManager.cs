using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System;
using System.Runtime.InteropServices;
using System.Threading;
using System.IO;

/*
 * Procedurally generate bookshelves to hold books as they are added. Generates the library
 * around it as it goes.
 */
public class BookShelfManager : MenuInteraction {
  
  [Tooltip ("A unit cube object to be a plank for making the book case")]
  public GameObject plankTemplate;
  [Tooltip ("A section of the library that can be tiled down the z axis to extend the library.")]
  public GameObject sectionTemplate;
  [Tooltip ("The end wall (far z axis) of the library")]
  public GameObject endWall;
  
  // library dimensions based on size of pieces.
  private float endwallPosition = -10.0f;
  private float endwallSectionSize = 10.0f;
  
  [Tooltip ("A reference object to become parent of the bookcase geometry")]
  public GameObject bookcase;
  
  // Internal counter of room left on existing shelves - before new shelves need to be generated.
  private float spareCapacity = 0.0f;
  // Shelf coordinates: (stack, rack, shelf, offset in shelf)
  private float offsetinShelf = 0.0f;
  private int shelf = 0;
  private int rack = 0;
  private int stack = 0;
  
  // Parameters for the book shelves.
  public float bookThickness = 0.03f;
  public float bookcaseHeight = 2.0f;
  public float bookcaseDepth = 0.3f;
  public float shelfLength = 0.5f;
  public float plankThickness = 0.02f;
  public float floorOffset = 0.1f;
  public int numberShelves = 5;
  public int setsPerRow = 4;
  public float rowGap = 2.0f;
  
  [Tooltip ("A teleport element placed with every rack, and at the ends of stacks")]
  public GameObject teleport;
  
  // Access to the cool reader engine, to get meta data about books as they are added.
  private BookEngineInterface bookEngine;
  
  // Internal handle to current book.
  private IntPtr bookHandle;
  
  [Tooltip ("A book in the form that will be placed on a shelf")]
  public GameObject bookTemplate;
  
  // Set up access to the book parser.
  void Awake ()
  {
    bookEngine = new BookEngine ();
    
  }
  
  // A separate thread to handle loading of books.
  private bool loading;
  private void doLoading (object obp)
  {
    BookPropertySet bp = (BookPropertySet) obp;
    int rx = 1;
    int ry = 1;
    
    Debug.Log ("Try to open : " + bp.filename);
    bookHandle = bookEngine.BEIDocViewCreate (bp.filename);
    bookEngine.BEILoadDocument (bookHandle, bp.filename, rx, ry);
    bp.title = Marshal.PtrToStringAnsi (bookEngine.BEIGetTitle (bookHandle));
    if (bp.title == null)
    {
      bp.title = "";
    }
    bp.author = Marshal.PtrToStringAnsi (bookEngine.BEIGetAuthors (bookHandle));
    if (bp.author == null)
    {
      bp.author = "No author";
    }
    if (bp.title == "")
    {
      bp.title = Path.GetFileNameWithoutExtension (bp.filename);
    }
    Debug.Log ("Found book " + bp.title + " " + bp.author + " " + bp.filename);
    loading = false;
    
    // Each book is given a colour to distinguish it. Similar books (same author) get similar colours.
    bp.colour[0] = 0.75f + 0.25f * (float) bp.filename.GetHashCode () / Int32.MaxValue;
    bp.colour[1] = 0.75f + 0.25f * (float) bp.title.GetHashCode () / Int32.MaxValue;
    bp.colour[2] = 0.75f + 0.25f * (float) bp.author.GetHashCode () / Int32.MaxValue;
  }
  
  // Use this for initialization
  override protected void Start () {
    base.Start ();
    
    // This has the side effect of generating some initial bookshelf.
    StartCoroutine (addBook (null));
    StartCoroutine (addBook (null));
  }
  
  // Create one plank.
  private GameObject createPlank ()
  {
    GameObject plank = Instantiate (plankTemplate);
    plank.transform.SetParent (bookcase.transform);
    plank.transform.localPosition = new Vector3 (0, 0, 0);
    plank.transform.localRotation = Quaternion.identity;
    
    return plank;
  }
  
  // Create a plank, orient it vertically to be one end of a stack.
  private void createUpright (Vector3 position)
  {
    GameObject plank = createPlank ();
    plank.transform.localPosition = position;
    plank.transform.localScale = new Vector3 (plankThickness, bookcaseHeight, bookcaseDepth);
  }
  
  // Create a horizontal plank to be a shelf.
  private void createShelf (Vector3 position)
  {
    GameObject plank = createPlank ();
    plank.transform.localPosition = position;
    plank.transform.localScale = new Vector3 (shelfLength, plankThickness, bookcaseDepth);
  }
  
  // Create a teleport node.
  private void createTeleport (Vector3 position, Quaternion rotation)
  {
    GameObject t = Instantiate (teleport);
    t.transform.SetParent (bookcase.transform);
    t.transform.localPosition = position;
    t.transform.localRotation = rotation;
    t.transform.localScale = new Vector3 (0.5f * shelfLength, 0.5f, 0.5f * shelfLength);
    // set the teleport offset to represent a standing height above the pad.
    t.GetComponent <TeleportMenuInteraction> ().teleportOffset = new Vector3 (0.0f, 1.5f, 0.0f);
  }
  
  // Add a new region to the library.
  private void createSection (Vector3 position)
  {
    GameObject t = Instantiate (sectionTemplate);
    t.transform.SetParent (bookcase.transform.parent);
    t.transform.localPosition = position;
  }
  
  // Respond to a book being selected from a shelf. Leave the book on the shelf, but create
  // a readable version and load the book into it.
  public void selectBook (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      string fname = button.GetComponent <BookProperties> ().props.filename;
      GameObject book = Instantiate (bookTemplate);
      BookManager bookManager = book.GetComponent <BookManager> ();
      StartCoroutine (bookManager.loadBook (fname, button.GetComponent <BookProperties> ().props));
      book.GetComponent <BookMenuInteraction> ().pickupBook (controller, controllerObject, button, avatar);
    }
  }
  
  // Advance counters to the next spot after placing a book of the given width.
  private void advanceShelfCounter (float bookWidth)
  {
    spareCapacity -= bookWidth;
    offsetinShelf += bookWidth;
    if (offsetinShelf + bookWidth > shelfLength)
    {
      spareCapacity -= (shelfLength - offsetinShelf);
      offsetinShelf = 0;
      shelf += 1;
    }
    
    if (shelf >= numberShelves)
    {
      shelf = 0;
      rack += 1;
    }
    if (rack >= setsPerRow)
    {
      rack = 0;
      stack += 1;
    }
    
  }

  // Open the ebook, so its metadata can be accessed.
  private IEnumerator getBookProperties (BookPropertySet bp)
  {
    bool retrieved = false;
    if (BookPropertySet.haveRecord (bp.filename))
    {
      try 
      {
        bp.Load (bp.filename);
        retrieved = true;
      }
      catch (Exception)
      {
        Debug.Log ("Bad file " + bp.filename);
      }
      
    }
    
    if (!retrieved)
    {
      loading = true;
      Thread t = new Thread (new ParameterizedThreadStart (doLoading));
      t.Start (bp);
      while (loading)
      {
        yield return new WaitForSeconds (0.1f);
      }
      
      // save record.
      bp.Save ();
    }
  }
  
  // Add a new book to the library.
  public IEnumerator addBook (string book)
  {
    // Find the spot for the next book on the shelves, 
    float bookWidth = bookThickness;
    float shelfSeparation = (bookcaseHeight - floorOffset - plankThickness) / numberShelves;
    float bookOffset = offsetinShelf;
    float x = rack * (shelfLength + plankThickness) - (0.5f * shelfLength * setsPerRow);
    float y = floorOffset + shelf * shelfSeparation;
    float z = stack * rowGap;
    
    if (book != null)
    {
      // Get the properties.
      BookPropertySet bp = new BookPropertySet ();
      bp.filename = book;
      yield return getBookProperties (bp);
      
      // Make a spine label with author and title.
      string bookSpine = bp.author + "\n" + bp.title;
      
      // Create the book.
      GameObject bookObject = addMenuOption (bookSpine, new Vector3 (x + plankThickness + bookOffset, y + plankThickness, z), selectBook);
      bookObject.GetComponent <BookProperties> ().props = bp;
      bookObject.transform.localScale = new Vector3 (0.98f * bookWidth, 0.8f * shelfSeparation, 0.95f * bookcaseDepth);
      bookObject.transform.Find ("CheapBook").GetComponent <MeshRenderer> ().material.color = new Color (bp.colour[0], bp.colour[1], bp.colour[2]);
      
      // seems to fix a unity bug that prevents some colliders from being spotted.
      bookObject.GetComponent <BoxCollider> ().enabled = false;
      bookObject.GetComponent <BoxCollider> ().enabled = true;
    }
    
    // Make space for the book.
    if (z + endwallSectionSize > endwallPosition)
    {
      createSection (new Vector3 (0, 0, endwallPosition));
      endWall.transform.localPosition = new Vector3 (0, 0, endwallPosition);
      
      endwallPosition += endwallSectionSize;
    }
    
    if (spareCapacity <= 0)
    {
      
      // Extend the shelves.
      if (rack == 0)
      {
        createUpright (new Vector3 (x, 0, z));
      }
      for (int i = 0; i < numberShelves + 1; i++)
      {
        y = floorOffset + i * ((bookcaseHeight - floorOffset - plankThickness) / numberShelves);
        createShelf (new Vector3 (x + plankThickness, y, z));
      }
      createUpright (new Vector3 (x + shelfLength + plankThickness, 0, z));
      
      spareCapacity += shelfLength * numberShelves;
      
      createTeleport (new Vector3 (x + 0.5f * shelfLength, 0.1f, z - 0.5f * rowGap),
                      Quaternion.AngleAxis (0, Vector3.up));
      
      if (rack == 0)
      {
        createTeleport (new Vector3 (x - 1.5f * shelfLength, 0.1f, z),
                        Quaternion.AngleAxis (90, Vector3.up));              
        createTeleport (new Vector3 (x - 1.5f * shelfLength, 0.1f, z - 0.5f * rowGap),
                        Quaternion.AngleAxis (90, Vector3.up));              
      }
      if (rack == setsPerRow - 1)
      {
        createTeleport (new Vector3 (x + 2.5f * shelfLength, 0.1f, z),
                        Quaternion.AngleAxis (-90, Vector3.up));              
        createTeleport (new Vector3 (x + 2.5f * shelfLength, 0.1f, z - 0.5f * rowGap),
                        Quaternion.AngleAxis (-90, Vector3.up));              
      }
    }
    if (book != null)
    {
      advanceShelfCounter (bookWidth);
    }
    
  }
  
}
