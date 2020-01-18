using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class TrolleyManager : MenuInteraction
{
    [Tooltip ("A unit cube object to be a plank for making the trolley")]
    public GameObject plankTemplate;

    public float trolleyLength = 1.0f;
    public float trolleyDepth = 0.3f;
    public float plankThickness = 0.01f;
    
    public float forceStrength = 3.0f;
    
    private GameObject trolley;
    
    public GameObject bookTemplate;
    
    // Start is called before the first frame update
    void Start()
    {
      trolley = this.gameObject;
      trolley.GetComponent <BoxCollider> ().center = new Vector3 (trolleyLength / 2.0f, plankThickness / 2.0f, trolleyDepth / 2.0f);
      trolley.GetComponent <BoxCollider> ().size = new Vector3 (trolleyLength, plankThickness, trolleyDepth);
      
      createShelf (new Vector3 (0, 0, 0));

      base.Start ();
    }

  override public void populateMenu () 
  {
    addItemAsMenuOption (trolley, trolleyManoeuvre, null);

  }
  
  private float bookOffset = 0.0f;
   public float bookThickness = 0.03f;

  public void addBook (BookPropertySet bp)
  {
    // Make a spine label with author and title.
    string bookSpine = bp.author + "\n" + bp.title;
      
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    
    // Create the book.
    GameObject bookObject = addMenuOption (bookSpine, new Vector3 (x + plankThickness + bookOffset, y + plankThickness, z), selectBook, moveResponse);
    bookObject.GetComponent <BookProperties> ().props = bp;
    bookObject.transform.localScale = new Vector3 (0.98f * bookThickness, 0.8f * 1.0f, 0.95f * trolleyDepth);
    bookObject.transform.Find ("CheapBook").GetComponent <MeshRenderer> ().material.color = new Color (bp.colour[0], bp.colour[1], bp.colour[2]);
      
    // seems to fix a unity bug that prevents some colliders from being spotted.
    bookObject.GetComponent <BoxCollider> ().enabled = false;
    bookObject.GetComponent <BoxCollider> ().enabled = true;
    
    bookOffset += bookThickness;
  }
  
  public void trolleyManoeuvre (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      GetComponent <Rigidbody> ().AddForce (-forceStrength * Vector3.Normalize (trolley.transform.position - avatar.transform.position));
    }       
  }
  
  // Create one plank.
  private GameObject createPlank ()
  {
    GameObject plank = Instantiate (plankTemplate);
    plank.transform.SetParent (trolley.transform);
    plank.transform.localPosition = new Vector3 (0, 0, 0);
    plank.transform.localRotation = Quaternion.identity;
    
    return plank;
  }

  // Create a horizontal plank to be a shelf.
  private void createShelf (Vector3 position)
  {
    GameObject plank = createPlank ();
    plank.transform.localPosition = position;
    plank.transform.localScale = new Vector3 (trolleyLength, plankThickness, trolleyDepth);
  }
   
   // Respond to a book being selected from a trolley.
  public void selectBook (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      string fname = button.GetComponent <BookProperties> ().props.filename;
      GameObject book = Instantiate (bookTemplate);
      book.GetComponent <BookProperties> ().props = button.GetComponent <BookProperties> ().props;
      BookManager bookManager = book.GetComponent <BookManager> ();
//      StartCoroutine (bookManager.loadBook (fname, button.GetComponent <BookProperties> ().props));
//      book.GetComponent <BookMenuInteraction> ().pickupBook (controller, controllerObject, button, avatar);
      book.GetComponent <BookPlaceholdMenu> ().pickupBook (controller, controllerObject, button, avatar);
      
      removeMenuOption (button);
      Destroy (button);
    }
  }
 
}
