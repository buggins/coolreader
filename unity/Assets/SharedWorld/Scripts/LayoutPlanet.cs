using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;

// Apply this to the terrain object.
public class LayoutPlanet : MonoBehaviour
{
  public Material planetMaterial;
  
  public GameObject treePrefab;
  
  public float TerrainSize = 100.0f;
  
  private Vector4 tp;

  private Dictionary <GameObject, Vector3> objOriginalPositions;
  
  private void placeTrees ()
  {
    for (float x = 0.0f; x < 1.0f; x += 0.1f)
      {
        for (float y = 0.0f; y < 1.0f; y += 0.1f)
        {
          Vector3 p = new Vector3 ((x - 0.5f) * TerrainSize, 0.0f, (y - 0.5f) * TerrainSize);
          GameObject g = Instantiate (treePrefab, p, Quaternion.identity);
          g.tag = "PlanetSurfaceLocked";
        }
      }

  }
  
  private void findObjectOriginals ()
  {
    objOriginalPositions = new Dictionary <GameObject, Vector3> ();
    GameObject [] g = GameObject.FindGameObjectsWithTag ("PlanetSurfaceLocked");
    Debug.Log ("Find " + g.Length);
    foreach (GameObject go in g)
    {
      objOriginalPositions[go] = go.transform.position;
    }
  }
  
  private void replaceObjects ()
  {
    GameObject [] g = GameObject.FindGameObjectsWithTag ("PlanetSurfaceLocked");
    foreach (GameObject go in g)
    {
      if (objOriginalPositions.ContainsKey (go))
      {
        Vector3 pos = objOriginalPositions[go];

        // FIXME: need to get height scaling right (see in shader as well).
        float theight = 0.4f * GetComponent <Terrain> ().SampleHeight (pos ) + pos.y;
//         Debug.Log ("Placing " + go.name + " " + theight + " " + pos);
        float tx = (-tp.z + (pos.z / TerrainSize));
        float ty = (-tp.x + (pos.x / TerrainSize));
        float tlen = new Vector2 (tx, ty).magnitude;
        float trad = 0.0f;
        float theta = 0.0f;
        float phi = 0.0f;
        if (tlen > 0)
        {
          Vector2 sqrToCir = new Vector2 (tx, ty).normalized;
          if (Mathf.Abs (sqrToCir.x) > Mathf.Abs (sqrToCir.y))
          {
            sqrToCir = new Vector2 (sqrToCir.x / Mathf.Abs (sqrToCir.x), sqrToCir.y / Mathf.Abs (sqrToCir.x)); 
          }
          else
          {
            sqrToCir = new Vector2 (sqrToCir.x / Mathf.Abs (sqrToCir.y), sqrToCir.y / Mathf.Abs (sqrToCir.y)); 
          }
          trad = 1 / sqrToCir.magnitude;
          theta = Mathf.PI * tlen * trad;
          phi = Mathf.Atan2 (tx, ty);
        }          
        
//           float theta = 1.0f * Mathf.PI * (tp.x + x - 0.5f);
//           float phi = 1.0f * Mathf.PI * (tp.z + y - 0.5f);
        float r = 60.0f;
        Vector3 n = new Vector3 (Mathf.Sin (theta) * Mathf.Cos (phi), Mathf.Cos (theta), Mathf.Sin (theta) * Mathf.Sin (phi));
        Vector3 p = (r + theight) * (new Vector3 (Mathf.Sin (theta) * Mathf.Cos (phi), Mathf.Cos (theta), Mathf.Sin (theta) * Mathf.Sin (phi))) - (r * new Vector3 (0, 1, 0));
        go.transform.position = p;
        go.transform.up = n;
      }
    }
  }
  
//     void placeTrees ()
//     {
//       TreeInstance [] t = GetComponent <Terrain> ().terrainData.treeInstances;
//       
//       for (int i = 0; i < t.Length; i++)
//       {
//         TreeInstance tree = t[i];
// //        t[i].widthScale = 1.0f;
// //         t[i].position = t[i].position + new Vector3 (0.1f, 0, 0);
// //        GetComponent <Terrain> ().terrainData.SetTreeInstance (i, tree);
// //         Debug.Log ("Set " + tree + " " + tree.widthScale);
//       }
//       GetComponent <Terrain> ().terrainData.SetTreeInstances (t, false);
//     }
//     public Camera cam;
//     void OnPreCull()
//      {
//        Debug.Log (">per");
//          cam.cullingMatrix = Matrix4x4.Ortho(-99999, 99999, -99999, 99999, 0.001f, 99999) * 
//                              Matrix4x4.Translate(Vector3.forward * -99999 / 2f) * 
//                              cam.worldToCameraMatrix;
//      }
     
    void Start()
    {
      placeTrees ();
      
      findObjectOriginals ();
//       placeTrees ();    
      
//       for (int x = 0; x < 10; x++)
//       {
//         for (int y = 0; y < 10; y++)
//         {
//           float theta = 1.0f * Mathf.PI * (x - 5) / 10.0f;
//           float phi = 1.0f * Mathf.PI * (y - 5) / 10.0f;
//           float r = 60.0f;
//           Vector3 n = new Vector3 (Mathf.Sin (theta) * Mathf.Cos (phi), Mathf.Cos (theta), Mathf.Sin (theta) * Mathf.Sin (phi));
//           Vector3 p = r * (new Vector3 (Mathf.Sin (theta) * Mathf.Cos (phi), Mathf.Cos (theta), Mathf.Sin (theta) * Mathf.Sin (phi)) - new Vector3 (0, 1, 0));
//           GameObject g = Instantiate (treePrefab, p, Quaternion.identity);
//           g.transform.up = n;
//           Destroy (g, 0.05f);
//         }
//       }
      
    }

    void Update()
    {
      replaceObjects ();
  //    tp += new Vector4 (0.001f, 0.0f, 0.0023425f, 0.0f);
//       tp.x = tp.x % 1.0f;
//       tp.z = tp.z % 1.0f;
      
//       RenderTexture myRenderTexture = GetComponent <Terrain> ().terrainData.heightmapTexture;
//       Texture2D myTexture2D = new Texture2D (myRenderTexture.width, myRenderTexture.height, TextureFormat.RGB24, false);
//       RenderTexture.active = myRenderTexture;
//  myTexture2D.ReadPixels(new Rect(0, 0, myRenderTexture.width, myRenderTexture.height), 0, 0);
//  myTexture2D.Apply();
      planetMaterial.SetVector ("TerrainPosition", tp);
      planetMaterial.SetTexture ("TerrainHeightmapTexture", GetComponent <Terrain> ().terrainData.heightmapTexture, RenderTextureSubElement.Color);
    //  Debug.Log ("Pos: " + tp + " " + GetComponent <Terrain> ().terrainData.heightmapTexture + " " + GetComponent <Terrain> ().terrainData.heightmapResolution + " " + GetComponent <Terrain> ().terrainData.heightmapScale);// + " " + myTexture2D.GetPixel (13, 110));
      
//       for (float x = 0.0f; x < 1.0f; x += 0.1f)
//       {
//         for (float y = 0.0f; y < 1.0f; y += 0.1f)
//         {
//           float tx = (-tp.z + y - 0.5f);
//           float ty = (-tp.x + x - 0.5f);
//           float tlen = new Vector2 (tx, ty).magnitude;
//           float trad = 0.0f;
//           float theta = 0.0f;
//           float phi = 0.0f;
//           if (tlen > 0)
//           {
//             Vector2 sqrToCir = new Vector2 (tx, ty).normalized;
//             if (Mathf.Abs (sqrToCir.x) > Mathf.Abs (sqrToCir.y))
//             {
//               sqrToCir = new Vector2 (sqrToCir.x / Mathf.Abs (sqrToCir.x), sqrToCir.y / Mathf.Abs (sqrToCir.x)); 
//             }
//             else
//             {
//               sqrToCir = new Vector2 (sqrToCir.x / Mathf.Abs (sqrToCir.y), sqrToCir.y / Mathf.Abs (sqrToCir.y)); 
//             }
//             trad = 1 / sqrToCir.magnitude;
//             theta = Mathf.PI * tlen * trad;
//             phi = Mathf.Atan2 (tx, ty);
//           }          
//           
// //           float theta = 1.0f * Mathf.PI * (tp.x + x - 0.5f);
// //           float phi = 1.0f * Mathf.PI * (tp.z + y - 0.5f);
//           float r = 60.0f;
//           Vector3 n = new Vector3 (Mathf.Sin (theta) * Mathf.Cos (phi), Mathf.Cos (theta), Mathf.Sin (theta) * Mathf.Sin (phi));
//           Vector3 p = r * (new Vector3 (Mathf.Sin (theta) * Mathf.Cos (phi), Mathf.Cos (theta), Mathf.Sin (theta) * Mathf.Sin (phi)) - new Vector3 (0, 1, 0));
//           GameObject g = Instantiate (treePrefab, p, Quaternion.identity);
//           g.transform.up = n;
//           Destroy (g, 0.05f);
//         }
//       }
      
    }
}
