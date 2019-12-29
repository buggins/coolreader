using System.Collections;
using System.Collections.Generic;
using System.IO;
using System;
using UnityEngine;

public class SettingsMenu : MenuInteraction {

  public GameObject rabbitHole;
  
  public BookFinder bookFiles;
  
  public Vector3 boundsMin = new Vector3 (-0.4f, -0.7f, 0.0f);
  public Vector3 boundsMax = new Vector3 (0.4f, 0.7f, 0.0f);
  public float lineSkip = 0.1f;
  public float charSkip = 0.03f;
  
  [Tooltip ("The image used when an item is checked.")]
  public Texture checkOn;
  [Tooltip ("The image used when an item is unchecked.")]
  public Texture checkOff;
  [Tooltip ("The image used to indicate a directory can expand.")]
  public Texture expandIcon;
  [Tooltip ("The image used to indicate a directory can contract.")]
  public Texture contractIcon;
  [Tooltip ("The image used to indicate a file is not relevant.")]
  public Texture badFileIcon;
  [Tooltip ("The image used to indicate scroll up.")]
  public Texture scrollUpIcon;
  [Tooltip ("The image used to indicate scroll down.")]
  public Texture scrollDownIcon;
  [Tooltip ("The image used to indicate increase.")]
  public Texture incrementIcon;
  [Tooltip ("The image used to indicate decrease.")]
  public Texture decrementIcon;
  
  // Used for scrolling.
  private float totalOffset = 0.0f;
    
  // Time delay between a button being touched, and it returning to rest.
  private float buttonColourRecoveryTime = 0.2f;
 
  protected override void Start ()
  {
    base.Start ();
  }
  
  // reset position when nothing touching the button.
  private IEnumerator returnToNormalColour (MenuItem option, Color originalColour)
  {
    while (Time.time < option.lastTouch)
    {
      // sleep until time to reset button. The time may have been extended during
      // this process.
      yield return new WaitForSeconds (option.lastTouch - Time.time);
    }
    foreach (MeshRenderer r in option.button.GetComponentsInChildren <MeshRenderer> ())
    {
      r.material.color = originalColour;
    }
    option.outOfPosition = false;
  }
    
  private void colourResponse (MenuItem menuOption, ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject avatar)
  {
    // Remember where the button was.
    Color buttonOrigin = new Color (0, 0, 0);
    
    foreach (MeshRenderer r in menuOption.button.GetComponentsInChildren <MeshRenderer> ())
    {
      buttonOrigin = r.material.color;
    }
    // Set/reset the time when focus is lost.
    menuOption.lastTouch = Time.time + buttonColourRecoveryTime;
    if (!menuOption.outOfPosition)
    {
      // If not focus, then move.
      foreach (MeshRenderer r in menuOption.button.GetComponentsInChildren <MeshRenderer> ())
      {
        r.material.color = new Color (0, 1, 0);
      }
      menuOption.outOfPosition = true;
      // Play sound.
      if (touchSound != null)
      {
        touchSound.Play ();
      }
      // Start timer to recover if moves out of focus.
      StartCoroutine (returnToNormalColour (menuOption, buttonOrigin));
    }
  }

  // Create a boolean variable to represent the value in a checkbox.
  private class ToggleVariable
  {
    public bool toggle { get; set; }
    
    public ToggleVariable (bool b = false)
    {
      toggle = b;
    }
  }

  // Create a handler for toggle/checkbox interactions. This accepts two handlers, one to respond
  // when the toggle is set, the other when it is reset.
  private void toggleButton (ToggleVariable toggleVariable, string onString, string offString, buttonHandlerType handlerOn, buttonHandlerType handlerOff, ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      toggleVariable.toggle = !toggleVariable.toggle;
    }
    if (toggleVariable.toggle)
    {
      setLabel (button, onString);
      GameObject icon = button.transform.Find ("CheckOptionSymbol").gameObject;
      if (icon != null)
      {
        icon.GetComponent <MeshRenderer> ().material.mainTexture = checkOn;
      }
      if (handlerOn != null)
      {
        handlerOn (controller, controllerObject, button, avatar, initialize);
      }
    }
    else
    {
      setLabel (button, offString);
      GameObject icon = button.transform.Find ("CheckOptionSymbol").gameObject;
      if (icon != null)
      {
        icon.GetComponent <MeshRenderer> ().material.mainTexture = checkOff;
      }
      if (handlerOff != null)
      {
        handlerOff (controller, controllerObject, button, avatar, initialize);
      }
    }
  }
  
  // Curry a toggle handler into a regular button handler, so that some of the parameters can be
  // provided when the button is set up.
  buttonHandlerType toggleHandler (ToggleVariable toggleVariable, string onString, string offString, buttonHandlerType handlerOn, buttonHandlerType handlerOff)
  {
    return (controller, controllerObject, button, avatar, initialize) => toggleButton (toggleVariable, onString, offString, handlerOn, handlerOff, controller, controllerObject, button, avatar, initialize);
  }
  
  private delegate void fileButtonHandlerType (TreeNode n, ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false); 
  
  // Provides standard menu functions, but call a more advanced handler since the button knows more
  // information about itself.
  private void fileToggleButton (TreeNode n, ToggleVariable toggleVariable, string onString, string offString, fileButtonHandlerType handlerOn, fileButtonHandlerType handlerOff, Texture onIcon, Texture offIcon, ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      toggleVariable.toggle = !toggleVariable.toggle;
    }
    if (toggleVariable.toggle)
    {
      setLabel (button, onString);
      GameObject icon = button.transform.Find ("CheckOptionSymbol").gameObject;
      if (icon != null)
      {
        icon.GetComponent <MeshRenderer> ().material.mainTexture = onIcon;
      }
      if (handlerOn != null)
      {
        handlerOn (n, controller, controllerObject, button, avatar, initialize);
      }
    }
    else
    {
      setLabel (button, offString);
      GameObject icon = button.transform.Find ("CheckOptionSymbol").gameObject;
      if (icon != null)
      {
        icon.GetComponent <MeshRenderer> ().material.mainTexture = offIcon;
      }
      if (handlerOff != null)
      {
        handlerOff (n, controller, controllerObject, button, avatar, initialize);
      }
    }
  }
  
  // A variation on the toggle handler that allows some extra information to be passed.
  buttonHandlerType fileToggleHandler (TreeNode n, ToggleVariable toggleVariable, string onString, string offString, fileButtonHandlerType handlerOn, fileButtonHandlerType handlerOff, Texture onIcon, Texture offIcon)
  {
    return (controller, controllerObject, button, avatar, initialize) => fileToggleButton (n, toggleVariable, onString, offString, handlerOn, handlerOff, onIcon, offIcon, controller, controllerObject, button, avatar, initialize);
  }

  // Rewrite the string with newlines to ensure no line is longer than l.
  string brk (string n, int l)
  {
    string result = "";
    bool added = false;
    while (n.Length > 0)
    {
      if (added)
      {
        result += Environment.NewLine;
      }
      int len = Mathf.Min (n.Length, l);
      result += n.Substring (0, len);
      n = n.Substring (len);
      added = true;
    }
    return result;
  }
  
  private class TreeNode
  {
    public GameObject node { get; private set; }
    public string filepath { get; private set; }
    public int depth { get; set; }
    private List<TreeNode> children;
    
    public TreeNode () : this (null, null, 0)
    {
    }
    public TreeNode (string f, GameObject n, int d)
    {
      filepath = f;
      node = n;
      depth = d;
      children = new List<TreeNode> ();
    }
    public void add (TreeNode n)
    {
      n.depth = depth + 1;
      children.Add (n);
    }
    // Will only work once - once node is set, it shouldn't be changed.
    public void setNode (string f, GameObject n)
    {
      if (node == null)
      {
        filepath = f;
        node = n;
      }
    }
    public void removeChildren (MenuInteraction m)
    {
      foreach (TreeNode c in children)
      {
        c.removeChildren (m);
        m.removeMenuOption (c.node);
      }
      children.Clear ();
    }
    // Depth first travel to set the position of each item in the tree.
    public float reflow (float hoffset, float voffset, float lineSkip, float charSkip, float totalOffset, float minoffset, float maxoffset)
    {
      if (node != null)
      {
        int lines = 1;
        string l = getLabel (node);
        if (l != null)
        {
          int p = 0;
          while ((p = l.IndexOf (Environment.NewLine, p)) >= 0)
          {
            p += 1;
            lines += 1;
          }
        }
        if ((voffset + totalOffset <= maxoffset) && (voffset - lines * lineSkip + totalOffset >= minoffset))
        {
          node.transform.localPosition = new Vector3 (hoffset + depth * charSkip, voffset + totalOffset, 0);
          node.SetActive (true);
        }
        else
        {
          node.SetActive (false);
        }
        voffset -= lines * lineSkip;
      }
      foreach (TreeNode child in children)
      {
        voffset = child.reflow (hoffset, voffset, lineSkip, charSkip, totalOffset, minoffset, maxoffset);
      }
      return voffset;
    }
  }
  
  private TreeNode fileRoot = new TreeNode (null, null, 0);

  private float minoffset = 0.0f;
  
  private void reflow ()
  {
    minoffset = fileRoot.reflow (boundsMin.x, boundsMax.y, lineSkip, charSkip, totalOffset, boundsMin.y, boundsMax.y);
  }
  
  public void addToScroll (GameObject menuElement)
  {
    if (menuElement.transform.parent == null)
    {
      menuElement.transform.SetParent (this.transform, false);
    }
    TreeNode n = new TreeNode ();
    n.setNode (null, menuElement);
    fileRoot.add (n);
    reflow ();    
  }
  
  override public void populateMenu () {
    addMenuOption ("Up", new Vector3 (boundsMin.x, boundsMax.y + lineSkip, boundsMin.z), scrollUp, scrollUpColour);
    addMenuOption ("Down", new Vector3 (boundsMin.x, boundsMin.y, boundsMin.z), scrollDown, scrollDownColour);
    
    int linelength = (int) ((boundsMax.x - boundsMin.x) / charSkip);
    
    List <string> sources = bookFiles.getSources ();

//     sources.Clear ();
//     sources.Add ("/storage/self/primary/");
//     sources.Add ("/storage/");
//     sources.Add ("/");
    
    
    foreach (string source in sources)
    {
      TreeNode n = new TreeNode ();
      GameObject g = addMenuOption (brk (source, linelength), new Vector3 (0.0f, 0.0f, 0.0f), fileToggleHandler (n, new ToggleVariable (), brk (source, linelength), brk (source, linelength), expandNode, collapseNode, contractIcon, expandIcon), colourResponse, scrollHandler);
      n.setNode (source, g);
      fileRoot.add (n);
    }
    
    fileRoot.add (new TreeNode (null, addMenuOption ("Enable Rabbit Hole", new Vector3 (0.0f, 0.0f, 0.0f), toggleHandler (new ToggleVariable (), "Enable Rabbit Hole", "Enable Rabbit Hole", rabbitHoleOn, rabbitHoleOff), colourResponse, scrollHandler), 0));
    
    reflow ();
  }
  
  public void rabbitHoleOn (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    rabbitHole.SetActive (true);
  }
  public void rabbitHoleOff (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  { 
    rabbitHole.SetActive (false);
  }

  private void scroll (float skip)
  {
    if ((skip < 0.0f) || (minoffset + totalOffset < boundsMin.y + lineSkip))
    {
      skip = Mathf.Min (skip, boundsMin.y + lineSkip - (minoffset + totalOffset));
      totalOffset += skip;
    }
    if (totalOffset < 0 - lineSkip)
    {
      totalOffset = 0 - lineSkip;
    }
  }

  public void scrollHandler (MenuItem menuOption, Vector2 scrollDir)
  {
    scroll (100.0f * scrollDir.y);
    reflow ();
  }
  
  public void scrollUpColour (MenuItem menuOption, ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject avatar)
  {
    scroll (lineSkip / 20.0f);
    colourResponse (menuOption, controller, controllerObject, avatar);
    reflow ();
  }
  public void scrollDownColour (MenuItem menuOption, ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject avatar)
  {
    scroll (-lineSkip / 20.0f);
    colourResponse (menuOption, controller, controllerObject, avatar);
    reflow ();
  }
  
  public void scrollUp (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      scroll (lineSkip);
    }
    else
    {
      GameObject icon = button.transform.Find ("CheckOptionSymbol").gameObject;
      if (scrollUpIcon != null)
      {
        icon.GetComponent <MeshRenderer> ().material.mainTexture = scrollUpIcon;
      }
    }
    reflow ();
  }
  public void scrollDown (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      scroll (-lineSkip);
    }
    else
    {
      GameObject icon = button.transform.Find ("CheckOptionSymbol").gameObject;
      if (scrollDownIcon != null)
      {
        icon.GetComponent <MeshRenderer> ().material.mainTexture = scrollDownIcon;
      }
    }
    reflow ();
  }
  
  private void expandNode (TreeNode n, ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      if (n.filepath != null)
      {
        int linelength = (int) ((boundsMax.x - boundsMin.x) / charSkip);
        
        List <string> files = bookFiles.getFilesIn (n.filepath);
        foreach (string s in files)
        {
          string source = Path.GetFileName (s);
          TreeNode nn = new TreeNode ();
          GameObject g;
          if (bookFiles.isBook (s) && (source[0] != '.'))
          {
            bool b = bookFiles.haveBook (s);
            string notHaveOption = brk (source, linelength - n.depth);
            string haveOption = brk (source, linelength - n.depth);
            string initOption = notHaveOption;
            if (b)
            {
              initOption = haveOption;
            }
            g = addMenuOption (initOption, new Vector3 (0.0f, 0.0f, 0.0f), fileToggleHandler (nn, new ToggleVariable (b), haveOption, notHaveOption, addBook, removeBook, checkOn, checkOff), colourResponse, scrollHandler);
            nn.setNode (s, g);
            n.add (nn);
          }
          else
          {
            // FIXME: if used, make use of the icon provided.
  //          string noOption = brk (new String (' ', 2 * (n.depth + 1)) + "✖ " + source, linelength);
  //          g = addMenuOption (noOption, new Vector3 (0.0f, 0.0f, 0.0f), fileToggleHandler (nn, new ToggleVariable (), noOption, noOption, null, null));
  //          nn.setNode (s, g);
  //          n.add (nn);
            
          }
        }

        List <string> dirs = bookFiles.getDirectoriesIn (n.filepath);
        if (dirs != null)
        {
          foreach (string s in dirs)
          {
            string source = Path.GetFileName (s);
            if (source[0] != '.')
            {
              TreeNode nn = new TreeNode ();
              GameObject g = addMenuOption (brk (source, linelength - n.depth), new Vector3 (0.0f, 0.0f, 0.0f), fileToggleHandler (nn, new ToggleVariable (), brk (source, linelength - n.depth), brk (source, linelength - n.depth), expandNode, collapseNode, contractIcon, expandIcon), colourResponse, scrollHandler);
              nn.setNode (s, g);
              n.add (nn);
            }
          }
        }
        else
        {
          foreach (MeshRenderer r in n.node.GetComponentsInChildren <MeshRenderer> ())
          {
            // gets overwritten by the time out of the colour response co-routine.
            r.material.color = new Color (1, 0, 0);
          }
        }

        reflow ();
      }
    }
  }
  private void collapseNode (TreeNode n, ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      if (n.filepath != null)
      {
        Debug.Log ("Collapsing");
        n.removeChildren (this);
        reflow ();
      }
    }
  }
  
  private void addBook (TreeNode n, ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      Debug.Log ("Add " + n.filepath);
      StartCoroutine (bookFiles.registerBook (n.filepath));
    }
  }
  private void removeBook (TreeNode n, ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      Debug.Log ("Remove"  + n.filepath);
    }
  }
  
  private Vector2 scrollStart;
  private bool haveOffset = false;
  
  void checkScroll (ControlInput controller, ControlInput.ControllerDescription controllerObject, bool trigger, bool debounceTrigger, Vector3 direction, Vector3 position, GameObject avatarout, bool touchpad, Vector2 touchposition)
  {
    if (touchpad)
    {
      if (!haveOffset)
      {
        scrollStart = touchposition;
        haveOffset = true;
      }

      Vector2 scrollOffset = 0.5f * (touchposition - scrollStart);
    Debug.Log ("Touchpad " + touchpad + " " + touchposition + " " + haveOffset + " " + scrollOffset);
      scroll (scrollOffset.y);
      reflow ();
      scrollStart = touchposition;
    }
    else
    {
//       if (haveOffset)
//       {
//         Vector2 scrollOffset = 100.0f * (touchposition - scrollStart);
//     Debug.Log ("Touchpad " + touchpad + " " + touchposition + " " + haveOffset + " " + scrollOffset);
//         scroll (scrollOffset.y);
//         reflow ();
        haveOffset = false; 
//       }
    }
  }
  
  override public void handleFocus (ControlInput controller, ControlInput.ControllerDescription controllerObject)
  {
    Debug.Log ("Have touch");
    controller.addHandler (checkScroll, controllerObject, false);
  }  
  override public void handleUnfocus (ControlInput controller, ControlInput.ControllerDescription controllerObject)
  {
    Debug.Log ("No touch");
    controller.removeHandler (checkScroll, controllerObject);
  }  

}
