using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.SceneManagement;

/*
 * The "door" menu, change from one scene to another when a portal is activated.
 */
public class RoomChangeMenuInteraction : MenuInteraction {
  
  [Tooltip ("The name of the destination scene when the portal is activated. Note: ensure that this scene is added to the build settings.")]
  public string destinationRoom;
  
  [Tooltip ("The parent of any objects that will be transported to the destination scene.")]
  public GameObject book;
  
  [Tooltip ("Sound effect associated with activating the portal")]
  public AudioSource doorOpenSound;
  
  [Tooltip ("The door component that will be respond to controller presence")]
  public GameObject doorPart;
  
  private bool touchingDoor = false;
  
  override protected void Start () {
    base.Start ();
  }

  override public void populateMenu () {
    
    addItemAsMenuOption (doorPart, doorActivate, doorRespond);
  }

  // Respond to the door being selected.
  public void doorActivate (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      // Start the animation.
      Animator animator = GetComponent<Animator>();
      animator.SetBool ("OpenDoor", true);
      
      // Schedule a room change a bit later.
      if (destinationRoom != null)
      {
        StartCoroutine (changeRooms ());
      }
    }
  }
            
  // Get the door to respond to pointer goes over it.
  public void doorRespond (MenuItem menuOption, ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject avatar)
  {
      if ((touchSound != null) && (!touchingDoor))
      {
        touchSound.Play ();
      }
      touchingDoor = true;
  }
            

  // Run the sequence of steps associated with changing rooms.
  private IEnumerator changeRooms ()
  {
    // Start the door opening sound playing.
    if (doorOpenSound != null)
    {
      doorOpenSound.Play ();
    }
    // Start the door opening animation.
    Animator animator = GetComponent<Animator>();
    // Yield until the door is fully open.
    while (!animator.GetBool ("DoorIsOpen"))
    {
      yield return new WaitForSeconds (0.1f);
    }
    animator.SetBool ("OpenDoor", false);
    
    // Transfer any objects that need to go to the next scene.
    if (book != null)
    {
      DontDestroyOnLoad (book);
    }
    // Change scenes.
    SceneManager.LoadScene(destinationRoom, LoadSceneMode.Single);
  }
  
  // Re-enable sound cues once the pointer leaves the door completely.
  override public void handleUnfocus (ControlInput controller, ControlInput.ControllerDescription controllerObject)
  {
    touchingDoor = false;
  }  
}
