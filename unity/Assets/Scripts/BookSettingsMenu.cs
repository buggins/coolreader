using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class BookSettingsMenu : SettingsMenu {

    [Tooltip ("The template for a two button labelled menu element")]
    public GameObject twoButtonTemplate;
  
    protected override void Start ()
      {
        base.Start ();
      }

    override public void populateMenu () {
    addMenuOption ("Up", new Vector3 (boundsMin.x, boundsMax.y + lineSkip, boundsMin.z), scrollUp, scrollUpColour);
    addMenuOption ("Down", new Vector3 (boundsMin.x, boundsMin.y, boundsMin.z), scrollDown, scrollDownColour);

    // Add the font size control.    
    GameObject g = Instantiate (twoButtonTemplate);
    BookManager bm = GetComponentInParent <BookManager> ();
    g.transform.Find ("LabelText").GetComponent <TextMesh> ().text = "Font Size";
    g.transform.Find ("ContentText").GetComponent <TextMesh> ().text = "" + bm.getFontSize ();
    addToScroll (g);

    GameObject icon;
    icon = g.transform.Find ("LeftSymbol").gameObject;
    if (decrementIcon != null)
    {
      icon.GetComponent <MeshRenderer> ().material.mainTexture = decrementIcon;
    }
    icon = g.transform.Find ("RightSymbol").gameObject;
    if (incrementIcon != null)
    {
      icon.GetComponent <MeshRenderer> ().material.mainTexture = incrementIcon;
    }

    addItemAsMenuOption (g.gameObject.transform.Find ("LeftSymbol").gameObject, decreaseFont);
    addItemAsMenuOption (g.gameObject.transform.Find ("RightSymbol").gameObject, increaseFont);
    
    }
   
   // Increase the font. Also has a side effect exploited during initialization of displaying the current
   // font size.
   public void increaseFont (ControlInput controller, GameObject controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    BookManager bm = GetComponentInParent <BookManager> ();
    if (!initialize)
    {
      bm.changeFontSize (10);
    }
    button.transform.parent.Find ("ContentText").GetComponent <TextMesh> ().text = "" + bm.getFontSize ();    
  }

  public void decreaseFont (ControlInput controller, GameObject controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      BookManager bm = GetComponentInParent <BookManager> ();
      bm.changeFontSize (-5);
      button.transform.parent.Find ("ContentText").GetComponent <TextMesh> ().text = "" + bm.getFontSize ();
    }
  }
}
