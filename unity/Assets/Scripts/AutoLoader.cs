using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class AutoLoader : MonoBehaviour
{
  public BookManager bm;
  
    void Start ()
    {
      Debug.Log ("Loading book");
      StartCoroutine (bm.loadBook ("/tmp/algorithms-05-00588.pdf", new BookPropertySet ()));
//       StartCoroutine (bm.loadBook ("/tmp/pg14838-images.epub", new BookPropertySet ()));
      Debug.Log ("Loaded book");
    }

}
