using System.Collections;
using System.Collections.Generic;
using UnityEngine;

[RequireComponent (typeof (Rigidbody))]

/* 
 * Note that menus built using this class must also have a rigidbody attached
 * to the object containing this component, so that individual button collisions
 * will be correctly associated with the menu itself. 
 */

/* 
 * This serves as a base class for any element that needs a menu; specifically
 * components that serve as buttons. A button must have a collider associated with
 * it. A handler is then provided for each menu button, and receives an event
 * (callback) whenever the button is clicked. 
 */

public class MenuInteraction : MonoBehaviour {

  [Tooltip ("Deprecated. Menus can be created from the editor, by providing labels here. Rather derive from this class, and provide specific handlers.")]
  public string [] menuOptions;

  [Tooltip ("A backing object for the menu")]
  public GameObject menuBaseTemplate;
  [Tooltip ("The shape of a menu button, if only text labels are being provided. This prefab must have a TextMesh component")]
  public GameObject menuButtonTemplate;
  
  // Time delay between a button being touched, and it returning to rest.
  private float buttonRecoveryTime = 0.2f;

  // Direction to offset buttons when in focus.
  private Vector3 buttonFocusMovement = new Vector3 (0, 0, -0.02f);
  
  // The actual menu parent object.
  protected GameObject menu = null;
  // The backing object for the menu (background).
  protected GameObject menuBase = null;
  
  // Menus are usually toggled visible or not, often by other menus.
  private bool visible = true;
  
  // Button handlers are provided: the controller that selected the button, and the
  // button that was selected.
  public delegate void buttonHandlerType (ControlInput controller, GameObject controllerObject, GameObject button); 
            
  [Tooltip ("The sound source for any sound played when the pointer hovers over the menu button")]
  public AudioSource touchSound = null;

  // Internal structure containing menu items for this menu.
  class MenuItem
  {
    // Button object.
    public GameObject button;
    // Handler when button is pressed.
    public buttonHandlerType handler;
    // Internal value to reset button if not touched within a set period.
    public float lastTouch;
    // Keep track of whether button is in focus or not.
    public bool outOfPosition;
  };
  
  // The internal menu data structure.
  private List<MenuItem> menuItems;
  
  protected virtual void Start () {
    initializeMenu ();
    populateMenu ();
  }

  // Set up a menu on demand.
  private void initializeMenu ()
  {
    // Initialise list of items.
    menuItems = new List<MenuItem> ();

    // Prepare the scene graph representation of the menu.
    menu = new GameObject ();
    menu.name = "menuRoot";

    // Add a background object, if provided.
    if (menuBaseTemplate != null)
    {
      menuBase = Instantiate (menuBaseTemplate);
      menuBase.transform.SetParent (menu.transform);
    }
    
    // Attach menu to current game object, to avoid clutter.
    menu.transform.SetParent (this.gameObject.transform);
    menu.transform.localPosition = new Vector3 (0, 0, 0);
    menu.transform.localRotation = Quaternion.identity;
    
    updateVisibility ();
  }

  // Make the scene graph match the internal visible variable.
  private void updateVisibility ()
  {
    if (menu == null)
    { 
      initializeMenu ();
    }
    
    menu.SetActive (visible);
  }
  
  // Conceal menu.
  public void hideMenu ()
  {
    visible = false;
    updateVisibility ();
  }
  
  // Reveal menu.
  public void showMenu ()
  {
    visible = true;
    updateVisibility ();
  }

  // Add a button to the menu, using a previously created game object. Specifies the
  // handler invoked when the button is pressed.
  public GameObject addItemAsMenuOption (GameObject menuOption, buttonHandlerType handler)
  {
    if (menu == null)
      { 
        initializeMenu ();
      }
      
      MenuItem mi = new MenuItem ();
      mi.button = menuOption;
      mi.handler = handler;
      menuItems.Add (mi);
      
      return menuOption;
  }
  
  // Add a button to the menu, by instantiating the button template, and setting the text
  // field to the string specified. The position of the menu button is also provided.
  public GameObject addMenuOption (string option, Vector3 position, buttonHandlerType handler)
  {
      if (menu == null)
      { 
        initializeMenu ();
      }
      
      // Create an scene object for the button.
      GameObject menuOption = Instantiate (menuButtonTemplate);
      menuOption.transform.localPosition = position;
      menuOption.transform.SetParent (menu.transform, false);
      // Set the label, if possible.
      if (menuOption.GetComponentInChildren <TextMesh> () != null)
      {
        menuOption.GetComponentInChildren <TextMesh> ().text = option;
      }
      
      // Add the object to the menu.
      return addItemAsMenuOption (menuOption, handler);
  }
  
  // Deprecated. Create a menu from a list of options provided. Currently
  // no way of specifying handlers, so just looks good.
  virtual public void populateMenu () {
    float row = -1.4f;
    //float col = 0;
    foreach (string option in menuOptions)
    {
      addMenuOption (option, new Vector3 (0, row * 0.25f, 0), null);
      
      row++;
    }
  }
  
  // reset position when nothing touching the button.
  private IEnumerator returnToHome (MenuItem option, Vector3 originalPosition)
  {
    while (Time.time < option.lastTouch)
    {
      // sleep until time to reset button. The time may have been extended during
      // this process.
      yield return new WaitForSeconds (option.lastTouch - Time.time);
    }
    option.button.transform.localPosition = originalPosition;
    option.outOfPosition = false;
  }
  
  // Take care of any actions associated with losing pointer after having it.
  virtual public void handleUnfocus ()
  {
  }
  
  // Check for interactions with the menu, and call any handlers as required.
  virtual public void handleControllerInput (ControlInput controller, GameObject controllerObject, bool trigger, bool debounceTrigger, Vector3 direction, Vector3 position, GameObject avatar, bool touchpad, Vector2 touchposition)
  {
    if (menu == null)
    { 
      initializeMenu ();
    }
    
    // Ray cast in the direction of the controller, and trigger any button affected.
    MenuItem whichButton = null;
    
    RaycastHit hit;
    if (Physics.Raycast (position, direction, out hit))
    {
//             print ("Hit " +  hit.point + " " + hit.transform.gameObject.name + " " + hit.collider.name);
      
      // Find the button. Loop efficiency could be improved with a hash table, 
      // or tagging the hit object with some menu information. Not an issue until
      // menus become very big though.
      foreach (MenuItem menuOption in menuItems)
      {
        if (menuOption.button.transform == hit.collider.transform)
        {
          // If a button is touched, then provide feedback to the user.
          whichButton = menuOption;
          
          // Remember where the button was.
          Vector3 buttonOrigin = menuOption.button.transform.localPosition;
          // Set/reset the time when focus is lost.
          menuOption.lastTouch = Time.time + buttonRecoveryTime;
          if (!menuOption.outOfPosition)
          {
            // If not focus, then move.
            menuOption.button.transform.localPosition = menuOption.button.transform.localPosition + buttonFocusMovement;
            menuOption.outOfPosition = true;
            // Play sound.
            if (touchSound != null)
            {
              touchSound.Play ();
            }
            // Start timer to recover if moves out of focus.
            StartCoroutine (returnToHome (menuOption, buttonOrigin));
          }
          break;
        }
      }
    }
      
    // If the trigger is pressed, then activate the button by calling its handler.
    if (debounceTrigger)
    {
      if (whichButton != null)
      {
        whichButton.handler (controller, controllerObject, whichButton.button);
      }
    }
              
  }
}
