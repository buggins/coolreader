using System.Collections;
using System.Collections.Generic;
using UnityEngine;

/*
 * A book worm, to add some interest to the scene. Initial version will clean up any 
 * dropped books.
 * 
 * The intent is to have an inch worm type of movement, procedurally managed.
 */
public class Caterpillar : MonoBehaviour {

  [Tooltip ("The template for a segment of the caterpillar")]
  public GameObject segmentTemplate;

  // Speed of movement.
  private float speed = 1.0f;
  
  // The maximum expansion length, between front and back "feet"
  private float maxLength = 0.3f;
  // The height of the hump of the caterpillar.
  private float maxHeight = 0.15f;
  // The minimum inter-"foot" distance.
  private float minGap = 0.05f;
  // How many segments to use between feet. This also affects the number
  // of segments used for neck and tail.
  private int numSegments = 15;
  // An internal counter for the number of segments used in neck and tail.
  private int overshoot;

  // A list of each segment, so the position of these can be procedurally manipulated.
  private List<GameObject> segments;

  // Position of the back "foot"
  private Vector3 tailPosition;
  // Direction of movement.
  private Vector3 bodyDirection;
  
  // Internal state, to keep track of change from extending to compressing.
  private float lastphase = 0.0f;
  
  void Start () {
    // Head and tail length use the same number of segments in total as the body.
    overshoot = numSegments / 2;
    
    // Create the individual body segments.
    segments = new List<GameObject> ();
    for (int i = 0; i < numSegments + 2 * overshoot; i++)
    {
      segments.Add (Instantiate (segmentTemplate));
      // Some nice, random, pastel colours.
      segments[i].GetComponent <MeshRenderer> ().material.color = new Color (0.5f + 0.5f * Random.value, 0.5f + 0.5f * Random.value, 0.5f + 0.5f * Random.value);
      segments[i].transform.SetParent (transform);
    }
    
    // Pick a random starting point.
    tailPosition = new Vector3 (Random.Range (-5.0f, 5.0f), 0, Random.Range (-5.0f, 5.0f));
  //   tailPosition = new Vector3 (-2, 1, 2);
    // And a random direction for travel.
    bodyDirection = new Vector3 (Random.Range (-1.0f, 1.0f), 0, Random.Range (-1.0f, 1.0f));
  //   bodyDirection = new Vector3 (1, 0, 0);
  }
  
  void Update () {
    
    // The inch worm animation. The body needs to bulge as the front and back feet
    // come together, to give the impression of conserving length.
    
    // Pick a value between 0 and 1 that varies over time. This is the front-back distance.
    float animationFactor = Mathf.Abs (Mathf.Sin (Time.time * speed));
    // Another value to work out whether we're expanding or contracting.
    float phaseFactor = Mathf.Sin (2.0f * Time.time * speed);

    // Anchor one foot, or the other. Front when contracting, back when expanding.
    if ((phaseFactor < 0) && (lastphase > 0))
    {
      tailPosition += maxLength * bodyDirection;
    }
    if ((phaseFactor > 0) && (lastphase < 0))
    {
      // Set direction. Random, unless there is a juicy book to eat.
      tailPosition -= minGap * bodyDirection;
      bool wander = true;            
      foreach (GameObject go in         UnityEngine.SceneManagement.SceneManager.GetActiveScene().GetRootGameObjects())
      {
        if (go.GetComponent <BookMenuInteraction> () != null)
        {
          if ((go.transform.position - tailPosition).magnitude < 1.0f)
          { 
            Destroy (go);
          }
                          
          else if (go.GetComponent <BookMenuInteraction> ().isDropped ())
          {
            bodyDirection = Vector3.Normalize (go.transform.position - tailPosition);
            bodyDirection = new Vector3 (bodyDirection.x, 0, bodyDirection.z);
            wander = false;
            break;
          }
        }
      }
      
      if (wander)
      {
        if (Random.value > 0.9f)
        {
          bodyDirection = new Vector3 (Random.Range (-1.0f, 1.0f), 0, Random.Range (-1.0f, 1.0f));
        }
        if (bodyDirection.magnitude < 0.01f)
        {
          bodyDirection = Vector3.forward;
        }
        
        // FIXME: currently restricted to a finite region. In future, sense obstacles and 
        // walk around them.
        if (tailPosition.x < -5.0f) bodyDirection = new Vector3 (Mathf.Abs (bodyDirection.x), bodyDirection.y, bodyDirection.z);
        if (tailPosition.x > 5.0f) bodyDirection = new Vector3 (-Mathf.Abs (bodyDirection.x), bodyDirection.y, bodyDirection.z);
        if (tailPosition.z < -5.0f) bodyDirection = new Vector3 (bodyDirection.x, bodyDirection.y, Mathf.Abs (bodyDirection.z));
        if (tailPosition.z > 5.0f) bodyDirection = new Vector3 (bodyDirection.x, bodyDirection.y, -Mathf.Abs (bodyDirection.z));
      }
    }
    
    // Compute the shape of the worm - a distorted sine curve.
    lastphase = phaseFactor;
    for (int i = 0; i < numSegments + 2 * overshoot; i++)
    {
      float t = i - overshoot;
      float headtailgap = (maxLength - minGap) * (animationFactor) + minGap;
      float offset = ((float) t / (numSegments - 1));
      float heighti = maxHeight * Mathf.Abs (Mathf.Sin (offset * Mathf.PI));
      float hoffset = offset * headtailgap;

      // inch worm shaping; distort sin curve according to height.
      hoffset += 4.0f * (maxLength - headtailgap) * heighti * ((float) (t - numSegments / 2) / (numSegments - 1));
      
      if (phaseFactor > 0)
      {
        segments[i].transform.position = tailPosition + new Vector3 (hoffset * bodyDirection.x, heighti, hoffset * bodyDirection.z);
      }
      else
      {
        segments[i].transform.position = tailPosition - headtailgap * bodyDirection + new Vector3 (hoffset * bodyDirection.x, heighti, hoffset * bodyDirection.z);
      }
    }
  }
}
