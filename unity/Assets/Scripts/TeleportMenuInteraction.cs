using System.Collections;
using System.Collections.Generic;
using UnityEngine;

/*
 * A variation on the menu interface that provides teleportation.
 * 
 * Clicking on an active destination "button" teleports to that location.
 */
public class TeleportMenuInteraction : MenuInteraction {

  [Tooltip ("Sound effect associated with teleportation destination being touched by the pointer.")]
  public AudioSource teleportAudio = null;

  [Tooltip ("The position relative to the teleport button that the user will be placed.")]
  public Vector3 teleportOffset = new Vector3 (0.0f, 0.0f, 0.0f);
  
  private Dictionary <ControlInput.ControllerDescription, bool> touchingTeleport;
   
  override protected void Start () {
    base.Start ();
    touchingTeleport = new Dictionary <ControlInput.ControllerDescription, bool> ();
  }

  override public void populateMenu () {
    
    addItemAsMenuOption (gameObject, teleportActivate, teleportRespond);
  }

  // Respond to the door being selected.
  public void teleportActivate (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      StartCoroutine (teleport (avatar));      
    }
  }
            
  // Show indicator of teleportation
  public void teleportRespond (MenuItem menuOption, ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject avatar)
  {   
    if (!touchingTeleport.ContainsKey (controllerObject))
    {
      touchingTeleport[controllerObject] = false;
    }
    
    if (!touchingTeleport[controllerObject])
    {
      touchSound.Play ();
      controller.setTeleportBeam (controllerObject);
      touchingTeleport[controllerObject] = true;
    }
  }
  
  private IEnumerator teleport (GameObject avatar)
  {
      if (teleportAudio != null)
      {
        teleportAudio.Play ();
      }
      // slight delay.
      yield return new WaitForSeconds (0.2f);
      avatar.transform.position = transform.position + teleportOffset;
      avatar.transform.rotation = transform.rotation;
  }
    
  override public void handleUnfocus (ControlInput controller, ControlInput.ControllerDescription controllerObject)
  {
    if (!touchingTeleport.ContainsKey (controllerObject))
    {
      touchingTeleport[controllerObject] = false;
    }
    
    controller.setStandardBeam (controllerObject);
    touchingTeleport[controllerObject] = false;
  }

//   override public void handleControllerInput (ControlInput controller, GameObject controllerObject, bool trigger, bool debounceTrigger, Vector3 direction, Vector3 position, GameObject avatar, bool touchpad, Vector2 touchposition)
//   {
//     if (!touchingTeleport)
//     {
//       touchSound.Play ();
//       touchingTeleport = true;
//     }
//     
//     if (debounceTrigger)
//     {
//       StartCoroutine (teleport (avatar));
//     }
//   }
}
