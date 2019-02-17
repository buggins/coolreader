using System.Collections;
using System.Collections.Generic;
using System;
using UnityEngine;
using UnityEngine.XR;

/*
 * Enables/disables current object depending on whether a particular XR platform
 * is detected. This is intended to be attached to controller objects, so that
 * only the correct model for a given platform is visible.
 * 
 * This class also provides functions that require insight into different VR APIs,
 * such as detecting handedness.
 */
public class SelectController : MonoBehaviour {
  
  public enum DeviceOptions
  {
    NoDevice,
    OculusGo, 
    Daydream, 
  };
  public DeviceOptions device;
  
  public static DeviceOptions getActivePlatform ()
  {
    DeviceOptions detectedDevice = DeviceOptions.NoDevice;
    if (XRSettings.loadedDeviceName.Equals ("Oculus"))
    {
      detectedDevice = DeviceOptions.OculusGo;
    }
    if (XRSettings.loadedDeviceName.Equals ("daydream"))
    {
      detectedDevice = DeviceOptions.Daydream;
    }

    return detectedDevice;
  }
  
  public static bool isLeftHanded ()
  {
    bool result = true;
    switch (getActivePlatform ())
    {
      case DeviceOptions.OculusGo:
        result = OVRInput.IsControllerConnected (OVRInput.Controller.LTrackedRemote);
        break;
      case DeviceOptions.Daydream:
        result = GvrSettings.Handedness == GvrSettings.UserPrefsHandedness.Left;
        break;
    }
    return result;
  }
  
  void Start () {
//     Debug.Log ("Running on " + XRSettings.isDeviceActive + "-" + XRSettings.loadedDeviceName + " ");
    
    gameObject.SetActive (device == getActivePlatform ());
  }
}
