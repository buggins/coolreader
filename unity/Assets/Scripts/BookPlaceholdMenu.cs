using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class BookPlaceholdMenu : MenuInteraction
{
  public AudioSource bookDropSound;
  
  [Tooltip ("The prefab for a full book representation, used to replace this object when it is opened")]
  public GameObject fullBookTemplate;
  
  // Add the menu buttons.
  override public void populateMenu () {
    
    addItemAsMenuOption (this.gameObject.transform.Find ("BookShape").gameObject, getBook, moveResponse);

    addMenuOption ("Position\nBook", new Vector3 (-0.3f, 0.65f, 0.05f), moveBook, moveResponse);
    addMenuOption ("Retrieve\nBook", new Vector3 (-0.1f, 0.65f, 0.05f), retrieveBook, moveResponse);
    addMenuOption ("Rotate\nBook", new Vector3 (0.1f, 0.65f, 0.05f), rotateBook, moveResponse);
    addMenuOption ("Drop\nBook", new Vector3 (0.0f, -0.6f, 0.05f), dropBook, moveResponse);
  }
  
  void Start()
  {
    base.Start ();
  }
  
  void Update()
  {
  }
  
  // A response when clicking on the book itself. Replace the book with an full book object.
  public void getBook (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      BookPropertySet props = button.GetComponentInParent <BookProperties> ().props;
      string fname = props.filename;
      GameObject book = Instantiate (fullBookTemplate);
      BookManager bookManager = book.GetComponent <BookManager> ();
//       StartCoroutine (bookManager.loadBook (fname, props));
      bookManager.loadBook (fname, props);
      Debug.Log ("Loading book " + fname);
      book.GetComponent <BookMenuInteraction> ().pickupBook (controller, controllerObject, button, avatar);
      Destroy (button.transform.parent.gameObject);
    }
  }
 
 private ControlInput currentMoveController = null;
 private ControlInput currentRetrieveController = null;
 private ControlInput currentRotateController = null;
 private ControlInput.ControllerDescription currentMoveControllerObj = null;
 private ControlInput.ControllerDescription currentRetrieveControllerObj = null;
 private ControlInput.ControllerDescription currentRotateControllerObj = null;
 
 void OnCollisionEnter(Collision collision)
    {
      Debug.Log ("Collide");
      if (canCollide && (collision.gameObject.GetComponent <TrolleyManager> () != null))
      {
        Debug.Log ("Handing to trolley");

// TODO : replace with remove all handlers.        
        if (currentMoveController != null)
        {
               currentMoveController.removeHandler (bookMove, currentMoveControllerObj);
        }
        if (currentRetrieveController != null)
        {
               currentRetrieveController.removeHandler (bookRetrieve, currentRetrieveControllerObj);
        }
        if (currentRotateController != null)
        {
               currentRotateController.removeHandler (bookRotate, currentRotateControllerObj);
        }
      hideMenu ();
      collision.gameObject.GetComponent <TrolleyManager> ().addBook (this.gameObject.GetComponent <BookProperties> ().props);
      Destroy (this.gameObject);
      }
      
    }
    

  
  ////////// TODO: duplicates from BookMenuInteraction. To an interface class ....

    // The trigger is usually pressed when most menu response handlers start.
  // This helps by waiting till it is released, before the next press is accepted.
  private bool triggerCleared = false;
  // Various alignment operations keep some of the book's transformation intact.
  // Distance is one of these constants.
  private float bookDistance;
  // Direction to the book is another constant preserved during some manipulations.
  private Vector3 bookDirection;
// Flag when the book is dropped, and no longer of interest.
  private bool dropped = false;
  
  // Disable collisions until the book has been put down for the first time.
  private bool canCollide = false;
  
  // Pick up a book, also used when a book is first created.
  public void pickupBook (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    // Open the book.
 //   setOpen ();

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
  
  
// Drop the book.
  public void dropBook (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      // Close it first.
//       setClosed ();

      // Switch off menu - a dropped book is no longer in use.
      hideMenu ();
      
      // Switch off settings.
//       settingsOpen = false;
//       updateSettings ();
      
      // Let physics have it.
      GetComponent <Rigidbody> ().useGravity = true;
      GetComponent <Rigidbody> ().isKinematic = false;
      GetComponent <Rigidbody> ().angularVelocity = new Vector3 (-1.1f, -1.2f, -1.3f);
      
      // Remove it from the managed set.
      transform.SetParent (null);
      dropped = true;
      // Ensure it doesn't get transferred between scenes.
//       SceneManager.MoveGameObjectToScene (gameObject, SceneManager.GetActiveScene ());
      
      // Sound effect.
      bookDropSound.Play ();
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
      currentMoveController = controller;
      currentMoveControllerObj = controllerObject;
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
      currentMoveController = null;
      controller.removeHandler (bookMove, controllerObject);
      canCollide = true;
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
      currentRetrieveController = controller;
      currentRetrieveControllerObj = controllerObject;
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
      currentRetrieveController = null;
      controller.removeHandler (bookRetrieve, controllerObject);
    }
    
    transform.position = avatar.transform.position + bookDirection * bookDistance * Mathf.Pow (2.0f, 5.0f * direction.y);
  }

  // Rotate the book to get the best reading angle.
  public void rotateBook (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      triggerCleared = false;
      GetComponentInChildren <BoxCollider> ().enabled = false;
      controller.addHandler (bookRotate, controllerObject, true);
      currentRotateController = controller;
      currentRotateControllerObj = controllerObject;
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
      currentRotateController = null;
      controller.removeHandler (bookRotate, controllerObject);
      GetComponentInChildren <BoxCollider> ().enabled = true;
    }
    
    transform.transform.rotation = controllerObject.controllerObject.transform.rotation;
  }
  
}
