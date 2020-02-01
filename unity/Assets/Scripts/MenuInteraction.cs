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
  // button that was selected. When initialize is set to true, the handler invokes no
  // action but modifies the appearance of the button to set any initial visual.
  public delegate void buttonHandlerType (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false); 
            
  public delegate void buttonPointerOverHandler (MenuItem menuOption, ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject avatar); 
            
  public delegate void buttonScrollHandler (MenuItem menuOption, Vector2 scrollMovement); 
            
  [Tooltip ("The sound source for any sound played when the pointer hovers over the menu button")]
  public AudioSource touchSound = null;

  // Internal structure containing menu items for this menu.
  public class MenuItem
  {
    // Button object.
    public GameObject button;
    // Handler when button is pressed and released.
    public buttonHandlerType handler;
    // Handler for any action when pointer hovers over the menu item.
    public buttonPointerOverHandler pointerOverHandler;
    // Handler for any scrolling action required.
    public buttonScrollHandler scrollHandler;
    // Internal value to reset button if not touched within a set period.
    public float lastTouch;
    // Keep track of whether button is in focus or not.
    public bool outOfPosition;
  };
  
  // The internal menu data structure.
  private List<MenuItem> menuItems;

  // The item currently clicked on, in any menu interactions. There can
  // be only one. Map each controller to its own interaction state.
  public Dictionary <ControlInput.ControllerDescription, MenuInteraction.MenuItem> activeButtons;

  protected virtual void Start () {
    activeButtons = new Dictionary <ControlInput.ControllerDescription, MenuInteraction.MenuItem> ();
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
      menuBase.transform.SetParent (menu.transform, false);
    }
    
    // Attach menu to current game object, to avoid clutter.
    menu.transform.SetParent (this.gameObject.transform, false);
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

  // Rewrite the label on a button. Requires that all buttons have
  // a text field.
  static public void setLabel (GameObject button, string label)
  {
    if (button.GetComponentInChildren <TextMesh> () != null)
    {
      button.GetComponentInChildren <TextMesh> ().text = label;
    }
  }

  static public string getLabel (GameObject button)
  {
    if (button.GetComponentInChildren <TextMesh> () != null)
    {
      return button.GetComponentInChildren <TextMesh> ().text;
    }
    return null;
  }
  
  // Add a button to the menu, using a previously created game object. Specifies the
  // handler invoked when the button is pressed. The handler will be called with
  // initialize set to true, so must respond by setting appearance and not calling its
  // handler.
  // The pointer response function provides some form of feedback when the pointer hovers
  // over the button.
  public GameObject addItemAsMenuOption (GameObject menuOption, buttonHandlerType handler, buttonPointerOverHandler pointerResponse = null, buttonScrollHandler scrollResponse = null)
  {
    if (menu == null)
      { 
        initializeMenu ();
      }
      
      MenuItem mi = new MenuItem ();
      mi.button = menuOption;
      mi.handler = handler;
      mi.pointerOverHandler = pointerResponse;
      mi.scrollHandler = scrollResponse;
      menuItems.Add (mi);
      
      handler (null, null, mi.button, null, true);
    
      return menuOption;
  }
  
  // Add a button to the menu, by instantiating the button template, and setting the text
  // field to the string specified. The position of the menu button is also provided.
  public GameObject addMenuOption (string option, Vector3 position, buttonHandlerType handler, buttonPointerOverHandler pointerResponse = null, buttonScrollHandler scrollResponse = null)
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
      setLabel (menuOption, option);
      
      // Add the object to the menu.
      return addItemAsMenuOption (menuOption, handler, pointerResponse, scrollResponse);
  }
  
  public void removeMenuOption (GameObject m)
  {
    List <MenuItem> remove = new List <MenuItem> ();
    foreach (MenuItem mi in menuItems)
    {
      if (mi.button == m)
      {
        remove.Add (mi);
        GameObject.Destroy (mi.button);
      }
    }
    
    foreach (MenuItem mi in remove)
    {
      menuItems.Remove (mi);
    }
  }
  
  // To be overridden for setting up menu (by adding menu options).
  virtual public void populateMenu () {
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
    if (option.button != null)
    {
      option.button.transform.localPosition = originalPosition;
    }
    option.outOfPosition = false;
  }
  
  // Called when a menu object receives the pointer.
  virtual public void handleFocus (ControlInput controller, ControlInput.ControllerDescription controllerObject)
  {
  }
  
  // Take care of any actions associated with losing pointer after having it.
  virtual public void handleUnfocus (ControlInput controller, ControlInput.ControllerDescription controllerObject)
  {
  }
  
  // A default response to a menu item being selected (pointer over). Move a small amount, and
  // then return to original position when pointer is no longer present.
  public void moveResponse (MenuItem menuOption, ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject avatar)
  {
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
  }
  
  // Check for interactions with the menu, and call any handlers as required.
  virtual public void handleControllerInput (ControlInput controller, ControlInput.ControllerDescription controllerObject, bool trigger, bool debounceTrigger, Vector3 direction, Vector3 position, GameObject avatar, bool touchpad, Vector2 touchposition)
  {
    if (menu == null)
    { 
      initializeMenu ();
    }
    
    if (!activeButtons.ContainsKey (controllerObject))
    {
      activeButtons[controllerObject] = null;
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
          
          if (menuOption.pointerOverHandler != null)
          {
            menuOption.pointerOverHandler (menuOption, controller, controllerObject, avatar);
          }
          break;
        }
      }
    }
      
    // If the trigger is pressed and released, then activate the button by calling its handler.
    if (debounceTrigger)
    {
      if ((activeButtons[controllerObject] == null) && (whichButton != null))
      {
        activeButtons[controllerObject] = whichButton;
        controller.addHandler (handleControllerInput, controllerObject);
      }
    }
    
    if (!trigger)
    {
      // trigger not pressed.
      if (activeButtons[controllerObject] != null)
      {
//       Debug.Log ("PressRelease " + (activeButton == whichButton));
        if (whichButton == activeButtons[controllerObject])
        {
          // release while still over the same button.
          activeButtons[controllerObject].handler (controller, controllerObject, activeButtons[controllerObject].button, avatar);
        }
        else
        { // a button was pressed, but we're not on it when releasing.
          // otherwise, release somewhere else. Maybe this is a scroll?
          if (activeButtons[controllerObject].scrollHandler != null)
          {
            Vector3 toButton = activeButtons[controllerObject].button.transform.position - position;
            Debug.Log (toButton + " " + direction);
            // scroll is component of direction perpendicular vector to button.
            Vector3 perp = toButton.magnitude * (toButton.normalized - Vector3.Project (direction.normalized, toButton.normalized));
            Vector2 scroll = new Vector2 (-Vector3.Dot (perp, controllerObject.controllerObject.transform.right), -Vector3.Dot (perp, controllerObject.controllerObject.transform.up));
//             Debug.Log ("Scroll " + 100.0f * scroll);
            activeButtons[controllerObject].scrollHandler (activeButtons[controllerObject], scroll);
          }
        }
      }
      activeButtons[controllerObject] = null;      
      controller.removeHandler (handleControllerInput, controllerObject);
    }
              
  }
}
