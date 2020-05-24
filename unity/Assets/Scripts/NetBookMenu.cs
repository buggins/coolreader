using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Photon.Pun;
using System.IO;

[RequireComponent (typeof (PhotonView))]
[RequireComponent (typeof (BookProperties))]
public class NetBookMenu : MenuInteraction, IPunObservable
{
  private PhotonView pView;  
  private BookProperties bProps;
  
  private byte [] bookData;
  
  // Buffer that will hold a local copy of the file.
  private byte [] localData;
  // Keeps track of the blocks of the file that have been received.
  private bool [] segments = null;
  // The number of unique segments received.
  private int segmentCount = -1;
  // The next segment required.
  private int nextSegmentStart;
  // Time between packet requests if we don't hear a response.
  private float retransmitTimeout = 3.0f;
  // Size of a block.
  private int packetLength = 1000;
  // Flag to indicate request still pending.
  private bool receivedResponse;
  
  public AudioSource bookDropSound;
  public GameObject bookTemplate;
   
  [Tooltip ("The prefab for a full book representation, used to replace this object when it is opened")]
  public GameObject fullBookTemplate;
  
  // Add the menu buttons.
  override public void populateMenu () {
    
    addItemAsMenuOption (this.gameObject.transform.Find ("BookShape").gameObject, downloadBook, moveResponse);

//     addMenuOption ("Position\nBook", new Vector3 (-0.3f, 0.65f, 0.05f), moveBook, moveResponse);
//     addMenuOption ("Retrieve\nBook", new Vector3 (-0.1f, 0.65f, 0.05f), retrieveBook, moveResponse);
//     addMenuOption ("Rotate\nBook", new Vector3 (0.1f, 0.65f, 0.05f), rotateBook, moveResponse);
//     addMenuOption ("Drop\nBook", new Vector3 (0.0f, -0.6f, 0.05f), dropBook, moveResponse);
  }
  
  void Start()
  {
    ExitGames.Client.Photon.PhotonPeer.RegisterType (typeof (BookPropertySet), 0x10, BookPropertySet.Serialize, BookPropertySet.Deserialize);

    pView = GetComponent <PhotonView> ();
    bProps = GetComponent <BookProperties> ();

//     bProps.props.filename = "/tmp/9.2 (55).pdf";      

    bookData = null;
    base.Start ();
  }
  
  void Update()
  {
  }
  
  public void OnPhotonSerializeView (PhotonStream stream, PhotonMessageInfo info)
  {
    if (stream.IsWriting)
    {
      // send data
      stream.SendNext (bProps.props); 
//       Debug.Log ("Send props" + bProps.props.title); 
    }
    else
    {
      bProps.props = (BookPropertySet) stream.ReceiveNext();
//       Debug.Log ("Rec props " + bProps.props.title);
    }
  }

  [PunRPC]
  void retrieveBook (int start, int length, PhotonMessageInfo info)
  {
    Debug.LogFormat ("RetrieveBook: {0} - {1} - {2} : {3}", info.Sender, info.photonView, info.SentServerTime, start);
    if (bookData == null)
    {
      bookData = File.ReadAllBytes (bProps.props.filename);
    }
    if ((bookData != null) && (start >= 0) && (length >= 0) && (start < bookData.Length))
    {
      length = Mathf.Min (length, bookData.Length - start);
      byte [] data = new byte [length];
      for (int i = 0; i < length; i++)
      {
        data[i] = bookData[start + i];
      }
      GetComponent <PhotonView> ().RPC ("receiveSegment", info.Sender, start, length, data, bookData.Length);
    }    
  }

  [PunRPC]
  void receiveSegment (int start, int length, byte [] data, int totalLength, PhotonMessageInfo info)
  {
    if (segments == null)
    {
      // first packet, initialize receiving process.
      localData = new byte [totalLength];
      segments = new bool [(totalLength + packetLength - 1) / packetLength];
      segmentCount = 0;
      nextSegmentStart = 0;
      
      for (int i = 0; i < segments.Length; i++)
      {
        segments[i] = false;
      }
    }
    
    if (totalLength != localData.Length)
    {
      Debug.Log ("Length changed mid tranmission: " + totalLength + " " + localData.Length);
    }
    else
    {
      if ((start >= 0) && (length >= 0) && (start + length <= localData.Length))
      {
        // valid block.
        Debug.Log ("La " + (length == packetLength) + " " + (start + length == localData.Length));
        if ((length == packetLength) || (start + length == localData.Length))
        {
          // valid data in block.
          for (int i = 0; i < length; i++)
          {
            localData[start + i] = data[i];
          }
          int segnum = start / packetLength;
          if (!segments[segnum])
          {
            segmentCount++;
          }
          segments[segnum] = true;
          bool found = false;
          for (int i = 1; i < segments.Length; i++)
          {
            if (!segments[(segnum + i) % segments.Length])
            {
              nextSegmentStart = ((segnum + i) % segments.Length) * packetLength;
              found = true;
              break;
            }
          }

          // request next segment.
          receivedResponse = true;
        }
      }
    }
    Debug.LogFormat ("ReceiveSegment: {0} - {1} - {2} {3} {4} {5} / {6}", info.Sender, info.photonView, info.SentServerTime, start, length, data.Length, totalLength);
  }

  IEnumerator timeoutCoroutine (float time)
  {
    yield return new WaitForSeconds (time);
    receivedResponse = true;
  }
  
   private string getFilePath ()
  {
    Debug.Log ("File path " + bProps.props.filename);
    string fp = System.Convert.ToBase64String (System.Text.Encoding.UTF8.GetBytes (bProps.props.filename)) + "-" + Path.GetFileName (bProps.props.filename);
    fp = fp.Substring (Mathf.Max(0, fp.Length - 32));
    return Application.persistentDataPath + "/" + fp;
  }

  
  IEnumerator downloadProcess ()
  {
    
    localData = null;
    segments = null;
    nextSegmentStart = 0;
    segmentCount = -1;
    
    while ((segments == null) || (segmentCount < segments.Length))
    {
      if (segments != null)
      {
        Debug.Log ("Requesting " + nextSegmentStart + " / " + segmentCount + " " + segments.Length);
      }
      else
      {
        Debug.Log ("Requesting " + nextSegmentStart + " / " + segmentCount);
      }
      receivedResponse = false;
// //       GetComponent <PhotonView> ().RPC ("retrieveBook", RpcTarget.All, nextSegmentStart, packetLength);
      Debug.Log ("Sending req to " + GetComponent <PhotonView> ().Owner);
      GetComponent <PhotonView> ().RPC ("retrieveBook", GetComponent <PhotonView> ().Owner, nextSegmentStart, packetLength);
      Coroutine c = StartCoroutine (timeoutCoroutine (retransmitTimeout));
      
      yield return new WaitUntil (() => receivedResponse);
      StopCoroutine (c);
    }
    
    File.WriteAllBytes (getFilePath (), localData);
      GameObject book = Instantiate (bookTemplate, new Vector3 (-3, 2, 2), Quaternion.identity);
      BookPropertySet bps = new BookPropertySet ();
      bps.filename = getFilePath ();
      book.GetComponent <BookProperties> ().props = bps;
      BookManager bookManager = book.GetComponent <BookManager> ();
  //    book.GetComponent <BookPlaceholdMenu> ().pickupBook (controller, controllerObject, button, avatar);
      Destroy (this.gameObject);
    
    Debug.Log ("All Done");
  }
  
  // A response when clicking on the book itself. Replace the book with an full book object.
  public void downloadBook (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
  {
    if (!initialize)
    {
      BookPropertySet props = bProps.props;
      
      string fname = props.filename;
      Debug.Log ("Downloading book" + fname); 
      StartCoroutine (downloadProcess ());
//      GetComponent <PhotonView> ().RPC ("retrieveBook", RpcTarget.All, 0, 1000);
//       GameObject book = Instantiate (fullBookTemplate);
//       BookManager bookManager = book.GetComponent <BookManager> ();
// //       StartCoroutine (bookManager.loadBook (fname, props));
//       bookManager.loadBook (fname, props);
//       Debug.Log ("Loading book " + fname);
//       book.GetComponent <BookMenuInteraction> ().pickupBook (controller, controllerObject, button, avatar);
//       Destroy (button.transform.parent.gameObject);
    }
  }
 
//  private ControlInput currentMoveController = null;
//  private ControlInput currentRetrieveController = null;
//  private ControlInput currentRotateController = null;
//  private ControlInput.ControllerDescription currentMoveControllerObj = null;
//  private ControlInput.ControllerDescription currentRetrieveControllerObj = null;
//  private ControlInput.ControllerDescription currentRotateControllerObj = null;
//  
//  void OnCollisionEnter(Collision collision)
//     {
//       Debug.Log ("Collide");
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
//     }
//     
// 
//   
//   ////////// TODO: duplicates from BookMenuInteraction. To an interface class ....
// 
//     // The trigger is usually pressed when most menu response handlers start.
//   // This helps by waiting till it is released, before the next press is accepted.
//   private bool triggerCleared = false;
//   // Various alignment operations keep some of the book's transformation intact.
//   // Distance is one of these constants.
//   private float bookDistance;
//   // Direction to the book is another constant preserved during some manipulations.
//   private Vector3 bookDirection;
// // Flag when the book is dropped, and no longer of interest.
//   private bool dropped = false;
//   
//   // Disable collisions until the book has been put down for the first time.
//   private bool canCollide = false;
//   
//   // Pick up a book, also used when a book is first created.
//   public void pickupBook (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
//   {
//     // Open the book.
//  //   setOpen ();
// 
//     // Enable the menu.
//     showMenu ();
//     
//     // Disable physics.
//     GetComponent <Rigidbody> ().useGravity = false;
//     GetComponent <Rigidbody> ().isKinematic = true;
//     transform.SetParent (null);
//     dropped = false;
//     
//     // Add the book to the collection of active books. 
//     GameObject activeBooks = GameObject.Find ("ActiveBooks");
//     if (activeBooks != null)
//     {
//       transform.SetParent (activeBooks.transform);
//     }
//     
//     // Bring it near the user and let them position it somewhere.
//     this.transform.position = controllerObject.controllerObject.transform.position + 1.0f * controllerObject.controllerObject.transform.forward;
//     moveBook (controller, controllerObject, null, avatar);
//   }
//   
//   
// // Drop the book.
//   public void dropBook (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
//   {
//     if (!initialize)
//     {
//       // Close it first.
// //       setClosed ();
// 
//       // Switch off menu - a dropped book is no longer in use.
//       hideMenu ();
//       
//       // Switch off settings.
// //       settingsOpen = false;
// //       updateSettings ();
//       
//       // Let physics have it.
//       GetComponent <Rigidbody> ().useGravity = true;
//       GetComponent <Rigidbody> ().isKinematic = false;
//       GetComponent <Rigidbody> ().angularVelocity = new Vector3 (-1.1f, -1.2f, -1.3f);
//       
//       // Remove it from the managed set.
//       transform.SetParent (null);
//       dropped = true;
//       // Ensure it doesn't get transferred between scenes.
// //       SceneManager.MoveGameObjectToScene (gameObject, SceneManager.GetActiveScene ());
//       
//       // Sound effect.
//       bookDropSound.Play ();
//     }
//   }
//   
//   
//   // Activate the book positioning process.
//   public void moveBook (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
//   {
//     if (!initialize)
//     {
//       triggerCleared = false;
//       bookDistance = (transform.position - controllerObject.controllerObject.transform.position).magnitude;
//       
//       // Disable collider while moving so that it doesn't interfere with other objects.
//       GetComponentInChildren <BoxCollider> ().enabled = false;
//       
//       controller.addHandler (bookMove, controllerObject, true);
//       currentMoveController = controller;
//       currentMoveControllerObj = controllerObject;
//     }    
//   }
//   
//   // Move the book at a constant distance from the controller, and still facing the controller.
//   public void bookMove (ControlInput controller, ControlInput.ControllerDescription controllerObject, bool trigger, bool debounceTrigger, Vector3 direction, Vector3 position, GameObject avatar, bool touchpad, Vector2 touchposition)
//   {
//     //  print ("Movingbook " + bookDistance + " " + direction);
//     if (!debounceTrigger)
//     {
//       triggerCleared = true;
//     }
//     if (debounceTrigger && triggerCleared)
//     {
//       currentMoveController = null;
//       controller.removeHandler (bookMove, controllerObject);
//       canCollide = true;
//       GetComponentInChildren <BoxCollider> ().enabled = true;
//     }
//     
//     transform.position = position + bookDistance * direction;
//     transform.up = Vector3.up;
//     transform.forward = direction;
//   }
// 
//   // Move the book closer.
//   public void retrieveBook (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
//   {
//     if (!initialize)
//     {
//       triggerCleared = false;
//       bookDirection = transform.position - controller.avatar.transform.position;
//       bookDistance = bookDirection.magnitude;
//       bookDirection = Vector3.Normalize (bookDirection);
//       controller.addHandler (bookRetrieve, controllerObject, true);
//       currentRetrieveController = controller;
//       currentRetrieveControllerObj = controllerObject;
//     }
//   }
//   
//   // Book moves backwards or forwards along the line between controller and its original position. Aiming the controller up or down controls distance.
//   public void bookRetrieve (ControlInput controller, ControlInput.ControllerDescription controllerObject, bool trigger, bool debounceTrigger, Vector3 direction, Vector3 position, GameObject avatar, bool touchpad, Vector2 touchposition)
//   {
// //           print ("Retrievingbook " + bookDistance + " " + direction);
//     if (!debounceTrigger)
//     {
//       triggerCleared = true;
//     }
//     if (debounceTrigger && triggerCleared)
//     {
//       currentRetrieveController = null;
//       controller.removeHandler (bookRetrieve, controllerObject);
//     }
//     
//     transform.position = avatar.transform.position + bookDirection * bookDistance * Mathf.Pow (2.0f, 5.0f * direction.y);
//   }
// 
//   // Rotate the book to get the best reading angle.
//   public void rotateBook (ControlInput controller, ControlInput.ControllerDescription controllerObject, GameObject button, GameObject avatar, bool initialize = false)
//   {
//     if (!initialize)
//     {
//       triggerCleared = false;
//       GetComponentInChildren <BoxCollider> ().enabled = false;
//       controller.addHandler (bookRotate, controllerObject, true);
//       currentRotateController = controller;
//       currentRotateControllerObj = controllerObject;
//     }
//   }
//   
//   // Set the rotation of the book to match the controller's orientation.
//   public void bookRotate (ControlInput controller, ControlInput.ControllerDescription controllerObject, bool trigger, bool debounceTrigger, Vector3 direction, Vector3 position, GameObject avatar, bool touchpad, Vector2 touchposition)
//   {
// //           print ("Rotatingbook " + bookDistance + " " + direction);
//     if (!debounceTrigger)
//     {
//       triggerCleared = true;
//     }
//     if (debounceTrigger && triggerCleared)
//     {
//       currentRotateController = null;
//       controller.removeHandler (bookRotate, controllerObject);
//       GetComponentInChildren <BoxCollider> ().enabled = true;
//     }
//     
//     transform.transform.rotation = controllerObject.controllerObject.transform.rotation;
//   }
  
}
