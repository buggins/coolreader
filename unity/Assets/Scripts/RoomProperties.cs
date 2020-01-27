using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.Runtime.Serialization.Formatters.Binary;
using System.IO;
using System;


/*
 * A persistent cache of room properties.
 * 
 * Any new properties should be added in corresponding positions
 * in Save, and in RawLoad.
 */
[System.Serializable]
public class RoomPropertySet {

  List <string> books;
//   // The current property list.
//   // Path and filename containing the ebook.
//   public string filename = "";
//   // Author, for quick access.
//   public string author = "";
//   // Title, for quick access.
//   public string title = "";
//   // A cover colour, to distinguish the book from its neighbours.
//   public float[] colour = new float [3];
//   
//   // User configurable options.
//   // The current font size.
//   public int fontSize = 100;
//   // The current page that the book is open at.
//   public int currentPage = 0;
  
  public RoomPropertySet ()
  {
    books = new List <string> ();
  }

  public void addBook (string bookIdentifier)
  {
    books.Add (bookIdentifier);
    Debug.Log ("Added " + bookIdentifier);
  }
  
  // Create a unique file name to store this room's properties, based on the name of the room.
  private static string getFilePath (string roomIdentifier)
  {
    string fp = System.Convert.ToBase64String (System.Text.Encoding.UTF8.GetBytes (roomIdentifier));
    fp = fp.Substring (Math.Max(0, fp.Length - 32)) + fp.GetHashCode ();
    return Application.persistentDataPath + "/" + fp + ".room";
  }
  
  // Overwrite existing room properties.
  public void Save (string roomIdentifier)
  {
    BinaryFormatter bf = new BinaryFormatter ();
    FileStream file = File.Open (getFilePath (roomIdentifier), FileMode.Create);
    Debug.Log ("Saved to: " + getFilePath (roomIdentifier));

    bf.Serialize (file, books.Count);
    
    foreach (string book in books)
    {
      bf.Serialize (file, book);
    }
    file.Close ();
  }
  
  // Utility function to retrieve directory with all book property files.
  public static string [] getPropertyCacheFiles ()
  {
    return Directory.GetFiles (Application.persistentDataPath, "*.room");
  }
  
  // Check if book properties already exist for the ebook stored in the given file.
  public static bool haveRecord (string roomIdentifier)
  {
    return File.Exists (getFilePath (roomIdentifier));
  }
  
  // Retrieve book properties from the given book property file.
  public void RawLoad (string directfilepath)
  {
    FileStream file = null;
    books = new List <string> ();
    try
    {
      BinaryFormatter bf = new BinaryFormatter ();
      file = File.Open (directfilepath, FileMode.Open);
      
      int numBooks = (int) bf.Deserialize (file);
      for (int i = 0; i < numBooks; i++)
      {
        addBook ((string) bf.Deserialize (file));
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

  // Given an ebook file path, retrieve the book properties for it, if they exist.
  public void Load (string roomIdentifier)
  {
    try
    {
      RawLoad (getFilePath (roomIdentifier));
    }
    catch (Exception)
    {
      throw;
    }
  }
}

/*
 * Wrapper for the room configuration.
 */
public class RoomProperties : MonoBehaviour {
  public static void persistRoom (string roomName)
  {
    RoomPropertySet props = new RoomPropertySet ();
    
    GameObject activeBooks = GameObject.Find ("ActiveBooks");
    if (activeBooks != null)
    {
      foreach (Transform child in activeBooks.transform)
      {
        BookManager bm = child.GetComponent <BookManager> ();
        if (bm != null)
        {
          Debug.Log ("Found book " + bm.getBookID ());
          props.addBook (bm.getBookID ());
        }
      }
    }
  
    Debug.Log ("Persisting room: " + roomName);
    props.Save (roomName);
  }

  public static void restoreRoom (string roomName)
  {
    RoomPropertySet props = new RoomPropertySet ();
    Debug.Log ("Restoring room: " + roomName);
    props.Load (roomName);
  }
}
