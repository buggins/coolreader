using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;
using Photon.Pun;

// Apply this to the terrain object.
public class LayoutPlanet : MonoBehaviour
{
  public Material planetMaterial;
  
  public GameObject treePrefab;
  public GameObject teleportPrefab;
  
  public float TerrainSize = 100.0f;
  
  public float planetRadius = 60.0f;
  
  public TerrainData terraind;
  
  public Vector4 tp;
  public Vector4 tf;

  private System.Random rand;
  
  private Dictionary <GameObject, Vector3> objOriginalPositions;
  
  private void placeObjects (GameObject prefab, float step = 0.1f)
  {
//    float step = 0.1f;
    for (float x = 0.0f; x < 2.0f; x += step)
      {
        for (float y = 0.0f; y < 3.0f; y += step)
        {
          float dx = 0.5f * step * ((float) rand.NextDouble () - 0.5f);
          float dy = 0.5f * step * ((float) rand.NextDouble () - 0.5f);

//dx = dy = 0;          
          
          Vector3 p = new Vector3 ((dx + x - 0.5f) * TerrainSize, 0.0f, (dy + y - 0.5f) * TerrainSize);
          GameObject g = Instantiate (prefab, p, Quaternion.identity);
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
  
  Vector3 sphere2cube (Vector3 s, out Vector2 uv)
  {
    Vector3 r = new Vector3 (0, 0, 0);
    uv = new Vector2 (0, 0);
    if ((Mathf.Abs (s.x) > Mathf.Abs (s.y)) && (Mathf.Abs (s.x) > Mathf.Abs (s.z)))
    {
      r = new Vector3 (Mathf.Sign (s.x), Mathf.Atan2 (s.y, Mathf.Abs (s.x)) / (Mathf.PI / 4.0f), Mathf.Atan2 (s.z, Mathf.Abs (s.x)) / (Mathf.PI / 4.0f));
      uv = 0.5f * new Vector2 (r.y, r.z) + new Vector2 (0.5f, 0.5f);
    }
    else 
    {
      if ((Mathf.Abs (s.y) > Mathf.Abs (s.x)) && ((Mathf.Abs (s.y) > Mathf.Abs (s.z))))
      {
        r = new Vector3 (Mathf.Atan2 (s.x, Mathf.Abs (s.y)) / (Mathf.PI / 4.0f), Mathf.Sign (s.y), Mathf.Atan2 (s.z, Mathf.Abs (s.y)) / (Mathf.PI / 4.0f));
        uv = 0.5f * new Vector2 (r.z, r.x) + new Vector2 (0.5f, 0.5f);
      }
      else
      {
        r = new Vector3 (Mathf.Atan2 (s.x, Mathf.Abs (s.z)) / (Mathf.PI / 4.0f), Mathf.Atan2 (s.y, Mathf.Abs (s.z)) / (Mathf.PI / 4.0f), Mathf.Sign (s.z));
        uv = 0.5f * new Vector2 (r.x, r.y) + new Vector2 (0.5f, 0.5f);
      }
    }
    return r;
  }

  Vector3 cube2sphere (Vector2 uv, int face)
  {
    Vector3 s = new Vector3 (0, 0, 0);
    
    float a = Mathf.Sin ((Mathf.PI / 4.0f) * 2.0f * (uv.x - 0.5f));
    float b = Mathf.Sin ((Mathf.PI / 4.0f) * 2.0f * (uv.y - 0.5f));
    float c = Mathf.Sqrt (1.0f - ((a * a) + (b * b)));

    switch (face)
    {
      case 2: s = new Vector3 (c, a, b); break;
      case 3: s = new Vector3 (-c, a, b); break;
      case 0: s = new Vector3 (b, c, a); break;
      case 1: s = new Vector3 (b, -c, a); break;
      case 4: s = new Vector3 (a, b, c); break;
      case 5: s = new Vector3 (a, b, -c); break;
    }
    return s;   
  }
  
  private float getHeightAtCube (Vector2 faceCoord, int face)
  {
    // Todo: adjust to select appropriate terrain if not all using the same on all faces.
    Debug.Log ("Scale " + terraind.heightmapScale);
    int px = (int) ((faceCoord.x * TerrainSize) / terraind.heightmapScale.x);
    int py = (int) ((faceCoord.y * TerrainSize) / terraind.heightmapScale.z);
    float theight = 0.5f * terraind.GetHeight (px, py);
    return theight;
  }
  
  private float getHeightAtSphere (Vector3 sphereCoords)
  {
    return 0.0f;
  }
  
  private void replaceObjects ()
  {
    GameObject [] g = GameObject.FindGameObjectsWithTag ("PlanetSurfaceLocked");
    foreach (GameObject go in g)
    {
      if (objOriginalPositions.ContainsKey (go))
      {
        Vector3 pos = objOriginalPositions[go];

        Vector2 uv = new Vector2 (Mathf.Repeat ((pos.x / TerrainSize) + 0.5f, 1.0f), Mathf.Repeat ((pos.z / TerrainSize) + 0.5f, 1.0f));
        int face = (((int) ((pos.x / TerrainSize) + 0.5f)) % 2) + 2 * (((int) ((pos.z / TerrainSize) + 0.5f)) % 3);
        Vector3 n = cube2sphere (uv, face);
        float h = getHeightAtCube (uv, face) + pos.y;
        Vector3 p = ((planetRadius + h) * n) - (planetRadius * new Vector3 (0, 1, 0));
        
//         // FIXME: need to get height scaling right (see in shader as well).
// //        float theight = 0.4f * GetComponent <Terrain> ().SampleHeight (pos ) + pos.y;
//         float theight = 0.5f * terraind.GetHeight ((int) ((pos.x + 50) / terraind.heightmapScale.x), (int) ((pos.z + 50) / terraind.heightmapScale.z)) + pos.y;
// //      Debug.Log ("Placing " + go.name + " " + theight + " " + pos + " " + terraind.heightmapScale + " " + terraind.heightmapResolution + " " + terraind.bounds);
// //         float tx = (-tp.z + (pos.z / TerrainSize));
// //         float ty = (-tp.x + (pos.x / TerrainSize));
//         float tx = ((pos.z / TerrainSize));
//         float ty = ((pos.x / TerrainSize));
//         float tlen = new Vector2 (tx, ty).magnitude;
//         float trad = 0.0f;
//         float theta = 0.0f;
//         float phi = 0.0f;
//         if (tlen > 0)
//         {
//           Vector2 sqrToCir = new Vector2 (tx, ty).normalized;
//           if (Mathf.Abs (sqrToCir.x) > Mathf.Abs (sqrToCir.y))
//           {
//             sqrToCir = new Vector2 (sqrToCir.x / Mathf.Abs (sqrToCir.x), sqrToCir.y / Mathf.Abs (sqrToCir.x)); 
//           }
//           else
//           {
//             sqrToCir = new Vector2 (sqrToCir.x / Mathf.Abs (sqrToCir.y), sqrToCir.y / Mathf.Abs (sqrToCir.y)); 
//           }
//           trad = 1 / sqrToCir.magnitude;
//           theta = Mathf.PI * tlen * trad;
//           phi = Mathf.Atan2 (tx, ty);
//         }          
//         
// //           float theta = 1.0f * Mathf.PI * (tp.x + x - 0.5f);
// //           float phi = 1.0f * Mathf.PI * (tp.z + y - 0.5f);
//         Vector3 n = new Vector3 (Mathf.Sin (theta) * Mathf.Cos (phi), Mathf.Cos (theta), Mathf.Sin (theta) * Mathf.Sin (phi));
//         Vector3 p = (planetRadius + theight) * (new Vector3 (Mathf.Sin (theta) * Mathf.Cos (phi), Mathf.Cos (theta), Mathf.Sin (theta) * Mathf.Sin (phi))) - (planetRadius * new Vector3 (0, 1, 0));
//         
// //        n = new Vector3 (0,1,0);
// //        p = pos + new Vector3 (0,theight,0);
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
    private void buildGrid (int mwidth, int mheight)
    {
        Vector3[] vertices = new Vector3[(mwidth + 1) * (mheight + 1)];
        Vector2[] uvs = new Vector2[(mwidth + 1) * (mheight + 1)];
        int[] triangles = new int[6 * mwidth * mheight];

        int triangleIndex = 0;
        for (int y = 0; y < mheight + 1; y++)
        {
            for (int x = 0; x < mwidth + 1; x++)
            {
                float xc = (float)x / mwidth;
                float zc = (float)y / mheight;
                float yc = 0.0f;
                vertices[y * (mwidth + 1) + x] = new Vector3(xc - 0.5f, yc, zc - 0.5f);
                uvs[y * (mwidth + 1) + x] = new Vector2(xc, zc);

                // Skip the last row/col
                if ((x != mwidth) && (y != mheight))
                {
                    int topLeft = y * (mwidth + 1) + x;
                    int topRight = topLeft + 1;
                    int bottomLeft = topLeft + mwidth + 1;
                    int bottomRight = bottomLeft + 1;

                    triangles[triangleIndex++] = topRight;
                    triangles[triangleIndex++] = topLeft;
                    triangles[triangleIndex++] = bottomLeft;
                    triangles[triangleIndex++] = topRight;
                    triangles[triangleIndex++] = bottomLeft;
                    triangles[triangleIndex++] = bottomRight;
                }
            }
        }

        Mesh m = new Mesh();
        m.vertices = vertices;
        m.uv = uvs;
        m.triangles = triangles;
        m.RecalculateNormals();
        // Set mesh to size of planet, to avoid frustum culling.
        m.bounds = new Bounds (new Vector3 (0, 0, 0), new Vector3 (1000, 1000, 1000));
        GetComponent <MeshFilter> ().mesh = m;
    }
     
    void Start()
    {
      rand = new System.Random (423);
      
      buildGrid (100, 100);
      
      placeObjects (treePrefab, 0.3f);
      placeObjects (teleportPrefab, 0.1f);
      
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

      replaceObjects ();
      tp = new Vector4 (0, 1, 0, 0);
      tf = new Vector4 (0, 0, 1, 0);
    }

    void Update()
    {
      
      if ((PhotonNetwork.LocalPlayer.TagObject != null) && 
          (Vector3.Distance (((GameObject) PhotonNetwork.LocalPlayer.TagObject).transform.position, new Vector3 ()) > 0.5f))
      {
        Vector3 ppos = (((GameObject) PhotonNetwork.LocalPlayer.TagObject).transform.position + new Vector3 (0, planetRadius, 0)).normalized;        
        Vector3 pfor = ((GameObject) PhotonNetwork.LocalPlayer.TagObject).transform.forward;
        tp = ppos;
        tf = pfor;
        Debug.Log ("Current player: " + PhotonNetwork.LocalPlayer.TagObject);
      }
//      //   ((GameObject) PhotonNetwork.LocalPlayer.TagObject).transform.position = new Vector3 ();
//         
//         Vector3 ppos = (((GameObject) PhotonNetwork.LocalPlayer.TagObject).transform.position + new Vector3 (0, planetRadius, 0)).normalized;
//         tp.z = Mathf.Acos (ppos.y) / Mathf.PI;
//         if ((ppos.x != 0.0f) || (ppos.z != 0.0f))
//         {
//           tp.x = Mathf.Atan2 (ppos.x, ppos.z) / (2.0f * Mathf.PI);
//         }
//         else
//         {
//           tp.x = 0.0f;
//         }
//         Debug.Log ("At " + tp + " " + ppos);
// //         tp.x = ;
// //         tp.z = 0.5f;
//       }
      
//      replaceObjects ();
  //    tp += new Vector4 (0.001f, 0.0f, 0.0023425f, 0.0f);
//       tp.x = tp.x % 1.0f;
//       tp.z = tp.z % 1.0f;
      
//       RenderTexture myRenderTexture = GetComponent <Terrain> ().terrainData.heightmapTexture;
//       Texture2D myTexture2D = new Texture2D (myRenderTexture.width, myRenderTexture.height, TextureFormat.RGB24, false);
//       RenderTexture.active = myRenderTexture;
//  myTexture2D.ReadPixels(new Rect(0, 0, myRenderTexture.width, myRenderTexture.height), 0, 0);
//  myTexture2D.Apply();
      planetMaterial.SetVector ("TerrainPosition", tp);
      planetMaterial.SetVector ("TerrainForward", tf);
      planetMaterial.SetTexture ("TerrainHeightmapTexture", terraind.heightmapTexture, RenderTextureSubElement.Color);
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
