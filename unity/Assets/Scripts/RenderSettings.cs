using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.XR;

public class RenderSettings : MonoBehaviour {

	// Use this for initialization
	void Start () {
		XRSettings.eyeTextureResolutionScale = 2.0f;
 OVRManager.display.displayFrequency = 72.0f;
	}
	
	// Update is called once per frame
	void Update () {
		
	}
}
