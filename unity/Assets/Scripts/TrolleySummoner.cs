using System.Collections;
using System.Collections.Generic;
using UnityEngine;

// Deprecated. Remove.
public class TrolleySummoner : MenuInteraction
{
  public GameObject button;
  
  public string trolleyIdentifier;
  
  public GameObject trolleyTemplate;
  
  override protected void Start () 
  {
    base.Start ();
  }
  
  override public void populateMenu () 
  {
    addItemAsMenuOption (button, summonTrolley);  
  }
  
  // Respond to the door being selected.
  public void summonTrolley (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      Debug.Log ("Summoning");
      TrolleyManager [] tm = Object.FindObjectsOfType <TrolleyManager> ();
      
      TrolleyManager thisTrolley = null;
      foreach (TrolleyManager t in tm)
      {
        Debug.Log ("Got tr " + t.trolleyName);
        if (t.trolleyName == trolleyIdentifier)
        {
          thisTrolley = t;
          break;
        }
      }
      
      if (thisTrolley == null)
      {
        Debug.Log ("Creating trolley ");
        thisTrolley = Instantiate (trolleyTemplate, transform.position + 0.0f * transform.forward, Quaternion.identity).GetComponent <TrolleyManager> ();
      }
      
      Debug.Log ("Calling");
      thisTrolley.track (avatar);
    }
  }
}
