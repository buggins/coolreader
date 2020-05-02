  using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.SceneManagement;

[RequireComponent (typeof (BoxCollider))]


/*
 * The interface to the book.
 * 
 * There are several interacting components involved.
 *   BookMenuInteraction presents the menu and controls the book through the BookManager.
 *   BookManager contain VR engine state, and manipulates the book representations through
 *     native interfaces such as CoolReaderInterface.
 *   BookMenuInteraction has a secondary setting menu from which it receives settings and
 *     sends updates to.
 *   BookManager contains some persistent properties through BookPropertySet.
 */
public class BookMenuInteraction : MenuInteraction {
  
  // A link to the book manager that handles book content operations.
  private BookManager bookManager;
  
  // A cached copy of the animator required for book movements.
  private Animator bookAnimator;

  // The trigger is usually pressed when most menu response handlers start.
  // This helps by waiting till it is released, before the next press is accepted.
  private bool triggerCleared = false;
  // Various alignment operations keep some of the book's transformation intact.
  // Distance is one of these constants.
  private float bookDistance;
  // Direction to the book is another constant preserved during some manipulations.
  private Vector3 bookDirection;

  // Keep track of the open/close book button, since this changes according to the state of the book.
  private GameObject opencloseButton = null;
  
  // Flag when the book is dropped, and no longer of interest.
  private bool dropped = false;
  // State indicating when the book is open or closed.
  private bool open = false;
  
  // Keep track of whether the settings menu is visible.
  private bool settingsOpen = false;
  
  // The actual settings menu object.
  private GameObject settingsMenu = null;
  
  [Tooltip ("The template for the settings menu")]
  public GameObject settingsMenuTemplate;
  
  // Audio sound effects
  public AudioSource pageTurnSound;
  public AudioSource bookCloseSound;
  public AudioSource bookDropSound;
  
  private void updateLocalState ()
  {
    if (settingsMenu != null)
    {
      settingsMenu.GetComponent <BookSettingsMenu> ().onStateChange ();
    }
  }
  
  // Initialize the book, as an active book currently in use.
  protected override void Start ()
  {
    base.Start ();
    bookAnimator = GetComponentInChildren <Animator> ();
    bookManager = GetComponent <BookManager> ();
    // Register for any delayed action events.
    bookManager.onStateChange += updateLocalState;
    setOpen ();
    
    // Active books are kept under one object, to allow transfer between scenes.
    GameObject activeBooks = GameObject.Find ("ActiveBooks");
    if (activeBooks != null)
    {
      transform.SetParent (activeBooks.transform);
    }
  }
  
  // Close the book.
  private void setClosed ()
  {
    // Sound and animation.
    if ((open) && (bookAnimator != null))
    {
      bookAnimator.SetBool ("CloseBook", true);
      bookCloseSound.Play ();
    }

    open = false;
    // Change the logo on the open/close button.
    if (opencloseButton != null)
    {
      opencloseButton.GetComponentInChildren <TextMesh> ().text = "Open\nBook";
    }
    // Adapt collider to closed dimensions.
    GetComponentInChildren <BoxCollider> ().center = new Vector3 (-0.85f, 0.0f, 0); 
    GetComponentInChildren <BoxCollider> ().size = new Vector3 (1.6f, 0.2f, 2.0f); 
  }
  
  // Open the book.
  private void setOpen ()
  {
    // Sound and animation.
    if ((!open) && (bookAnimator != null))
    {
      bookAnimator.SetBool ("CloseBook", false);
    }

    open = true;
    // Button label.
    if (opencloseButton != null)
    {
      opencloseButton.GetComponentInChildren <TextMesh> ().text = "Close\nBook";
    }
    // Collider dimensions.
    GetComponentInChildren <BoxCollider> ().center = new Vector3 (-0.05f, 0, 0); 
    GetComponentInChildren <BoxCollider> ().size = new Vector3 (3.2f, 0.2f, 2.0f); 
  }
  
  // Add the menu buttons.
  override public void populateMenu () {
    
    // Make clicking on the book the same as the next page button.
    addItemAsMenuOption (this.gameObject.transform.Find ("FBXShapeBook").gameObject, getBook, moveResponse);
    
    addMenuOption ("Next\nPage", new Vector3 (0.9f, -0.4f, 0.0f), nextPage, moveResponse);
    addMenuOption ("Prev\nPage", new Vector3 (-0.9f, 0.4f, 0.0f), prevPage, moveResponse);
//          addMenuOption ("Position\nBook", new Vector3 (0.0f, 1.3f, 0.0f), positionBook, moveResponse);
    addMenuOption ("Position\nBook", new Vector3 (-0.3f, 0.65f, 0.05f), moveBook, moveResponse);
    addMenuOption ("Retrieve\nBook", new Vector3 (-0.1f, 0.65f, 0.05f), retrieveBook, moveResponse);
    addMenuOption ("Rotate\nBook", new Vector3 (0.1f, 0.65f, 0.05f), rotateBook, moveResponse);
    opencloseButton = addMenuOption ("Close\nBook", new Vector3 (0.3f, 0.65f, 0.05f), toggleCloseBook, moveResponse);
    addMenuOption ("Drop\nBook", new Vector3 (0.0f, -0.6f, 0.05f), dropBook, moveResponse);
    addMenuOption ("Settings", new Vector3 (-0.3f, -0.6f, 0.05f), toggleSettings, moveResponse);
    addMenuOption ("Scale\nBook", new Vector3 (0.3f, -0.6f, 0.05f), scaleBook, moveResponse);
  }

  private void updateSettings ()
  {
      if (settingsOpen)
      {
        if (settingsMenu == null)
        {
          settingsMenu = Instantiate (settingsMenuTemplate);
          settingsMenu.transform.SetParent (this.gameObject.transform);
          settingsMenu.transform.localPosition = new Vector3 (-1.6f, 0.0f, 0.0f);
          settingsMenu.transform.localRotation = Quaternion.AngleAxis (-45.0f, Vector3.up);
          settingsMenu.transform.localScale = new Vector3 (0.6f, 0.6f, 0.6f);
        }
        settingsMenu.GetComponent <BookSettingsMenu> ().onStateChange ();
        settingsMenu.SetActive (true);
      }
      else
      {
        if (settingsMenu != null)
        {
          settingsMenu.SetActive (false);
        }
      }
  }
  
  // Toggle the settings menu. Create it, if it does not already exist and needs to open.
  public void toggleSettings (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      settingsOpen = !settingsOpen;

      updateSettings ();
    }
  }
  
  // Drop the book.
  public void dropBook (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      // Close it first.
      setClosed ();

      // Switch off menu - a dropped book is no longer in use.
      hideMenu ();
      
      // Switch off settings.
      settingsOpen = false;
      updateSettings ();
      
      // Let physics have it.
      GetComponent <Rigidbody> ().useGravity = true;
      GetComponent <Rigidbody> ().isKinematic = false;
      GetComponent <Rigidbody> ().angularVelocity = new Vector3 (-1.1f, -1.2f, -1.3f);
      
      // Remove it from the managed set.
      transform.SetParent (null);
      dropped = true;
      // Ensure it doesn't get transferred between scenes.
      SceneManager.MoveGameObjectToScene (gameObject, SceneManager.GetActiveScene ());
      
      // Sound effect.
      bookDropSound.Play ();
    }
  }

  // Pick up a book, also used when a book is first created.
  public void pickupBook (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    // Open the book.
    setOpen ();

    // Enable the menu.
    showMenu ();
    
    // Disable physics.
    GetComponent <Rigidbody> ().useGravity = false;
    GetComponent <Rigidbody> ().isKinematic = true;
    transform.SetParent (null);
    dropped = false;
    
    // Add the book to the collection of active books. 
    GameObject activeBooks = GameObject.Find ("ActiveBooks");
    if (activeBooks != null)
    {
      transform.SetParent (activeBooks.transform);
    }
    
    // Bring it near the user and let them position it somewhere.
    this.transform.position = controllerObject.controllerObject.transform.position + 1.0f * controllerObject.controllerObject.transform.forward;
    moveBook (controller, controllerObject, null, avatar);
  }
  
  // Checks if the book is dropped (not under active usage).
  public bool isDropped ()
  {
    return dropped;
  }
  
  // Switch between open and closed.
  public void toggleCloseBook (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (initialize)
    {
      // enforce current state.
      if (open)
      {
        setOpen ();
      }
      else
      {
        setClosed ();
      }
    }
    else
    {
      // switch state.
      if (open)
      {
        setClosed ();
      }
      else
      {
        setOpen ();
      }
    }
  }

  // A response when clicking on the book itself. Currently: turn page if open, otherwise pick up.
  public void getBook (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      if (open)
      {
        nextPage (controller, controllerObject, button, avatar);
      }
      else
      {
        pickupBook (controller, controllerObject, button, avatar);
      }
    }
  }
  
  // Turn to the next page.
  public void nextPage (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      if (bookManager.hasLoaded ())
      {
        if (!bookManager.changePage (2))
        {
          setClosed ();
        }
        else
        {
          if (bookAnimator != null)
          {
            bookAnimator.SetBool ("TurnPage", true);
          }
          pageTurnSound.Play ();
        }
        updateLocalState ();
      }
    }
  }

  // Turn to the previous page.
  public void prevPage (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      if (bookManager.hasLoaded ())
      {
        if (!bookManager.changePage (-2))
        {
          setClosed ();
        }
        else
        {
          if (bookAnimator != null)
          {
            bookAnimator.SetBool ("TurnPageReverse", true);
          }
          pageTurnSound.Play ();
        }
        updateLocalState ();
      }
    }
  }

  // Activate the book positioning process.
  public void moveBook (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      triggerCleared = false;
      bookDistance = (transform.position - controllerObject.controllerObject.transform.position).magnitude;
      
      // Disable collider while moving so that it doesn't interfere with other objects.
      GetComponentInChildren <BoxCollider> ().enabled = false;
      
      controller.addHandler (bookMove, controllerObject, true);
    }    
  }
  
  // Move the book at a constant distance from the controller, and still facing the controller.
  public void bookMove (ControlInput controller, ControlInput.ControllerDescription controllerObject, bool trigger, bool debounceTrigger, Vector3 direction, Vector3 position, GameObject avatar, bool touchpad, Vector2 touchposition)
  {
    //  print ("Movingbook " + bookDistance + " " + direction);
    if (!debounceTrigger)
    {
      triggerCleared = true;
    }
    if (debounceTrigger && triggerCleared)
    {
      controller.removeHandler (bookMove, controllerObject);
      GetComponentInChildren <BoxCollider> ().enabled = true;
    }
    
    transform.position = position + bookDistance * direction;
    transform.up = Vector3.up;
    transform.forward = direction;
  }

  // Move the book closer.
  public void retrieveBook (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      triggerCleared = false;
      bookDirection = transform.position - controller.avatar.transform.position;
      bookDistance = bookDirection.magnitude;
      bookDirection = Vector3.Normalize (bookDirection);
      controller.addHandler (bookRetrieve, controllerObject, true);
    }
  }
  
  // Book moves backwards or forwards along the line between controller and its original position. Aiming the controller up or down controls distance.
  public void bookRetrieve (ControlInput controller, ControlInput.ControllerDescription controllerObject, bool trigger, bool debounceTrigger, Vector3 direction, Vector3 position, GameObject avatar, bool touchpad, Vector2 touchposition)
  {
//           print ("Retrievingbook " + bookDistance + " " + direction);
    if (!debounceTrigger)
    {
      triggerCleared = true;
    }
    if (debounceTrigger && triggerCleared)
    {
      controller.removeHandler (bookRetrieve, controllerObject);
    }
    
    transform.position = avatar.transform.position + bookDirection * bookDistance * Mathf.Pow (2.0f, 5.0f * direction.y);
  }

  public void scaleBook (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      triggerCleared = false;
      controller.addHandler (bookScale, controllerObject, true);
    }
  }
  
  public void bookScale (ControlInput controller, ControlInput.ControllerDescription controllerObject, bool trigger, bool debounceTrigger, Vector3 direction, Vector3 position, GameObject avatar, bool touchpad, Vector2 touchposition)
  {
    if (!debounceTrigger)
    {
      triggerCleared = true;
    }
    if (debounceTrigger && triggerCleared)
    {
      controller.removeHandler (bookScale, controllerObject);
    }
    
    float s = Mathf.Abs (5.0f * direction.y);
    transform.localScale = new Vector3 (s, s, s);
  }

  // Rotate the book to get the best reading angle.
  public void rotateBook (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      triggerCleared = false;
      GetComponentInChildren <BoxCollider> ().enabled = false;
      controller.addHandler (bookRotate, controllerObject, true);
    }
  }
  
  // Set the rotation of the book to match the controller's orientation.
  public void bookRotate (ControlInput controller, ControlInput.ControllerDescription controllerObject, bool trigger, bool debounceTrigger, Vector3 direction, Vector3 position, GameObject avatar, bool touchpad, Vector2 touchposition)
  {
//           print ("Rotatingbook " + bookDistance + " " + direction);
    if (!debounceTrigger)
    {
      triggerCleared = true;
    }
    if (debounceTrigger && triggerCleared)
    {
      controller.removeHandler (bookRotate, controllerObject);
      GetComponentInChildren <BoxCollider> ().enabled = true;
    }
    
    transform.transform.rotation = controllerObject.controllerObject.transform.rotation;
  }
  
  // deprecated - to be removed.
  public void positionBook (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      triggerCleared = false;
      controller.addHandler (bookManipulate, controllerObject);
    }
  }

  // deprecated. Use the touchpad to control book size and position. Turned out to be 
  // awkward to control.
  public void bookManipulate (ControlInput controller, ControlInput.ControllerDescription controllerObject, bool trigger, bool debounceTrigger, Vector3 direction, Vector3 position, GameObject avatar, bool touchpad, Vector2 touchposition)
  {
//           print ("BM");
    if (!debounceTrigger)
    {
      triggerCleared = true;
    }
    if (debounceTrigger && triggerCleared)
    {
      controller.removeHandler (bookManipulate, controllerObject);
    }
    
    //transform.forward = -direction;
    float scale = Mathf.Pow (6.0f, touchposition.x);
    float offset = 3.0f * touchposition.y;
    gameObject.transform.Find ("BookShape").localScale = new Vector3 (scale, scale, scale);
    gameObject.transform.Find ("BookShape").localPosition = new Vector3 (0, 0, offset);
  }

}
