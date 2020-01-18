using System.Collections;
using System.Collections.Generic;
using UnityEngine;

// Render a large complex procedural texture in stages.
// Cunningly uses just one texture, with no copying by
// moving the camera over a plane containing the entire
// flattened texture, and controlling viewports to
// write to only one portion at a time.
public class PartialSceneRender : MonoBehaviour
{
    [Tooltip ("The render texture containing the rendered result")]
    public RenderTexture full;
    
    [Tooltip ("The material containing the procedural shader. Needs to have controls allowing region specific rendering")]
    public Material partialTarget;
    
    // Current region being rendered.
    private int rx;
    private int ry;
    private int rw;
    private int rh;
    
    // Also allows update of procedural texture, after each complete scan.
    private float pathtime;
    
    void Start()
    {
      rx = 0;
      ry = 0;
      rw = 64;
      rh = 64;
      
      pathtime = 0.0f;
      
      partialTarget.SetFloat ("UXWidth", (float) rw / (float) full.width);
      partialTarget.SetFloat ("UYHeight", (float) rh / (float) full.height);
    }

    public void OnPostRender()
    {
      // Advance to next region.
      rx += rw;
      if (rx >= full.width)
      {
        rx = 0;
        ry += rh;
        if (ry >= full.height)
        {
          ry = 0;
          
          pathtime += 0.1f;
          partialTarget.SetFloat ("PathTime", pathtime);
        }
      }

      // Aim camera at this region.
      GetComponent <Camera> ().rect = new Rect (((float) rx) / (float) full.width, ((float) ry) / (float) full.height, (float) rw / (float) full.width, (float) rh / (float) full.height);
      partialTarget.SetFloat ("UXStart", (float) rx / (float) full.width);
      partialTarget.SetFloat ("UYStart", (float) ry / (float) full.height);      
    }
}

