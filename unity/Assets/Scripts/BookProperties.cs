using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.Runtime.Serialization.Formatters.Binary;
using System.IO;
using System;


/*
 * A persistent cache of book properties.
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
  
  // Create a unique file name to store this book's properties, based on the filename used to store the book.
  private static string getFilePath (string filepath)
  {
    string fp = System.Convert.ToBase64String (System.Text.Encoding.UTF8.GetBytes (filepath));
    fp = fp.Substring (Math.Max(0, fp.Length - 32)) + fp.GetHashCode ();
    return Application.persistentDataPath + "/" + fp + ".book";
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
