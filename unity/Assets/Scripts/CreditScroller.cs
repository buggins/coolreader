using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.IO;
using System;

public class CreditScroller : MonoBehaviour {

        public string creditsFile;
        
        public TextMesh text;
  
        private string message = null;
        
        public int charHorizontal = 30;
        public int charVertical = 10;
        
        public float characterSize = 1.0f;
        public float scrollSpeed = 0.1f;
        
        private List<string> lines;
        
        private int offset = 0;
        private float suboffset = 0.0f;
        
        private Vector3 textBasePosition;
        
	// Use this for initialization
	void Start () {
          textBasePosition = text.transform.position;
          
          lines = new List<string> ();
          try
          {
            print ("Reading: " + Application.persistentDataPath + "/" + creditsFile);
            message = File.ReadAllText (Application.persistentDataPath + "/" + creditsFile);
            print ("Credits: " + message);
          }
          catch (Exception)
          {
          }
          finally
          {
          }
          
          if (message != null)
          {
            while (message.Length > 0)
            {
              int wspace = message.LastIndexOf (" ", Mathf.Min (charHorizontal, message.Length));
              int nspace = message.IndexOf ("\n");
              if (nspace >= 0)
              {
                wspace = Mathf.Min (wspace, nspace);
              }
              int afterspace = wspace + 1;
              if (wspace < 0)
              {
                // no whitespace, force break.
                wspace = Mathf.Min (charHorizontal, message.Length);
                afterspace = wspace; // no whitespace gap.
              }
              lines.Add (message.Substring (0, wspace));
              message = message.Substring (afterspace);
//               print ("A : " + wspace + " " + lines[lines.Count - 1] + "|||" + message );
            }
          }
	}
	
	// Update is called once per frame
	void Update () {
          if (lines.Count > 0)
          {
            suboffset += scrollSpeed * Time.deltaTime;
            if (suboffset > characterSize)
            {
              offset += 1;
              suboffset -= characterSize;
            }

            string message = "";
            for (int i = 0; i < charVertical; i++)
            {
              message += lines[(i + offset) % lines.Count] + "\n";
            }
            
            text.text = message;
            
            text.transform.position = textBasePosition + new Vector3 (0, suboffset, 0);
          }
	}
}
