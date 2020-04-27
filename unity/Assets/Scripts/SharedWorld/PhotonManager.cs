using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Photon.Pun;
using Photon.Realtime;

public class PhotonManager : MonoBehaviourPunCallbacks
{
  
  public GameObject avatarPrefab;
  
  void Start()
  {
    Debug.Log ("Connected: " + PhotonNetwork.IsConnected);
    PhotonNetwork.ConnectUsingSettings();
  }
  
  public override void OnConnectedToMaster()
  {
    Debug.Log ("Connected to master.");
    RoomOptions roomopt = new RoomOptions ();
    PhotonNetwork.JoinOrCreateRoom ("ReaderRoom", roomopt, new TypedLobby ("ReaderLobby", LobbyType.Default));
  }
  
  public override void OnJoinedRoom()
  {
    Debug.Log ("Joined room with " + PhotonNetwork.CurrentRoom.PlayerCount + " participants.");
    
    PhotonNetwork.Instantiate (avatarPrefab.name, new Vector3 (), Quaternion.identity, 0);
  }
}
