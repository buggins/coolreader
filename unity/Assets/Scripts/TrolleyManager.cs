using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class TrolleyManager : MenuInteraction
{
    //[Tooltip ("A unit cube object to be a plank for making the trolley")]
    //public GameObject plankTemplate;

//     public float trolleyLength = 1.0f;
    public float trolleyDepth = 0.3f;
//     public float plankThickness = 0.01f;
    
    public float forceStrength = 3.0f;
    
    private GameObject trolley;
    
    public GameObject bookTemplate;
    
    private bool [] bookSlots;
    private int freeSlots;
    
    // Start is called before the first frame update
    void Start()
    {
      trolley = this.gameObject;
//       trolley.GetComponent <BoxCollider> ().center = new Vector3 (trolleyLength / 2.0f, plankThickness / 2.0f, trolleyDepth / 2.0f);
//       trolley.GetComponent <BoxCollider> ().size = new Vector3 (trolleyLength, plankThickness, trolleyDepth);
      
//       createShelf (new Vector3 (0, 0, 0));

      int maxBooks = 8; // limit to number of books carried.
      bookSlots = new bool [maxBooks];
      freeSlots = maxBooks;
      for (int i = 0; i < bookSlots.Length; i++)
      {
        bookSlots[i] = false;
      }
      
      base.Start ();
    }

  override public void populateMenu () 
  {
    addItemAsMenuOption (trolley, trolleyManoeuvre, null);

  }
  
//   private float bookOffset = 0.0f;
  public float bookThickness = 0.03f;

  public bool addBook (BookPropertySet bp)
  {
    if (freeSlots > 0)
    {
      int slot;
      for (slot = 0; slot < bookSlots.Length; slot++)
      {
        if (bookSlots[slot] == false)
        {
          break;
        }
      }
      
      if (slot < bookSlots.Length)
      {
        bookSlots[slot] = true;
        freeSlots--;

        float bookOffset = slot * bookThickness;
        
        // Make a spine label with author and title.
        string bookSpine = bp.author + "\n" + bp.title;
          
        float x = 0.0f;
        float y = 0.0f;
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
  
//   // Create one plank.
//   private GameObject createPlank ()
//   {
//     GameObject plank = Instantiate (plankTemplate);
//     plank.transform.SetParent (trolley.transform);
//     plank.transform.localPosition = new Vector3 (0, 0, 0);
//     plank.transform.localRotation = Quaternion.identity;
//     
//     return plank;
//   }
// 
//   // Create a horizontal plank to be a shelf.
//   private void createShelf (Vector3 position)
//   {
//     GameObject plank = createPlank ();
//     plank.transform.localPosition = position;
//     plank.transform.localScale = new Vector3 (trolleyLength, plankThickness, trolleyDepth);
//   }
   
   // Respond to a book being selected from a trolley.
  public void selectBook (int slot, ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      if (bookSlots[slot])
      {
        bookSlots[slot] = false;
        freeSlots++;
      }
      
      string fname = button.GetComponent <BookProperties> ().props.filename;
      GameObject book = Instantiate (bookTemplate);
      book.GetComponent <BookProperties> ().props = button.GetComponent <BookProperties> ().props;
      BookManager bookManager = book.GetComponent <BookManager> ();
//      StartCoroutine (bookManager.loadBook (fname, button.GetComponent <BookProperties> ().props));
//      book.GetComponent <BookMenuInteraction> ().pickupBook (controller, controllerObject, button, avatar);
      book.GetComponent <BookPlaceholdMenu> ().pickupBook (controller, controllerObject, button, avatar);
      
      removeMenuOption (button);
      //Destroy (button);
    }
  }
 
}
