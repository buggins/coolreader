using System.Collections;
using System.Collections.Generic;
using UnityEngine;

/*
 * A facility for simulating head movement on a desktop platform.
 * 
 * Disable lines as follows in OVRCameraRig, to allow desktop use without oculus devices.
 * 	protected virtual void UpdateAnchors(bool updateEyeAnchors, bool updateHandAnchors)
	{
		//if (!OVRManager.OVRManagerinitialized)
		//	return;
                ...

 */
public class ViewKeyboardControl : MonoBehaviour {
  
  [Tooltip ("Angular speed of rotation.")]
  public float turnSpeed = 10.0f;
  
  // track current rotation about the vertical axis (yaw).
  private float turnAngle = 0.0f;
  // track current rotation about sideways axis (pitch).
  private float elevationAngle = 0.0f;
  
  void Start () {
    
  }
  
  void Update () {
    // Use unity input settings.
    float h = Input.GetAxis ("Horizontal");
    float v = Input.GetAxis ("Vertical");
    
    turnAngle += -h * Time.deltaTime * turnSpeed;
    elevationAngle += v * Time.deltaTime * turnSpeed;
    
    // Get desired orientation.
    Vector3 relori = (
      Quaternion.AngleAxis (turnAngle, Vector3.up) *
      Quaternion.AngleAxis (elevationAngle, Vector3.right)).eulerAngles;
    // Provide this via the Oculus manager, so components that employ
    // this information see no difference in the interface.
      
    if (SelectController.getActivePlatform () == SelectController.DeviceOptions.NoDevice)
    {
      if (GetComponent <OVRManager> () != null)
      {
        GetComponent <OVRManager> ().headPoseRelativeOffsetRotation = relori;
      }
    }
  }
}
