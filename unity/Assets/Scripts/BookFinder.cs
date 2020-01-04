using System.Collections;
using System.Collections.Generic;
using UnityEngine;
#if PLATFORM_ANDROID
using UnityEngine.Android;
#endif
using System.IO;
using System;

/* Finds all books on the device. Books that are found have their details
 * cached for fast lookup the next time. */
public class BookFinder : MonoBehaviour {
  
  // A list of directories that should be searched. Likely candidates can be
  // added to the head of this list.
  private List<string> knownSources;
  
  // The filenames (and path) to all books found, used as the unique identifier
  // for any book.
  private List<string> allBooks;
  
  // An internal progress indicator (from 0 to 1).
  private float progress;
  
  // External objects if a visible progress bar is required.
  [Tooltip ("Progress bar object. Should have unit scale, and the y-coordinate of scale represents progress.")]
  public GameObject progressBar;
  [Tooltip ("Activity indicator that will be rotated a small amount around the z-axis each will search continues.")]
  public GameObject activityIndicator;
  
  public static string GetAndroidExternalStoragePath ()
  {
    string path = "/";
    try
    {
      AndroidJavaClass jc = new AndroidJavaClass("android.os.Environment") ;
      path = jc.CallStatic<AndroidJavaObject>("getExternalStorageDirectory").Call<string>("getAbsolutePath");
    }
    catch (Exception)
    {
    }
    return path;
  }
  
  // Identify some locations where books might be found.
  private void prepareSources ()
  {
    knownSources = new List <string> ();
    
    // Hardwire a list of likely book locations, for the known platforms.
    knownSources.Add (Application.persistentDataPath + "/");
    knownSources.Add (GetAndroidExternalStoragePath ());
    
//     knownSources.Add ("/storage/self/primary/");
//     knownSources.Add ("/tmp");
//     knownSources.Add ("/home");
    knownSources.Add ("/");
  }
  
  void Start () {
    
    progress = 0.0f;
    
    prepareSources ();
    allBooks = new List <string> ();
    
    updateProgress ();
    // Run the search in a separate thread, checking at most one file/directory before
    // yielding.
    StartCoroutine (populateSourcesList ());
  }
  
  // Return a list of file sources that are used for finding books.
  public List<string> getSources ()
  {
    // Request any permissions required to access the file system.
    switch (SelectController.getActivePlatform ())  
    {
      case SelectController.DeviceOptions.Daydream:
  
#if UNITY_ANDROID
        string permission = "android.permission.READ_EXTERNAL_STORAGE";
        GvrPermissionsRequester permissionRequester = GvrPermissionsRequester.Instance; 
        if (permissionRequester != null)
        {
          if (!permissionRequester.IsPermissionGranted (permission))
          {
            permissionRequester.ShouldShowRational (permission);
            permissionRequester.RequestPermissions (new string [] { permission }, (GvrPermissionsRequester.PermissionStatus [] permissionResults) => {
              foreach (GvrPermissionsRequester.PermissionStatus p in permissionResults)
              {
                  Debug.Log ("Req perm " + p.Name + ": " + (p.Granted ? "Granted" : "Denied") + "\n");
              }

            });
            bool granted = permissionRequester.IsPermissionGranted (permission);
          }
        }
#endif
        break;
      case SelectController.DeviceOptions.OculusGo:
      case SelectController.DeviceOptions.OculusQuest:
#if UNITY_ANDROID
        if (!Permission.HasUserAuthorizedPermission(Permission.ExternalStorageRead))
        {
          Permission.RequestUserPermission(Permission.ExternalStorageRead);
        }
#endif
        break;
      default:
        break;
    }

    if (knownSources == null)
    {
// #if PLATFORM_ANDROID
//       if (!Permission.HasUserAuthorizedPermission(Permission.ExternalStorageRead))
//       {
//         Permission.RequestUserPermission(Permission.ExternalStorageRead);
//       }
// #endif
      prepareSources ();
    }
    return knownSources;
  }
  
  // Update the progress bar and activity indicator objects, based on the 
  // last calculated value of progress.
  private void updateProgress ()
  {
    if (progressBar != null)
    {
      progressBar.transform.localScale = new Vector3 (1, progress, 1);
      // Progress bar is red while searching, and changes to green on completion.
      if (progress >= 1.0f)
      {
        progressBar.GetComponent <MeshRenderer> ().material.color = new Color (0, 1, 0);
      }
      else
      {
        progressBar.GetComponent <MeshRenderer> ().material.color = new Color (1, 0.5f, 0);
      }
    }
    if (activityIndicator != null)
    {
      activityIndicator.transform.rotation *= Quaternion.AngleAxis (18.1345934f, Vector3.forward);
      // Activity indicator is hidden when search is complete.
      if (progress >= 1.0f)
      {
        activityIndicator.SetActive (false);
      }
    }
  }
  
  // Get the book shelf manager to add the book.
  IEnumerator createBook (string name)
  {
    if (GetComponent <BookShelfManager> () != null)
    {
      yield return GetComponent <BookShelfManager> ().addBook (name);
    }
  }
  
  public List <string> getFilesIn (string path)
  {
    List<string> f = new List<string> ();
    try
    {
      f.AddRange (Directory.GetFiles (path));
    }
    catch (Exception)
    {
    }
    return f;
  }
  
  public List <string> getDirectoriesIn (string path)
  {
    List<string> d = new List<string> ();
    try
    {
      d.AddRange (Directory.GetDirectories (path));
    }
    catch (Exception e)
    {
      Debug.Log ("Issues with directories in: " + path + " is " + e);
      return null;
    }
    return d;
  }
  
  // Check if file counts as a book.
  public bool isBook (string path)
  {
    if (BookEngine.getFormatFromName (path) != BookEngine.BookFormat.unknown)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
  
  public bool haveBook (string path)
  {
    if (allBooks.Contains (path))
    {
      return true;
    }
    return false;
  }
  
  public IEnumerator registerBook (string path)
  {
    if (isBook (path))
    {
      // If we don't already have this book, add it.
      if (!allBooks.Contains (path))
      {
        allBooks.Add (path);
        yield return createBook (path);
      }
    }
  }
  
  // Recursive file search. ProgressProp is the proportion of the progress bar 
  // that needs to be filled in by this part of the search.
  IEnumerator searchFiles (string root, float progressProp)
  {
    //print ("Searching in " + root);
    
    // Look for any books in this directory.
    List<string> f = new List<string> ();
    List<string> suffixes = BookEngine.getAcceptedFormats ();
    try
    {
      foreach (string suffix in suffixes)
      {
        f.AddRange (Directory.GetFiles (root, "*" + suffix));
      }
    }
    catch (Exception)
    {
    }
    
    foreach (string fname in f)
    {
      // print ("Found files " + fname +" in " + root);
      yield return registerBook (fname);
    }
    
    yield return "check files";
    
    // Now check all subdirectories, dividing progress evenly amongst each.
    List <string> d = getDirectoriesIn (root);
    
    if (d != null)
    {
      foreach (string dname in d)
      {
        //      print ("Checking " + dname);
        yield return searchFiles (dname, progressProp * 1.0f / d.Count);
        updateProgress ();
      }
    }
  }
  
  // Run through the previously found books, and retrieve these. Any
  // bad files are ignored, and these will be recreated if the books are
  // found on the explicit file search.
  private IEnumerator searchCache (float progressProp)
  {
    string [] cache = BookPropertySet.getPropertyCacheFiles ();
    foreach (string book in cache)
    {
      BookPropertySet bp = new BookPropertySet ();
      bool loaded = false;
      try
      {
        bp.RawLoad (book);
        loaded = true;
      }
      catch (Exception)
      {
        // bad file, ignore.
      }
      
      if (loaded && (File.Exists (bp.filename)))
      {
        yield return registerBook (bp.filename);
      }
      progress += progressProp * (1.0f / cache.Length);
      updateProgress ();
    }
  }
  
  IEnumerator populateSourcesList ()
  {
    float progressProp = 0.5f;
    
    // Search through existing cache.
    yield return searchCache (progressProp);
    
    // Now look for new books.
    int count = 0;
    foreach (string source in knownSources)
    {
      yield return searchFiles (source, progressProp);
      progress = 0.5f + progressProp * (float) count / knownSources.Count;
      updateProgress ();
      count++;
    }
    Debug.Log ("Found " + allBooks.Count + " books");
    
    progress = 1.0f;
    updateProgress ();
    yield return "done";
  }
}
