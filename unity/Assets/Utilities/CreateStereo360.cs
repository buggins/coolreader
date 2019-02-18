using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering;

public class CreateStereo360 : MonoBehaviour {

        public Camera cam;
        public RenderTexture cubemapLeft;
        public RenderTexture cubemapRight;
        public RenderTexture equirect;
        public bool renderStereo = true;

        void saveStereo360Image (string filename)
        {
 
          cam.RenderToCubemap(cubemapLeft, 63, Camera.MonoOrStereoscopicEye.Left);
          cam.RenderToCubemap(cubemapRight, 63, Camera.MonoOrStereoscopicEye.Right);
           
          if (equirect == null)
              return;

          if (renderStereo)
          {
              cubemapLeft.ConvertToEquirect(equirect, Camera.MonoOrStereoscopicEye.Left);
              cubemapRight.ConvertToEquirect(equirect, Camera.MonoOrStereoscopicEye.Right);
          }
          else
          {
              cubemapLeft.ConvertToEquirect(equirect, Camera.MonoOrStereoscopicEye.Mono);
          }
     
          Texture2D image = new Texture2D (equirect.width, equirect.height, TextureFormat.RGB24, false);
          image.ReadPixels (new Rect(0, 0, equirect.width, equirect.height), 0, 0);
          byte[] bytes;
          bytes = image.EncodeToPNG ();
     
          System.IO.File.WriteAllBytes (filename, bytes);
        }
  
	void Update () {
		
          if (Input.GetKey (KeyCode.F2))
          {
            saveStereo360Image ("/tmp/render.png");
          }
	}
}
