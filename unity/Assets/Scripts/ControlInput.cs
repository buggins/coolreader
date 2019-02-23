using System.Collections;
using System.Collections.Generic;
using UnityEngine;

// Control handler. This class represents the abstraction between the (evolving) VR input
// process and the rest of the application. It also provides simulated control input for
// desktop applications, allowing testing during development but also potentially some
// use of the application outside an immersive VR setting.
public class ControlInput : MonoBehaviour {
  
  [Tooltip ("The object representing the left controller")]
  public GameObject leftControllerObject;
  
  [Tooltip ("The beam being emitted from the left controller")]
  public GameObject leftBeam;
  
  [Tooltip ("An object that will be placed where the left beam intersects other objects")]
  public GameObject leftTarget;
  
  [Tooltip ("The object representing the right controller")]
  public GameObject rightControllerObject;
  
  [Tooltip ("The beam being emitted from the right controller")]
  public GameObject rightBeam;
  
  [Tooltip ("An object that will be placed where the right beam intersects other objects")]
  public GameObject rightTarget;
  
  [Tooltip ("Material used on the target when the beam touches objects of no particular significance")]
  public Material defaultTarget;
  
  [Tooltip ("A material used for the target when it touches objects that can be interacted with")]
  public Material menuTarget;
  
  [Tooltip ("The user camera used in desktop mode to direct the controller based on where the mouse appears to be positioned")]
  public Camera viewcamera;
  
  [Tooltip ("The menu that pops up when the back button is pressed")]
  public GameObject applicationMenu;
//   public TextMesh debugText;
  
  [Tooltip ("The user avatar, for any manipulation of the user that is required")]
  public GameObject avatar;
  
  [Tooltip ("The standard beam material")]
  public Material standardBeamMaterial;
  
  [Tooltip ("A beam material that highlights the target and can be used to indicate teleporting")]
  public Material teleportBeamMaterial;
  
  private bool lastTrigger = false;
  private bool lastBackButton = false;
  private GameObject lastHit = null;
  
  public delegate void HandleControllerInputType (ControlInput controller, GameObject controllerObject, bool trigger, bool debounceTrigger, Vector3 direction, Vector3 position, GameObject avatarout, bool touchpad, Vector2 touchposition); 
  
  private List<HandleControllerInputType> exclusiveRegisteredHandlers;
  private List<HandleControllerInputType> registeredHandlers;
  
  // Use this for initialization
  void Start () {
    exclusiveRegisteredHandlers = new List<HandleControllerInputType> ();
    registeredHandlers = new List<HandleControllerInputType> ();
    if (SelectController.getActivePlatform () == SelectController.DeviceOptions.NoDevice)
    {
      leftControllerObject.transform.localPosition = new Vector3 (-0.2f, 0.0f, 0.3f);	
      rightControllerObject.transform.localPosition = new Vector3 (0.2f, 0.0f, 0.3f);	
    }
    
    // Part of daydream shutdown requirements.
    Input.backButtonLeavesApp = true;
  }
  
  // Register a callback for control events. If the handler is exclusive, then
  // only the most recent handler will receive events until it deregisters (similar
  // to modal dialog).
  public void addHandler (HandleControllerInputType h, bool exclusive = false)
  {
    if (exclusive)
    {
      if (!exclusiveRegisteredHandlers.Contains (h))
      {
        exclusiveRegisteredHandlers.Insert (0, h);
      }
    }
    else
    {
      if (!registeredHandlers.Contains (h))
      {
        registeredHandlers.Add (h);
      }
    }
  }
  
  public void removeHandler (HandleControllerInputType h)
  {
    if (exclusiveRegisteredHandlers.Contains (h))
    {
      exclusiveRegisteredHandlers.Remove (h);
    }
    if (registeredHandlers.Contains (h))
    {
      registeredHandlers.Remove (h);
    }
  }

  // Retrieve controller parameters from an Oculus Go controller.
  private void GetOculusControllerStatus (GameObject controllerObject, out bool trigger, out Vector3 direction, out bool touchpad, out Vector2 touchposition, out bool backButton)
  {
    trigger = OVRInput.Get (OVRInput.Button.PrimaryIndexTrigger);
    backButton = OVRInput.Get (OVRInput.Button.Back);
    direction = controllerObject.transform.forward;
    touchpad = OVRInput.Get (OVRInput.Button.PrimaryTouchpad);
    touchposition = OVRInput.Get (OVRInput.Axis2D.PrimaryTouchpad);
  }
  
  // Retrieve controller parameters from a Daydream controller.
  private void GetDaydreamControllerStatus (GameObject controllerObject, out bool trigger, out Vector3 direction, out bool touchpad, out Vector2 touchposition, out bool backButton)
  {
    trigger = false;
    direction = new Vector3 (0, 0, 0);
    touchpad = false;
    touchposition = new Vector2 (0, 0);
    backButton = false;
    
    GvrTrackedController track = controllerObject.GetComponent <GvrTrackedController> ();
    if (track != null)
    {
      GvrControllerInputDevice inputDevice = track.ControllerInputDevice;
      if (inputDevice != null)
      {
        trigger = inputDevice.GetButton (GvrControllerButton.TouchPadButton);
        backButton = inputDevice.GetButton (GvrControllerButton.App);
        direction = controllerObject.transform.forward;
        touchpad = inputDevice.GetButton(GvrControllerButton.TouchPadTouch);
        touchposition = inputDevice.TouchPos;
      }
    }
  }
  
  // Retrieve controller parameters from a controller simulated with a mouse.
  private void GetMouseStatus (GameObject controllerObject, out bool trigger, out Vector3 direction, out bool backButton)
  {
    trigger = Input.GetAxis ("Fire1") > 0.0;
    backButton = Input.GetAxis ("Fire2") > 0.0;
    
    // Find the point under the mouse cursor.
    RaycastHit hit;
    Ray ray = viewcamera.ScreenPointToRay(Input.mousePosition);
    
    direction = controllerObject.transform.forward;
    if (Physics.Raycast(ray, out hit)) {
      // find the direction for the controller to hit the same point.
      direction = Vector3.Normalize (hit.point - controllerObject.transform.position);
    }
  }
  
  // Inform the last hit object that it is no longer targetted.
  private void clearLastHit ()
  {
    if ((lastHit != null) && (lastHit.GetComponent <MenuInteraction> () != null))
    {
      lastHit.GetComponent <MenuInteraction> ().handleUnfocus (this);
    }
    lastHit = null;
  }
  
  // Show the pointer beam using the teleport material.
  public void setTeleportBeam ()
  {
    leftBeam.GetComponent <MeshRenderer> ().material = teleportBeamMaterial;
    rightBeam.GetComponent <MeshRenderer> ().material = teleportBeamMaterial;
  }
  
  // Show the pointer beam using the standard material.
  public void setStandardBeam ()
  {
    leftBeam.GetComponent <MeshRenderer> ().material = standardBeamMaterial;
    rightBeam.GetComponent <MeshRenderer> ().material = standardBeamMaterial;
  }
  
  void Update () {
    bool trigger;
    bool backButton;
    Vector3 direction;
    Vector3 position;
    bool touchpad = false;
    Vector2 touchposition = new Vector2 (0, 0);

    // Only use one controller - whichever is associated with the specified dominant hand.
    GameObject controllerObject = null;
    GameObject beam = null;
    GameObject target = null;
    if (SelectController.isLeftHanded ())
    {
      controllerObject = leftControllerObject;
      beam = leftBeam;
      target = leftTarget;
      leftControllerObject.SetActive (true);
      rightControllerObject.SetActive (false);
    }
    else
    {
      controllerObject = rightControllerObject;
      beam = rightBeam;
      target = rightTarget;
      leftControllerObject.SetActive (false);
      rightControllerObject.SetActive (true);
    }
//     foreach (SelectController c in leftControllerObject.GetComponentsInChildren <SelectController>(true))
//     {
//       c.checkStatus ();
//     }
//     foreach (SelectController c in rightControllerObject.GetComponentsInChildren <SelectController>(true))
//     {
//       c.checkStatus ();
//     }
    
    // Get controller properties, depending on the device connected.
    position = controllerObject.transform.position;
    switch (SelectController.getActivePlatform ())
    {
      case SelectController.DeviceOptions.OculusGo:
        GetOculusControllerStatus (controllerObject, out trigger, out direction, out touchpad, out touchposition, out backButton);
        break;
      case SelectController.DeviceOptions.Daydream:
        GetDaydreamControllerStatus (controllerObject, out trigger, out direction, out touchpad, out touchposition, out backButton);
        break;
      default:
        GetMouseStatus (controllerObject, out trigger, out direction, out backButton);
        controllerObject.transform.forward = direction;
        break;
    }
    
    // Manage the back button. In future this may need to be coordinated with active menus,
    // once they start nesting.
    bool debounceBackButton = backButton;
    if (debounceBackButton && (backButton == lastBackButton))
    {
      debounceBackButton = false;
    }
    lastBackButton = backButton;
    if (debounceBackButton)
    {
      applicationMenu.SetActive (!applicationMenu.activeSelf);
    }
    // Part of daydream shutdown requirements.
    if (Input.GetKeyDown(KeyCode.Escape))
    {
      Application.Quit();
    }
    
    // Extract edge transitions of the trigger.
    bool debounceTrigger = trigger;
    if (debounceTrigger && (trigger == lastTrigger))
    {
      debounceTrigger = false;
    }
    lastTrigger = trigger;

    // Do raycasting, and passing on events to any menu related objects 
    // that are hit.    
    if (exclusiveRegisteredHandlers.Count == 0)
    {
      // Menu operations only allowed if no exclusive control handlers are available.
      RaycastHit hit;
      target.SetActive (false);
      target.GetComponent <MeshRenderer> ().material = defaultTarget;
      beam.SetActive (true);
      if (Physics.Raycast (position, direction, out hit))
      {
        GameObject hitObject = hit.transform.gameObject;
//         debugText.text += "\n" + hitObject.name;	
//           print ("hhhh" + hitObject.name);
        
        Vector3 beamScale = beam.transform.localScale;
        beamScale.z = hit.distance;
        beam.transform.localScale = beamScale;
//         if (hit.distance < beam.transform.localScale.z)
//         {
//           beam.SetActive (false);
//         }
        
        target.transform.position = hit.point;
        target.SetActive (true);
        
        if (hitObject != lastHit)
        {
          clearLastHit ();          
        }
        if (hitObject.GetComponent <MenuInteraction> () != null)
        {
          if (hitObject != lastHit)
          {
            hitObject.GetComponent <MenuInteraction> ().handleFocus (this);
          }
          
          lastHit = hitObject;
          
          target.GetComponent <MeshRenderer> ().material = menuTarget;
          hitObject.GetComponent <MenuInteraction> ().handleControllerInput (this, controllerObject, trigger, debounceTrigger, direction, position, avatar, touchpad, touchposition);
        }
      }
      else
      {
        clearLastHit ();
      }
    }
    
    // Pass on raw control information to specific handlers.
    if (exclusiveRegisteredHandlers.Count > 0)
    {
      exclusiveRegisteredHandlers[0] (this, controllerObject, trigger, debounceTrigger, direction, position, avatar, touchpad, touchposition);
    }
    else
    {
      List <HandleControllerInputType> regCopy = new List<HandleControllerInputType> (registeredHandlers);
      foreach (HandleControllerInputType h in regCopy)
      {
        h (this, controllerObject, trigger, debounceTrigger, direction, position, avatar, touchpad, touchposition);
      }
    }
  }
}
