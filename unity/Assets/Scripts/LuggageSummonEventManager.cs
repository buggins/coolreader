using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Events;

[System.Serializable]
public class LuggageSummonEvent : UnityEvent <GameObject>
{
}

/// A singleton class that allows luggage summon events
/// to be shared between beacons and trolleys.
public class LuggageSummonEventManager : MonoBehaviour
{
  static LuggageSummonEvent luggageSummon = new LuggageSummonEvent ();
  
  public static void summonLuggage (GameObject target)
  {
    luggageSummon.Invoke (target);
  }
  
  public static void listenSummonLuggage (UnityAction <GameObject> eHandler)
  {
    luggageSummon.AddListener (eHandler);
  }
}
