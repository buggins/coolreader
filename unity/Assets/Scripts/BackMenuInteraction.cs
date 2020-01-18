using System.Collections;
using System.Collections.Generic;
using UnityEngine;
// using UnityEditor;

public class BackMenuInteraction : MenuInteraction {

  override public void populateMenu () {
    menuBase.transform.localScale = new Vector3 (1.0f, 0.3f, 1.0f);
    addMenuOption ("Exit", new Vector3 (0.15f, 0.0f, 0.0f), quitApp, moveResponse);
    addMenuOption ("Resume", new Vector3 (-0.15f, 0.0f, 0.0f), resume, moveResponse);
  }
   
  public void quitApp (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      switch (SelectController.getActivePlatform ())  
      {
        case SelectController.DeviceOptions.Daydream:
          GvrDaydreamApi.LaunchVrHomeAsync (null);
          break;
        default:
          Application.Quit ();
#if UNITY_EDITOR
          UnityEditor.EditorApplication.isPlaying = false;
#endif          
          break;
      }
    }
  }

  public void resume (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      gameObject.SetActive (false);
    }
  }
  
}
