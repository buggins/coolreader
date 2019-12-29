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
    OculusQuest,
    Daydream, 
  };
  public DeviceOptions device;
  
  public static DeviceOptions getActivePlatform ()
  {
    DeviceOptions detectedDevice = DeviceOptions.NoDevice;
    if (XRSettings.loadedDeviceName.Equals ("Oculus"))
    {
      // Based on OVRControllerHelper.cs. See this file for other classes of oculus device.
      OVRPlugin.SystemHeadset headset = OVRPlugin.GetSystemHeadsetType();
      switch (headset)
      {
        case OVRPlugin.SystemHeadset.Oculus_Go:
          detectedDevice = DeviceOptions.OculusGo;
        break;
        case OVRPlugin.SystemHeadset.Oculus_Quest:
          detectedDevice = DeviceOptions.OculusQuest;
        break;
      }
    }
    if (XRSettings.loadedDeviceName.Equals ("daydream"))
    {
      detectedDevice = DeviceOptions.Daydream;
    }

    return detectedDevice;
  }
  
  // returns true if platform should only allow one controller to be active
  // at a time.
  public static bool singleController ()
  {
    bool result = true;
    switch (getActivePlatform ())
    {
      case DeviceOptions.OculusQuest:
        result = false;
        break;
    }
    return result;
  }
  
  public static bool isLeftHanded ()
  {
    bool result = true;
    switch (getActivePlatform ())
    {
      case DeviceOptions.OculusGo:
        result = (OVRInput.GetDominantHand () == OVRInput.Handedness.LeftHanded);
        break;
      case DeviceOptions.Daydream:
        result = (GvrSettings.Handedness == GvrSettings.UserPrefsHandedness.Left);
        break;
    }
    return result;
  }
  
  public void checkStatus ()
  {
    gameObject.SetActive (device == getActivePlatform ());
    Debug.Log ("Setting " + device + " " + getActivePlatform () + " " + (device == getActivePlatform ()));
  }
  
  void Start () {
//     Debug.Log ("Running on " + XRSettings.isDeviceActive + "-" + XRSettings.loadedDeviceName + " ");
    checkStatus ();
  }
}
