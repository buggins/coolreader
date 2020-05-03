using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class PlanetGenerator : MonoBehaviour
{
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
      buildGrid (100, 100);   
    }

    void Update()
    {
        
    }
}
