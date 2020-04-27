using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Photon.Pun;

[RequireComponent (typeof (PhotonView))]
public class SharedAvatarView : MonoBehaviour, IPunObservable
{
  private PhotonView pView;
  
  private bool init = false;
  private Vector3 netAvatarPosition;
  private float netAvatarDistance = 0.0f;
  private Quaternion netAvatarOrientation;
  private float netAvatarAngle = 0.0f;
  private Quaternion netHeadOrientation;
  private float netHeadAngle = 0.0f;
  
  private Vector3 leftControllerPosition;
  private float leftControllerDistance = 0.0f;
  private Quaternion leftControllerOrientation;
  private float leftControllerAngle = 0.0f;
  private SelectController.DeviceOptions leftControllerType = SelectController.DeviceOptions.NoDevice;
  
  private Vector3 rightControllerPosition;
  private float rightControllerDistance = 0.0f;
  private Quaternion rightControllerOrientation;
  private float rightControllerAngle = 0.0f;
  private SelectController.DeviceOptions rightControllerType = SelectController.DeviceOptions.NoDevice;
  
  [Tooltip ("The root of the avatar. Uses position and orientation.")]
  public Transform avatarBase;
  [Tooltip ("The visible head of the avatar")]
  public Transform headObject;
  [Tooltip ("The tracked head camera object of the avatar")]
  public Transform headCameraObject;
  
  [Tooltip ("Left controller")]
  public GameObject leftController;
  [Tooltip ("Right controller")]
  public GameObject rightController;
  
  
  void Awake()
  {
    pView = GetComponent <PhotonView> ();
  }
  
  // Update is called once per frame
  void Update()
  {
    if (!pView.IsMine)
    {
      avatarBase.position = Vector3.MoveTowards (avatarBase.position, netAvatarPosition, netAvatarDistance * (1.0f /PhotonNetwork.SerializationRate));
      avatarBase.rotation = Quaternion.RotateTowards (avatarBase.rotation, netAvatarOrientation, netAvatarAngle * (1.0f / PhotonNetwork.SerializationRate));
      headObject.rotation = Quaternion.RotateTowards (headObject.rotation, netHeadOrientation, netHeadAngle * (1.0f / PhotonNetwork.SerializationRate));

      leftController.transform.position = Vector3.MoveTowards (leftController.transform.position, leftControllerPosition, leftControllerDistance * (1.0f /PhotonNetwork.SerializationRate));
      leftController.transform.rotation = Quaternion.RotateTowards (leftController.transform.rotation, leftControllerOrientation, leftControllerAngle * (1.0f / PhotonNetwork.SerializationRate));
      
      rightController.transform.position = Vector3.MoveTowards (rightController.transform.position, rightControllerPosition, rightControllerDistance * (1.0f /PhotonNetwork.SerializationRate));
      rightController.transform.rotation = Quaternion.RotateTowards (rightController.transform.rotation, rightControllerOrientation, rightControllerAngle * (1.0f / PhotonNetwork.SerializationRate));
      
      
    }  
  }
  
  public void OnPhotonSerializeView (PhotonStream stream, PhotonMessageInfo info)
  {
    if (stream.IsWriting)
    {
      // send data
      stream.SendNext (avatarBase.position);
      stream.SendNext (avatarBase.rotation);
      stream.SendNext (headCameraObject.rotation);
      
      stream.SendNext (leftController.activeSelf);      
      stream.SendNext (SelectController.getActivePlatform ());      
      stream.SendNext (leftController.transform.position);
      stream.SendNext (leftController.transform.rotation);
      
      stream.SendNext (rightController.activeSelf);      
      stream.SendNext (SelectController.getActivePlatform ());      
      stream.SendNext (rightController.transform.position);
      stream.SendNext (rightController.transform.rotation);
      
    }
    else
    {
      // receieve data
      netAvatarPosition = (Vector3) stream.ReceiveNext();
      netAvatarOrientation = (Quaternion) stream.ReceiveNext();
      netHeadOrientation = (Quaternion) stream.ReceiveNext();

      bool leftActive = (bool) stream.ReceiveNext();
      leftController.SetActive (leftActive);
      leftControllerType = (SelectController.DeviceOptions) stream.ReceiveNext();
      leftControllerPosition = (Vector3) stream.ReceiveNext();
      leftControllerOrientation = (Quaternion) stream.ReceiveNext();
      
      bool rightActive = (bool) stream.ReceiveNext();
      rightController.SetActive (rightActive);
      rightControllerType = (SelectController.DeviceOptions) stream.ReceiveNext();
      rightControllerPosition = (Vector3) stream.ReceiveNext();
      rightControllerOrientation = (Quaternion) stream.ReceiveNext();
      
      // set up interpolation values. 
      if (!init)
      {
        netAvatarDistance = 0.0f;
        netAvatarAngle = 0.0f;
        netHeadAngle = 0.0f;
        leftControllerDistance = 0.0f;
        leftControllerAngle = 0.0f;
        rightControllerDistance = 0.0f;
        rightControllerAngle = 0.0f;
        init = true;
      }
      else
      {
        netAvatarDistance = Vector3.Distance (avatarBase.position, netAvatarPosition);
        netAvatarAngle = Quaternion.Angle (avatarBase.rotation, netAvatarOrientation);
        netHeadAngle = Quaternion.Angle (headObject.rotation, netHeadOrientation);
        leftControllerDistance = Vector3.Distance (leftController.transform.position, leftControllerPosition);
        leftControllerAngle = Quaternion.Angle (leftController.transform.rotation, leftControllerOrientation);
        rightControllerDistance = Vector3.Distance (rightController.transform.position, rightControllerPosition);
        rightControllerAngle = Quaternion.Angle (rightController.transform.rotation, rightControllerOrientation);
      }
    }
  }
}
