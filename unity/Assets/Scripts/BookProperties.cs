using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.Runtime.Serialization.Formatters.Binary;
using System.IO;
using System;


/*
 * A persistent cache of book properties.
 * 
 * Any new properties should be added in corresponding positions
 * in Save, and in RawLoad.
 */
[System.Serializable]
public class BookPropertySet {

  // The current property list.
  // Path and filename containing the ebook.
  public string filename = "";
  // Author, for quick access.
  public string author = "";
  // Title, for quick access.
  public string title = "";
  // A cover colour, to distinguish the book from its neighbours.
  public float[] colour = new float [3];
  
  // User configurable options.
  // The current font size.
  public int fontSize = 100;
  // The current page that the book is open at.
  public int currentPage = 0;
  
  // Create a unique file name to store this book's properties, based on the filename used to store the book.
  private static string getFilePath (string filepath)
  {
    string fp = System.Convert.ToBase64String (System.Text.Encoding.UTF8.GetBytes (filepath));
    fp = fp.Substring (Math.Max(0, fp.Length - 32)) + fp.GetHashCode ();
    return Application.persistentDataPath + "/" + fp + ".book";
  }
  
  public static BookPropertySet Deserialize (byte[] data)
  {
    MemoryStream ms = new MemoryStream (data);
    BookPropertySet bp = new BookPropertySet ();
    try
    {
      BinaryFormatter bf = new BinaryFormatter ();
      bp.filename = (string) bf.Deserialize (ms);
      bp.author = (string) bf.Deserialize (ms);
      bp.title = (string) bf.Deserialize (ms);
      bp.colour = (float []) bf.Deserialize (ms);
      
      bp.currentPage = (int) bf.Deserialize (ms);
      bp.fontSize = (int) bf.Deserialize (ms);
    }
    catch (Exception)
    {
      throw;
    }
    
    return bp;
  }

  public static byte[] Serialize (object bpo)
  {
    BinaryFormatter bf = new BinaryFormatter ();
    BookPropertySet bp = (BookPropertySet) bpo;
    MemoryStream ms = new MemoryStream ();
    bf.Serialize (ms, bp.filename);
    bf.Serialize (ms, bp.author);
    bf.Serialize (ms, bp.title);
    bf.Serialize (ms, bp.colour);
    
    bf.Serialize (ms, bp.currentPage);
    bf.Serialize (ms, bp.fontSize);
    return ms.GetBuffer ();
  }
  
  // Overwrite existing book properties.
  public void Save ()
  {
    BinaryFormatter bf = new BinaryFormatter ();
    FileStream file = File.Open (getFilePath (filename), FileMode.Create);
    Debug.Log ("Saved to: " + getFilePath (filename));
    bf.Serialize (file, filename);
    bf.Serialize (file, author);
    bf.Serialize (file, title);
    bf.Serialize (file, colour);
    
    bf.Serialize (file, currentPage);
    bf.Serialize (file, fontSize);
    file.Close ();
  }
  
  // Utility function to retrieve directory with all book property files.
  public static string [] getPropertyCacheFiles ()
  {
    return Directory.GetFiles (Application.persistentDataPath, "*.book");
  }
  
  // Check if book properties already exist for the ebook stored in the given file.
  public static bool haveRecord (string filepath)
  {
    return File.Exists (getFilePath (filepath));
  }
  
  // Retrieve book properties from the given book property file.
  public void RawLoad (string directfilepath)
  {
    FileStream file = null;
    try
    {
      BinaryFormatter bf = new BinaryFormatter ();
      file = File.Open (directfilepath, FileMode.Open);
      filename = (string) bf.Deserialize (file);
      author = (string) bf.Deserialize (file);
      title = (string) bf.Deserialize (file);
      colour = (float []) bf.Deserialize (file);
      
      currentPage = (int) bf.Deserialize (file);
      fontSize = (int) bf.Deserialize (file);
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
  public void Load (string filepath)
  {
    try
    {
      RawLoad (getFilePath (filepath));
    }
    catch (Exception)
    {
      throw;
    }
  }
}

/*
 * Wrapper for the book properties.
 */
public class BookProperties : MonoBehaviour {
  public BookPropertySet props;
}
