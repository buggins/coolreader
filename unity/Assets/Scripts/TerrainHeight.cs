using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class TerrainHeight : MonoBehaviour {

//        public Terrain terrain;
        
        public float offsetHeight = 0.0f;
        
	// Update is called once per frame
	void Update () {
          Terrain [] terrains = Terrain.activeTerrains;
          Terrain terrain = null;
          float mindist = 0.0f;
          foreach (Terrain t in terrains)
          {
            Vector3 tpos = t.GetPosition () + new Vector3(t.terrainData.size.x / 2, 0, t.terrainData.size.z / 2);
            float sdis = (tpos - transform.position).sqrMagnitude;
            if ((terrain == null) || (sdis < mindist))
            {
              mindist = sdis;
              terrain = t;
            }
          }
//           Debug.Log ("Terr " + terrain);
          if (Terrain.activeTerrain != null)
          {
            float y = terrain.SampleHeight (transform.position);
//           Debug.Log ("Terr " + y + " " + terrain.SampleHeight (transform.position));
            
            if (transform.position.y < y + offsetHeight)
            {
              transform.position = new Vector3 (transform.position.x, y + offsetHeight, transform.position.z);
            }
          }
	}
}
