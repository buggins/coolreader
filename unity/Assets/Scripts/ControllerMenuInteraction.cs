using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ControllerMenuInteraction : MenuInteraction {

  public GameObject luggageBeaconTemplate;

  public GameObject trolleyTemplate;

  override public void populateMenu () {
    menuBase.transform.localScale = new Vector3 (1.0f, 0.3f, 1.0f);
    addMenuOption ("Drop\nLuggage\nBeacon", new Vector3 (-0.15f, 0.0f, 0.0f), dropBeacon, moveResponse);
    addMenuOption ("Back\nto\nLibrary", new Vector3 (0.15f, 0.0f, 0.0f), returnToLibrary, moveResponse);
  }
   
  public void quitApp (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
    }
  }

  public void dropBeacon (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      TrolleyManager [] tm = Object.FindObjectsOfType <TrolleyManager> ();
      if (tm.Length == 0)
      {
        Debug.Log ("Creating trolley ");
        GameObject thisTrolley = Instantiate (trolleyTemplate, transform.position + 0.0f * transform.forward, Quaternion.identity);
      }
      
      GameObject beacon = Instantiate (luggageBeaconTemplate, button.transform.position, button.transform.rotation);
      Destroy (beacon, 20.0f);
      LuggageSummonEventManager.summonLuggage (beacon);
      gameObject.SetActive (false);
    }
  }

  public void returnToLibrary (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      RoomChangeMenuInteraction.changeToRoom ("Library", this);
    }
  }
  
}
