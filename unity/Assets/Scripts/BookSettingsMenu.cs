using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class BookSettingsMenu : SettingsMenu {

    [Tooltip ("The template for a two button labelled menu element")]
    public GameObject twoButtonTemplate;
  
    // Local settings used internally by the menu until they are applied.
    // Text font size.
    private int fontSize;
    // Current page open.
    private int currentPage;
    
    // Individual widgets for various settings.
    private GameObject fontSelect;
    private GameObject pageSelect;
    
    protected override void Start ()
      {
        base.Start ();
      }

    private GameObject createTwoButtonOption (string label, buttonHandlerType up, buttonHandlerType down, buttonHandlerType apply)
    {
      GameObject g = Instantiate (twoButtonTemplate);
      g.transform.Find ("LabelText").GetComponent <TextMesh> ().text = label;
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

      addItemAsMenuOption (g.gameObject.transform.Find ("LeftSymbol").gameObject, up);
      addItemAsMenuOption (g.gameObject.transform.Find ("RightSymbol").gameObject, down);
      addItemAsMenuOption (g.gameObject.transform.Find ("ApplySymbol").gameObject, apply);
      
      return g;
    }
      
    override public void populateMenu () {
      addMenuOption ("Up", new Vector3 (boundsMin.x, boundsMax.y + lineSkip, boundsMin.z), scrollUp, scrollUpColour);
      addMenuOption ("Down", new Vector3 (boundsMin.x, boundsMin.y, boundsMin.z), scrollDown, scrollDownColour);

      // Add page control control.    
      pageSelect = createTwoButtonOption ("Page", decreasePage, increasePage, applyPage);
      // Add the font size control.    
      fontSelect = createTwoButtonOption ("Font Size", decreaseFont, increaseFont, applyFont);
      
      retrieveSettings ();
      updateView ();    
    }
   
   // Update internal state from book settings.
   public void onStateChange ()
   {
     retrieveSettings ();
     updateView ();
   }

   // Ensure that menu state is current with the book state.
   private void retrieveSettings ()
   {
     BookManager bm = GetComponentInParent <BookManager> ();
     currentPage = bm.getCurrentPage ();
     fontSize = bm.getFontSize ();
   }
   
   // Ensure the menu state is transferred to the visible part of the menu. May need
   // to retrieveSettings first if book has been manipulated.
   private void updateView ()
   {
     if (pageSelect != null)
     {
       BookManager bm = GetComponentInParent <BookManager> ();
       pageSelect.gameObject.transform.Find ("ContentText").GetComponent <TextMesh> ().text = "" + currentPage + "/" + bm.getMaxPages ();
     }
     if (fontSelect != null)
     {
       fontSelect.gameObject.transform.Find ("ContentText").GetComponent <TextMesh> ().text = "" + fontSize;
     }
   }

   public void increasePage (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      currentPage += 10;
      BookManager bm = GetComponentInParent <BookManager> ();
      if (currentPage >= bm.getMaxPages ())
      {
        currentPage = bm.getMaxPages () - 1;
      }
    }
    updateView ();
  }

  public void decreasePage (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      currentPage -= 10;
      if (currentPage < 0)
      {
        currentPage = 0;
      }
    }
    updateView ();
  }

  public void applyPage (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      BookManager bm = GetComponentInParent <BookManager> ();
      bm.setCurrentPage (currentPage);
      bm.pageTurnComplete ();
    }
    retrieveSettings ();
    updateView ();
  }
   
   // Increase the local state represent font. 
   public void increaseFont (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      fontSize += 5;
    }
    updateView ();
  }

  public void decreaseFont (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      fontSize -= 2;
      if (fontSize < 1)
      {
        fontSize = 1;
      }
    }
    updateView ();
  }

  // Apply local font value to the book.
  public void applyFont (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      BookManager bm = GetComponentInParent <BookManager> ();
      bm.setFontSize (fontSize);
    }
    retrieveSettings ();
    updateView ();
  }
}
