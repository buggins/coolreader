using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.SceneManagement;

public class RoomPersist : MonoBehaviour
{
  public GameObject bookMarkerPrefab;
  
  IEnumerator persistRoom (float delay)
  {
    while (true)
    {
      yield return new WaitForSeconds (delay);
      Debug.Log ("Persisting");
      RoomProperties.persistRoom (SceneManager.GetActiveScene().name);
    }
  }
  
  void Start()
  {
   // StartCoroutine (persistRoom (5.0f));

    RoomProperties.restoreRoom (SceneManager.GetActiveScene().name, bookMarkerPrefab);    
  }
}
