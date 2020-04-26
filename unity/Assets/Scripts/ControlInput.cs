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
  
  [Tooltip ("The default controller menu template. Can be null")]
  public GameObject controllerMenuTemplate;
  
  [Tooltip ("The user avatar, for any manipulation of the user that is required")]
  public GameObject avatar;
  
  [Tooltip ("The standard beam material")]
  public Material standardBeamMaterial;
  
  [Tooltip ("A beam material that highlights the target and can be used to indicate teleporting")]
  public Material teleportBeamMaterial;
  
  public delegate void HandleControllerInputType (ControlInput controller, ControlInput.ControllerDescription controllerObject, bool trigger, bool debounceTrigger, Vector3 direction, Vector3 position, GameObject avatarout, bool touchpad, Vector2 touchposition); 
  
  private class HandlerList : List<HandleControllerInputType> {}
  
  private Dictionary <ControlInput.ControllerDescription, HandlerList> exclusiveRegisteredHandlers;
  private Dictionary <ControlInput.ControllerDescription, HandlerList> registeredHandlers;

  // All the state associated with an individual controller.
  public class ControllerDescription
  {
    // The system controller object.
    public GameObject controllerObject;
    // The beam object, connecting controller to target.
    public GameObject beam;
    // The marker, indicating what the controller is aimed at.
    public GameObject target;
    // Which hand it is.
    public bool isLeft;

    public bool trigger;
    public bool backButton;
    public Vector3 direction;
    public Vector3 position;
    public bool touchpad = false;
    public Vector2 touchposition = new Vector2 (0, 0);

    public bool lastTrigger = false;
    public bool lastBackButton = false;
    public bool lastTouchpad = false;
    public GameObject lastHit = null;

    // A controller specific menu.
    public GameObject controllerMenu;
    
    public ControllerDescription (GameObject controller, bool isLeft, GameObject beam, GameObject target)
    {
      this.controllerObject = controller;
      this.beam = beam;
      this.target = target;
      this.isLeft = isLeft;
      this.controllerMenu = null;
    }
  }
  
  private List <ControllerDescription> controllerObjects;

  private void setupController (GameObject controllerObject, bool isLeft, GameObject beam, GameObject target)
  {
    ControllerDescription controller = new ControllerDescription (controllerObject, isLeft, beam, target);
    if (controllerMenuTemplate != null)
    {
      controller.controllerMenu = Instantiate (controllerMenuTemplate);
    }
    controllerObjects.Add (controller);
    controllerObject.SetActive (true);
  }
  
  // Use this for initialization
  void Awake () {
    exclusiveRegisteredHandlers = new Dictionary <ControlInput.ControllerDescription, HandlerList> ();
    registeredHandlers = new Dictionary <ControlInput.ControllerDescription, HandlerList> ();
    
    if (SelectController.getActivePlatform () == SelectController.DeviceOptions.NoDevice)
    {
      leftControllerObject.transform.localPosition = new Vector3 (-0.2f, 0.0f, 0.3f);	
      rightControllerObject.transform.localPosition = new Vector3 (0.2f, 0.0f, 0.3f);	
    }
    
    // Part of daydream shutdown requirements.
    Input.backButtonLeavesApp = true;
    
    // Identify, enable and disable controllers.
    leftControllerObject.SetActive (false);
    rightControllerObject.SetActive (false);
    controllerObjects = new List <ControllerDescription> ();
    // Only use one controller - whichever is associated with the specified dominant hand.
    if (!SelectController.singleController () || SelectController.isLeftHanded ())
    {
      setupController (leftControllerObject, true, leftBeam, leftTarget);
//       ControllerDescription leftControl = new ControllerDescription (leftControllerObject, true, leftBeam, leftTarget);
//       if (controllerMenuTemplate != null)
//       {
//         leftControl.controllerMenu = Instantiate (controllerMenuTemplate);
//         leftControl.controllerMenu.transform.SetParent (leftControllerObject.transform);
//       }
//       controllerObjects.Add (leftControl);
//       leftControllerObject.SetActive (true);
      Debug.Log ("Enabled left controller");
    }
    if (!SelectController.singleController () || !SelectController.isLeftHanded ())
    {
      setupController (rightControllerObject, false, rightBeam, rightTarget);
//       controllerObjects.Add (new ControllerDescription (rightControllerObject, false, rightBeam, rightTarget));
//       rightControllerObject.SetActive (true);
      Debug.Log ("Enabled right controller");
    }    
    
    foreach (ControllerDescription co in controllerObjects)
    {
      exclusiveRegisteredHandlers[co] = new HandlerList ();
      registeredHandlers[co] = new HandlerList ();
    }
  }
  
  // Register a callback for control events. If the handler is exclusive, then
  // only the most recent handler will receive events until it deregisters (similar
  // to modal dialog).
  public void addHandler (HandleControllerInputType h, ControlInput.ControllerDescription controllerObject, bool exclusive = false)
  {
    if (exclusive)
    {
      if (!exclusiveRegisteredHandlers[controllerObject].Contains (h))
      {
        exclusiveRegisteredHandlers[controllerObject].Insert (0, h);
      }
    }
    else
    {
      if (!registeredHandlers[controllerObject].Contains (h))
      {
        registeredHandlers[controllerObject].Add (h);
      }
    }
  }
  
  public void removeHandler (HandleControllerInputType h, ControlInput.ControllerDescription controllerObject)
  {
    if (exclusiveRegisteredHandlers[controllerObject].Contains (h))
    {
      exclusiveRegisteredHandlers[controllerObject].Remove (h);
    }
    if (registeredHandlers[controllerObject].Contains (h))
    {
      registeredHandlers[controllerObject].Remove (h);
    }
  }

  // Retrieve controller parameters from an Oculus Go controller.
  private void GetOculusGoControllerStatus (GameObject controllerObject, bool isLeft, out bool trigger, out Vector3 direction, out bool touchpad, out Vector2 touchposition, out bool backButton)
  {
    trigger = OVRInput.Get (OVRInput.Button.PrimaryIndexTrigger);
    backButton = OVRInput.Get (OVRInput.Button.Back);
    direction = controllerObject.transform.forward;
    touchpad = OVRInput.Get (OVRInput.Button.PrimaryTouchpad);
    touchposition = OVRInput.Get (OVRInput.Axis2D.PrimaryTouchpad);
  }
  
  // Retrieve controller parameters from an Oculus Quest controller.
  private void GetOculusQuestControllerStatus (GameObject controllerObject, bool isLeft, out bool trigger, out Vector3 direction, out bool touchpad, out Vector2 touchposition, out bool backButton)
  {
    if (isLeft)
    {
      trigger = OVRInput.Get (OVRInput.RawButton.LIndexTrigger);
      backButton = OVRInput.Get (OVRInput.RawButton.Start);
      direction = controllerObject.transform.forward;
      touchpad = OVRInput.Get (OVRInput.RawButton.LThumbstick);
      touchposition = OVRInput.Get (OVRInput.RawAxis2D.LThumbstick);
    }
    else
    {
      trigger = OVRInput.Get (OVRInput.RawButton.RIndexTrigger);
      backButton = OVRInput.Get (OVRInput.RawButton.Start);
      direction = controllerObject.transform.forward;
      touchpad = OVRInput.Get (OVRInput.RawButton.RThumbstick);
      touchposition = OVRInput.Get (OVRInput.RawAxis2D.RThumbstick);
    }
  }
  
  // Retrieve controller parameters from a Daydream controller.
  private void GetDaydreamControllerStatus (GameObject controllerObject, bool isLeft, out bool trigger, out Vector3 direction, out bool touchpad, out Vector2 touchposition, out bool backButton)
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
  private void GetMouseStatus (GameObject controllerObject, bool isLeft, out bool trigger, out Vector3 direction, out bool touchpad, out bool backButton)
  {
    trigger = Input.GetAxis ("Fire1") > 0.0;
    backButton = Input.GetAxis ("Fire2") > 0.0;
    touchpad = Input.GetAxis ("Fire3") > 0.0;
    
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
  private void clearLastHit (ControllerDescription co)
  {
    if ((co.lastHit != null) && (co.lastHit.GetComponent <MenuInteraction> () != null))
    {
      co.lastHit.GetComponent <MenuInteraction> ().handleUnfocus (this, co);
    }
    co.lastHit = null;
  }
  
  // Show the pointer beam using the teleport material.
  public void setTeleportBeam (ControllerDescription co)
  {
    co.beam.GetComponent <MeshRenderer> ().material = teleportBeamMaterial;
  }
  
  // Show the pointer beam using the standard material.
  public void setStandardBeam (ControllerDescription co)
  {
    co.beam.GetComponent <MeshRenderer> ().material = standardBeamMaterial;
  }
  
  void Update () {
    
    // Handle input from each controller.
    foreach (ControllerDescription co in controllerObjects)
    {
      // Get controller properties, depending on the device connected.
      co.position = co.controllerObject.transform.position;
      switch (SelectController.getActivePlatform ())
      {
        case SelectController.DeviceOptions.OculusGo:
          GetOculusGoControllerStatus (co.controllerObject, co.isLeft, out co.trigger, out co.direction, out co.touchpad, out co.touchposition, out co.backButton);
          break;
        case SelectController.DeviceOptions.OculusQuest:
          GetOculusQuestControllerStatus (co.controllerObject, co.isLeft, out co.trigger, out co.direction, out co.touchpad, out co.touchposition, out co.backButton);
          break;
        case SelectController.DeviceOptions.Daydream:
          GetDaydreamControllerStatus (co.controllerObject, co.isLeft, out co.trigger, out co.direction, out co.touchpad, out co.touchposition, out co.backButton);
          break;
        default:
          GetMouseStatus (co.controllerObject, co.isLeft, out co.trigger, out co.direction, out co.touchpad, out co.backButton);
          co.controllerObject.transform.forward = co.direction;
          break;
      }
      
      // Manage the back button. In future this may need to be coordinated with active menus,
      // once they start nesting.
      bool debounceBackButton = co.backButton;
      if (debounceBackButton && (co.backButton == co.lastBackButton))
      {
        debounceBackButton = false;
      }
      co.lastBackButton = co.backButton;
      if (debounceBackButton)
      {
        applicationMenu.SetActive (!applicationMenu.activeSelf);
      }
      // Part of daydream shutdown requirements.
      if (Input.GetKeyDown(KeyCode.Escape))
      {
        Application.Quit();
      }
      
      // Use the touchpad to access a controller application menu.
      bool debounceTouchpad = co.touchpad;
      if (debounceTouchpad && (co.touchpad == co.lastTouchpad))
      {
        debounceTouchpad = false;
      }
      co.lastTouchpad = co.touchpad;
      if (debounceTouchpad && (co.controllerMenu != null))
      {
        co.controllerMenu.SetActive (!co.controllerMenu.activeSelf);
        co.controllerMenu.transform.position = co.controllerObject.transform.position + 0.5f * co.controllerObject.transform.forward;
        co.controllerMenu.transform.rotation = co.controllerObject.transform.rotation;
      }
      
      // Extract edge transitions of the trigger.
      bool debounceTrigger = co.trigger;
      if (debounceTrigger && (co.trigger == co.lastTrigger))
      {
        debounceTrigger = false;
      }
      co.lastTrigger = co.trigger;

      // Do raycasting, and passing on events to any menu related objects 
      // that are hit.    
      if (exclusiveRegisteredHandlers[co].Count == 0)
      {
        // Menu operations only allowed if no exclusive control handlers are available.
        RaycastHit hit;
        co.target.SetActive (false);
        co.target.GetComponent <MeshRenderer> ().material = defaultTarget;
        co.beam.SetActive (true);
        if (Physics.Raycast (co.position, co.direction, out hit))
        {
          GameObject hitObject = hit.transform.gameObject;
  //         debugText.text += "\n" + hitObject.name;	
  //           print ("hhhh" + hitObject.name);
          
          Vector3 beamScale = co.beam.transform.localScale;
          beamScale.z = hit.distance;
          co.beam.transform.localScale = beamScale;
  //         if (hit.distance < beam.transform.localScale.z)
  //         {
  //           beam.SetActive (false);
  //         }
          
          co.target.transform.position = hit.point;
          co.target.SetActive (true);
          
          if (hitObject != co.lastHit)
          {
            clearLastHit (co);          
          }
          if (hitObject.GetComponent <MenuInteraction> () != null)
          {
            if (hitObject != co.lastHit)
            {
              hitObject.GetComponent <MenuInteraction> ().handleFocus (this, co);
            }
            
            co.lastHit = hitObject;
            
            co.target.GetComponent <MeshRenderer> ().material = menuTarget;
            hitObject.GetComponent <MenuInteraction> ().handleControllerInput (this, co, co.trigger, debounceTrigger, co.direction, co.position, avatar, co.touchpad, co.touchposition);
          }
        }
        else
        {
          clearLastHit (co);
        }
      }
      
      // Pass on raw control information to specific handlers.
      if (exclusiveRegisteredHandlers[co].Count > 0)
      {
        exclusiveRegisteredHandlers[co][0] (this, co, co.trigger, debounceTrigger, co.direction, co.position, avatar, co.touchpad, co.touchposition);
      }
      else
      {
        List <HandleControllerInputType> regCopy = new List<HandleControllerInputType> (registeredHandlers[co]);
        foreach (HandleControllerInputType h in regCopy)
        {
          h (this, co, co.trigger, debounceTrigger, co.direction, co.position, avatar, co.touchpad, co.touchposition);
        }
      }
    }
  }
}
