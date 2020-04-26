using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.Runtime.Serialization.Formatters.Binary;
using System.IO;
using System;

// A mechanism for moving books around, particularly between rooms.
// Each troller has an existence independent of the particular world 
// that it is in. It is identified soley by its name, which needs to
// be unique if more than one trolley is used.
public class TrolleyManager : MenuInteraction
{
  public float trolleyDepth = 0.3f;
  
  public float forceStrength = 3.0f;
  
  private GameObject trolley;
  
  public GameObject bookTemplate;
  
  private BookPropertySet [] bookSlots;
  private int freeSlots;
  
  [Tooltip ("A name to uniquely identify this particular trolley.")]
  public string trolleyName = "Trolley";
  
  public Material trolleyMaterial;
  
  private bool tracking = false;
  private GameObject trackedAvatar;
  
  // Start is called before the first frame update
  void Start()
  {
    trolley = this.gameObject;
    
    int maxBooks = 8; // limit to number of books carried.
    bookSlots = new BookPropertySet [maxBooks];
    freeSlots = maxBooks;
    for (int i = 0; i < bookSlots.Length; i++)
    {
      bookSlots[i] = null;
    }
    base.Start ();
    
    LuggageSummonEventManager.listenSummonLuggage (track);
    
    restore ();
    
  }
  
  public void Update ()
  {
    float nearDistance = 0.5f;
    float farDistance = 3.0f;
    
    if (trackedAvatar == null)
    {
      tracking = false;
    }
    
    if (tracking)
    {
      float trolleyDistance = (trolley.transform.position - trackedAvatar.transform.position).magnitude;
      Debug.Log ("Tral " + trolleyDistance);
      if (trolleyDistance > nearDistance)
      {
        GetComponent <Rigidbody> ().AddForce (-forceStrength * (Vector3.Normalize (trolley.transform.position - trackedAvatar.transform.position) + UnityEngine.Random.insideUnitSphere));
        transform.position += -1.0f * Time.deltaTime * Vector3.Normalize (trolley.transform.position - trackedAvatar.transform.position);
      }
      else
      {
        GetComponent <Rigidbody> ().velocity = new Vector3 (0, 0, 0);
      }
      trolleyMaterial.SetFloat ("LidOpen", ((Mathf.Clamp (trolleyDistance, nearDistance, farDistance) - nearDistance) / (farDistance - nearDistance)));
    }
  }
  
  public void track (GameObject av)
  {
    Debug.Log ("Tracking " + av);
    tracking = !tracking;
    trackedAvatar = av;
  }
  
  override public void populateMenu () 
  {
    addItemAsMenuOption (trolley, trolleyManoeuvre, null);
    
  }
  
  public float bookThickness = 0.03f;
  
  // Internal method to ensure the given slot has a book corresponding to
  // the specified property set.
  private void placeBook (BookPropertySet bp, int slot)
  {
    float bookOffset = slot * bookThickness;
    
    // Make a spine label with author and title.
    string bookSpine = bp.author + "\n" + bp.title;
    
    float x = 0.0f;
    float y = 0.9f;
    float z = 0.0f;
    
    // Create the book.
    GameObject bookObject = addMenuOption (bookSpine, new Vector3 (x + bookOffset, y, z), (controller, controllerObject, button, avatar, initialize) => selectBook (slot, controller, controllerObject, button, avatar, initialize), moveResponse);
    bookObject.GetComponent <BookProperties> ().props = bp;
    bookObject.transform.localRotation = Quaternion.AngleAxis (90.0f, Vector3.right);
    bookObject.transform.localScale = new Vector3 (0.98f * bookThickness, 0.6f * trolleyDepth, trolleyDepth);
    bookObject.transform.Find ("CheapBook").GetComponent <MeshRenderer> ().material.color = new Color (bp.colour[0], bp.colour[1], bp.colour[2]);
    
    // seems to fix a unity bug that prevents some colliders from being spotted.
    bookObject.GetComponent <BoxCollider> ().enabled = false;
    bookObject.GetComponent <BoxCollider> ().enabled = true;
  }
  
  public bool addBook (BookPropertySet bp)
  {
    if (freeSlots > 0)
    {
      int slot;
      for (slot = 0; slot < bookSlots.Length; slot++)
      {
        if (bookSlots[slot] == null)
        {
          break;
        }
      }
      
      if (slot < bookSlots.Length)
      {
        bookSlots[slot] = bp;
        freeSlots--;
        persist ();
        
        placeBook (bp, slot);
        return true;
      }
    }
    return false;
  }
  
  public void trolleyManoeuvre (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      GetComponent <Rigidbody> ().AddForce (-forceStrength * Vector3.Normalize (trolley.transform.position - avatar.transform.position));
    }       
  }
  
  // Respond to a book being selected from a trolley.
  public void selectBook (int slot, ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      if (bookSlots[slot] != null)
      {
        bookSlots[slot] = null;
        freeSlots++;
      }
      persist ();
      
      string fname = button.GetComponent <BookProperties> ().props.filename;
      GameObject book = Instantiate (bookTemplate);
      book.GetComponent <BookProperties> ().props = button.GetComponent <BookProperties> ().props;
      BookManager bookManager = book.GetComponent <BookManager> ();
      book.GetComponent <BookPlaceholdMenu> ().pickupBook (controller, controllerObject, button, avatar);
      
      removeMenuOption (button);
    }
  }
  
  // Create a unique file name to store this trolley's properties, based on the name of the trolley.
  private string getFilePath ()
  {
    string fp = System.Convert.ToBase64String (System.Text.Encoding.UTF8.GetBytes (trolleyName));
    fp = fp.Substring (Math.Max(0, fp.Length - 32)) + fp.GetHashCode ();
    return Application.persistentDataPath + "/" + fp + ".trolley";
  }
  
  public void persist ()
  {
    BinaryFormatter bf = new BinaryFormatter ();
    FileStream file = File.Open (getFilePath (), FileMode.Create);
    Debug.Log ("Saved to: " + getFilePath ());
    
    bf.Serialize (file, bookSlots.Length);
    for (int i = 0; i < bookSlots.Length; i++)
    {
      if (bookSlots[i] != null)
      {
        bf.Serialize (file, bookSlots[i].filename);
      }
      else
      {
        bf.Serialize (file, "");
      }
    }
    file.Close ();
  }
  
  public void restore ()
  {
    FileStream file = null;
    try
    {
      BinaryFormatter bf = new BinaryFormatter ();
      file = File.Open (getFilePath (), FileMode.Open);
      
      int maxBooks = (int) bf.Deserialize (file);
      bookSlots = new BookPropertySet [maxBooks];
      freeSlots = maxBooks;
      for (int i = 0; i < bookSlots.Length; i++)
      {
        string fn = (string) bf.Deserialize (file);
        if (fn.Length > 0)
        {
          bookSlots[i] = new BookPropertySet ();
          bookSlots[i].Load (fn);
          freeSlots--;
          placeBook (bookSlots[i], i);
        }
        else
        {
          bookSlots[i] = null;
        }
      }
      file.Close();
    }
    catch (Exception)
    {
      if (file != null)
      {
        file.Close ();
      }
      throw;
    }
  }
}
