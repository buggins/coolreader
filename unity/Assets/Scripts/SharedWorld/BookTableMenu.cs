using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Photon.Pun;

public class BookTableMenu : MenuInteraction
{
  public GameObject netbookTemplate;
  
   override public void populateMenu () {
//     addItemAsMenuOption (this.gameObject, makeNetBook, moveResponse);
  }
  
  void Start()
  {
    base.Start ();
  }
  
  void Update()
  {
  }
  
  // A response when clicking on the book itself. Replace the book with an full book object.
  public void makeNetBook (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      Debug.Log ("making book"); 
      GameObject book = PhotonNetwork.Instantiate (netbookTemplate.name, new Vector3 (0, 2, 3), Quaternion.identity, 0);
      BookProperties bps = book.GetComponent <BookProperties> ();
      bps.props.filename = "This filename";
      bps.props.title = "Book title";
//       BookManager bookManager = book.GetComponent <BookManager> ();
// //       StartCoroutine (bookManager.loadBook (fname, props));
//       bookManager.loadBook (fname, props);
//       Debug.Log ("Loading book " + fname);
//       book.GetComponent <BookMenuInteraction> ().pickupBook (controller, controllerObject, button, avatar);
//       Destroy (button.transform.parent.gameObject);
    }
  }
  
  void OnTriggerEnter(Collider other)
   //void OnCollisionEnter(Collision collision)
    {
      if (other.gameObject.GetComponentInParent <BookProperties> () != null)
      {
      Debug.Log ("Collide " + other.gameObject.name);
        BookProperties bps = other.gameObject.GetComponentInParent <BookProperties> ();
        Destroy (other.gameObject.GetComponentInParent <BookProperties> ().gameObject);
        
      Debug.Log ("making book"); 
      GameObject book = PhotonNetwork.Instantiate (netbookTemplate.name, new Vector3 (0, 2, 3), Quaternion.identity, 0);
      book.GetComponent <BookProperties> ().props = bps.props;
//       bps.props.filename = "This filename";
//       bps.props.title = "Book title";
        
      }
//       if (canCollide && (collision.gameObject.GetComponent <TrolleyManager> () != null))
//       {
//         Debug.Log ("Handing to trolley");
// 
// // TODO : replace with remove all handlers.        
//       if (collision.gameObject.GetComponent <TrolleyManager> ().addBook (this.gameObject.GetComponent <BookProperties> ().props))
//       {
//         if (currentMoveController != null)
//         {
//                currentMoveController.removeHandler (bookMove, currentMoveControllerObj);
//         }
//         if (currentRetrieveController != null)
//         {
//                currentRetrieveController.removeHandler (bookRetrieve, currentRetrieveControllerObj);
//         }
//         if (currentRotateController != null)
//         {
//                currentRotateController.removeHandler (bookRotate, currentRotateControllerObj);
//         }
//         hideMenu ();
//         Destroy (this.gameObject);
//       }
//       }
//       
    }
}
