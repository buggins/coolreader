using System.Collections;
using System.Collections.Generic;
using UnityEngine;
// using UnityEditor;

public class BackMenuInteraction : MenuInteraction {

  override public void populateMenu () {
    menuBase.transform.localScale = new Vector3 (1.0f, 0.3f, 1.0f);
    addMenuOption ("Exit", new Vector3 (0.15f, 0.0f, 0.0f), quitApp);
    addMenuOption ("Resume", new Vector3 (-0.15f, 0.0f, 0.0f), resume);
  }
   
  public void quitApp (ControlInput controller, GameObject controllerObject, GameObject button, bool initialize = false)
  {
    if (!initialize)
    {
      Application.Quit ();
//       UnityEditor.EditorApplication.isPlaying = false;
    }
  }

  public void resume (ControlInput controller, GameObject controllerObject, GameObject button, bool initialize = false)
  {
    if (!initialize)
    {
      gameObject.SetActive (false);
    }
  }
}
