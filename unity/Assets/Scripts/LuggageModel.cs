using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

// Derived from:
//
//  Chantelle Morkel and Shaun Bangay. Procedural modeling facilities for hierarchical object generation. In Afrigraph '06: Proceedings of the 4th international conference on Computer graphics, virtual reality, visualisation and interaction in Africa, pages 145-154, Cape Town, South Africa, January 2006. ACM Press, New York, NY, USA.
//
// and
//
// Shaun Bangay and Chantelle Morkel. Graph matching with subdivision surfaces for texture synthesis on surfaces. In Afrigraph '06: Proceedings of the 4th international conference on Computer graphics, virtual reality, visualisation and interaction in Africa, pages 65-74, Cape Town, South Africa, January 2006. ACM Press, New York, NY, USA.
public class LuggageModel : MonoBehaviour
{
  
  public GameObject luggageObject;
  
  public GameObject elementTemplate;
  
  void Start()
  {
     Model model = Luggage (3.0f, 3.0f, 5.0f, numLegs: 20, legRows : 4, divBy : 30.0f);
    //   Model cubec = cube (1.0f, "cube");
    //   FaceSet luggageSet = new FaceSet ("luggage", cubec.mesh.faces);  
    //     
    //   cubec.mesh.addFaceSet (luggageSet);
    //   FaceSet foot2 = cubec.mesh.select (0.0f, 1.0f, 0.0f, 0.01f, 0.0f, 1.0f, luggageSet, "rightfoot"); 
    //   Model model = foot (2.0f, cubec, "leftfoot1", foot2);
//     Model cubec = cube (1.0f, "cube");
//     FaceSet luggageSet = new FaceSet ("luggage", cubec.mesh.faces);          
//     cubec.mesh.addFaceSet (luggageSet);
//     FaceSet t = cubec.mesh.select (0.0f, 1.0f, 0.0f, 0.01f, 0.0f, 1.0f, luggageSet, "rightfoot"); 
//     cubec.mesh.addFaceSet (t);
//    Model model = Leg (cubec, numToes : 3, threshold : 2, height : 1.0f, length : 0.5f, width : 1.0f, heads : 1.0f, name : "rightfoot");
//    Model model = Foot (cubec, numToes : 5, threshold : 5, needed : 1, bigToe : 0.4f, extrudeUnit : 1.0f*(3.0f/3.0f), name = "rightfoot");
//     Model model = ConstrFoot (cubec, numToes : 5, threshold : 5, bigToe : 0.4f, extrudeUnit : 1.0f*(3.0f/3.0f), name = "rightfoot");
    
    model.updateMesh (luggageObject, elementTemplate);
    Debug.Log ("Created luggage " + model.mesh.vertices.Count);
  }
  
  
  Model Luggage (float height = 2.0f, float width = 1.0f, float length = 2.0f, int numLegs = 4, int legRows = 4, float divBy = 30.0f, bool file = false, String fileName = null)
  {
    float cubeDim = height / divBy;
    float extrudeDim = cubeDim * 2.0f;
    
    //      model = #Cube (dimension = cubeDim, name = "luggage");
    Model model = cube (dimension : cubeDim, name : "luggage");
    
    String name = "luggage";
    
    FaceSet t = new FaceSet (name, model.mesh.faces);
    model.mesh.addFaceSet (t);
    
    Vector3 direction;
    Vector3 Z = Vector3.forward;
    FaceSet cap = new FaceSet ();
    
    t = model.mesh.select (0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.1f, model.mesh.getFaceSet (name), name);
    
    for (float i = extrudeDim; i <= length; i += extrudeDim)
    {
      cap = new FaceSet ();
      direction = -extrudeDim * Z;
      model.mesh = model.mesh.extrude (faceList : t, extrudeBy : direction, partName : name, capFaces : cap);
      t = cap;
    }
    
    t = model.mesh.select (0.0f, 0.01f, 0.0f, 1.0f, 0.0f, 1.0f, model.mesh.getFaceSet (name), name);
    
    Vector3 X = Vector3.right;
    
    for (float i = extrudeDim; i <= width; i += extrudeDim)
    {
      cap = new FaceSet ();
      direction = -extrudeDim * X;
      model.mesh = model.mesh.extrude (faceList : t, extrudeBy : direction, partName : name, capFaces : cap);
      t = cap;
    }
    
    FaceSet sides = model.mesh.select (0.0f, 0.1f, 0.99f, 1.0f, 0.0f, 1.0f, model.mesh.getFaceSet (name), "sides");
    sides.mergeFaceSets (model.mesh.select (0.9f, 1.0f, 0.99f, 1.0f, 0.0f, 1.0f, model.mesh.getFaceSet (name), "sides"));
    sides.mergeFaceSets (model.mesh.select (0.01f, 0.99f, 0.99f, 1.0f, 0.0f, 0.1f, model.mesh.getFaceSet (name), "sides"));
    sides.mergeFaceSets (model.mesh.select (0.01f, 0.99f, 0.99f, 1.0f, 0.9f, 1.0f, model.mesh.getFaceSet (name), "sides"));
    
    model.mesh.addFaceSet (sides);
    
    //model.mesh.visibleFace = name;
    Vector3 Y = Vector3.up;
    
    for (float i = extrudeDim; i < height; i += extrudeDim)
    {
      cap = new FaceSet ();
      direction = extrudeDim * Y;
      model.mesh = model.mesh.extrude (faceList : sides, extrudeBy : direction, partName : name, capFaces : cap);
      sides = cap;
    }
    
    t = model.mesh.select (0.0f, 0.1f, 0.99f, 1.0f, 0.0f, 1.0f, model.mesh.getFaceSet ("sides"), "lid");
    model.mesh.addFaceSet (t);
    
    for (float i = extrudeDim; i < width; i += extrudeDim)
    {
      cap = new FaceSet ();
      direction = extrudeDim * Y;
      model.mesh = model.mesh.extrude (faceList : t, extrudeBy : direction, partName : name, capFaces : cap);
      t = cap;
    }
    
    BoundingBox myBox = new BoundingBox (model.mesh.getFaceSet ("lid"));
    
    float minX = myBox.min[0];
    float minY = myBox.min[1];
    float minZ = myBox.min[2];
    float maxX = myBox.max[0];
    float maxY = myBox.max[1];
    float maxZ = myBox.max[2];
    
    CurvePoint [] points = new CurvePoint [3];
    
    float offSet = 2.5f;
    
    /*points [0] = new CurvePoint (minX, maxY, 0.0);
     *      points [1] = new CurvePoint (minX - offSet, (maxY + height)/2, 0.0);
     *      points [2] = new CurvePoint (minX, maxY - height, 0.0);*/
    
    points [0] = new CurvePoint (new Vector3 (minX, maxY, 0.0f), 1.0f);
    points [1] = new CurvePoint (new Vector3 (minX - offSet*2, (maxY + height)/2, 0.0f), 1.0f);
    points [2] = new CurvePoint (new Vector3 (minX, minY, 0.0f), 1.0f);
    
    Curve lidCurve = new BezierSpline (points.Length - 1, points);
    
    points [0] = new CurvePoint (new Vector3 (minX, maxY, 0.0f), 1.0f);
    points [1] = new CurvePoint (new Vector3 (minX - offSet/4, (maxY + height)/2, 0.0f), 1.0f);
    points [2] = new CurvePoint (new Vector3 (minX + extrudeDim, maxY - height, 0.0f), 1.0f);
    
    Curve lidInnerCurve = new BezierSpline (points.Length - 1, points);
    
    //FaceSet lid = model.mesh.select (0.0, 0.01, 0.50, 1.0, 0.0, 1.0, model.mesh.getFaceSet ("sides"), "sides");
    FaceSet lid = model.mesh.select (0.0f, 0.01f, 0.0f, 1.0f, 0.0f, 1.0f, model.mesh.getFaceSet ("lid"), "outerlid");
    
    //FaceSet innerLid = model.mesh.select (0.01, 0.1, 0.50, 0.95, 0.2, 0.8, model.mesh.getFaceSet ("sides"), "sides");
    FaceSet innerLid = model.mesh.select (0.99f, 1.0f, 0.15f, 0.85f, 0.2f, 0.8f, model.mesh.getFaceSet ("lid"), "innerlid");
    Debug.Log (innerLid.ToString ());
    
    float quarter = (minZ + maxZ)/4.0f;
    
    offSet = 0.5f;
    
    FaceSet basef = model.mesh.select (0.0f, 1.0f, 0.0f, 0.01f, 0.0f, 1.0f, model.mesh.getFaceSet (name), "base");
    model.mesh.addFaceSet (basef);
    
    
    
    ////Do the lid     
    model.mesh.shapeToCurve (innerLid, lidInnerCurve, new Vector3 (-1, 0, 0), new Vector3 (0, 0, 1));
    model.mesh.shapeToCurve (lid, lidCurve, new Vector3 (-1, 0, 0), new Vector3 (0, 0, 1));

    // Prepare the base for feet.
    model.mesh = model.mesh.sqrt2AdaptiveSubdivide (basef, 3, 0.0f);
    
    basef = model.mesh.select (0.0f, 1.0f, 0.0f, 0.01f, 0.0f, 1.0f, model.mesh.getFaceSet (name), "base");
    model.mesh.addFaceSet (basef);
    
    int threshold = 5;
    int numToes = 5;
    
    int half = numLegs/2;
    int onesLeft = numLegs%2;
    
    FaceSet right = model.mesh.select (0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, basef, "rightbase");
    model.mesh.addFaceSet (right);
    
    FaceSet left = model.mesh.select (0.5f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, basef, "leftbase");
    model.mesh.addFaceSet (left);
    
    //FaceSet center = model.mesh.select (0.4, 0.6, 0.0, 0.01, 0.0, 1.0, model.mesh.getFaceSet (name), "centerbase");
    //model.mesh.addFaceSet (center);
    int rows = (legRows/2);
    
    float offSetFoot = (1.0f/(float) (half/rows));
    float overlap = offSetFoot/5.0f;
    Debug.Log (overlap);
    
    float xRatio = (1.0f/(float) (legRows/2));
    
    float xMin = 0.0f;
    float xMax = xRatio;
    
    float zMin = 0.0f + overlap;
    float zMax = offSetFoot;
    
    //model.mesh.visibleFace = "lid";
    
    Model feet = new Model ();
    for (int i = 0; i < (legRows/2); i++)
    {
      for (int j = 0; j < (half/rows); j++)
      {
        Debug.Log ("Foot " + i + " " + j);
//         t = model.mesh.select (xMin, xMax, 0.0f, 1.0f, zMin, zMax - overlap, model.mesh.getFaceSet ("rightbase"), "rightfoot" + i + j);
//         model.mesh.addFaceSet (t);
//         model = Leg (model, numToes : numToes, threshold : threshold, height : 1.0f, length : 0.5f, width : 1.0f, heads : 1.0f, name : ("rightfoot" + i +j));
//         
//         t = model.mesh.select (xMin, xMax, 0.0f, 1.0f, zMin, zMax - overlap, model.mesh.getFaceSet ("leftbase"), "leftfoot" + i + j);
//         model.mesh.addFaceSet (t);
//         model = Leg (model, numToes : numToes, threshold : threshold, height : 1.0f, length : 0.5f, width : 1.0f, heads : 1.0f, name : ("leftfoot" + i + j));
        
        //Debug.Log (feet.mesh.numFaces());
        Model footMod = new Model ();
        t = model.mesh.select (xMin, xMax, 0.0f, 1.0f, zMin, zMax - overlap, model.mesh.getFaceSet ("rightbase"), "rightfoot" + i + j);
        model.mesh.addFaceSet (t);
        
        BoundingBox legBound = new BoundingBox (t);
        float y = legBound.min[1];
        float x = ((legBound.min[0] + legBound.max[0])/2.0f);
        float z = ((legBound.min[2] + legBound.max[2])/2.0f);
  
        float footsize = 0.9f;
        //Transform moveFoot = new Transform (new Vector3 (i * 4, 0, j * 2));
        Transform moveFoot = getEmptyTransform ();
        moveFoot.Translate (new Vector3 (x, y, z));
        
        footMod = ConstrFoot (numToes : numToes, threshold : threshold, bigToe : 0.4f, extrudeUnit : footsize, name : "rightFoot");
        
        //Debug.Log ("Foot model: " + footMod.mesh.numFaces () + " with trans: " + moveFoot + " vert " + footMod.mesh.getVertex (0));
        PolygonMesh.transformMesh (footMod.mesh, moveFoot);
        feet.mesh.addMesh (footMod.mesh);
        //feet = footMod;
        
        t = model.mesh.select (xMin, xMax, 0.0f, 1.0f, zMin, zMax - overlap, model.mesh.getFaceSet ("leftbase"), "leftfoot" + i + j);
        model.mesh.addFaceSet (t);
        
        legBound = new BoundingBox (t);
        y = legBound.min[1];
        x = ((legBound.min[0] + legBound.max[0])/2.0f);
        z = ((legBound.min[2] + legBound.max[2])/2.0f);
        
        moveFoot = getEmptyTransform ();
        moveFoot.Translate (new Vector3 (x, y, z));
        // moveFoot = new Transform (new Vector3 (1, 0, 1*(i+1)*(j)));
        
        footMod = new Model ();
        footMod = ConstrFoot (numToes : numToes, threshold : threshold, bigToe : 0.4f, extrudeUnit : footsize, name : "leftFoot");
        //Debug.Log ("Foot model: " + footMod.mesh.numFaces ());
        PolygonMesh.transformMesh (footMod.mesh, moveFoot);
        feet.mesh.addMesh (footMod.mesh);
        
        zMin = zMax + overlap;
        zMax = zMax + offSetFoot;
      }
      
      zMin = 0.0f;
      zMax = offSetFoot;
      
      xMin = xMax;
      xMax = xMax + xRatio;
    }
    
    model.mesh.addMesh (feet.mesh);
    
    model.mesh.updateNormals ();
    
    if (file)
    {
      if (fileName != null)
      {
        //          PolygonMesh.writeGeometry (model.mesh, fileName);
      }
      else
      {
        Debug.Log ("The filename may not be null!");
      }
      //System.exit(0);
    }
    
    return model;
  }
  
  Model Leg (Model model, int numToes = 5, int threshold = 3, float height = 1.0f, float length = 1.0f, float width = 1.0f, float heads = 4.0f, String name = null, bool cons = false, String objName = null)
  {
    //Debug.Log (name);
    
    FaceSet facesset = model.mesh.getFaceSet (name);
    BoundingBox bb = new BoundingBox (facesset);
    
    CurvePoint [] points = new CurvePoint [2];
    points[0] = new CurvePoint (new Vector3 (bb.min[0], bb.min[1], 0.0f), 1.0f);
    points[1] = new CurvePoint (new Vector3 (bb.max[0], bb.min[1], 0.0f), 1.0f);
    
    Curve minY = new BezierSpline (points.Length - 1, points);
    //minY.render();
    model.mesh.shapeToCurve (facesset, minY, new Vector3 (0, -1, 0), new Vector3 (0, 0, 1));
    
    float extrude = heads/4.0f; //Legs are 4 heads 
    
    String oldName = name;

    ///Thigh        
    model = LegPart (model, height = height*extrude, length = length, width = width, name = name);

    ///Knee
    String legComp = oldName + "knee";
    FaceSet knee = model.mesh.select (0.0f, 1.0f, 0.0f, 0.05f, 0.0f, 1.0f, model.mesh.getFaceSet (name), legComp);
    
    knee.setParent (name);
    model.mesh.removeLabelFromFaceSet (knee, name);
    model.mesh.addFaceSet (knee);
    
    model = LegPart (model, height = height*extrude, length = length, width = width, name = legComp);
    
    ///Shin
    name = legComp;
    legComp = oldName + "shin";
    FaceSet shin = model.mesh.select (0.0f, 1.0f, 0.0f, 0.05f, 0.0f, 1.0f, model.mesh.getFaceSet (name), legComp);
    
    shin.setParent (name);
    model.mesh.removeLabelFromFaceSet (shin, name);
    model.mesh.addFaceSet (shin);
    
    model = LegPart (model, height = height*extrude, length = length, width = width, name = legComp);
    
    ///lower shin
    name = legComp;
    legComp = oldName + "lowershin";
    FaceSet lowershin = model.mesh.select (0.0f, 1.0f, 0.0f, 0.05f, 0.0f, 1.0f, model.mesh.getFaceSet (name), legComp);
    
    lowershin.setParent (name);
    model.mesh.removeLabelFromFaceSet (lowershin, name);
    model.mesh.addFaceSet (lowershin);
    
    model = LegPart (model, height = height*extrude*(2.0f/3.0f), length = length, width = width, name = legComp);
    
    ///foot
    /*name = legComp;
     *        legComp = oldName + "foot";
     *        FaceSet foot = model.mesh.select (0.0, 1.0, 0.0, 0.1, 0.0, 1.0, model.mesh.getFaceSet (name), legComp);
     * 
     *        foot.setParent (name);
     *        model.mesh.removeLabelFromFaceSet (foot, name);
     *        model.mesh.addFaceSet (foot);
     */      
    //numToes = (int) #random ();
    //float x = #random (randomKey = 33);
    
    //Debug.Log ("foot");
    
    //bool cons = true;
    
    //Debug.Log ("Feet");
//     if (!(cons))
    {
      Debug.Log ("Extrude");
      model = Foot (model, numToes : numToes, threshold : threshold, needed : 1, bigToe : 0.4f, extrudeUnit : height*(3.0f/3.0f), name = legComp);
    }
//     else
//     {
//       BoundingBox legBound = new BoundingBox (model.mesh.getFaceSet (oldName + "lowershin"));
//       float y = legBound.min[1];
//       float x = ((legBound.min[0] + legBound.max[0])/2.0f);
//       float z = ((legBound.min[2] + legBound.max[2])/2.0f);
//       
//       Transform moveFoot = getEmptyTransform ();
//       moveFoot.Translate (new Vector3 (x, y, z));
//       
//       Model footMod = ConstrFoot (numToes : numToes, threshold : 3, bigToe : 0.4f, extrudeUnit : height*(3.0f/3.0f), name : legComp/*, needed : 1, objName = objName*/);
//       
//       PolygonMesh.transformMesh (footMod.mesh, moveFoot);
//       
//       model.mesh.addMesh (footMod.mesh);
//     }
    
    return model;
  }
  
  Model LegPart (Model model, float height = 1.0f, float length = 1.0f, float width = 1.0f, String name = null)
  {

    float heightUnit = height/6.0f;
    float lengthUnit = length/6.0f;
    float widthUnit = width/6.0f;
    
    FaceSet t = model.mesh.getFaceSet (name);
    
    t = model.mesh.select (0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, t, name);
    
    Vector3 direction;
    FaceSet cap = new FaceSet ();
    
    Vector3 Y = Vector3.up;
    ////Let's handle height first
    for (float i = 0; i < height; i += heightUnit)
    {
      cap = new FaceSet ();
      direction = -heightUnit * Y;
      model.mesh = model.mesh.extrude (faceList : t, extrudeBy : direction, partName : name, capFaces : cap);
      t = cap;  
    }        
    
    ////Length should be catered for by the selection process...
    ///Width should also be catered for by the selection process....
    
    return model;
  }  
  
  Model Foot (Model model, int numToes = 5, int threshold = 5, int needed = 0, float bigToe = 0.3f, float extrudeUnit = 1.0f, String name =  null)
  {
    
    // Creates a foot. Down corresponds to -Y.
//     Vector3 a = new Vector3 (3.0f, 2.0f, 3.0f);
    //FIXME         Vector3 p = transform (coordinates = "world", point = a);
//     Vector3 p = a;
    
    //Debug.Log ("In foot");
    
    FaceSet t = model.mesh.getFaceSet (name);
    
    Vector3 direction;
    FaceSet cap = new FaceSet ();
    
    t = model.mesh.select (0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, t, name);
    
    
    // squares off the foot under subdivision.
    int ankle = 3;
    Vector3 Y = Vector3.up;
    
    // FIXME: needed
    if (needed == 1)
    {
      for (int i = 0; i < ankle; i++)
      {
        cap = new FaceSet ();
        direction = (-extrudeUnit/(float) ankle) * Y;
        model.mesh = model.mesh.extrude (faceList : t, extrudeBy : direction, partName : name, capFaces : cap);
        t = cap;
      }
    }
    
    t = model.mesh.select (0.0f, 1.0f, 0.0f, 0.35f, 0.9f, 1.0f, model.mesh.getFaceSet (name), name + "toes");
    //Debug.Log (t);
    model.mesh.removeLabelFromFaceSet (t, name);
    model.mesh.addFaceSet (t);
    
    //model.mesh.visibleFace = name;
    
    Vector3 Z = Vector3.forward;
    for (int i = 0; i < ankle/3; i++)
    {
      cap = new FaceSet ();
      direction = (extrudeUnit / (float) ankle) * Z;
      model.mesh = model.mesh.extrude (faceList : t, extrudeBy : direction, partName : name, capFaces : cap);
      t = cap;
    }
    
    //t = model.mesh.select (0.0, 1.0, 0.0, 1.0, 0.9, 1.0, model.mesh.getFaceSet (name), name + "toes");
    
    
    Debug.Log (name);
    
    float remainder = 1.0f - bigToe;
    
    float toeSize = remainder / ((float) (numToes - 1)); //-1, as we have catered for the big toe already
    
    //Debug.Log("Big " + bigToe + " numToes " + numToes + " remainder " + remainder + " size " + toeSize);
    //System.exit(0);
    
    bool allToes = false; //Assume we have no selections for all the toes
    
    int counter = 0;
    
    FaceSet [] toes = new FaceSet [numToes];
    
    float minX;
    float maxX;
    
    float minZ = 0.75f;
    float maxZ = 1.0f;
    
    float minY = 0.0f;
    float maxY = 1.0f;
    
    //int threshold = 3;
    
    while (!(allToes))
    {
      Debug.Log(counter);
      
      counter++;
      if (counter > 30)
        break;
      bool temp = true;
      
      FaceSet toeRegion = model.mesh.select (0.0f, 1.0f, minY, maxY, minZ, maxZ, model.mesh.getFaceSet (name + "toes"), name + "toeRegion");
      //Debug.Log(toeRegion);
      
      minX = 0.0f;
      maxX = bigToe;
      for (int i = 0; i < numToes; i++)
      {
        float mx = minX;
        float Mx = maxX;
        
        if (name[0] != 'l')
        {
          mx = 1.0f - maxX;
          Mx = 1.0f - minX;
        }
        toes[i] = model.mesh.select (mx, Mx, 0.0f, 1.0f, 0.0f, 1.0f, toeRegion, name + "toe" + i);
        temp = temp && !(toes[i].numFaces () < threshold);
        Debug.Log(i + " hello " + toes[i].isEmpty () + " " + toes[i].numFaces () + " " + threshold);
        //Debug.Log("Ratios " + minX + " " + maxX + " size " + toeSize);
        minX = maxX;
        maxX += toeSize;
      }
      
      if (!(temp))
      {
        model.mesh = model.mesh.sqrt2AdaptiveSubdivide(toeRegion, 2, 0.5f);
      }
      
      allToes = temp;
    }
    
    model.mesh.visibleFace = name + "toeRegion";

    for (int i = 0; i < toes.Length; i++)
    {
      model.mesh.removeLabelFromFaceSet (toes[i], name);
      model.mesh.removeLabelFromFaceSet (toes[i], name + "toeRegion");
      model.mesh.addFaceSet (toes[i]);
    }
    
    
    for (int i = 0; i < toes.Length; i++)
    {
      t = model.mesh.select (0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, model.mesh.getFaceSet (name + "toe" + i), "toefront");
      
      cap = new FaceSet ();
      direction = (extrudeUnit/(i+2)) *  Z;
      model.mesh = model.mesh.extrude (faceList : t, extrudeBy : direction, partName : name + "toe" + i, capFaces : cap);
    }
    
    
    return model;
  }  
  
  Model ConstrFoot (int numToes = 5, int threshold = 5, float bigToe = 0.3f, float extrudeUnit = 1.0f, String name = "stump")
  {
    // Creates a foot using a constructivist modelling approach.
    // Top of ankle occurs at origin, with down corresponding to -Y.
    
    // Create a base shape for the object to be generated from.
    Model model = NGon (sides : 12, diameter : 0.2f);
    
    // label the faces from which the foot need be extruded.
    FaceSet basef = new FaceSet (name, model.mesh.faces);
    model.mesh.addFaceSet (basef);
    
    // Use the extrusionist routine to build the foot from this.
    model = ExtruFoot (model, numToes : numToes, threshold : threshold, bigToe : bigToe, extrudeUnit : extrudeUnit, stump : name);
    
    model.mesh = model.mesh.sqrt2Subdivide (3, 1.0f);
    
    model.mesh.updateNormals ();
    
    return model;
  }
  
  Model ExtruFoot (Model model, int numToes = 5, int threshold = 5, float bigToe = 0.3f, float extrudeUnit = 1.0f, String name = "foot", String stump = null)
  {
    /// Creates a foot. Down corresponds to -Y.
    
    /// stump is the label for a face set containing the portion of the model from which the
    /// foot must be grown.
    
    FaceSet currentFaces = model.mesh.getFaceSet (stump);
    if (currentFaces != null)
    {
      //Debug.Log ("Faces " + currentFaces.numFaces ());
      
      // squares off the foot under subdivision.
      int ankle = 6;
      
      Vector3 Y = Vector3.up;
      Vector3 Z = Vector3.forward;
      for (int i = 0; i < ankle; i++)
      {
        FaceSet cap = new FaceSet ();
        Vector3 direction = (-extrudeUnit / (float) ankle) * Y;
        model.mesh = model.mesh.extrude (faceList : currentFaces, extrudeBy : direction, partName : name, capFaces : cap);
        currentFaces = cap;
      }
      
      currentFaces = model.mesh.select (0.0f, 1.0f, 0.0f, 0.3f, 0.5f, 1.0f,
                                        model.mesh.getFaceSet (name), "ankle");
      for (int i = 0; i < ankle / 3; i++)
      {
        FaceSet cap = new FaceSet ();
        Vector3 direction = (extrudeUnit / (float) ankle) * Z;
        model.mesh = model.mesh.extrude (faceList : currentFaces, extrudeBy : direction, partName : name, capFaces : cap);
        currentFaces = cap;
      }
      
      
      float remainder = 1.0f - bigToe;
      
      float toeSize = remainder / ((float) (numToes - 1));	//-1, as we have catered for the big toe already
      
      //Debug.Log("Big " + bigToe + " numToes " + numToes + " remainder " + remainder + " size " + toeSize);
      //System.exit(0);
      
      bool allToes = false;	//Assume we have no selections for all the toes
      
      int counter = 0;
      
      FaceSet [] toes = new FaceSet[numToes];
      
      float minX;
      float maxX;
      
      float minZ = 0.75f;
      float maxZ = 1.0f;
      
      float minY = 0.0f;
      float maxY = 0.4f;
      
      //int threshold = 3;
      
      while (!(allToes))
      {
        //Debug.Log (counter);
        
        counter++;
        if (counter > 30)
          break;
        bool temp = true;
        
        FaceSet toeRegion =
        model.mesh.select (0.0f, 1.0f, minY, maxY, minZ, maxZ,
                           model.mesh.getFaceSet (name),
                           name + "toeRegion");
        
        minX = 0.0f;
        maxX = bigToe;
        for (int i = 0; i < numToes; i++)
        {
          float mx = minX;
          float Mx = maxX;
          
          //Debug.Log ("In foot " + stump);
          
          if (stump[0] != 'l')
          {
            mx = 1.0f - maxX;
            Mx = 1.0f - minX;
            
            //Debug.Log ("We have a right");
          }
          else
          {
            //Debug.Log ("We have a left");
          }
          
          toes[i] = model.mesh.select (mx, Mx, 0.0f, 1.0f, 0.0f, 1.0f, toeRegion, name + "toe" + i);
          temp = temp && !(toes[i].numFaces () < threshold);
          minX = maxX;
          maxX += toeSize;
        }
        
        if (!(temp))
        {
          model.mesh =
          model.mesh.sqrt2AdaptiveSubdivide (toeRegion, 2, 0.5f);
        }
        
        allToes = temp;
      }
      
      model.mesh.visibleFace = name + "toeRegion";
      
      for (int i = 0; i < toes.Length; i++)
      {
        model.mesh.removeLabelFromFaceSet (toes[i], name);
        model.mesh.addFaceSet (toes[i]);
      }
      
      for (int i = 0; i < toes.Length; i++)
      {
        FaceSet toe =
        model.mesh.select (0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
                           model.mesh.getFaceSet (name + "toe" + i),
                           "toefront");
        for (int j = 0; j < 1; j++)
        {
          FaceSet cap = new FaceSet ();
          Vector3 direction = 0.5f * (extrudeUnit * (i + 4) / (3 * (i + 2))) * Z;
          model.mesh = model.mesh.extrude (faceList : toe, extrudeBy : direction, partName : name + "toe" + i, capFaces : cap);
          toe = cap;
        }
      }
    }
    
    return model;
  }
  
  Model NGon (int sides = 4, float diameter = 1.0f)
  {
    Model model = new Model ();
    
    // Create an n sided polygon, on the XZ plane,
    // centered about the origin.
    
    List <Vertex> v = new List <Vertex>();
    
    for (int i = 0; i < sides; i++)
    {
      v.Add (model.mesh.addVertex (new Vector3 (diameter * Mathf.Sin ((sides - 1 - i) * Mathf.PI * 2 / sides), 
                                                0.0f,
                                                diameter * Mathf.Cos ((sides - 1 - i) * Mathf.PI * 2 / sides))));
    }
    
    model.mesh.addFace (v);
    
    return model;
  }
  
  
  Model basicluggage (float height = 2.0f, float width = 1.0f, float length = 2.0f)
  {
    Model model = new Model ();
    
    float cubeDim = height/20;
    float extrudeDim = cubeDim*2;
    
    model = cube (/*transformation, randomKey, */cubeDim, "luggage");
    
    FaceSet luggageSet = new FaceSet ("luggage", model.mesh.faces);  
    
    model.mesh.addFaceSet (luggageSet);
    
    for (float i = extrudeDim; i <= length; i += extrudeDim)
    {
      luggageSet = new FaceSet ("luggage", model.mesh.faces);  
      FaceSet faces = model.mesh.select (0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.1f, luggageSet, "luggage");
      model.mesh = model.mesh.extrude(faces, new Vector3 (0.0f, 0.0f, -extrudeDim), "luggage", null);
    }
    
    for (float i = extrudeDim; i <= width; i += extrudeDim)
    {
      luggageSet = new FaceSet ("luggage", model.mesh.faces);  
      FaceSet faces = model.mesh.select (0.0f, 0.01f, 0.0f, 1.0f, 0.0f, 1.0f, luggageSet, "luggage");
      model.mesh = model.mesh.extrude(faces, new Vector3 (-extrudeDim, 0.0f, 0.0f), "luggage", null);
    }
    
    /*for (float i = extrudeDim; i < extrudeDim*2; i += extrudeDim)
     *  {
     *    FaceSet faces = model.mesh.select (0.0f, 1.0f, 0.99f, 1.0f, 0.0f, 1.0f, luggageSet, "luggage");
     *    model.mesh = model.mesh.extrude(faces, new Vector3 (0.0f, extrudeDim, 0.0f), "luggage");
  }*/
    
    string name = "luggage";
    
    FaceSet faces1 = model.mesh.select (0.0f, 0.1f, 0.99f, 1.0f, 0.0f, 1.0f, model.mesh.getFaceSet (name), "luggage");
    FaceSet faces2 = model.mesh.select (0.9f, 1.0f, 0.99f, 1.0f, 0.0f, 1.0f, model.mesh.getFaceSet (name), "luggage");
    FaceSet faces3 = model.mesh.select (0.0f, 1.0f, 0.99f, 1.0f, 0.0f, 0.1f, model.mesh.getFaceSet (name), "luggage");
    FaceSet faces4 = model.mesh.select (0.0f, 1.0f, 0.99f, 1.0f, 0.9f, 1.0f, model.mesh.getFaceSet (name), "luggage");
    
    Debug.Log ("FC1 " + faces1.ToString ());
    
    for (float i = extrudeDim; i < height; i += extrudeDim)
    {
      model.mesh = model.mesh.extrude(faces1, new Vector3 (0.0f, extrudeDim, 0.0f), "luggage", null);
      faces1 = model.mesh.select (0.0f, 0.1f, 0.99f, 1.0f, 0.0f, 1.0f, model.mesh.getFaceSet (name), "luggage");
      model.mesh = model.mesh.extrude(faces2, new Vector3 (0.0f, extrudeDim, 0.0f), "luggage", null);
      faces2 = model.mesh.select (0.9f, 1.0f, 0.99f, 1.0f, 0.0f, 1.0f, model.mesh.getFaceSet (name), "luggage");
      
      model.mesh = model.mesh.extrude(faces3, new Vector3 (0.0f, extrudeDim, 0.0f), "luggage", null);
      faces3 = model.mesh.select (0.0f, 1.0f, 0.99f, 1.0f, 0.0f, 0.1f, model.mesh.getFaceSet (name), "luggage");
      model.mesh = model.mesh.extrude(faces4, new Vector3 (0.0f, extrudeDim, 0.0f), "luggage", null);
      faces4 = model.mesh.select (0.0f, 1.0f, 0.99f, 1.0f, 0.9f, 1.0f, model.mesh.getFaceSet (name), "luggage");
    }
    
    for (float i = extrudeDim; i < width; i += extrudeDim)
    {
      FaceSet faces = model.mesh.select (0.0f, 0.1f, 0.99f, 1.0f, 0.0f, 1.0f, model.mesh.getFaceSet (name), "luggage");
      model.mesh = model.mesh.extrude(faces, new Vector3 (0.0f, extrudeDim, 0.0f), "luggage", null);
    }
    
    BoundingBox myBox = new BoundingBox (model.mesh.getFaceSet ("Luggage"));
    
    float minX = myBox.min[0];
    float minY = myBox.min[1];
    float minZ = myBox.min[2];
    float maxX = myBox.max[0];
    float maxY = myBox.max[1];
    float maxZ = myBox.max[2];
    
    Vector3 [] points = new Vector3 [ 3 ];
    
    float offSet = 2.5f;
    
    points [0] = new Vector3 (minX, maxY, 0.0f);
    points [1] = new Vector3 (minX - offSet, (maxY + height)/2, 0.0f);
    points [2] = new Vector3 (minX, maxY - height, 0.0f);
    
    //     Curve lidCurve = new BezierSpline (points.length - 1, points);
    //lidCurve.render();
    
    points [0] = new Vector3 (minX, maxY, 0.0f);
    points [1] = new Vector3 (minX - offSet/4, (maxY + height)/2, 0.0f);
    points [2] = new Vector3 (minX + extrudeDim, maxY - height, 0.0f);
    
    //     Curve lidInnerCurve = new BezierSpline (points.length - 1, points);
    //lidInnerCurve.render();
    
    FaceSet lid = model.mesh.select (0.0f, 0.01f, 0.50f, 1.0f, 0.0f, 1.0f, luggageSet, "luggage");
    FaceSet innerLid = model.mesh.select (0.01f, 0.1f, 0.50f, 1.0f, 0.2f, 0.8f, luggageSet, "luggage");
    
    float quarter = (minZ + maxZ) / 4.0f;
    
    offSet = 0.5f;
    
    ////Do the lid     
    //     model.mesh.shapeToCurve (innerLid, lidInnerCurve, new Vector3 (-1, 0, 0), new Vector3 (0, 0, 1));
    //     model.mesh.shapeToCurve (lid, lidCurve, new Vector3 (-1, 0, 0), new Vector3 (0, 0, 1));
    
    FaceSet foot1 = model.mesh.select (0.85f, 1.0f, 0.0f, 0.01f, 0.85f, 1.0f, luggageSet, "rightfoot1"); 
    
    FaceSet foot2 = model.mesh.select (0.85f, 1.0f, 0.0f, 0.01f, 0.0f, 0.15f, luggageSet, "leftfoot1"); 
    
    
    model = basicfoot(/*transformation, randomKey, */extrudeDim, model, "leftfoot1", foot2);
    model = basicfoot(/*transformation, randomKey, */extrudeDim, model, "rightfoot1", foot1);
    
    //    model.mesh = model.mesh.sqrt2Subdivide (2, 1.0f);
    //    model.mesh.updateNormals ();
    
    return model;
  }
  
  
  Model basicfoot (float extrudeUnit, Model torsoModel, string name, FaceSet selectedFaces)
  {
    Model model = new Model ();
    
    
    //model = Foot.foot(transformation, randomKey, new Double(extrudeDim), model, "rightfoot1", foot1);
    //model = Cube.cube (transformation, randomKey, new Double(extrudeUnit/4), name);
    
    model = torsoModel; 
    model.mesh.addFaceSet ( selectedFaces );
    
    
    FaceSet currentFaces = selectedFaces;
    
    for (int i = 0; i < 3; i++)
    {
      Debug.Log ("AAAA" + currentFaces.numFaces () + " " + name);
      model.mesh = model.mesh.extrude(currentFaces, new Vector3 (0.0f, -extrudeUnit, 0.0f), name, null);
      
      currentFaces = model.mesh.select (0.0f, 1.0f, 0.0f, 0.1f, 0.0f, 1.0f, model.mesh.getFaceSet (name), name);
    }
    
    BoundingBox bb = new BoundingBox (model.mesh.getFaceSet (name));
    
    float minX = bb.min [0];
    float minY = bb.min [1];
    float minZ = bb.min [2];
    float maxX = bb.max [0];
    float maxY = bb.max [1];
    float maxZ = bb.max [2];
    
    float midX = (minX + maxX) / 2.0f;
    float midY = (minY + maxY) / 2.0f;
    float midZ = (minZ + maxZ) / 2.0f;
    
    //float offSet = 0.25;
    float offSet = 0.125f;
    
    Vector3 [] points = new Vector3 [ 2 ];
    
    points [ 0 ] = new Vector3 (minX + offSet, midY, minZ - (offSet * 1.5f));
    points [ 1 ] = new Vector3 (minX + offSet, midY, maxZ + (offSet * 1.5f));
    
    //points [ 0 ] = new Vector3 (minX, midY, minZ - (offSet * 1.5f));
    //points [ 1 ] = new Vector3 (minX, midY, maxZ + (offSet * 1.5f));
    
    //     Curve arb =  new BezierSpline (points.length - 1, points);
    //arb.render();
    
    points [ 0 ] = new Vector3 (maxX - offSet, midY, minZ - (offSet * 1.5f));
    points [ 1 ] = new Vector3 (maxX - offSet, midY, maxZ + (offSet * 1.5f));
    
    //     Curve arb2 =  new BezierSpline (points.length - 1, points);
    //arb2.render();
    
    currentFaces = model.mesh.select (0.0f, 0.01f, 0.0f, 1.0f, 0.0f, 1.0f, model.mesh.getFaceSet (name), name);
    //     model.mesh.shapeToCurve (currentFaces, arb, new Vector3 (1, 0, 0), new Vector3 (0, 1, 0));
    
    currentFaces = model.mesh.select (0.99f, 1.0f, 0.0f, 0.5f, 0.0f, 1.0f, model.mesh.getFaceSet (name), name);
    //     model.mesh.shapeToCurve (currentFaces, arb2, new Vector3 (-1, 0, 0), new Vector3 (0, 1, 0));
    
    points [ 0 ] = new Vector3 (minX - offSet, midY, maxZ - (offSet * 1.5f));
    points [ 1 ] = new Vector3 (maxX + offSet, midY, maxZ - (offSet * 1.5f));
    
    //     Curve arb3 =  new BezierSpline (points.length - 1, points);
    //arb3.render();
    
    points [ 0 ] = new Vector3 (minX - offSet, midY, minZ + (offSet * 1.5f));
    points [ 1 ] = new Vector3 (maxX + offSet, midY, minZ + (offSet * 1.5f));
    
    //     Curve arb4 =  new BezierSpline (points.length - 1, points);
    //arb4.render();
    
    currentFaces = model.mesh.select (0.0f, 1.0f, 0.0f, 0.4f, 0.7f, 1.0f, model.mesh.getFaceSet (name), name);
    
    //     model.mesh.shapeToCurve (currentFaces, arb3, new Vector3 (0, 0, -1), new Vector3 (0, 1, 0));
    
    currentFaces = model.mesh.select (0.0f, 1.0f, 0.0f, 0.5f, 0.0f, 0.2f, model.mesh.getFaceSet (name), name);
    
    //     model.mesh.shapeToCurve (currentFaces, arb4, new Vector3 (0, 0, 1), new Vector3 (0, 1, 0));
    
    for (int i = 0; i < 3; i++)
    {
      currentFaces = model.mesh.select (0.0f, 1.0f, 0.0f, 0.1f, 0.0f, 1.0f, model.mesh.getFaceSet (name), name);
      model.mesh = model.mesh.extrude(currentFaces, new Vector3 (0.0f, -extrudeUnit/4, 0.0f), name, null);
    }
    
    currentFaces = model.mesh.select (0.0f, 1.0f, 0.0f, 0.3f, 0.0f, 0.5f, model.mesh.getFaceSet (name), name);
    model.mesh = model.mesh.extrude(currentFaces, new Vector3 (0.0f, 0.0f, -extrudeUnit), name, null);
    
    for (int i = 0; i < 4; i++)
    {
      currentFaces = model.mesh.select (0.0f, 1.0f, 0.0f, 0.2f, 0.6f, 1.0f, model.mesh.getFaceSet (name), name);
      model.mesh = model.mesh.extrude(currentFaces, new Vector3 (0.0f, 0.0f, extrudeUnit/2), name, null);
    }
    
    FaceSet toe1 = new FaceSet ();
    FaceSet toe2 = new FaceSet ();
    FaceSet toe3 = new FaceSet ();
    FaceSet toe4 = new FaceSet ();
    FaceSet toe5 = new FaceSet ();
    
    float quarter = extrudeUnit/4.0f;
    float third = extrudeUnit/3.0f;
    float eightth = extrudeUnit/8.0f;
    
    if (name[0] == 'l')
    {
      float extrude = extrudeUnit/2;
      
      currentFaces = model.mesh.select (0.75f, 1.0f, 0.0f, 0.2f, 0.0f, 1.0f, model.mesh.getFaceSet (name), name);
      
      for (int i = 0; i < 2; i++)
      {
        model.mesh = model.mesh.extrude(currentFaces, new Vector3 (extrude, 0.0f, 0.0f), name, null);
        currentFaces = model.mesh.select (0.9f, 1.0f, 0.0f, 0.2f, 0.0f, 1.0f, model.mesh.getFaceSet (name), name);
        extrude *= 0.75f;
      }
      
      for (int i = 0; i < 1; i++)
      {
        currentFaces = model.mesh.select (0.0f, 0.25f, 0.0f, 0.2f, 0.0f, 1.0f, model.mesh.getFaceSet (name), name);
        model.mesh = model.mesh.extrude(currentFaces, new Vector3 (-(extrudeUnit * 0.75f), 0.0f, 0.0f), name, null);
      }
      
      toe1 = model.mesh.select (0.0f, 0.3f, 0.0f, 1.0f, 0.99f, 1.0f, model.mesh.getFaceSet (name), name);
      toe2 = model.mesh.select (0.2f, 0.5f, 0.0f, 1.0f, 0.99f, 1.0f, model.mesh.getFaceSet (name), name);
      toe3 = model.mesh.select (0.4f, 0.7f, 0.0f, 1.0f, 0.99f, 1.0f, model.mesh.getFaceSet (name), name);
      toe4 = model.mesh.select (0.6f, 0.9f, 0.0f, 1.0f, 0.99f, 1.0f, model.mesh.getFaceSet (name), name);
      toe5 = model.mesh.select (0.8f, 1.0f, 0.0f, 1.0f, 0.99f, 1.0f, model.mesh.getFaceSet (name), name);
      
      for (int i = 0; i < 2; i++)
      {
        model.mesh = model.mesh.extrude(toe1, new Vector3 (0.0f, 0.0f, extrudeUnit), name, null);
        model.mesh = model.mesh.extrude(toe2, new Vector3 (0.0f, 0.0f, quarter*3 + eightth), name, null);
        model.mesh = model.mesh.extrude(toe3, new Vector3 (0.0f, 0.0f, quarter*2 + eightth*2), name, null);
        model.mesh = model.mesh.extrude(toe4, new Vector3 (0.0f, 0.0f, third + eightth*2 + eightth/2), name, null);
        model.mesh = model.mesh.extrude(toe5, new Vector3 (0.0f, 0.0f, quarter + eightth*2), name, null);
        
        toe1 = model.mesh.select (0.0f, 0.3f, 0.0f, 1.0f, 0.9f, 1.0f, model.mesh.getFaceSet (name), name);
        toe2 = model.mesh.select (0.2f, 0.5f, 0.0f, 1.0f, 0.9f, 1.0f, model.mesh.getFaceSet (name), name);
        toe3 = model.mesh.select (0.4f, 0.7f, 0.0f, 1.0f, 0.8f, 1.0f, model.mesh.getFaceSet (name), name);
        toe4 = model.mesh.select (0.6f, 0.9f, 0.0f, 1.0f, 0.8f, 1.0f, model.mesh.getFaceSet (name), name);
        toe5 = model.mesh.select (0.8f, 1.0f, 0.0f, 1.0f, 0.8f, 1.0f, model.mesh.getFaceSet (name), name);
      }
      
      for (int i = 0; i < 3; i++)
      {
        model.mesh = model.mesh.extrude(toe1, new Vector3 (0.0f, 0.0f, extrudeUnit/6), name, null);
        model.mesh = model.mesh.extrude(toe2, new Vector3 (0.0f, 0.0f, (quarter*3 + eightth)/6), name, null);
        model.mesh = model.mesh.extrude(toe3, new Vector3 (0.0f, 0.0f, (quarter*2 + eightth*2)/6), name, null);
        model.mesh = model.mesh.extrude(toe4, new Vector3 (0.0f, 0.0f, (third + eightth*2 + eightth/2)/6), name, null);
        model.mesh = model.mesh.extrude(toe5, new Vector3 (0.0f, 0.0f, (quarter + eightth*2)/6), name, null);
        
        toe1 = model.mesh.select (0.0f, 0.3f, 0.0f, 1.0f, 0.9f, 1.0f, model.mesh.getFaceSet (name), name);
        toe2 = model.mesh.select (0.2f, 0.5f, 0.0f, 1.0f, 0.9f, 1.0f, model.mesh.getFaceSet (name), name);
        toe3 = model.mesh.select (0.4f, 0.7f, 0.0f, 1.0f, 0.8f, 1.0f, model.mesh.getFaceSet (name), name);
        toe4 = model.mesh.select (0.6f, 0.9f, 0.0f, 1.0f, 0.8f, 1.0f, model.mesh.getFaceSet (name), name);
        toe5 = model.mesh.select (0.8f, 1.0f, 0.0f, 1.0f, 0.8f, 1.0f, model.mesh.getFaceSet (name), name);
      }
    }
    
    if (name[0] == 'r')
    {
      float extrude = extrudeUnit/2;
      
      currentFaces = model.mesh.select (0.0f, 0.25f, 0.0f, 0.2f, 0.0f, 1.0f, model.mesh.getFaceSet (name), name);
      
      for (int i = 0; i < 2; i++)
      {
        model.mesh = model.mesh.extrude(currentFaces, new Vector3 (-extrude, 0.0f, 0.0f), name, null);
        currentFaces = model.mesh.select (0.0f, 0.1f, 0.0f, 0.2f, 0.0f, 1.0f, model.mesh.getFaceSet (name), name);
        extrude *= 0.75f;
      }
      
      for (int i = 0; i < 1; i++)
      {
        currentFaces = model.mesh.select (0.75f, 1.0f, 0.0f, 0.2f, 0.0f, 1.0f, model.mesh.getFaceSet (name), name);
        model.mesh = model.mesh.extrude(currentFaces, new Vector3 ((extrudeUnit * 0.75f), 0.0f, 0.0f), name, null);
      }
      
      toe1 = model.mesh.select (0.7f, 1.0f, 0.0f, 1.0f, 0.99f, 1.0f, model.mesh.getFaceSet (name), name);
      toe2 = model.mesh.select (0.5f, 0.8f, 0.0f, 1.0f, 0.99f, 1.0f, model.mesh.getFaceSet (name), name);
      toe3 = model.mesh.select (0.3f, 0.7f, 0.0f, 1.0f, 0.99f, 1.0f, model.mesh.getFaceSet (name), name);
      toe4 = model.mesh.select (0.1f, 0.5f, 0.0f, 1.0f, 0.99f, 1.0f, model.mesh.getFaceSet (name), name);
      toe5 = model.mesh.select (0.0f, 0.2f, 0.0f, 1.0f, 0.99f, 1.0f, model.mesh.getFaceSet (name), name);
      
      for (int i = 0; i < 2; i++)
      {
        model.mesh = model.mesh.extrude(toe1, new Vector3 (0.0f, 0.0f, extrudeUnit), name, null);
        model.mesh = model.mesh.extrude(toe2, new Vector3 (0.0f, 0.0f, (quarter*3 + eightth)), name, null);
        model.mesh = model.mesh.extrude(toe3, new Vector3 (0.0f, 0.0f, (quarter*2 + eightth*2)), name, null);
        model.mesh = model.mesh.extrude(toe4, new Vector3 (0.0f, 0.0f, (third + eightth*2 + eightth/2)), name, null);
        model.mesh = model.mesh.extrude(toe5, new Vector3 (0.0f, 0.0f, (quarter + eightth*2)), name, null);
        
        toe1 = model.mesh.select (0.7f, 1.0f, 0.0f, 1.0f, 0.9f, 1.0f, model.mesh.getFaceSet (name), name);
        toe2 = model.mesh.select (0.5f, 0.8f, 0.0f, 1.0f, 0.9f, 1.0f, model.mesh.getFaceSet (name), name);
        toe3 = model.mesh.select (0.3f, 0.7f, 0.0f, 1.0f, 0.9f, 1.0f, model.mesh.getFaceSet (name), name);
        toe4 = model.mesh.select (0.1f, 0.5f, 0.0f, 1.0f, 0.9f, 1.0f, model.mesh.getFaceSet (name), name);
        toe5 = model.mesh.select (0.0f, 0.2f, 0.0f, 1.0f, 0.9f, 1.0f, model.mesh.getFaceSet (name), name);
      }
      
      for (int i = 0; i < 3; i++)
      {
        model.mesh = model.mesh.extrude(toe1, new Vector3 (0.0f, 0.0f, extrudeUnit/6), name, null);
        model.mesh = model.mesh.extrude(toe2, new Vector3 (0.0f, 0.0f, (quarter*3 + eightth)/6), name, null);
        model.mesh = model.mesh.extrude(toe3, new Vector3 (0.0f, 0.0f, (quarter*2 + eightth*2)/6), name, null);
        model.mesh = model.mesh.extrude(toe4, new Vector3 (0.0f, 0.0f, (third + eightth*2 + eightth/2)/6), name, null);
        model.mesh = model.mesh.extrude(toe5, new Vector3 (0.0f, 0.0f, (quarter + eightth*2)/6), name, null);
        
        toe1 = model.mesh.select (0.7f, 1.0f, 0.0f, 1.0f, 0.9f, 1.0f, model.mesh.getFaceSet (name), name);
        toe2 = model.mesh.select (0.5f, 0.8f, 0.0f, 1.0f, 0.9f, 1.0f, model.mesh.getFaceSet (name), name);
        toe3 = model.mesh.select (0.3f, 0.7f, 0.0f, 1.0f, 0.9f, 1.0f, model.mesh.getFaceSet (name), name);
        toe4 = model.mesh.select (0.1f, 0.5f, 0.0f, 1.0f, 0.9f, 1.0f, model.mesh.getFaceSet (name), name);
        toe5 = model.mesh.select (0.0f, 0.2f, 0.0f, 1.0f, 0.9f, 1.0f, model.mesh.getFaceSet (name), name);
      }
    }
    
    BoundingBox footBox = new BoundingBox (model.mesh.getFaceSet (name));
    
    minX = footBox.min [0];
    minY = footBox.min [1];
    minZ = footBox.min [2];
    maxX = footBox.max [0];
    maxY = footBox.max [1];
    maxZ = footBox.max [2];
    
    midX = (minX + maxX) / 2.0f;
    midY = (minY + maxY) / 2.0f;
    midZ = (minZ + maxZ) / 2.0f;
    
    points = new Vector3 [ 7 ];
    
    offSet = 0.175f;
    
    if (name[0] == 'l')
    {
      points [0] = new Vector3 (minX + (offSet * 1.5f), minY, maxZ + offSet);
      points [1] = new Vector3 (minX - offSet, minY, maxZ + offSet);
      points [2] = new Vector3 (minX - offSet, minY, midZ);
      points [3] = new Vector3 (midX + offSet*2, minY, midZ);
      points [4] = new Vector3 (minX , minY, midZ);
      points [5] = new Vector3 (minX - offSet, minY, minZ - offSet);
      points [6] = new Vector3 (minX + offSet*3, minY, minZ - offSet);
      
      //       Curve curveMinX =  new BezierSpline (points.length - 1, points);
      
      points [0] = new Vector3 (maxX - offSet, minY, maxZ + offSet);
      points [1] = new Vector3 (maxX, minY, maxZ + offSet);
      points [2] = new Vector3 (maxX + offSet/2, minY, midZ);
      points [3] = new Vector3 (midX + offSet*2, minY, midZ);
      points [4] = new Vector3 (maxX + offSet/2, minY, midZ);
      points [5] = new Vector3 (maxX - offSet, minY, minZ - offSet);
      points [6] = new Vector3 (maxX - offSet, minY, minZ - offSet);
      
      //       Curve curveMaxX =  new BezierSpline (points.length - 1, points);
      
      ///HEEL curve          
      points = new Vector3 [ 7 ];
      
      points [0] = new Vector3 (minX - offSet, minY, minZ + offSet*3);
      points [1] = new Vector3 (minX + offSet, minY, minZ + offSet*3);
      points [2] = new Vector3 (minX + offSet*2, minY, minZ + offSet);
      points [3] = new Vector3 (midX, minY, minZ - offSet*4);
      points [4] = new Vector3 (maxX - offSet, minY, minZ + offSet);
      points [5] = new Vector3 (maxX + offSet, minY, minZ + offSet);
      points [6] = new Vector3 (maxX + offSet*2, minY, minZ + offSet*3);
      
      //       Curve curveMinZ =  new BezierSpline (points.length - 1, points);
      
      points = new Vector3 [ 6 ];
      
      points [ 0 ] = new Vector3 (minX - offSet/2, minY, minZ - offSet/4);
      points [ 1 ] = new Vector3 (midX, minY, minZ - offSet/2);
      points [ 2 ] = new Vector3 (midX, minY, minZ + offSet);
      points [ 3 ] = new Vector3 (midX, minY, minZ + 2*offSet/2);
      points [ 4 ] = new Vector3 (midX, minY, minZ + offSet);
      points [ 5 ] = new Vector3 (maxX, minY, minZ + 3*offSet/2);
      
      //       Curve curveMaxZ =  new BezierSpline (points.length - 1, points);
      
      toe1 = model.mesh.select (0.0f, 0.1f, 0.0f, 1.0f, 0.0f, 1.0f, model.mesh.getFaceSet (name), name);
      toe5 = model.mesh.select (0.99f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, model.mesh.getFaceSet (name), name);
      
      FaceSet heel = model.mesh.select (0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.2f, model.mesh.getFaceSet (name), name);        
      
      //       model.mesh.shapeToCurve (heel, curveMinZ, new Vector3 (0, 0, -1), new Vector3 (0, 1, 0));
      //       model.mesh.shapeToCurve (toe1, curveMinX, new Vector3 (-1, 0, 0), new Vector3 (0, 1, 0));
      //       //model.mesh.shapeToCurve (toe5, curveMaxX, new Vector3 (1, 0, 0), new Vector3 (0, 1, 0));
      //       model.mesh.shapeToCurve (toe5, curveMaxX, new Vector3 (-1, 0, 0), new Vector3 (0, 1, 0));
      
      //curveMinX.render ();
      //curveMaxX.render ();
      //curveMaxZ.render ();
      //curveMinZ.render ();
    }
    
    if (name[0] == 'r')
    {
      points [0] = new Vector3 (maxX - (offSet * 1.5f), minY, maxZ + offSet);
      points [1] = new Vector3 (maxX + offSet, minY, maxZ + offSet);
      points [2] = new Vector3 (maxX + offSet, minY, midZ);
      points [3] = new Vector3 (midX - offSet*2, minY, midZ);
      points [4] = new Vector3 (maxX , minY, midZ);
      points [5] = new Vector3 (maxX + offSet, minY, minZ - offSet);
      points [6] = new Vector3 (maxX - offSet*3, minY, minZ - offSet);
      
      //       Curve curveMinX =  new BezierSpline (points.length - 1, points);
      
      points [0] = new Vector3 (minX + offSet, minY, maxZ + offSet);
      points [1] = new Vector3 (minX, minY, maxZ + offSet);
      points [2] = new Vector3 (minX - offSet/2, minY, midZ);
      points [3] = new Vector3 (midX - offSet*2, minY, midZ);
      points [4] = new Vector3 (minX - offSet/2, minY, midZ);
      points [5] = new Vector3 (minX + offSet, minY, minZ - offSet);
      points [6] = new Vector3 (minX + offSet, minY, minZ - offSet);
      
      //       Curve curveMaxX =  new BezierSpline (points.length - 1, points);
      
      ///HEEL curve          
      points = new Vector3 [ 7 ];
      
      points [0] = new Vector3 (minX - offSet, minY, minZ + offSet*3);
      points [1] = new Vector3 (minX + offSet, minY, minZ + offSet*3);
      points [2] = new Vector3 (minX + offSet*2, minY, minZ + offSet);
      points [3] = new Vector3 (midX, minY, minZ - offSet*4);
      points [4] = new Vector3 (maxX - offSet, minY, minZ + offSet);
      points [5] = new Vector3 (maxX + offSet, minY, minZ + offSet);
      points [6] = new Vector3 (maxX + offSet*2, minY, minZ + offSet*3);
      
      //       Curve curveMinZ =  new BezierSpline (points.length - 1, points);
      
      points = new Vector3 [ 6 ];
      
      points [ 0 ] = new Vector3 (minX - offSet/2, minY, minZ - offSet/4);
      points [ 1 ] = new Vector3 (midX, minY, minZ - offSet/2);
      points [ 2 ] = new Vector3 (midX, minY, minZ + offSet);
      points [ 3 ] = new Vector3 (midX, minY, minZ + 2*offSet/2);
      points [ 4 ] = new Vector3 (midX, minY, minZ + offSet);
      points [ 5 ] = new Vector3 (maxX, minY, minZ + 3*offSet/2);
      
      //       Curve curveMaxZ =  new BezierSpline (points.length - 1, points);
      
      //toe1 = model.mesh.select (0.0f, 0.1f, 0.0f, 1.0f, 0.0f, 1.0f, model.mesh.getFaceSet (name), name);
      toe1 = model.mesh.select (0.99f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, model.mesh.getFaceSet (name), name);
      //toe5 = model.mesh.select (0.99f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, model.mesh.getFaceSet (name), name);
      toe5 = model.mesh.select (0.0f, 0.1f, 0.0f, 1.0f, 0.0f, 1.0f, model.mesh.getFaceSet (name), name);
      
      FaceSet heel = model.mesh.select (0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.2f, model.mesh.getFaceSet (name), name);        
      
      //       model.mesh.shapeToCurve (heel, curveMinZ, new Vector3 (0, 0, -1), new Vector3 (0, 1, 0));
      //       model.mesh.shapeToCurve (toe1, curveMinX, new Vector3 (1, 0, 0), new Vector3 (0, 1, 0));
      //       //model.mesh.shapeToCurve (toe5, curveMaxX, new Vector3 (1, 0, 0), new Vector3 (0, 1, 0));
      //       model.mesh.shapeToCurve (toe5, curveMaxX, new Vector3 (-1, 0, 0), new Vector3 (0, 1, 0));
      
      //curveMinX.render ();
      //curveMaxX.render ();
      //curveMaxZ.render ();
      //curveMinZ.render ();
    }
    
    //model.mesh.updateNormals();
    return model;
  }
  
  Model cube (float dimension, string name)
  {
    Model model = new Model ();
    
    List <Vertex> v = new List <Vertex>();
    
    v.Add(model.mesh.addVertex (new Vector3 (0-dimension, dimension, 0-dimension)));
    v.Add(model.mesh.addVertex (new Vector3 (0-dimension, 0-dimension, 0-dimension)));
    v.Add(model.mesh.addVertex (new Vector3 (dimension, 0-dimension, 0-dimension)));
    v.Add(model.mesh.addVertex (new Vector3 (dimension, dimension, 0-dimension)));
    v.Add(model.mesh.addVertex (new Vector3 (0-dimension, dimension, dimension)));
    v.Add(model.mesh.addVertex (new Vector3 (0-dimension, 0-dimension, dimension)));
    v.Add(model.mesh.addVertex (new Vector3 (dimension, 0-dimension, dimension)));
    v.Add(model.mesh.addVertex (new Vector3 (dimension, dimension, dimension)));
    
    List <Vertex> t = new List <Vertex>();
    
    // front - face 1       
    t.Clear ();
    t.Add(v[4]);
    t.Add(v[5]);
    t.Add(v[6]);
    t.Add(v[7]);
    Face temp = model.mesh.addFace(t);
    temp.setLabel (name);
    
    // back - face 2        
    t.Clear ();
    t.Add(v[3]);
    t.Add(v[2]);
    t.Add(v[1]);
    t.Add(v[0]);
    temp = model.mesh.addFace(t);
    temp.setLabel (name);
    
    // left - face 3
    t.Clear ();
    t.Add(v[0]);
    t.Add(v[1]);
    t.Add(v[5]);
    t.Add(v[4]);
    temp = model.mesh.addFace(t);
    temp.setLabel (name);
    
    // right - face 4
    t.Clear ();
    t.Add(v[7]);
    t.Add(v[6]);
    t.Add(v[2]);
    t.Add(v[3]);
    temp = model.mesh.addFace(t);
    temp.setLabel (name);
    
    // top - face 5
    t.Clear ();
    t.Add(v[0]);
    t.Add(v[4]);
    t.Add(v[7]);
    t.Add(v[3]);
    temp = model.mesh.addFace(t);
    temp.setLabel (name);
    
    // bottom - face 6
    t.Clear ();
    t.Add(v[6]);
    t.Add(v[5]);
    t.Add(v[1]);
    t.Add(v[2]);
    temp = model.mesh.addFace(t);
    temp.setLabel (name);
    
    return model;
  }
  
  
  
  Transform getEmptyTransform ()
  {
    GameObject emptyGO = new GameObject();
    Transform t = emptyGO.transform;
    Destroy (emptyGO);
    return t;
  }
}

public class Vertex
{
  public Vector3 position;	//Coordinates of the vertex
  public Vector3 normal;	//The normal
  public List <Face> faces;		//keeps track of the faces using this vertex
  public List <Edge> edges;		//list of edges that have this vertex as an end point
  
  ///Attributes, will be adjusted as required
  //  public Vector attributes;	//to be determined
  public Color colour;
  public Vector3 orientation;
  public int orientationFixed;
  public float sweepDistance;
  public bool sweepAssigned;
  public long neighbourused;
  
  public int level;
  
  public int id;
  public static int instance = 0;
  
  // Used to keep track of position in list
  // maintained by mesh. Value not valid unless
  // recent call to generateXXXIndices has been
  // performed.
  public int index;
  
  // just for texture synthesis.
  //   public int compareTo (Object o)
  //   {
  //     Vertex v = (Vertex) o;
  //     if (v.sweepDistance < sweepDistance)
  //       return 1;
  //     else
  //       return -1;
  //   }
  
  public Vertex (Vector3 pointPos)
  {
    edges = new List <Edge> ();
    faces = new List <Face> ();
    id = instance++;
    
    position = pointPos;
    normal = new Vector3 (0.0f, 1.0f, 0.0f);	//Fix Me!
    
    ///Attributes
    //     attributes = new Vector (0, 1);
    
    colour = new Color ();
    orientationFixed = 0;
    sweepAssigned = false;
  }
  
  public Vertex () : this (new Vector3 ())
  {
  }
  
  //   public Object getAttribute (Class c)
  //   {
  //     for (int i = 0; i < attributes.Count; i++)
  //     {
  //       Object o = attributes[i];
  //       if (o.getClass () == c)
  //         return o;
  //     }
  //     return null;
  //   }
  
  //   void setAttribute (Object o)
  //   {
  //     Class c = o.getClass ();
  //     for (int i = 0; i < attributes.Count; i++)
  //     {
  //       Object oa = attributes[i];
  //       if (oa.getClass () == c)
  //       {
  //         attributes.setElementAt (o, i);
  //         return;
  //       }
  //     }
  //     attributes.Add (o);
  //   }
  
  public int numFaces ()
  {
    return faces.Count;
  }
  
  public int numEdges ()
  {
    return edges.Count;
  }
  
  public void delete ()
  {
    ////Remove references to this vertex from each of the faces that use it
    for (int i = 0; i < this.numFaces (); i++)
    {
      Face currFace = (Face) this.faces[i];
      
      for (int j = 0; j < currFace.numVertices (); j++)
      {
        Vertex currVertex = (Vertex) currFace.vertices[j];
        
        if (currVertex == this)
        {
          currFace.vertices.RemoveAt (j);
          break;
        }
      }
    }
    
    /////Do I need to check for edges??????????
  }
  
  public bool checkEdge (Edge compEdge)
  {
    bool found = false;
    
    for (int i = 0; i < numEdges (); i++)
    {
      Edge currEdge = (Edge) edges[i];
      
      if (currEdge.sameEdge (compEdge))
      {
        found = true;
        break;
      }
    }
    
    return found;
  }
  
  /*FIX ME!
   * public bool checkFace(Face compFace)
   * {
   * bool found = false;
   * 
   * for (int i = 0; i < numFaces(); i++)
   * {
   * Face currFace = (Face)faces[i];
   * 
   * if(currFace.sameFace(compFace))
   * {
   * found = true;
   * break;
}
}

return found;
} */
  
  public void addFace (Face faceToAdd)
  {
    faces.Add (faceToAdd);
  }
  
  public void addEdge (Edge edgeToAdd)
  {
    if (!checkEdge (edgeToAdd))
    {
      edges.Add (edgeToAdd);
    }
  }
  
  public string tostring ()
  /*Print statement for a vertex */
  {
    return position + "[" + id + "-" + numEdges () + "-" + numFaces () + "]";
  }
  
  public void resetFaces ()
  {
    faces.Clear ();
  }
  
  public void resetEdges ()
  {
    edges.Clear ();
  }
  
  public void reset ()
  {
    instance = 0;
  }
  
  public Face getFace (int i)
  {
    return (Face) faces[i];
  }
  public Edge getEdge (int i)
  {
    return (Edge) edges[i];
  }
};

public class Edge
{
  public Vertex start;		//The start of an edge
  public Vertex end;		//The end of an edge
  public List <Face> faces;		//list of faces that are adjacent to this edge
  public int id;
  public static int instance = 0;
  
  // Used to keep track of position in list
  // maintained by mesh. Value not valid unless
  // recent call to generateXXXIndices has been
  // performed.
  public int index;
  
  public Edge (Vertex startPoint, Vertex endPoint)
  {
    start = startPoint;
    end = endPoint;
    id = instance++;
    
    faces = new List <Face> ();
  }
  
  public Edge () : this (new Vertex (), new Vertex ())
  {
  }
  
  public void delete ()
  {
    ////Remove references to this edge from associated faces 
    for (int i = 0; i < this.numFaces (); i++)
    {
      Face currFace = (Face) this.faces[i];
      
      for (int j = 0; j < currFace.numEdges (); j++)
      {
        Edge currEdge = (Edge) currFace.edges[j];
        
        if (currEdge == this)
        {
          currFace.edges.RemoveAt (j);
          break;
        }
      }
    }
    
    ////Remove references to this edge from associated vertices
    for (int i = 0; i < this.start.numEdges (); i++)
    {
      Edge currEdge = (Edge) this.start.edges[i];
      
      if (currEdge == this)
      {
        this.start.edges.RemoveAt (i);
        break;
      }
    }
    
    for (int i = 0; i < this.end.numEdges (); i++)
    {
      Edge currEdge = (Edge) this.end.edges[i];
      
      if (currEdge == this)
      {
        this.end.edges.RemoveAt (i);
        break;
      }
    }
  }
  
  public Vector3 calcMid ()
  {
    Vector3 result = new Vector3 ();
    result = 0.5f * start.position + 0.5f * end.position;
    
    return result;
  }
  
  public Face getAdjFace (Face currFace)
  {
    Face returnMe = null;
    
    if (this.numFaces () > 1)
    {
      if (currFace == faces[0])
      {
        returnMe = (Face) faces[1];
      }
      else
      {
        returnMe = (Face) faces[0];
      }
    }
    
    return returnMe;
  }
  
  public bool isBoundary ()
  {
    bool boundary = false;
    
    if (numFaces () > 1)
      boundary = false;
    else
      boundary = true;
    
    return boundary;
  }
  
  public int numFaces ()
  {
    return faces.Count;
  }
  
  public void addFace (Face faceToAdd)
  {
    faces.Add (faceToAdd);
  }
  
  public bool sameEdge (Edge compEdge)
  {
    bool sameEdge = false;
    
    if ((start == compEdge.start) || (start == compEdge.end))
    {
      if ((end == compEdge.start) || (end == compEdge.end))
      {
        sameEdge = true;
      }
      else
      {
        sameEdge = false;
      }
    }
    else
    {
      sameEdge = false;
    }
    
    return sameEdge;
  }
  
  /// Return the vertex at the other end of the edge.
  /// Assumes given vertex is actually on the edge.
  public Vertex otherEnd (Vertex v)
  {
    if (v == start)
    {
      return end;
    }
    else
    {
      return start;
    }
  }
  
  public string tostring ()
  {
    string faceList = "";
    
    for (int i = 0; i < numFaces (); i++)
    {
      Face currFace = (Face) faces[i];
      faceList = faceList + "[" + currFace + "]";
    }
    
    return id + " " + numFaces () + " " + faceList + " start " + start +
    " end " + end;
    
  }
  
  public void reset ()
  {
    instance = 0;
  }
  
};

public class Face
{
  public List <Vertex> vertices;	//keeps track of each vertex
  public List <Edge> edges;		//keeps track of each edge
  public Vector3 normal;	//The normal
  public int id;
  public static int instance = 0;
  
  // texture parameters.
  public string texture = "";
  public float [] s = null;
  public float [] t = null;
  
  public List <string> labels;
  
  // Used to keep track of position in list
  // maintained by mesh. Value not valid unless
  // recent call to generateXXXIndices has been
  // performed.
  public int index;
  
  public Face (List <Vertex> verts)
  {
    vertices = new List <Vertex> ();
    edges = new List <Edge> ();
    labels = new List <string> ();
    normal = new Vector3 ();	//Fix Me!
    id = instance++;
    
    addVertices (verts);
  }
  
  public Face () : this (null)
  {
  }
  
  public void setLabel (string label)
  {
    if ((label != null) && !(findLabel (label)))
      labels.Add (label);
  }
  
  public int numLabels ()
  {
    return labels.Count;
  }
  
  public bool findLabel (string label)
  {
    bool found = false;
    
    for (int i = 0; i < numLabels (); i++)
    {
      if (((string) labels[i]).Equals (label, StringComparison.OrdinalIgnoreCase))
      {
        found = true;
        break;
      }
    }
    return found;
  }
  
  public string getLabel (int index)
  {
    if (labels.Count > 0)
      return (string) labels[index];
    else
    {
      return null;
    }
  }
  
  public void removeLabel (string label)
  {
    for (int i = 0; i < this.numLabels (); i++)
    {
      if (((string) labels[i]).Equals (label))
      {
        labels.RemoveAt (i);
        i--;
      }
    }
  }
  
  public int numVertices ()
  {
    return vertices.Count;
  }
  
  public int numEdges ()
  {
    return edges.Count;
  }
  
  public Vertex getVertex (int index)
  {
    return (Vertex) this.vertices[index];
  }
  
  public Edge getEdge (int index)
  {
    return (Edge) this.edges[index];
  }
  
  public Vector3 calcCenter ()
  {
    Vector3 result = new Vector3 ();
    for (int i = 0; i < numVertices (); i++)
    {
      Vertex currVertex = (Vertex) vertices[i];
      result = 1.0f * result + 1.0f * currVertex.position;
    }
    result = (1.0f / (float) numVertices ()) * result;
    
    return result;
  }
  
  public void delete ()
  {
    ////Delete the reference to this face from each of it's vertices
    for (int i = 0; i < this.numVertices (); i++)
    {
      Vertex currVertex = (Vertex) this.vertices[i];
      
      for (int j = 0; j < currVertex.numFaces (); j++)
      {
        Face currFace = (Face) currVertex.faces[j];
        
        if (currFace == this)
        {
          currVertex.faces.RemoveAt (j);
          break;
        }
      }
    }
    
    ////Delete the reference to this face from each of it's edges
    for (int i = 0; i < this.numEdges (); i++)
    {
      Edge currEdge = (Edge) this.edges[i];
      
      for (int j = 0; j < currEdge.numFaces (); j++)
      {
        Face currFace = (Face) currEdge.faces[j];
        
        if (currFace == this)
        {
          currEdge.faces.RemoveAt (j);
          break;
        }
      }
    }
  }
  
  public void addVertices (List <Vertex> verts)
  {
    for (int i = 0; i < verts.Count; i++)
    {
      vertices.Add (verts[i]);
    }				//end for
  }
  
  public void addEdge (Edge currEdge)
  /*Assumption: The edges will be created in the order that the vertices have
   *   been used to create the face, ie the vertices must be in anti-clockwise order.  Second assumption: An edge list has already been created in the PolygonMesh representation. */
  {
    edges.Add (currEdge);
  }
  
  public string tostring ()
  {
    string info = "";
    info = id + " " + numVertices () + " " + numEdges ();
    return info;
  }
  
  // return the area of this face.
  public float area ()
  {
    // Area of triangle = 0.5 * side a * side b * sin (angle between a and b).
    // Startling resemblance to cross product.
    float area = 0.0f;
    
    // iterate over triangles in face. 
    for (int i = 1; i < numVertices () - 1; i++)
    {
      area +=
      0.5f *
      Vector3.Cross (getVertex (i).position - getVertex (0).position,
                     getVertex (i + 1).position - getVertex (0).position).
      magnitude;
    }
    return area;
  }
};

public class FaceSet
{
  public string identifier;
  protected List <Face> faces;
  protected string parentFaceSet;
  
  public FaceSet ()
  {
    identifier = "";
    faces = new List <Face> ();
    parentFaceSet = "";
  }
  
  public FaceSet (string id) : this ()
  {
    identifier = id;
  }
  
  public FaceSet (string id, List <Face> faceList) : this ()
  {
    identifier = id;
    faces = new List <Face> (faceList);
  }
  
  public FaceSet (string id, string criterion, List <Face> faceList) : this ()
  {
    identifier = id;
    parentFaceSet = null;
    faces = createFaceSet (criterion, faceList);
  }
  
  public FaceSet (FaceSet fc) : this (fc.identifier, fc.faces)
  {
    fc.parentFaceSet = parentFaceSet;
  }
  
  public List <Face> createFaceSet (string criterion, List <Face> faceList)
  {
    List <Face> validFaces = new List <Face> ();
    
    for (int i = 0; i < faceList.Count; i++)
    {
      Face currentFace = (Face)faceList[i];
      if (currentFace.findLabel(criterion))
      {
        validFaces.Add (currentFace);
      }
    }
    
    return validFaces;
  }
  
  public bool isEmpty ()
  {
    bool empty = false;
    
    if (this.numFaces () < 1)
      empty = true;
    else
      empty = false;
    
    return empty;
  }
  
  public void setParent (string parent)
  {
    this.parentFaceSet = parent;
  }
  
  public string getParent ()
  {
    return this.parentFaceSet;
  }
  
  public void mergeFaceSets (FaceSet faces)
  {
    if (faces == null)
      return;
    
    for (int i = 0; i < faces.numFaces (); i++)
    {
      this.addFace (faces.getFace (i));
    }
  }
  
  public void updateFaceSet (string identifier, List <Face> faceList)
  {
    this.faces = createFaceSet (identifier, faceList);
  }
  
  public void deleteFace (Face faceToDelete)
  {
    this.faces.Remove (faceToDelete);
//     for (int i = 0; i < this.numFaces (); i++)
//     {
//       if (this.getFace(i) == faceToDelete)
//       {
//         this.faces.RemoveAt (i);
//         break;
//       }
//     }
  }
  
  public string getID ()
  {
    return identifier;
  }
  
  public void setID (string id)
  {
    identifier = id;
  }
  
  public Face getFace (int index)
  {
    return (Face)faces[index];
  }
  
  public int numFaces ()
  {
    return faces.Count;
  }
  
  public void addFace (Face faceToAdd)
  {
    faces.Add (faceToAdd);
  }
  
  public string ToString ()
  {
    string information = "";
    
    information = "Face Set: " + identifier;
    
    for (int i = 0; i < this.numFaces(); i++)
    {
      Face currFace = this.getFace (i);
      
      information = information + " (" + currFace + ")";
    }
    
    return information;
  }
};

public class PolygonMesh
{
  public List <Vertex> vertices;	//PolygonMesh vertex list
  public List <Edge> edges;		//PolygonMesh edge list
  public List <Face> faces;		//PolygonMesh face list
  
  protected List <FaceSet> faceSets;	//A list of faces that can be used to identify model components
  
  private bool fancyVector;
  private float vectorLength = 0.1f;
  
  public string visibleFace = null;
  
  public PolygonMesh ()
  {
    vertices = new List <Vertex> ();
    edges = new List <Edge> ();
    faces = new List <Face> ();
    
    faceSets = new List <FaceSet> ();
    
    fancyVector = true;
  }
  
  public PolygonMesh (int i, int j) : this ()
  {
  }
  
  // apply the given transformation to the vertices of the mesh.                
  static public void transformMesh (PolygonMesh m, Transform t)
  {
    for (int i = 0; i < m.numVertices (); i++)
    {
      Vertex v = m.getVertex (i);
      Vector3 p = v.position;
      Vector3 q = t.TransformPoint (p);
      v.position = q;
    }    
  }                               
  
  
  public void cleanPolygonMesh ()
  {
    int numVertices = this.numVertices ();
    List <Vertex> tempVertices = new List <Vertex> ();
    
    for (int i = 0; i < numVertices; i++)
    {
      Vertex currVertex = this.getVertex (i);
      
      if ((currVertex.numFaces () == 0) && (currVertex.numEdges () == 0))
      {
        //this.vertices.RemoveAt (i);
      }
      else
      {
        tempVertices.Add (currVertex);
      }
    }
    vertices = tempVertices;
  }
  
  public int numEdges ()
  {
    return edges.Count;
  }
  
  public int numVertices ()
  {
    return vertices.Count;
  }
  
  public int numFaces ()
  {
    return faces.Count;
  }
  
  public FaceSet getFaceSet (int index)
  {
    return (FaceSet) faceSets[index];
  }
  
  public FaceSet getFaceSet (string identifier)
  {
    FaceSet currentFaceSet = new FaceSet ();
    
    bool found = false;
    
    for (int i = 0; i < this.numFaceSets (); i++)
    {
      currentFaceSet = new FaceSet (faceSets[i]);
      
      if (currentFaceSet.identifier.Equals (identifier, StringComparison.OrdinalIgnoreCase))
      {
        found = true;
        break;
      }
    }
    
    if (!(found))
      currentFaceSet = null;
    
    return currentFaceSet;
  }
  
  public void addFaceSet (FaceSet setToAdd)
  {
    deleteFaceSet (setToAdd);
    faceSets.Add (setToAdd);
    
    for (int i = 0; i < setToAdd.numFaces (); i++)
    {
      Face currentFace = setToAdd.getFace (i);
      ////FIXME:
      //currentFace.labels = new Vector ();
      currentFace.setLabel (setToAdd.getID ());
    }
  }
  
  /////FIXME::I am probably in the wrong place.....
  public void removeLabelFromFaceSet (FaceSet set, string toRemove)
  {
    for (int i = 0; i < set.numFaces (); i++)
    {
      Face currFace = set.getFace (i);
      
      currFace.removeLabel (toRemove);
    }
  }
  
  public int numFaceSets ()
  {
    return faceSets.Count;
  }
  
  public void deleteFaceSet (FaceSet setToDelete)
  {
    for (int i = 0; i < setToDelete.numFaces (); i++)
    {
      Face currentFace = setToDelete.getFace (i);
      currentFace.removeLabel (setToDelete.getID ());
    }
    
    for (int i = 0; i < this.numFaceSets (); i++)
    {
      if (getFaceSet (i).getID ().Equals (setToDelete.getID ()))
      {
        faceSets.RemoveAt (i);
        break;
      }
    }
  }
  
  public Vertex getVertex (int index)
  /// Get the ith vertex of the mesh, where
  /// i is between 0 and numVertices-1.
  {
    return (Vertex) this.vertices[index];
  }
  
  public Face getFace (int index)
  {
    return (Face) this.faces[index];
  }
  
  public void generateVertexIndices ()
  {
    for (int i = 0; i < numVertices (); i++)
    {
      ((Vertex) vertices[i]).index = i;
    }
  }
  
  public void generateEdgeIndices ()
  {
    for (int i = 0; i < numEdges (); i++)
    {
      ((Edge) edges[i]).index = i;
    }
  }
  
  public void generateFaceIndices ()
  {
    for (int i = 0; i < numFaces (); i++)
    {
      ((Face) faces[i]).index = i;
    }
  }
  
  // calculate normal vectors.
  public void updateNormals ()
  {
    for (int i = 0; i < numVertices (); i++)
    {
      Vertex currVertex = (Vertex) vertices[i];
      currVertex.normal = new Vector3 (0.0f, 0.0f, 0.0f);
    }
    
    for (int i = 0; i < numFaces (); i++)
    {
      Face currFace = (Face) faces[i];
      
      for (int j = 0; j < currFace.numVertices (); j++)
      {
        Vertex currVertex = (Vertex) currFace.vertices[j];
        
        Vertex currVertex1 =
        (Vertex) currFace.vertices[(j + 1) %
        currFace.numVertices ()];
        Vertex currVertex2 =
        (Vertex) currFace.vertices[(j + 2) %
        currFace.numVertices ()];
        
        Vector3 side1 = currVertex1.position - currVertex.position;
        Vector3 side2 = currVertex2.position - currVertex.position;
        Vector3 facenormal = Vector3.Cross (side1, side2);
        
        if (facenormal.magnitude != 0.0f)
        {
          facenormal = facenormal.normalized;
          currVertex.normal = currVertex.normal + facenormal;
        }
      }
    }
    
    for (int i = 0; i < numVertices (); i++)
    {
      Vertex currVertex = (Vertex) vertices[i];
      if (currVertex.normal.magnitude == 0.0f)
        currVertex.normal = new Vector3 (0.0f, 1.0f, 0.0f);
      else
        currVertex.normal = currVertex.normal.normalized;
    }
  }
  
  // returns a normalized normal for the polygon, assuming it is planar.
  public Vector3 faceNormal (int f)
  {
    if ((f >= 0) && (f < numFaces ()))
    {
      Face currFace = (Face) faces[f];
      
      if (currFace.numVertices () > 2)
      {
        Vertex currVertex = (Vertex) currFace.vertices[0];
        Vertex currVertex1 =
        (Vertex) currFace.vertices[(0 + 1) %
        currFace.numVertices ()];
        Vertex currVertex2 =
        (Vertex) currFace.vertices[(0 + 2) %
        currFace.numVertices ()];
        
        Vector3 side1 = currVertex1.position - currVertex.position;
        Vector3 side2 = currVertex2.position - currVertex.position;
        Vector3 facenormal = Vector3.Cross (side1, side2);
        facenormal = facenormal.normalized;
        return facenormal;
      }
      else
      {
        //         Debug.Log ("Polygon " + f +
        //         " has too few sides in faceNormal");
      }
    }
    else
    {
      //       Debug.Log ("Polygon " + f + " out of valid range [0:" +
      //       numFaces () + "] in faceNormal");
    }
    return new Vector3 (0.0f, 0.0f, 0.0f);
  }
  
  /*public void deleteFace (int faceIndex)
   * {
   * Face faceToDelete = (Face)this.faces.elementAt(faceIndex);
   * 
   * faceToDelete.delete();
   * 
   * for (int i = 0; i < faceToDelete.numEdges(); i++)
   * {
   * Edge currEdge = (Edge)faceToDelete.edges[i];
   * 
   * if (currEdge.numFaces() < 1)
   * currEdge.delete();
   * 
}

////Lastly, remove the face from the mesh's list of faces
this.faces.removeElementAt(faceIndex);
} */
  
  public void deleteFace (Face faceToDelete)
  {
    faceToDelete.delete ();
    
    for (int i = 0; i < faceToDelete.numEdges (); i++)
    {
      Edge currEdge = (Edge) faceToDelete.edges[i];
      
      if (currEdge.numFaces () < 1)
        currEdge.delete ();
      
    }
    
    for (int i = 0; i < this.numFaces (); i++)
    {
      Face currFace = this.getFace (i);
      
      if (currFace == faceToDelete)
      {
        ////Lastly, remove the face from the mesh's list of faces
        this.faces.RemoveAt (i);
        break;
      }
    }
  }
  
  public PolygonMesh copyPolygonMesh ()
  ////Create a new mesh by creating new vertices, faces and edges
  {
    PolygonMesh newPolygonMesh = new PolygonMesh ();
    
    Vertex [] newVerts = new Vertex[numVertices ()];
    
    for (int i = 0; i < numVertices (); i++)
    {
      Vertex currVertex = (Vertex) vertices[i];
      currVertex.index = i;
      newVerts[i] = newPolygonMesh.addVertex (currVertex.position);
    }
    
    for (int i = 0; i < numFaces (); i++)
    {
      Face currFace = (Face) faces[i];
      
      List <Vertex> t = new List <Vertex> (currFace.numVertices ());
      
      for (int j = 0; j < currFace.numVertices (); j++)
      {
        Vertex currVertex = (Vertex) currFace.vertices[j];
        Vertex current = newVerts[currVertex.index];
        t.Add (current);
      }
      newPolygonMesh.addFace (t);
      // FIXME: add labels?
    }
    
    return newPolygonMesh;
  }
  
  // return the next boundary vertex, that is not the one given as second parameter.
  // if second parameter is null, return any valid neighbouring boundary vertex.
  public Vertex nextVertexAlongBoundary (Vertex start, Vertex notthisone,
                                         bool[]boundary)
  {
    //Debug.Log ("At vertex: " + start);
    for (int i = 0; i < start.numEdges (); i++)
    {
      Edge e = start.getEdge (i);
      
      if (e.isBoundary ())
      {
        //         Debug.Log ("Checking edge: " + e);
        Vertex v = e.otherEnd (start);
        //         Debug.Log ("Other: " + v + "  - indices: " +
        //         start.index + " " + v.index + " boundarie " +
        //         boundary[start.index] + " " +
        //         boundary[v.index]);
        if (notthisone != null)
          //           Debug.Log ("Not this : " + notthisone.index);
          //                  if (boundary [v.index])
        {
          if ((notthisone == null) || (v != notthisone))
          {
            return v;
          }
        }
      }
    }
    
    //     Debug.Log ("Could not find valid next vertex along boundary");
    return null;
  }
  
  /// Combine other model with this one, joining along the boundaries defined by the
  /// face sets provided. Returns mesh, modified with joined portion. OtherBoundary
  /// represents faces that are to be copied onto this mesh.
  public PolygonMesh join (FaceSet thisBoundary, PolygonMesh other, FaceSet otherBoundary)
  {
    // select the vertices on the boundaries of each face set.
    List <Vertex> thiscontour = new List <Vertex> ();
    List <Vertex> othercontour = new List <Vertex> ();
    
    generateFaceIndices ();
    other.generateFaceIndices ();
    generateVertexIndices ();
    other.generateVertexIndices ();
    
    bool [] thisSeparable = new bool[numFaces ()];
    bool [] otherSeparable = new bool[other.numFaces ()];
    /// Tag faces according to whether they are separable or not.
    for (int i = 0; i < numFaces (); i++)
    {
      thisSeparable[i] = false;
    }
    for (int i = 0; i < other.numFaces (); i++)
    {
      otherSeparable[i] = false;
    }
    for (int i = 0; i < thisBoundary.numFaces (); i++)
    {
      Face f = thisBoundary.getFace (i);
      thisSeparable[f.index] = true;	// true if separable.
    }
    for (int i = 0; i < otherBoundary.numFaces (); i++)
    {
      Face f = otherBoundary.getFace (i);
      otherSeparable[f.index] = true;	// true if separable.
    }
    
    // FIXME: is there a more efficient way of finding boundary vertices?
    // maybe index according to position in the face set, rather than
    // the complete mesh.
    bool [] thisVertex = new bool[numVertices ()];
    for (int i = 0; i < numVertices (); i++)
    {
      thisVertex[i] = false;
    }
    
    // create a contour of boundary vertices in this mesh.
    for (int i = 0; i < thisBoundary.numFaces (); i++)
    {
      Face f = thisBoundary.getFace (i);
      
      for (int j = 0; j < f.numVertices (); j++)
      {
        Vertex v = f.getVertex (j);
        
        if (!thisVertex[v.index])	// hasn't been checked already.
        {
          thisVertex[v.index] = true;
          
          for (int k = 0; k < v.numFaces (); k++)
          {
            Face ff = v.getFace (k);
            if (!thisSeparable[ff.index])	// one adjacent face not in face set, thus boundary vertex.
            {
              thiscontour.Add (v);
              break;
            }
          }
        }
      }
    }
    
    // create a contour of boundary vertices in other mesh. Side effect of copying selection
    // from other mesh to this one. Contour refers to this mesh.
    Vertex [] otherVertex = new Vertex[other.numVertices ()];
    for (int i = 0; i < other.numVertices (); i++)
    {
      otherVertex[i] = null;
    }
    for (int i = 0; i < otherBoundary.numFaces (); i++)
    {
      Face f = otherBoundary.getFace (i);
      
      List <Vertex> nv = new List <Vertex> ();
      for (int j = 0; j < f.numVertices (); j++)
      {
        Vertex v = f.getVertex (j);
        
        if (otherVertex[v.index] == null)	// hasn't been checked already.
        {
          Vertex thisv = addVertex (v.position);
          nv.Add (thisv);
          otherVertex[v.index] = thisv;
          
          for (int k = 0; k < v.numFaces (); k++)
          {
            Face ff = v.getFace (k);
            if (!otherSeparable[ff.index])	// one adjacent face not in face set, thus boundary vertex.
            {
              othercontour.Add (thisv);
              //thisv.colour = new Color (1.0f, 0.0f, 0.0f);
              break;
            }
          }
        }
        else
        {
          nv.Add (otherVertex[v.index]);
        }
      }
      addFace (nv);
    }
    
    // have contours, remove faces from selection in this mesh.
    for (int i = 0; i < thisBoundary.numFaces (); i++)
    {
      Face f = thisBoundary.getFace (i);
      deleteFace (f);
    }
    
    generateVertexIndices ();
    bool [] thisbound = new bool[numVertices ()];
    bool [] otherbound = new bool[numVertices ()];
    for (int i = 0; i < numVertices (); i++)
    {
      thisbound[i] = false;
      otherbound[i] = false;
    }
    for (int i = 0; i < thiscontour.Count; i++)
    {
      //       Debug.Log ("INdex: " +
      //       ((Vertex) thiscontour[i]).index);
      thisbound[((Vertex) thiscontour[i]).index] = true;
    }
    for (int i = 0; i < othercontour.Count; i++)
    {
      otherbound[((Vertex) othercontour[i]).index] = true;
    }
    
    // find closest vertex pairs.
    Vertex v1 = null;
    Vertex v2 = null;
    float mindist = 0.0f;
    for (int i = 0; i < thiscontour.Count; i++)
    {
      Vertex tv = (Vertex) thiscontour[i];
      for (int j = 0; j < othercontour.Count; j++)
      {
        Vertex ov = (Vertex) othercontour[j];
        float dist = (tv.position - ov.position).magnitude;
        if ((v1 == null) || (dist < mindist))
        {
          mindist = dist;
          v1 = tv;
          v2 = ov;
        }
      }
    }
    
    // (v1, v2) is the closest vertex pair, v1 on this boundary, v2 on the other boundary.
    // create new geometry based on shortest edges.
    // Starting at v1 on this boundary, v2 on the other
    // v1next = successor to v1 in appropriate direction.
    // v2next = successor to v2 in appropriate direction.
    //   b = distance (v1, v2next)
    //   c = distance (v2, v1next)
    //  if b is min (b,c)
    //    new poly v1 v2 v2next
    //    v1 = no change
    //    v2 = v2next
    //  if c is min (b,c)
    //    new poly v1 v2 v1next
    //    v1 = v1next
    //    v2 = no change
    
    Vertex v1next;
    Vertex v2next;
    v1next = nextVertexAlongBoundary (v1, null, thisbound);
    v2next = nextVertexAlongBoundary (v2, null, otherbound);
    v2next = nextVertexAlongBoundary (v2, v2next, otherbound);
    
    //v2.colour = new Color (0.0f, 1.0f, 0.0f);
    //v2next.colour = new Color (0.0f, 0.0f, 1.0);
    while ((v1next != null) || (v2next != null))
    {
      //            Debug.Log ("V1 " + v1.index + " v2 " + v2.index + " v1n " + v1next.index + " v2n " + v2next.index);
      float b = 0;
      float c = 0;
      
      if (v2next != null)
        b = (v1.position - v2next.position).magnitude;
      if (v1next != null)
        c = (v2.position - v1next.position).magnitude;
      
      Vertex oldv1 = v1;
      Vertex oldv2 = v2;
      
      if ((v2next != null) && (v1next != null))
      {
        if (b < c)
        {
          // b smallest.
          List <Vertex> nf = new List <Vertex> ();
          nf.Add (v1);
          nf.Add (v2next);
          nf.Add (v2);
          addFace (nf);
          v2 = v2next;
          v2next = nextVertexAlongBoundary (v2next, oldv2, otherbound);
        }
        else
        {
          // c smallest.
          List <Vertex> nf = new List <Vertex> ();
          nf.Add (v1);
          nf.Add (v1next);
          nf.Add (v2);
          addFace (nf);
          v1 = v1next;
          v1next = nextVertexAlongBoundary (v1next, oldv1, thisbound);
        }
      }
      else
      {
        if (v1next == null)
        {
          // run out of v1s
          List <Vertex> nf = new List <Vertex> ();
          nf.Add (v1);
          nf.Add (v2next);
          nf.Add (v2);
          addFace (nf);
          v2 = v2next;
          v2next = nextVertexAlongBoundary (v2next, oldv2, otherbound);
        }
        else
        {
          // run out of v2s
          List <Vertex> nf = new List <Vertex> ();
          nf.Add (v1);
          nf.Add (v1next);
          nf.Add (v2);
          addFace (nf);
          v1 = v1next;
          v1next = nextVertexAlongBoundary (v1next, oldv1, thisbound);
        }
      }
    }
    
    return this;
  }
  
  public PolygonMesh extrude (FaceSet faceList, Vector3 extrudeBy, string partName, FaceSet capFaces)
  {
    List <Vertex> allVertices = new List <Vertex> ();
    
//     Debug.Log ("A");
    FaceSet partSet = null;
    if (partName != null)
    {
      partSet = getFaceSet (partName);
      if (partSet == null)
        partSet = new FaceSet (partName, new List <Face> ());
    }
    for (int i = 0; i < faceList.numFaces (); i++)
    {
      Face currFace = faceList.getFace (i);
      
      for (int j = 0; j < currFace.numVertices (); j++)
      {
        Vertex currVertex = (Vertex) currFace.vertices[j];
        
        bool found = false;
        
        for (int k = 0; k < allVertices.Count; k++)
        {
          Vertex current = (Vertex) allVertices[k];
          
          if (currVertex == current)
            ////This means we have a duplicate entry
          {
            found = true;
            break;
          }
        }
        
        if (!(found))
        {
          allVertices.Add (currVertex);
        }
      }			//end inner for
    }				//end outer for
    
//     Debug.Log ("B");
    ////Keep track of the new vertices added
    Vertex [] newVertices = new Vertex[allVertices.Count];
    
    ////Go through each vertex and extrude it
    for (int i = 0; i < allVertices.Count; i++)
    {
      Vertex currVertex = (Vertex) allVertices[i];
      
      ////Extrude the vertex
      Vector3 newVertex = currVertex.position + extrudeBy;
      
      newVertices[i] = addVertex (newVertex);
    }				//end for
    
    ////Go through each face looking for boundary edges from which to extrude new faces
    for (int i = 0; i < faceList.numFaces (); i++)
    {
      Face currFace = faceList.getFace (i);
      
      ////Keep track of vertex indices in the old vertex list and the new one
      int [] indices = new int[currFace.numVertices ()];
      
      ////For each vertex in the current face, find it's corresponding position in allVertices,
      ////and use these to construct the new faces            
      for (int z = 0; z < currFace.numVertices (); z++)
      {
        Vertex currVertex = (Vertex) currFace.vertices[z];
        
        for (int g = 0; g < allVertices.Count; g++)
        {
          Vertex current = (Vertex) allVertices[g];
          
          if (currVertex == current)
          {
            indices[z] = g;
            break;	//break if we find the vertex, otherwise we are wasting time
          }
        }
      }
      
      ////Traverse all the edges to find which ones are boundaries.
      ////The boundary edges will have faces extruded from them
      for (int j = 0; j < currFace.numEdges (); j++)
      {
        Edge currEdge = (Edge) currFace.edges[j];
        
        int occurrences = 1;
        
        for (int k = 0; k < currEdge.numFaces (); k++)
        {
          Face edgeFace = (Face) currEdge.faces[k];
          
          if (edgeFace == currFace)
          {
            continue;
          }
          else
          {
            for (int l = 0; l < faceList.numFaces (); l++)
            {
              Face currentFace = faceList.getFace (l);
              
              if (currentFace == edgeFace)
              {
                occurrences++;
                break;
              }
            }
          }
        }
        
        if (occurrences < 2)
          ////If the occurrence of this edge is < 2, then we are dealing with a boundary edge
        {
          List <Vertex> t = new List <Vertex> ();
          t.Add (allVertices[indices
          [(j +
          1) % currFace.numVertices ()]]);
          t.Add (newVertices
          [indices[(j + 1) % currFace.numVertices ()]]);
          t.Add (newVertices[indices[j]]);
          t.Add (allVertices[indices[j]]);
          
          Face temp2 = addFace (t);
          temp2.labels = new List <string> (currFace.labels);
          if (partSet != null)
          {
            partSet.addFace (temp2);
          }
        }
      }			//end edge for
//       Debug.Log ("C");
      
      List <Vertex> tt = new List <Vertex> ();
      for (int k = 0; k < currFace.numVertices (); k++)
      {
        tt.Add (newVertices[indices[k]]);
      }
//       Debug.Log ("C1 " + i + " " + faceList.numFaces ());
      
      Face temp = addFace (tt);
      temp.labels = new List <string> (currFace.labels);
      if (partSet != null)
      {
        partSet.addFace (temp);
      }
//       Debug.Log ("C2 " + i + " " + faceList.numFaces ());
      
      if (capFaces != null)
      {
        capFaces.addFace (temp);
      }
    }				//end outer for
    
//     Debug.Log ("C3");
    
    ////Delete the original faces          
    for (int i = 0; i < faceList.numFaces (); i++)
    {
      Face delFace = faceList.getFace (i);
      deleteFace (delFace);
    }
    
//     Debug.Log ("C4");
    
    for (int i = 0; i < numFaceSets (); i++)
    {
      FaceSet temp = getFaceSet (i);
      temp.updateFaceSet (temp.getID (), faces);
    }
    if (partSet != null)
    {
      addFaceSet (partSet);
    }
//     Debug.Log ("D");
    
    return this;
  }
  
  public Vector3 projectPoint (Vector3 point, Vector3 projectionDirection)
  {
    Vector3 vector =
    new Vector3 (point[0], point[1], point[2]);
    
    float dot = Vector3.Dot (vector, projectionDirection);
    
    Vector3 projected = vector - (dot * projectionDirection);
    
    return new Vector3 (projected[0], projected[1],
                        projected[2]);
  }
  
  public Vector3 projectVector (Vector3 direction,
                                Vector3 projectionDirection)
  {
    float dot = Vector3.Dot (direction, projectionDirection);
    
    Vector3 projected = direction - (dot * projectionDirection);
    
    return projected;
  }
  
  // returns the ray parameters of the point of intersection in
  // a two element array, the first element being the parameter
  // for this ray, the second for ray r.
  
  // assumes that they do intersect, and have been projected into the
  // same plane.
  public float [] intersect (Ray p, Ray r)
  {
    float [] res = new float [2];
    float den1 = (p.direction[0] * r.direction[1] - p.direction[1] * r.direction[0]);
    //Debug.Log ("Den 1 " + den1);
    if (den1 == 0.0)
    {
      // go to plan b. 
      den1 = (p.direction[0] * r.direction[2] - p.direction[2] * r.direction[0]);
      //Debug.Log ("Den 1b " + den1);
      if (den1 == 0.0)
      {
        // go to plan c
        den1 = (p.direction[1] * r.direction[2] - p.direction[2] * r.direction[1]);
        if (den1 == 0.0)
        {
          Debug.Log ("Rays do not seem to intersect in 2D: " + p + " with " + r);
          return null;
        }
        else
        {        
          res[0] = ((r.origin[1] - p.origin[1]) * r.direction[2] -
          (r.origin[2] - p.origin[2]) * r.direction[1]) / den1;
        }
      }
      else
      {        
        res[0] = ((r.origin[0] - p.origin[0]) * r.direction[2] -
        (r.origin[2] - p.origin[2]) * r.direction[0]) / den1;
      }
    }
    else
    {
      res[0] = ((r.origin[0] - p.origin[0]) * r.direction[1] -
      (r.origin[1] - p.origin[1]) * r.direction[0]) / den1;
    }  
    
    if (r.direction[0] != 0.0)
    {
      res[1] = (p.origin[0] + res[0] * p.direction[0] - r.origin[0]) / r.direction[0];
    }
    else
    {
      if (r.direction[1] != 0.0)
      {
        res[1] = (p.origin[1] + res[0] * p.direction[1] - r.origin[1]) / r.direction[1];
      }
      else
      {
        if (r.direction[2] != 0.0)
        {
          res[1] = (p.origin[2] + res[0] * p.direction[2] - r.origin[2]) / r.direction[2];
        }
        else
        {
          Debug.Log ("Total stuffup - ray intersection with no direction (A)");
        }
      }
    }
    
    return res;  
  }
  
  
  public Vector3 getDisplacement (Curve projectedCurve, Vector3 p,
                                  Vector3 d, Vector3 projectionDirection)
  {
    Vector3 offSet = new Vector3 ();
    
    float min_t = 0.0f;
    float max_t = 1.0f;
    Vector3 minPoint = projectedCurve.curveAt (min_t).position;
    Vector3 maxPoint = projectedCurve.curveAt (max_t).position;
    
    bool found = false;
    
    Ray ray =
    new Ray (new Vector3 (0.0f, 0.0f, 0.0f), new Vector3 (0.0f, 0.0f, 0.0f));
    Ray curveraymin =
    new Ray (new Vector3 (0.0f, 0.0f, 0.0f), new Vector3 (0.0f, 0.0f, 0.0f));
    Ray curveraymax =
    new Ray (new Vector3 (0.0f, 0.0f, 0.0f), new Vector3 (0.0f, 0.0f, 0.0f));
    
    while (max_t - min_t > 0.02)
    {
      float mid_t = (min_t + max_t) / 2.0f;
      Vector3 midPoint = projectedCurve.curveAt (mid_t).position;
      
      float [] sumin = new float[2];
      float [] sumax = new float[2];
      
      /*Ray ray = new Ray (p, d);
       * Ray curveraymin = new Ray (minPoint, Vector3.subtract (midPoint, minPoint));
       * Ray curveraymax = new Ray (maxPoint, Vector3.subtract (midPoint, maxPoint)); */
      
      ray = new Ray (p, d);
      curveraymin = new Ray (minPoint, midPoint - minPoint);
      curveraymax = new Ray (maxPoint, midPoint - maxPoint);
      float curveminlen = (midPoint - minPoint).magnitude;
      float curvemaxlen = (midPoint - maxPoint).magnitude;
      
      sumin = intersect (ray, curveraymin);
      sumax = intersect (ray, curveraymax);
      
      float lineDistMin = sumin[1] / curveminlen;
      float lineDistMax = sumax[1] / curvemaxlen;
      float rayDistMin = sumin[0] / curveminlen;
      float rayDistMax = sumax[0] / curvemaxlen;
      
      if (lineDistMin >= 0.0 && lineDistMin <= 1.0)
      {
        if (lineDistMax >= 0.0 && lineDistMax <= 1.0)
        {
          if (rayDistMin < rayDistMax)
          {
            max_t = mid_t;
            maxPoint = midPoint;
          }
          else
          {
            min_t = mid_t;
            minPoint = midPoint;
          }
        }
        else
        {
          max_t = mid_t;
          maxPoint = midPoint;
        }
      }
      else
      {
        if (lineDistMax >= 0.0 && lineDistMax <= 1.0)
        {
          min_t = mid_t;
          minPoint = midPoint;
        }
        else
        {
          //Debug.Log("WTF");
          //Debug.Log("Ray: " + ray + " \n::: RayMin " + curveraymin + " \n::: RayMax " + curveraymax);
          return new Vector3 ();
        }
      }
      
      //Debug.Log("SUCCESS!!!!!!!!!!!!!" + " Ray: " + ray + " \n::: RayMin " + curveraymin + " \n::: RayMax " + curveraymax);
    }
    
    //Debug.Log("\n\n\n\nSUCCESS!!!!!!!!!!!!!" + " Ray: " + ray + " \n::: RayMin " + curveraymin + " \n::: RayMax " + curveraymax);
    
    Vector3 displacement = minPoint - p;
    
    return displacement;
  }
  
  public void shapeToCurve (FaceSet faces, Curve curve, Vector3 direction,
                            Vector3 projectionDirection)
  {
    ///Ensure that we are using a normalized projection plane.
    projectionDirection = projectionDirection.normalized;
    
    ////Start by projecting all of the control points onto our projection plane.  
    ////This only needs to be done once per method call
    CurvePoint [] projectedPoints = new CurvePoint[curve.numberControlPoints ()];
    
    for (int i = 0; i < projectedPoints.Length; i++)
    {
      projectedPoints[i] =
      new CurvePoint (projectPoint (curve.getControlPoint (i).position, projectionDirection), 0.0f);
    }
    
    ////Create a new curve that has already been projected onto our projection plane.
    Curve projectedCurve =
    new BezierSpline (projectedPoints.Length - 1, projectedPoints, 0);
    
    ////Project our direction vector onto our projection plane.  This also only needs to 
    ////be done once.
    direction = projectVector (direction, projectionDirection);
    direction = direction.normalized;
    
    ////Start working with the faces in the faceset.  To begin with, we are going to traverse
    ////each vertex of a face, and displace it.  This can be extended to work on the face...
    
    ////Find all duplicate vertices and remove them.  This way we are only displacing each vertex
    ////once
    List <Vertex> allVertices = new List <Vertex> (numVertices ());
    
    for (int i = 0; i < faces.numFaces (); i++)
    {
      Face currFace = faces.getFace (i);
      
      for (int j = 0; j < currFace.numVertices (); j++)
      {
        Vertex currVertex = currFace.getVertex (j);
        
        bool found = false;
        
        for (int k = 0; k < allVertices.Count; k++)
        {
          Vertex current = (Vertex) allVertices[k];
          
          if (currVertex == current)
            ////This means we have a duplicate entry
          {
            found = true;
            break;
          }
        }
        
        if (!(found))
        {
          allVertices.Add (currVertex);
        }
      }
    }
    
    
    for (int j = 0; j < allVertices.Count; j++)
    {
      Vertex currVertex = (Vertex) allVertices[j];
      
      Vector3 projectedPoint =
      projectPoint (currVertex.position, projectionDirection);
      
      Vector3 displacement =
      getDisplacement (projectedCurve, projectedPoint, direction,
                       projectionDirection);
      
      currVertex.position = currVertex.position + displacement;

    }
  }
  
  // Returns a list of faces that fall within the bounds given.
  public FaceSet select (float ratiominx, float ratiomaxx,
                         float ratiominy, float ratiomaxy,
                         float ratiominz, float ratiomaxz,
                         FaceSet currentSet, string selectionName)
  {
    BoundingBox bb = new BoundingBox (currentSet);
    
    List <Face> selection = new List <Face> ();
    if (currentSet != null)
    {
      for (int i = 0; i < currentSet.numFaces (); i++)
      {
        Face currFace = currentSet.getFace (i);
        
        bool found = true;
        
        for (int j = 0; j < currFace.numVertices (); j++)
        {
          Vertex currVertex = (Vertex) currFace.vertices[j];
          
          float x =
          (currVertex.position[0] - bb.min[0]) / (bb.max[0] -
          bb.min[0]);
          float y =
          (currVertex.position[1] - bb.min[1]) / (bb.max[1] -
          bb.min[1]);
          float z =
          (currVertex.position[2] - bb.min[2]) / (bb.max[2] -
          bb.min[2]);
          if (!
            ((x >= ratiominx) && (x <= ratiomaxx) && (y >= ratiominy)
            && (y <= ratiomaxy) && (z >= ratiominz)
            && (z <= ratiomaxz)))
          {
            found = false;
            break;
          }
        }
        
        if (found)
        {
          selection.Add (currFace);
        }
      }
    }
    FaceSet selectedSet = new FaceSet (selectionName, selection);
    return selectedSet;
  }
  
  public Vector3 sqrt2AdjustVertex (Vertex currVertex)
  /// newVertex = (1 - alpha)*currVertex + (1/valence)*Summation(alpha*adjVertex)
  {
    Vector3 result = new Vector3 ();
    
    int valence = currVertex.numEdges ();
    
    float alpha =
    (0.5f) * (1.0f - Mathf.Cos ((2.0f * Mathf.PI) / (float) valence));
    
    for (int i = 0; i < valence; i++)
    {
      Vertex adjVertex = new Vertex ();
      Edge currEdge = (Edge) currVertex.edges[i];
      
      Vertex edgeEnd = currEdge.otherEnd (currVertex);
      result = 1.0f * result + alpha * edgeEnd.position;
    }
    
    result = (1 - alpha) * currVertex.position + (1.0f / (float) valence) * result;
    return result;
  }
  
  public Vector3 sqrt2AdjustOddVertex (Vertex currVertex)
  /// 1/8 (2 otherends) + 6/8 currVertex. 
  {
    Vector3 result = new Vector3 ();
    
    int count = 0;
    for (int p = 0; p < currVertex.numEdges (); p++)
    {
      Edge currEdge = (Edge) currVertex.edges[p];
      if (currEdge.isBoundary ())
      {
        Vertex edgeEnd = currEdge.otherEnd (currVertex);
        result = 1.0f * result + 1.0f * edgeEnd.position;
        count++;
      }
    }
    
    if (count != 2)
    {
      //       Debug.Log ("Warning: consistency check failed - " + count +
      //       " boundary edges from a vertex. " + currVertex);
    }
    
    result = (6.0f / 8.0f) * currVertex.position + (1.0f / 8.0f) * result;
    return result;
  }
  
  public Vector3 sqrt2AdjustEvenVertex (Vertex currVertex)
  /// 1/4 (2 otherends) + 1/2 currVertex. 
  {
    Vector3 result = new Vector3 ();
    
    int count = 0;
    for (int p = 0; p < currVertex.numEdges (); p++)
    {
      Edge currEdge = (Edge) currVertex.edges[p];
      if (currEdge.isBoundary ())
      {
        Vertex edgeEnd = currEdge.otherEnd (currVertex);
        result = 1.0f * result + 1.0f * edgeEnd.position;
        count++;
      }
    }
    
    if (count != 2)
    {
      //       Debug.Log ("Warning: consistency check failed - " + count +
      //       " boundary edges from a vertex.");
    }
    
    result = (1.0f / 2.0f) * currVertex.position + (1.0f / 4.0f) * result;
    return result;
  }
  
  public PolygonMesh sqrt2Subdivide (int refineLevel, float smoothness)
  {
    PolygonMesh orgPolygonMesh = this;
    
    for (int l = 1; l <= refineLevel; l++)
    {
      /// Step 1: allocate memory for defining vertices and faces.
      PolygonMesh subPolygonMesh = new PolygonMesh ();
      
      /// Step 2: establish intermediate data structure for retrieving
      /// elements.
      orgPolygonMesh.generateFaceIndices ();
      orgPolygonMesh.generateEdgeIndices ();
      orgPolygonMesh.generateVertexIndices ();
      
      /// Step 3: Generate new vertices for the mesh.
      /// Step 3.1 Compute the V vertices, by adjusting the positions of the old vertices
      Vertex [] V = new Vertex[orgPolygonMesh.numVertices ()];
      for (int i = 0; i < orgPolygonMesh.numVertices (); i++)
      {
        Vertex v = (Vertex) orgPolygonMesh.vertices[i];
        
        /// Check whether it is an boundary vertex. Are any of its 
        /// connected edges a boundary.
        bool boundary = false;
        for (int p = 0; p < v.numEdges (); p++)
        {
          Edge currEdge = (Edge) v.edges[p];
          if (currEdge.isBoundary ())
          {
            boundary = true;
            break;
          }
        }
        
        Vector3 newVert;
        if (boundary)
        {
          if ((l % 2) == 0)	// even
          {
            newVert = sqrt2AdjustEvenVertex (v);
            newVert = smoothness * newVert + (1 - smoothness) * v.position;
          }
          else		// odd
          {
            newVert = sqrt2AdjustOddVertex (v);
            newVert = smoothness * newVert + (1 - smoothness) * v.position;
          }
        }
        else
        {
          newVert = orgPolygonMesh.sqrt2AdjustVertex (v);
          newVert = smoothness * newVert + (1 - smoothness) * v.position;
        }
        
        V[i] = subPolygonMesh.addVertex (newVert);
        V[i].colour = v.colour;
        V[i].orientation = v.orientation;
        V[i].orientationFixed = v.orientationFixed;
        V[i].sweepDistance = v.sweepDistance;
        V[i].sweepAssigned = v.sweepAssigned;
      }
      
      /// Step 3.2 Compute the F vertices, by obtaining the centroid of each face
      Vertex [] F = new Vertex[orgPolygonMesh.numFaces ()];
      for (int i = 0; i < orgPolygonMesh.numFaces (); i++)
      {
        Face currFace = (Face) orgPolygonMesh.faces[i];
        bool hasBoundary = false;
        
        for (int k = 0; k < currFace.numEdges (); k++)
        {
          Edge currEdge = (Edge) currFace.edges[k];
          
          hasBoundary = hasBoundary || currEdge.isBoundary ();
        }
        
        if ((l % 2) == 0 && hasBoundary)
        {
          // find the vertex of valence 2.
          for (int q = 0; q < currFace.numVertices (); q++)
          {
            Vertex v = (Vertex) currFace.vertices[q];
            if (v.numEdges () == 2)
            {
              F[i] = V[v.index];
            }
          }
          
          if (F[i] == null)
          {
            Debug.Log ("Could not find vertex of valence 2.");
          }
        }
        else
        {
          /// the else clause for (even step and boundary)
          Vector3 newVert = currFace.calcCenter ();
          F[i] = subPolygonMesh.addVertex (newVert);
          
        }
      }
      
      /// Step 3.3 Compute E vertices.
      Vertex [] E = null;
      if ((l % 2) != 0)	// odd refinement step.
      {
        E = new Vertex[orgPolygonMesh.numEdges ()];
        for (int i = 0; i < orgPolygonMesh.numEdges (); i++)
        {
          Edge currEdge = (Edge) orgPolygonMesh.edges[i];
          
          if (currEdge.isBoundary ())
          {
            Vector3 newVert = currEdge.calcMid ();
            E[i] = subPolygonMesh.addVertex (newVert);
          }
        }
      }
      
      /// Step 4: Generate new faces.
      
      /// mark edges as unused, initially.                    
      bool [] edgeused = new bool[orgPolygonMesh.numEdges ()];
      
      for (int m = 0; m < orgPolygonMesh.numEdges (); m++)
      {
        edgeused[m] = false;
      }
      
      /// effectively iterate over all edges.  
      for (int j = 0; j < orgPolygonMesh.numFaces (); j++)
      {
        Face currFace = (Face) orgPolygonMesh.faces[j];
        
        for (int i = 0; i < currFace.numVertices (); i++)
        {
          int estart = ((Vertex) currFace.vertices[i]).index;
          int eend =
          ((Vertex) currFace.vertices[(i + 1) % currFace.numVertices ()]).index;
          
          Edge currEdge = (Edge) currFace.edges[i];
          
          if (!(edgeused[currEdge.index]))
          {
            List <Vertex> tempVertices;
            tempVertices = null;
            
            // if odd, or (even and not boundary)
            if (((l % 2) != 0)
              || (((l % 2) == 0) && !(currEdge.isBoundary ())))
            {
              if (!(currEdge.isBoundary ()))
              {
                tempVertices = new List <Vertex> ();
                
                int adjFace = currEdge.getAdjFace (currFace).index;
                
                tempVertices.Add (V[estart]);
                tempVertices.Add (F[adjFace]);
                tempVertices.Add (V[eend]);
                tempVertices.Add (F[j]);
                
//                 Debug.Log ("Add F " + V[estart] + " - " + F[adjFace] + " - " + V[eend] + " - " + F[j] + " - " + adjFace);
                Face outf = subPolygonMesh.addFace (tempVertices);
                outf.labels = new List <string> (currFace.labels);
              }
              else
              {
                tempVertices = new List <Vertex> ();
                
                tempVertices.Add (V[estart]);
                tempVertices.Add (E[currEdge.index]);
                tempVertices.Add (V[eend]);
                tempVertices.Add (F[j]);
                
                Face outf = subPolygonMesh.addFace (tempVertices);
                outf.labels = new List <string> (currFace.labels);
              }
              
            }
          }
          
          edgeused[currEdge.index] = true;
        }
      }
      
      
      for (int i = 0; i < orgPolygonMesh.numFaceSets (); i++)
      {
        FaceSet temp = orgPolygonMesh.getFaceSet (i);
        FaceSet newSet = new FaceSet (temp.getID (), temp.getID (), subPolygonMesh.faces);
        subPolygonMesh.addFaceSet (newSet);
      }
      
      orgPolygonMesh = subPolygonMesh;
      orgPolygonMesh.vertices.TrimExcess ();
      orgPolygonMesh.edges.TrimExcess ();
      orgPolygonMesh.faces.TrimExcess ();
    }
    return orgPolygonMesh;
  }
  
  public PolygonMesh sqrt2AdaptiveSubdivide (FaceSet separableFaces, int refineLevel,
                                             float smoothness)
  {
    // No vertex is inserted into a fixed face.
    // If a boundary edge belongs to a fixed face, no vertex is added to the edge.
    // Face creation:
    //   - If an edge is adjacent to two fixed faces, no new face is generated.
    //   - If an edge is adjacent to one fixed face, then a triangle is generated.
    
    PolygonMesh orgPolygonMesh = this;
    FaceSet orgSeparable = separableFaces;
    
    for (int l = 1; l <= refineLevel; l++)
    {
      /// Step 0: establish intermediate data structure for retrieving
      /// elements.
      orgPolygonMesh.generateFaceIndices ();
      orgPolygonMesh.generateEdgeIndices ();
      orgPolygonMesh.generateVertexIndices ();
      
      bool [] separable = new bool[orgPolygonMesh.numFaces ()];
      
      /// Step 1: Tag faces according to whether they are separable or not.
      for (int i = 0; i < orgPolygonMesh.numFaces (); i++)
      {
        separable[i] = false;
      }
      for (int i = 0; i < orgSeparable.numFaces (); i++)
      {
        Face f = orgSeparable.getFace (i);
        separable[f.index] = true;	// true if separable.
      }
      
      /// Step 2: allocate memory for defining vertices and faces.
      PolygonMesh subPolygonMesh = new PolygonMesh ();
      FaceSet subSeparable = new FaceSet ();
      
      /// Step 3: Generate new vertices for the mesh.
      /// Step 3.1 Compute the V vertices, by adjusting the positions of the old vertices
      Vertex [] V = new Vertex[orgPolygonMesh.numVertices ()];
      for (int i = 0; i < orgPolygonMesh.numVertices (); i++)
      {
        Vertex v = (Vertex) orgPolygonMesh.vertices[i];
        
        // Modify vertex if ALL adjacent faces are separable.
        // FIXME: Can optimize by extracting vertices only in separable faces.
        bool vertexSeparable = true;
        for (int k = 0; k < v.numFaces (); k++)
        {
          Face f = (Face) v.faces[k];
          if (!separable[f.index])
          {
            vertexSeparable = false;
            break;
          }
        }
        
        if (vertexSeparable)
        {
          /// Check whether it is an boundary vertex. Are any of its 
          /// connected edges a boundary.
          bool boundary = false;
          for (int p = 0; p < v.numEdges (); p++)
          {
            Edge currEdge = (Edge) v.edges[p];
            if (currEdge.isBoundary ())
            {
              boundary = true;
              break;
            }
          }
          
          Vector3 newVert;
          if (boundary)
          {
            if ((l % 2) == 0)	// even
            {
              newVert = sqrt2AdjustEvenVertex (v);
              newVert = smoothness * newVert + (1 - smoothness) * v.position;
            }
            else	// odd
            {
              newVert = sqrt2AdjustOddVertex (v);
              newVert = smoothness * newVert + (1 - smoothness) * v.position;
            }
          }
          else
          {
            newVert = orgPolygonMesh.sqrt2AdjustVertex (v);
            newVert = smoothness * newVert + (1 - smoothness) * v.position;
          }
          
          V[i] = subPolygonMesh.addVertex (newVert);
          //                            V[i].colour = new Color (1.0f, 0.0f, 1.0);
        }
        else
        {
          V[i] = subPolygonMesh.addVertex (v.position);
          //                            V[i].colour = new Color (1.0f, 0.0f, 0.0f);
        }
        V[i].colour = v.colour;
        V[i].orientation = v.orientation;
        V[i].orientationFixed = v.orientationFixed;
        V[i].sweepDistance = v.sweepDistance;
        V[i].sweepAssigned = v.sweepAssigned;
      }
      
      /// Step 3.2 Compute the F vertices, by obtaining the centroid of each face
      Vertex [] F = new Vertex[orgPolygonMesh.numFaces ()];
      for (int i = 0; i < orgPolygonMesh.numFaces (); i++)
      {
        Face currFace = (Face) orgPolygonMesh.faces[i];
        
        if (separable[currFace.index])
        {
          bool hasBoundary = false;
          
          for (int k = 0; k < currFace.numEdges (); k++)
          {
            Edge currEdge = (Edge) currFace.edges[k];
            
            hasBoundary = hasBoundary || currEdge.isBoundary ();
          }
          
          if ((l % 2) == 0 && hasBoundary)
          {
            // find the vertex of valence 2.
            for (int q = 0; q < currFace.numVertices (); q++)
            {
              Vertex v = (Vertex) currFace.vertices[q];
              if (v.numEdges () == 2)
              {
                F[i] = V[v.index];
                F[i].colour = new Color (1.0f, 1.0f, 0.0f);
              }
            }
            
            if (F[i] == null)
            {
              //               System.err.
              //               println ("Could not find vertex of valence 2.");
            }
          }
          else
          {
            /// the else clause for (even step and boundary)
            Vector3 newVert = currFace.calcCenter ();
            
            newVert = newVert;
            
            F[i] = subPolygonMesh.addVertex (newVert);
          }
        }
        else
        {
          F[i] = null;
        }
      }
      
      /// Step 3.3 Compute E vertices.
      Vertex [] E = null;
      if ((l % 2) != 0)	// odd refinement step.
      {
        E = new Vertex[orgPolygonMesh.numEdges ()];
        
        for (int i = 0; i < orgPolygonMesh.numEdges (); i++)
        {
          Edge currEdge = (Edge) orgPolygonMesh.edges[i];
          
          if (currEdge.isBoundary ())
          {
            Vector3 newVert = currEdge.calcMid ();
            newVert = newVert;
            E[i] = subPolygonMesh.addVertex (newVert);
          }
        }
      }
      
      /// Step 4: Generate new faces.
      /// mark edges as unused, initially.                    
      bool [] edgeused = new bool[orgPolygonMesh.numEdges ()];
      
      for (int m = 0; m < orgPolygonMesh.numEdges (); m++)
      {
        edgeused[m] = false;
      }
      
      /// effectively iterate over all edges.  
      for (int j = 0; j < orgPolygonMesh.numFaces (); j++)
      {
        Face currFace = (Face) orgPolygonMesh.faces[j];
        List <Vertex> tempVertices;
        
        if (!separable[currFace.index])
        {
          tempVertices = new List <Vertex> ();
          for (int i = 0; i < currFace.numVertices (); i++)
          {
            int v = ((Vertex) currFace.vertices[i]).index;
            tempVertices.Add (V[v]);
          }
          Face outf = subPolygonMesh.addFace (tempVertices);
          outf.labels = new List <string> (currFace.labels);
        }
        else
        {
          // currFace is separable.
          for (int i = 0; i < currFace.numVertices (); i++)
          {
            int estart =
            ((Vertex) currFace.vertices[i]).index;
            int eend =
            ((Vertex) currFace.vertices[(i + 1) % currFace.numVertices ()]).index;
            
            Edge currEdge = (Edge) currFace.edges[i];
            
            //Debug.Log(currEdge + " " + currEdge.isBoundary());
            
            if (!(edgeused[currEdge.index]))
            {
              // An edge, which hasn't been tested before.
              
              // Check number of adjacent fixed faces.
              Face otherFace = currEdge.getAdjFace (currFace);
              
              // FIXME: check for case of no other face.
              if (!currEdge.isBoundary ())
              {
                if (!separable[otherFace.index])
                {
                  // add in triangle.
                  tempVertices = new List <Vertex> ();
                  
                  tempVertices.Add (V[estart]);
                  tempVertices.Add (V[eend]);
                  tempVertices.Add (F[j]);
                  
                  Face outf = subPolygonMesh.addFace (tempVertices);
                  outf.labels = new List <string> (currFace.labels);
                }
                else
                {
                  // both faces are separable.
                  tempVertices = null;
                  
                  // if odd, or (even and not boundary)
                  if (((l % 2) != 0)
                    || (((l % 2) == 0)
                    && !(currEdge.isBoundary ())))
                  {
                    if (!(currEdge.isBoundary ()))
                    {
                      tempVertices = new List <Vertex> ();
                      
                      int adjFace = otherFace.index;
                      
                      tempVertices.Add (V[estart]);
                      tempVertices.Add (F[adjFace]);
                      tempVertices.Add (V[eend]);
                      tempVertices.Add (F[j]);
                      
                      Face outf =
                      subPolygonMesh.addFace (tempVertices);
                      outf.labels = new List <string> (currFace.labels);
                      subSeparable.addFace (outf);
                    }
                    else
                    {
                      //                       System.err.
                      //                       println
                      //                       ("Adaptive Subdivision does not cater for this!");
                      
                      tempVertices = new List <Vertex> ();
                      
                      tempVertices.Add (V[estart]);
                      tempVertices.Add (E[currEdge.index]);
                      tempVertices.Add (V[eend]);
                      tempVertices.Add (F[j]);
                      
                      Face outf =
                      subPolygonMesh.addFace (tempVertices);
                      outf.labels = new List <string> (currFace.labels);
                      subSeparable.addFace (outf);
                    }
                  }
                }
              }
              //////causing trouble!!!!!!!!!!!!!!!!!!!!!
              else
              {
                //Debug.Log("I am in the famous else! " + currEdge);
                tempVertices = new List <Vertex> ();
                
                tempVertices.Add (V[estart]);
                tempVertices.Add (E[currEdge.index]);
                tempVertices.Add (V[eend]);
                tempVertices.Add (F[j]);
                
                
                //                 Debug.Log (tempVertices.Count);
                //                 
                //                 for (int z = 0; z < tempVertices.Count; z++)
                //                 {
                //                   Debug.Log (z + " " +
                //                   (Vertex) tempVertices.
                //                   elementAt (z));
                //                 }
                
                Face outf = subPolygonMesh.addFace (tempVertices);
                outf.labels = new List <string> (currFace.labels);
              }
              
              
            }
            edgeused[currEdge.index] = true;
          }
        }
      }
      
      for (int i = 0; i < orgPolygonMesh.numFaceSets (); i++)
      {
        FaceSet temp = orgPolygonMesh.getFaceSet (i);
        FaceSet newSet =
        new FaceSet (temp.getID (), temp.getID (), subPolygonMesh.faces);
        subPolygonMesh.addFaceSet (newSet);
      }
      
      orgPolygonMesh = subPolygonMesh;
      orgSeparable = subSeparable;
      orgPolygonMesh.vertices.TrimExcess ();
      orgPolygonMesh.edges.TrimExcess ();
      orgPolygonMesh.faces.TrimExcess ();
    }
    return orgPolygonMesh;
  }
  
  public Vertex addVertex (Vector3 position)
  {
    Vertex newVertex = new Vertex (position);
    vertices.Add (newVertex);
    
    return newVertex;
  }
  
  public int getVertexIndex (Vertex v)
  {
    
    for (int i = 0; i < vertices.Count; i++)
    {
      if (vertices[i] == v)
        return i;
    }
    return 0;
  }
  
  public void addEdges (List <Vertex> currVertices, Face currFace)
  {
    int vecSize = currVertices.Count;
    
    for (int i = 0; i < vecSize; i++)
    {
      Vertex start = (Vertex) currVertices[i];
      Vertex end = (Vertex) currVertices[(i + 1) % vecSize];
      Edge newEdge = createEdge (start, end);
      
      /*Update the Vertices involved in the Edge creation process */
      start.addEdge (newEdge);
      end.addEdge (newEdge);
      newEdge.faces.Add (currFace);
      currFace.edges.Add (newEdge);
    }
  }
  
  public Edge createEdge (Vertex start, Vertex end)
  // If an edge already exists, the reference to the existing edge will be returned.
  {
    Edge newEdge = new Edge (start, end);
    
    bool found = false;
    
    // If the edge already exists, it must be in the
    // edge lists for both end vertices.
    // Just check in the list for one. This should be
    // quick, since most vertices will have only a
    // few edges.
    for (int i = 0; i < start.numEdges (); i++)
    {
      Edge currEdge = (Edge) start.edges[i];
      
      if (currEdge.sameEdge (newEdge))
      {
        found = true;
        newEdge = currEdge;
        break;
      }
    }
    
    if (!found)
    {
      edges.Add (newEdge);
    }
    return newEdge;
  }
  
  public Face addFace (List <Vertex> currVertices)
  // We need to know the number of vertices for the face, as well as what these vertices are.
  {
    Face newFace = new Face (currVertices);
    faces.Add (newFace);
    
    // Update the vertices involved in the creation of the face.
    for (int i = 0; i < currVertices.Count; i++)
    {
      Vertex currVertex = (Vertex) currVertices[i];
      currVertex.addFace (newFace);
    }
    // Update all edges involved in the creation of the face.
    addEdges (currVertices, newFace);
    
    return newFace;
  }
  
  public void addMesh (PolygonMesh mesh)
  {
    //Add the vertices from mesh to the current mesh
    for (int i = 0; i < mesh.numVertices (); i++)
    {
      vertices.Add (mesh.vertices[i]);
    }
    
    //Add the faces from mesh to the current mesh
    for (int i = 0; i < mesh.numFaces (); i++)
    {
      faces.Add (mesh.faces[i]);
    }
    
    //Add the edges from mesh to the current mesh
    for (int i = 0; i < mesh.numEdges (); i++)
    {
      edges.Add (mesh.edges[i]);
    }
    
    generateFaceIndices ();
    generateEdgeIndices ();
    generateVertexIndices ();
  }
  
  public string tostring ()
  {
    string information = "";
    
    for (int i = 0; i < numFaces (); i++)
    {
      Face currFace = (Face) faces[i];
      
      information += "Face " + i + " " + currFace.tostring () + "\n";
      
      //Cater for Vertices
      for (int j = 0; j < currFace.numVertices (); j++)
      {
        Vertex currVertex = (Vertex) currFace.vertices[j];
        information +=
        "Vertex " + j + " " + currVertex.tostring () + "\n";
      }
      
      //Cater for Edges
      for (int k = 0; k < currFace.numEdges (); k++)
      {
        Edge currEdge = (Edge) currFace.edges[k];
        information += "Edge " + k + " " + currEdge.tostring () + "\n";
      }
      
    }
    
    return information;
  }

  private void resetLocal (int [] vertIndexMap, ref int currentVert, ref int tri)
  {
    currentVert = 0;
    tri = 0;
    for (int i = 0; i < vertIndexMap.Length; i++)
    {
      vertIndexMap[i] = -1;
    }
  }
  
  private void addLocalTriangle (int t, int index, int [] vertIndexMap, Vector3 [] verts, Vector3 [] norms, ref int currentVert, int [] triangles)
  {
    if (vertIndexMap[index] < 0)
    {
      vertIndexMap[index] = currentVert;
      verts[currentVert] = vertices[index].position;
      norms[currentVert] = vertices[index].normal;
      currentVert++;
    }
    
    triangles[t] = vertIndexMap[index];
  }

  private void generateLocal (int [] triangles, Vector3 [] verts, Vector3 [] normals, int currentVert, int tri, GameObject parent, GameObject template)
  {
    if (tri <= 0)
    {
      return;
    }
    
    Vector3 [] v = new Vector3 [currentVert];
    Vector3 [] n = new Vector3 [currentVert];
    int [] t = new int [tri];
    
    for (int i = 0; i < currentVert; i++)
    {
      v[i] = verts[i];
      n[i] = normals[i];
    }
    for (int i = 0; i < tri; i++)
    {
      t[i] = triangles[i];
    }
    
    GameObject go = GameObject.Instantiate (template, parent.transform);
    Mesh mesh = go.GetComponent <MeshFilter> ().mesh;
    mesh.Clear (false);
    mesh.vertices = v;
    mesh.normals = n;
    mesh.triangles = t;
    // mesh.uv = uv;
    //mesh.RecalculateNormals ();
  }
  
  public void updateMesh (GameObject parent, GameObject template)
  {
    // may go slightly over this to fill a triangle, so leave some slack.
    int maxVerticesPerObject = 20000;
    
    generateVertexIndices ();

    // Count maximum number of triangles.
    int numTris = 0;
    for (int i = 0; i < numFaces (); i++)
    {
      numTris += Math.Max (0, faces[i].vertices.Count - 2);
    }
    
    // Create objects, breaking them when the triangle limit is reached.
    Vector3 [] verts = new Vector3 [numVertices ()];
    Vector3 [] norms = new Vector3 [numVertices ()];
    int [] vertIndexMap = new int [numVertices ()];
    int [] triangles = new int [numTris * 3];
    
    int currentFace = 0;
    int currentVert = 0;
    int tri = 0;
    resetLocal (vertIndexMap, ref currentVert, ref tri);
    while (currentFace < numFaces ())
    {
      for (int t = 0; t < faces[currentFace].vertices.Count - 2; t++)
      {        
        addLocalTriangle (tri + 0, faces[currentFace].vertices[0].index, vertIndexMap, verts, norms, ref currentVert, triangles);
        addLocalTriangle (tri + 1, faces[currentFace].vertices[t + 1].index, vertIndexMap, verts, norms, ref currentVert, triangles);
        addLocalTriangle (tri + 2, faces[currentFace].vertices[t + 2].index, vertIndexMap, verts, norms, ref currentVert, triangles);
        tri += 3;
      }

      currentFace++;
      
      if (currentVert > maxVerticesPerObject)
      {
        generateLocal (triangles, verts, norms, currentVert, tri, parent, template);
        resetLocal (vertIndexMap, ref currentVert, ref tri);
      }
    }
      
    generateLocal (triangles, verts, norms, currentVert, tri, parent, template);
    
    
//     for (int i = 0; i < numVertices (); i++)
//     {
//       verts[i] = vertices[i].position;
//     }
//     
//     int numTris = 0;
//     for (int i = 0; i < numFaces (); i++)
//     {
//       numTris += Math.Max (0, faces[i].vertices.Count - 2);
//     }
//     
//     /*        
//      *        Vector2 [] uv = new Vector2[(meshx + 1) * (meshy + 1)];
//      *        for (i = 0; i <= meshx; i++)
//      *        {
//      *            for (j = 0; j <= meshy; j++)
//      *            {
//      *				float xp = 1.0f * (i - (meshx / 2.0f)) / meshx;   
//      *				float yp = 1.0f * (j - (meshy / 2.0f)) / meshy;
//      *				float u = Mathf.PerlinNoise (patternFrequency * (xp + seed), patternFrequency * (yp + seed));
//      *				float v = 0.3f * (vertices[indexOfVertex (i, j, meshx, meshy)].y - 0.2f);
//      *                uv[indexOfVertex (i, j, meshx, meshy)] = new Vector2 (u, v);
//   }
//   }*/
//     
//     int [] triangles = new int [numTris * 3];
//     int tri = 0;
//     for (int i = 0; i < numFaces (); i++)
//     {
//       for (int t = 0; t < faces[i].vertices.Count - 2; t++)
//       {        
//         triangles[tri + 0] = faces[i].vertices[0].index;
//         triangles[tri + 1] = faces[i].vertices[t + 1].index;
//         triangles[tri + 2] = faces[i].vertices[t + 2].index;
//         
//         tri += 3;
//         
//       }
//     }
//     
//     Mesh mesh = go.GetComponent <MeshFilter> ().mesh;
//     mesh.Clear (false);
//     mesh.vertices = verts;
//     mesh.triangles = triangles;
//     // mesh.uv = uv;
//     mesh.RecalculateNormals ();
    
  }
  
  //   public void render ()
  //   {
  //     string currenttexture = "";
  //     //Debug.Log("Here a ");
  //     Color currentColour = new Color (0.5f, 0.5f, 0.5f, 1.0f);
  //     Ti.setFaceSurfaceShaderParameter ("texturename", currenttexture);
  //     Ti.setSurface ();
  //     
  //     for (int i = 0; i < faces.Count; i++)
  //     {
  //       Face currFace = (Face) faces[i];
  //       
  //       if (currFace.texture != currenttexture)
  //       {
  //         currenttexture = currFace.texture;
  //         Ti.setFaceSurfaceShaderParameter ("texturename", currenttexture);
  //         Ti.setSurface ();
  //       }
  //       //Debug.Log("Here b ");
  //       Ti.primitiveBegin (Ti.TYPE_POLYGON);
  //       //Ti.setSurface();                        
  //       
  //       for (int j = 0; j < currFace.numVertices (); j++)
  //       {
  //         Vertex currVertex = (Vertex) currFace.vertices[j];
  //         
  //         for (int k = 0; k < currVertex.attributes.Count; k++)
  //         {
  //           if (currVertex.attributes[k] is ShaderParameters)
  //           {
  //             ((ShaderParameters) currVertex.attributes[k]).activateParameters ();
  //           }
  //         }
  //         
  //         if ((currFace.s != null) && (currFace.t != null))
  //         {
  //           Ti.primitiveTextureCoordinate (currFace.s[j], currFace.t[j]);
  //         }
  //         else
  //         {
  //           Ti.primitiveTextureCoordinate (0.0f, 0.0f);
  //         }
  //         
  //         Color newColour = currentColour;
  //         if (currVertex.colour != null)
  //         {
  //           newColour = currVertex.colour;
  //         }
  //         else
  //         {
  //           newColour = new Color (0.5f, 0.5f, 0.5f, 1.0);
  //         }
  //         if (!newColour.sameColour (currentColour))
  //         {
  //           currentColour = newColour;
  //           Ti.primitiveColour (currentColour.r, currentColour.g, currentColour.b, currentColour.a);
  //         }
  //         
  //         if ((visibleFace != null) && (currFace.findLabel (visibleFace)))
  //         {
  //           Ti.primitiveColour (1.0f, 1.0f, 0.0f);
  //         }
  //         
  //         Ti.primitiveNormal (currVertex.normal[0],
  //                             currVertex.normal[1],
  //                             currVertex.normal[2]);
  //         
  //         Ti.primitiveVertex (currVertex.position[0],
  //                             currVertex.position[1],
  //                             currVertex.position[2]);
  //       }
  //       
  //       Ti.primitiveEnd ();
  //     }
  //   }
  
  //   public void renderVector (Vector3 from, Vector3 to)
  //   {
  //     if (fancyVector)
  //     {
  //       int sections = 10;
  //       // create coordinate axes perpendicular to normal.
  //       Vector3 uvect = new Vector3 (1.0f, 0.0f, 0.0f);
  //       uvect = Vector3.crossProduct (uvect, to);
  //       if (uvect.magnitude == 0.0f)
  //       {
  //         uvect = new Vector3 (0.0f, 1.0f, 0.0f);
  //         uvect = Vector3.crossProduct (uvect, to);
  //       }
  //       Vector3 vvect = Vector3.crossProduct (uvect, to);
  //       uvect = uvect.normalized;
  //       vvect = vvect.normalized;
  //       Vector3 snormal = Vector3.multiply (vectorLength, to);
  //       
  //       for (int i = 0; i < sections; i++)
  //       {
  //         float a1 = ((float) i / (float) sections) * (2.0 * Math.PI);
  //         float a2 =
  //         ((float) (i + 1) / (float) sections) * (2.0 * Math.PI);
  //         float l = 0.05 * vectorLength;
  //         float u1 = l * Mathf.Cos (a1);
  //         float v1 = l * Math.sin (a1);
  //         float u2 = l * Mathf.Cos (a2);
  //         float v2 = l * Math.sin (a2);
  //         
  //         Ti.primitiveBegin (Ti.TYPE_POLYGON);
  //         Ti.primitiveNormal (u1 * uvect[0] + v1 * vvect[0],
  //                             u1 * uvect[1] + v1 * vvect[1],
  //                             u1 * uvect[2] + v1 * vvect[2]);
  //         Ti.primitiveVertex (from[0] + u1 * uvect[0] +
  //         v1 * vvect[0],
  //         from[1] + u1 * uvect[1] +
  //         v1 * vvect[1],
  //         from[2] + u1 * uvect[2] +
  //         v1 * vvect[2]);
  //         Ti.primitiveVertex (from[0] + u1 * uvect[0] +
  //         v1 * vvect[0] + 0.9 * snormal[0],
  //         from[1] + u1 * uvect[1] +
  //         v1 * vvect[1] + 0.9 * snormal[1],
  //         from[2] + u1 * uvect[2] +
  //         v1 * vvect[2] + 0.9 * snormal[2]);
  //         Ti.primitiveNormal (u2 * uvect[0] + v2 * vvect[0],
  //                             u2 * uvect[1] + v2 * vvect[1],
  //                             u2 * uvect[2] + v2 * vvect[2]);
  //         Ti.primitiveVertex (from[0] + u2 * uvect[0] +
  //         v2 * vvect[0] + 0.9 * snormal[0],
  //         from[1] + u2 * uvect[1] +
  //         v2 * vvect[1] + 0.9 * snormal[1],
  //         from[2] + u2 * uvect[2] +
  //         v2 * vvect[2] + 0.9 * snormal[2]);
  //         Ti.primitiveVertex (from[0] + u2 * uvect[0] +
  //         v2 * vvect[0],
  //         from[1] + u2 * uvect[1] +
  //         v2 * vvect[1],
  //         from[2] + u2 * uvect[2] +
  //         v2 * vvect[2]);
  //         Ti.primitiveEnd ();
  //         
  //         Ti.primitiveBegin (Ti.TYPE_POLYGON);
  //         Ti.primitiveNormal (u1 * uvect[0] + v1 * vvect[0],
  //                             u1 * uvect[1] + v1 * vvect[1],
  //                             u1 * uvect[2] + v1 * vvect[2]);
  //         Ti.primitiveVertex (from[0] + 3.0 * u1 * uvect[0] +
  //         3.0 * v1 * vvect[0] +
  //         0.75 * snormal[0],
  //         from[1] + 3.0 * u1 * uvect[1] +
  //         3.0 * v1 * vvect[1] +
  //         0.75 * snormal[1],
  //         from[2] + 3.0 * u1 * uvect[2] +
  //         3.0 * v1 * vvect[2] +
  //         0.75 * snormal[2]);
  //         Ti.primitiveNormal (to[0], to[1], to[2]);
  //         Ti.primitiveVertex (from[0] + snormal[0],
  //                             from[1] + snormal[1],
  //                             from[2] + snormal[2]);
  //         Ti.primitiveNormal (u2 * uvect[0] + v2 * vvect[0],
  //                             u2 * uvect[1] + v2 * vvect[1],
  //                             u2 * uvect[2] + v2 * vvect[2]);
  //         Ti.primitiveVertex (from[0] + 3.0 * u2 * uvect[0] +
  //         3.0 * v2 * vvect[0] +
  //         0.75 * snormal[0],
  //         from[1] + 3.0 * u2 * uvect[1] +
  //         3.0 * v2 * vvect[1] +
  //         0.75 * snormal[1],
  //         from[2] + 3.0 * u2 * uvect[2] +
  //         3.0 * v2 * vvect[2] +
  //         0.75 * snormal[2]);
  //         Ti.primitiveEnd ();
  //       }
  //     }
  //     else
  //     {
  //       Vector3 top =
  //       Vector3.add (from, Vector3.multiply (vectorLength, to));
  //       Ti.primitiveBegin (Ti.TYPE_LINE);
  //       Ti.primitiveVertex (from[0], from[1], from[2]);
  //       Ti.primitiveVertex (top[0], top[1], top[2]);
  //       Ti.primitiveEnd ();
  //     }
  //   }
  
  //   public void renderWireframe ()
  //   {
  //     for (int i = 0; i < faces.Count; i++)
  //     {
  //       /*
  //        * if (faces[i].texture != currenttexture)
  //        * {
  //        * currenttexture = face[i].texture;
  //        * Ti.setSurfaceShaderParameter ("texturename", currenttexture);
  //        * Ti.setSurface ();
  //     }
  //     */
  //       Face currFace = (Face) faces[i];
  //       Ti.primitiveColour (0.2f, 0.0f, 0.0f, 1.0);
  //       
  //       if ((visibleFace != null) && (currFace.findLabel (visibleFace)))
  //       {
  //         Ti.primitiveColour (1.0f, 1.0f, 0.0f);
  //       }
  //       
  //       Ti.primitiveBegin (Ti.TYPE_LINE);
  //       
  //       for (int j = 0; j < currFace.numVertices (); j++)
  //       {
  //         /*if ((face[i].s != null) && (face[i].t != null))
  //          *    {
  //          *    Ti.primitiveTextureCoordinate (face[i].s[j], face[i].t[j]);
  //       }
  //       else
  //       {
  //       Ti.primitiveTextureCoordinate (0.0f, 0.0f);
  //       } */
  //         Vertex currVertex = (Vertex) currFace.vertices[j];
  //         
  //         Ti.primitiveWidth (0.01);
  //         
  //         Ti.primitiveNormal (currVertex.normal[0],
  //                             currVertex.normal[1],
  //                             currVertex.normal[2]);
  //         
  //         /*
  //          *        			Ti.primitiveVertex (currVertex.position[0],
  //          *                            	            currVertex.position[1],
  //          *                             		    currVertex.position[2]);*/
  //         Ti.primitiveVertex (currVertex.position[0] +
  //         0.01 * currVertex.normal[0],
  //         currVertex.position[1] +
  //         0.01 * currVertex.normal[1],
  //         currVertex.position[2] +
  //         0.01 * currVertex.normal[2]);
  //       }
  //       
  //       Ti.primitiveEnd ();
  //     }
  //     
  //     /*
  //      *            BoundingBox box = new BoundingBox (this);
  //      * 
  //      *            Ti.primitiveColour (0.0f,1.0f,0.0f);
  //      *            
  //      *            Ti.primitiveBegin (Ti.TYPE_LINE);
  //      *            Ti.primitiveVertex (box.min[0], box.min[1], box.min[2]);
  //      *            Ti.primitiveVertex (box.max[0], box.min[1], box.min[2]);
  //      *            Ti.primitiveEnd ();
  //      *            Ti.primitiveBegin (Ti.TYPE_LINE);
  //      *            Ti.primitiveVertex (box.max[0], box.min[1], box.min[2]);
  //      *            Ti.primitiveVertex (box.max[0], box.min[1], box.max[2]);
  //      *            Ti.primitiveEnd ();
  //      *            Ti.primitiveBegin (Ti.TYPE_LINE);
  //      *            Ti.primitiveVertex (box.max[0], box.min[1], box.max[2]);
  //      *            Ti.primitiveVertex (box.min[0], box.min[1], box.max[2]);
  //      *            Ti.primitiveEnd ();
  //      *            Ti.primitiveBegin (Ti.TYPE_LINE);
  //      *            Ti.primitiveVertex (box.min[0], box.min[1], box.max[2]);
  //      *            Ti.primitiveVertex (box.min[0], box.min[1], box.min[2]);
  //      *            Ti.primitiveEnd ();
  //      *            Ti.primitiveBegin (Ti.TYPE_LINE);
  //      *            Ti.primitiveVertex (box.min[0], box.max[1], box.min[2]);
  //      *            Ti.primitiveVertex (box.max[0], box.max[1], box.min[2]);
  //      *            Ti.primitiveEnd ();
  //      *            Ti.primitiveBegin (Ti.TYPE_LINE);
  //      *            Ti.primitiveVertex (box.max[0], box.max[1], box.min[2]);
  //      *            Ti.primitiveVertex (box.max[0], box.max[1], box.max[2]);
  //      *            Ti.primitiveEnd ();
  //      *            Ti.primitiveBegin (Ti.TYPE_LINE);
  //      *            Ti.primitiveVertex (box.max[0], box.max[1], box.max[2]);
  //      *            Ti.primitiveVertex (box.min[0], box.max[1], box.max[2]);
  //      *            Ti.primitiveEnd ();
  //      *            Ti.primitiveBegin (Ti.TYPE_LINE);
  //      *            Ti.primitiveVertex (box.min[0], box.max[1], box.max[2]);
  //      *            Ti.primitiveVertex (box.min[0], box.max[1], box.min[2]);
  //      *            Ti.primitiveEnd ();
  //      */
  //   }
  
  //   public void renderNormals ()
  //   {
  //     for (int i = 0; i < numFaces (); i++)
  //     {
  //       Face currFace = (Face) faces[i];
  //       
  //       for (int j = 0; j < currFace.numVertices (); j++)
  //       {
  //         Vertex currVertex = (Vertex) currFace.vertices[j];
  //         Ti.primitiveColour (1.0f, 1.0f, 1.0);
  //         renderVector (currVertex.position, currVertex.normal);
  //       }
  //     }
  //   }
  
  public BoundingBox getBoundingBox ()
  {
    if (numVertices () <= 0)
      return null;
    
    BoundingBox bb = new BoundingBox ();
    Vector3 p = (getVertex (0)).position;
    
    for (int i = 0; i < 3; i++)
    {
      bb.min[i] = p[i];
      bb.max[i] = p[i];
    }  
    
    for (int i = 1; i < numVertices(); i++)
    {
      p = (getVertex (i)).position;
      
      for (int j = 0; j < 3; j++)
      {
        if (p[j] < bb.min[j])
          bb.min[j] = p[j];
        if (p[j] > bb.max[j])
          bb.max[j] = p[j];
      }    
    }
    
    for (int j = 0; j < 3; j++)
    {
      bb.max[j] += BoundingBox.e;
      bb.min[j] -= BoundingBox.e;
      //Debug.Log("min = " + bb.min[j] + ", max = " + bb.max[j]);
      //bb.centre[j] = (bb.min[j] + bb.max[j]) / 2.0f;
      //bb.radi[j] = Mathf.Abs(bb.min[j] - bb.max[j]) / 2.0f;
    }
    
    
    return bb;
  }
  
  //   public Vertex findClosestVertex (Ray r)
  //   {
  //     Vertex minvert = (Vertex) vertices[0];
  //     float mindist = r.distanceTo (minvert.position);
  //     
  //     for (int i = 1; i < numVertices (); i++)
  //     {
  //       Vertex v = (Vertex) vertices[i];
  //       
  //       float d = r.distanceTo (v.position);
  //       
  //       if (d < mindist)
  //       {
  //         mindist = d;
  //         minvert = v;
  //       }
  //     }
  //     
  //     return minvert;
  //   }
  //   
  public void scaleFaceSet (FaceSet f, float s)
  {
    Vector3 center = new Vector3 ();
    float num = 0.0f;
    for (int i = 0; i < f.numFaces (); i++)
    { 
      Face p = f.getFace (i);
      for (int j = 0; j < p.numVertices (); j++)
      {
        center += p.getVertex (j).position;
        num += 1.0f;
        p.index = 0;
      }
    }
    center *= (1.0f / num);
    Vector3 offset = center - new Vector3 ();    
    Vector3 negoffset = new Vector3 () - center;    
    
    //     Transform scale = Transform.multiply (new Transform (offset), 
    //                                                         Transform.multiply (new Transform (s, s, s), new Transform (negoffset)));
    for (int i = 0; i < f.numFaces (); i++)
    { 
      Face p = f.getFace (i);
      for (int j = 0; j < p.numVertices (); j++)
      {
        Vertex v = p.getVertex (j);
        if (v.index == 0)
        {
          //           v.position = v.position.multiply (scale, v.position);
          v.index = 1;
        }
      }
    }
  }
  
  //   public void transform (Transform t)
  //   {
  //     for (int i = 0; i < vertices.Count; i++)
  //     {
  //       Vertex v = (Vertex) vertices[i];
  //       v.position = Vector3.multiply (t, v.position);
  //     }
  //   }
  
};

public class Model
{
  public PolygonMesh mesh;
  //modelData is a variable still to be determined
  public Model()
  {
    mesh = new PolygonMesh ();
  }
  
  public void updateMesh (GameObject parent, GameObject template)
  {
    mesh.updateMesh (parent, template);
  }
  
  //   Model (string fileName)
  //   {
  //     mesh = new PolygonMesh ();
  //     
  //     try
  //     {
  //       getModel (fileName);
  //     }
  //     catch (Exception e)
  //     {
  //       //        Debug.Log ("An error has occurred " + e.getMessage());
  //     }
  //   }
  
  //   public void getModel (string fileName)// throws IOException
  //   {
  //     FileReader fileReader = new FileReader (fileName + ".off");;
  //     BufferedReader buffReader = new BufferedReader (fileReader);
  //     
  //     try
  //     { 
  //       string line = new string();
  //       line = buffReader.readLine(); //Read the header
  //       
  //       if (line.charAt(0) == 'O')
  //         line = buffReader.readLine(); //Read the first line with number of vertices and number of faces
  //         
  //         int [] values = new int [3];
  //       
  //       stringTokenizer st = new stringTokenizer(line);
  //       
  //       int i = 0;
  //       
  //       while (st.hasMoreTokens())
  //       {
  //         string word = st.nextToken();
  //         values[i++] = Integer.parseInt(word);
  //       }
  //       
  //       //        Debug.Log("Value 0: " + values[0] + " Value 1: " + values[1] + " Value 2: " + values[2]);
  //       System.exit(0);
  //       
  //       Vertex [] verts = new Vertex [values[1]];
  //       
  //       int counter = 0;
  //       
  //       Vector meshVerts = new Vector ();
  //       Vector meshFaces = new Vector ();
  //       
  //       //////ADD all the vertices to the PolygonMesh
  //       while (counter < values[0])
  //       {
  //         line = buffReader.readLine();
  //         
  //         st = new stringTokenizer(line);
  //         
  //         string coords = new string();
  //         
  //         float [] coordinates = new float [3];
  //         
  //         int count = 0;
  //         
  //         while (st.hasMoreTokens())
  //         {
  //           string word = st.nextToken();
  //           coordinates[count++] = Double.parseDouble(word);
  //         }
  //         
  //         meshVerts.addElement(mesh.addVertex (new Vector3 (coordinates[0], coordinates[1], coordinates[2])));
  //         
  //         counter++;
  //       }
  //       
  //       counter = 0;
  //       
  //       while (counter < values[1])
  //       {
  //         line = buffReader.readLine();
  //         
  //         st = new stringTokenizer(line);
  //         string word = st.nextToken();
  //         
  //         Vector faceList = new Vector();
  //         
  //         int count = 0;
  //         
  //         while (st.hasMoreTokens() && count < Integer.parseInt(word))
  //         {
  //           word = st.nextToken();
  //           faceList.addElement((Vertex)meshVerts.elementAt(Integer.parseInt(word)));
  //           count++;
  //         }
  //         
  //         meshFaces.addElement(mesh.addFace (faceList));
  //         counter++;
  //         
  //       }
  //       
  //       //Debug.Log("Number of faces " + mesh.numFaces());
  //       //Debug.Log("Number of vertices " + mesh.numVertices() + " should be " + values[0]);
  //       
  //     }
  //     catch (IOException e)
  //     {
  //       //        Debug.Log ("An error has occurred " + e.getMessage());
  //     }
  //     finally
  //     {
  //       if (fileReader != null)
  //         fileReader.close();
  //     }                
  //   }    
};

public class BoundingBox
{
  public float [] min = new float [3];
  public float [] max = new float [3];
  //private Vector3 centre;
  
  //public float radi[] = new float[3];
  // expand bounding boxes by small amount to avoid degenerate cases.
  public static float e = 0.00001f;
  
  public BoundingBox ()
  {
    //centre = new Vector3(0,0,0);
    
    for (int i = 0; i < 3; i++)
    {
      min[i] = 0.0f;
      max[i] = 0.0f;
    }  
  }
  
  public Vector3 getCentre(){
    Vector3 point = new Vector3((min[0] + max[0]) / 2.0f, (min[1] + max[1]) / 2.0f, (min[2] + max[2]) / 2.0f);
    return point;
  }
  
  public float[] getRadi(){
    float[] d = new float[3];
    
    d[0] = Mathf.Abs((min[0] - max[0])) / 2.0f;
    d[1] = Mathf.Abs((min[1] - max[1])) / 2.0f;
    d[2] = Mathf.Abs((min[2] - max[2])) / 2.0f;
    
    return d;
  }
  
  // move this to FaceSet.getBoundingBox      
  public BoundingBox (FaceSet setOfFaces)
  {
    //centre = new Vector3(0,0,0);
    
    if ((setOfFaces != null) && (setOfFaces.numFaces () > 0))
    {
      Vector3 p = ((setOfFaces.getFace(0)).getVertex(0)).position;
      
      for (int i = 0; i < 3; i++)
      {
        min[i] = p[i];
        max[i] = p[i];
      }  
      
      for (int i = 0; i < setOfFaces.numFaces (); i++)
      {
        for (int j = 0; j < (setOfFaces.getFace(i)).numVertices(); j++)
        {
          p = ((setOfFaces.getFace(i)).getVertex(j)).position;
          
          for (int k = 0; k < 3; k++)
          {
            if (p[k] < min[k])
              min[k] = p[k];
            if (p[k] > max[k])
              max[k] = p[k];
          }    
        }
      }
    }
    else
    {
      // Hmm, what to do here?
      for (int i = 0; i < 3; i++)
      {
        min[i] = 1.0f;
        max[i] = -1.0f;
      }  
    }
    
    for (int j = 0; j < 3; j++)
    {
      max[j] += e;
      min[j] -= e;
      
      //centre[j] = (min[j] + max[j]) / 2.0f;
      //radi[j] = Mathf.Abs(min[j] - max[j]) / 2.0f;
    }
  }
  
  public string tostring()
  {
    string xRange = "X range: (" + min[0] + "; " + max[0] + ") ";
    string yRange = "Y range: (" + min[1] + "; " + max[1] + ") ";
    string zRange = "Z range: (" + min[2] + "; " + max[2] + ")";
    
    return xRange + yRange + zRange;
  }
  
  //     private bool intersectsFaces (int ca, int cb, int cc, Ray r)
  //       {
  //         // check if ray strikes the faces perpendicular to the cc axis
  //         // within the bounds of the ca and cb axes.
  //         if (r.dir[cc] != 0.0f) // parallel to these faces, ignore
  //           {
  //             // check min.
  //             float tmin = (min[cc] - r.origin[cc]) / r.dir[cc];
  //             float amin = r.origin[ca] + tmin * r.dir[ca];
  //             float bmin = r.origin[cb] + tmin * r.dir[cb];
  //             if ((amin < min[ca]) || (amin > max[ca]))
  //                return false;
  //             if ((bmin < min[cb]) || (bmin > max[cb]))
  //                return false;
  //             
  //             // check max.
  //             float tmax = (max[cc] - r.origin[cc]) / r.dir[cc];
  //             float amax = r.origin[ca] + tmax * r.dir[ca];
  //             float bmax = r.origin[cb] + tmax * r.dir[cb];
  //             if ((amax < min[ca]) || (amax > max[ca]))
  //                return false;
  //             if ((bmax < min[cb]) || (bmax > max[cb]))
  //                return false;
  //                
  //             return true;
  //           }
  //         return false;
  //       }
  
  
  //     private bool partialIntersectsFaces (int ca, int cb, int cc, Ray r)
  //       {
  //         // check if ray strikes the faces perpendicular to the cc axis
  //         // within the bounds of the ca and cb axes.
  //         if (r.dir[cc] != 0.0f) // parallel to these faces, ignore
  //           {
  //             // check min.
  //             float tmin = (min[cc] - r.origin[cc]) / r.dir[cc];
  //             if (tmin > 0 && tmin < 1){
  //                 float amin = r.origin[ca] + tmin * r.dir[ca];
  //                 float bmin = r.origin[cb] + tmin * r.dir[cb];
  //                 if ((amin >= min[ca]) && (amin <= max[ca]))
  //                     return true;
  //                 if ((bmin >= min[cb]) && (bmin <= max[cb]))
  //                    return true;
  //             }
  //             
  //             // check max.
  //             float tmax = (max[cc] - r.origin[cc]) / r.dir[cc];
  //             if (tmax > 0 && tmax < 1){
  //                 float amax = r.origin[ca] + tmax * r.dir[ca];
  //                 float bmax = r.origin[cb] + tmax * r.dir[cb];
  //                 if ((amax >= min[ca]) && (amax <= max[ca]))
  //                    return true;
  //                 if ((bmax >= min[cb]) && (bmax <= max[cb]))
  //                    return true;
  //             }
  //                
  //             return false;
  //           }
  //         return false;
  //       }
  
  //     public bool rayIntersects (Ray r)
  //       {
  //         if (intersectsFaces (0, 1, 2, r))
  //           return true;
  //         if (intersectsFaces (2, 0, 1, r))
  //           return true;
  //         if (intersectsFaces (1, 2, 0, r))
  //           return true;
  //            
  //         return false;
  //       } 
  //     
  //     public bool rayIntersectsOnLineSegment(Ray r){
  //         if (partialIntersectsFaces (0, 1, 2, r))
  //           return true;
  //         if (partialIntersectsFaces (2, 0, 1, r))
  //           return true;
  //         if (partialIntersectsFaces (1, 2, 0, r))
  //           return true;
  //            
  //         return false;
  //     }
  //     
  public bool intersects(BoundingBox other){
    Vector3 Tv = this.getCentre() - other.getCentre();
    float [] thisradi = this.getRadi();
    float [] otherradi = other.getRadi();
    
    //Debug.Log("Distance vector: " + Tv.tostring());
    //Debug.Log("This: (" + this.radi[0] + ", " + this.radi[1] + ", " + this.radi[2] + ")");
    //Debug.Log("Other: (" + other.radi[0] + ", " + other.radi[1] + ", " + other.radi[2] + ")");
    return ( (Mathf.Abs(Tv[0]) <= Mathf.Abs(thisradi[0] + otherradi[0])) && 
    (Mathf.Abs(Tv[1]) <= Mathf.Abs(thisradi[1] + otherradi[1])) && 
    (Mathf.Abs(Tv[2]) <= Mathf.Abs(thisradi[2] + otherradi[2])) ); 
    
  }
  
  
  /*returns a score based on the level of intersection. 0 = no intersection, the larger the number, 
   *     the greater the extent of intersection
   */
  public float intersectionScore(BoundingBox other){
    Vector3 Tv = this.getCentre() - other.getCentre();
    float [] thisradi = this.getRadi();
    float [] otherradi = other.getRadi();
    
    //Debug.Log("Distance vector: " + Tv.tostring());
    //Debug.Log("This: (" + this.radi[0] + ", " + this.radi[1] + ", " + this.radi[2] + ")");
    //Debug.Log("Other: (" + other.radi[0] + ", " + other.radi[1] + ", " + other.radi[2] + ")");
    float scorex = (Mathf.Abs(Tv[0]) - Mathf.Abs(thisradi[0] + otherradi[0]));
    float scorey = (Mathf.Abs(Tv[1]) - Mathf.Abs(thisradi[1] + otherradi[1]));
    float scorez = (Mathf.Abs(Tv[2]) - Mathf.Abs(thisradi[2] + otherradi[2]));
    
    if (scorex > 0 || scorey > 0 || scorez > 0) return 0;
    //there is an intersection, i.e. allscores are <= 0
    float total = Mathf.Abs(scorex + scorey + scorez);
    //Debug.Log("total = " + total);
    return total;
    
    /*return ((Mathf.Abs(Tv[0]) - Mathf.Abs(thisradi[0] + otherradi[0])) + 
     *                 (Mathf.Abs(Tv[1]) - Mathf.Abs(thisradi[1] + otherradi[1])) + 
     *                 (Mathf.Abs(Tv[2]) - Mathf.Abs(thisradi[2] + otherradi[2])) );
     *        
     *        return ( (Mathf.Abs(Tv[0]) <= Mathf.Abs(thisradi[0] + otherradi[0])) && 
     *                 (Mathf.Abs(Tv[1]) <= Mathf.Abs(thisradi[1] + otherradi[1])) && 
     *                 (Mathf.Abs(Tv[2]) <= Mathf.Abs(thisradi[2] + otherradi[2])) ); 
     *        
     */
  }
};

public class CurvePoint
{
  public Vector3 position;
  public float width;
  
  public CurvePoint (Vector3 p, float w)
  {
    position = p;
    width = w;
  }
  
  public void addCurvePoint (CurvePoint p)
  {
    position += p.position;
    width = width + p.width;
  }
  
  public static CurvePoint multiply (float a, CurvePoint b)
  {
    CurvePoint result = new CurvePoint (a * b.position, a * b.width);
    
    return result;
  }  
};

public abstract class Curve
{
  protected int degree; // degree of spline.
  protected int numControlPoints; // number of control points in the spline. 
  protected CurvePoint [] points; // set of control points for the spline.
  protected int numKnots; // number of knots;
  protected float [] knots; // set of knots;
  protected int samples; // number of points to sample at.
  
  public abstract CurvePoint curveAt (float t);
  public abstract Vector3 derivativeOfCurveAt (float t);
  
  public String shader;
  
  public Curve ()
  {
    degree = 0;
    numControlPoints = 0;
    numKnots = 0;
    points = null;
    knots = null;
    samples = 2;
    shader = "";
  }
  
  public Curve (int splineDegree, int controlPoints, int knotCount, int numSamples)
  {
    degree = splineDegree;
    numControlPoints = controlPoints;
    numKnots = knotCount;
    points = new CurvePoint [controlPoints];
    knots = new float [knotCount];
    samples = numSamples;
  }
  
  public int splineDegree ()
  {
    return degree;
  }
  
  public int numberControlPoints ()
  {
    return numControlPoints;
  }
  
  public int numberKnots ()
  {
    return numKnots;
  }
  
  public int numberSamples ()
  {
    return samples;
  }
  
  public void setSamples (int n)
  {
    samples = n;
  }
  
  public CurvePoint getControlPoint (int index)
  {
    if ((index >= 0) && (index < numControlPoints))
    {
      return points [index];
    }
    else
    {
      Debug.Log ("Requested control point " + index + " lies outside the valid range of [0:" + numControlPoints);
      
      return null;
    }
  }
  
  public void setControlPoint (int index, CurvePoint point)
  {
    if ((index >= 0) && (index < numControlPoints))
    {
      points [index] = point;
    }
    else
    {
      Debug.Log ("Requested control point " + index + " lies outside the valid range of [0:" + numControlPoints);
    }
  }
  
  public float getKnot (int index)
  {
    if ((index >= 0) && (index < numKnots))
    {
      return knots [index];
    }
    else
    {
      Debug.Log ("Requested knot " + index + " lies outside the valid range of [0:" + numKnots);
      
      return 0.0f;
    }
  }
  
  public void setKnot (int index, float knot)
  {
    if ((index >= 0) && (index < numKnots))
    {
      knots [index] = knot;
    }
    else
    {
      Debug.Log ("Requested knot " + index + " lies outside the valid range of [0:" + numKnots);
    }
  }
  
  public void setShader (String s)
  {
    shader = s;
  }
  
  //   public void render ()
  //   {
  //     Ti.setSurface (shader);	
  //     
  //     float time = 0.0;
  //     float delta = 1.0 / (float) samples;
  //     
  //     Ti.primitiveBegin (Ti.TYPE_LINE);
  //     for (int i = 0; i < samples; i++)
  //     {
  //       CurvePoint p = curveAt(time);
  //       Ti.primitiveNormal (1.0, 1.0, 1.0);
  //       Ti.primitiveWidth (p.width);
  //       Ti.primitiveVertex (p.position[0], p.position[1], p.position[2]);
  //       time += delta;
  //     }
  //     Ti.primitiveEnd ();
  //   }
  //   
  //   public void renderWireframe ()
  //   {
  //     Ti.setSurface ("");	
  //     Ti.primitiveColour (0.2, 0.0, 0.0, 1.0);
  //    
  //     float time = 0.0;
  //     float delta = 1.0 / (float) samples;
  //     
  //     Ti.primitiveBegin (Ti.TYPE_LINE);
  //     for (int i = 0; i < samples; i++)
  //     {
  //       CurvePoint p = curveAt(time);
  //       Ti.primitiveNormal (1.0, 1.0, 1.0);
  //       Ti.primitiveWidth (0.01);
  //       Ti.primitiveVertex (p.position[0], p.position[1], p.position[2]);
  //       time += delta;
  //     }
  //     Ti.primitiveEnd ();
  //   }
  //   
  //   public void renderNormals ()
  //   {
  //     render ();
  //   }
  
  public BoundingBox getBoundingBox ()
  {
    return new BoundingBox ();
  }
  
  public void transform (Transform t)
  {
    for (int i = 0; i < numberControlPoints (); i++)
    {
      CurvePoint p = getControlPoint (i);
      p.position = t.TransformPoint (p.position);
    }
  }
};

public class BezierSpline : Curve
{
  private static int smallNumber = 10;
  
  public BezierSpline (int curveDegree, CurvePoint [] controlPoints, int samples = 5) : base (curveDegree, curveDegree + 1, 0, samples)
  {
    
    for (int i = 0; i < numberControlPoints (); i++)
    {
      setControlPoint (i, controlPoints[i]);  
    }
  }
  
  /*  public BezierSpline ()
   *  {
   *    super (0, 0, 0);
}
*/
  public float choice (int i, int n)
  {
    float [,] cache = new float [smallNumber, smallNumber];
    bool initCache = false;
    
    if (!(initCache))
    {
      for (int ii = 0; ii < smallNumber; ii++)
      {
        for (int nn = 0; nn < smallNumber; nn++)
        {
          cache[ii, nn] = 0.0f;
        }
      }
      initCache = true;
    }
    
    if ((i >= 0) && (i < smallNumber) &&
      (n >= 0) && (n < smallNumber) &&  
      (cache[i, n] > 0.0))
    {  
      return cache[i, n];
    }
    
    float result = factorial (n) / (factorial (i) * factorial (n - i));
    
    if ((i >= 0) && (i < smallNumber) &&
      (n >= 0) && (n < smallNumber))
    {  
      cache[i, n] = result;
    }
    
    return result;
  }
  
  public float Bernstein (int i, int n, float t)
  {
    float ti = 1.0f;
    
    for (int k = 0; k < i; k++)
      ti *= t;
    
    float tim1 = 1.0f;
    
    for (int k = 0; k < n - i; k++)
      tim1 *= (1.0f - t);
    
    float c = choice (i, n) * ti * tim1;
    
    return c;
  }
  
  public override CurvePoint curveAt(float t)
  {
    CurvePoint point = new CurvePoint (new Vector3 (), 0.0f);
    
    int n = splineDegree ();
    
    for (int i = 0; i < numberControlPoints (); i++)
    {
      float c = Bernstein (i, n, t);
      
      CurvePoint controlPoint = getControlPoint (i);
      
      controlPoint = CurvePoint.multiply (c, controlPoint);
      
      point.addCurvePoint (controlPoint);
    }
    
    return point;
  }
  
  public override Vector3 derivativeOfCurveAt (float t)
  {
    return new Vector3 ();
  }
  
  public float factorial (int n)
  {
    if (n <= 1)
      return 1.0f;
    else
      return (float) n * factorial (n - 1);  
  }
  
  public String toString ()
  {
    String curveInfo = "";
    
    curveInfo = "BezierSpline, with degree " + splineDegree () + " [";
    
    for (int i = 0; i < numberControlPoints (); i++)
    {
      curveInfo = curveInfo + " " + points[i];
    }
    
    curveInfo = curveInfo + "]";
    
    return curveInfo;
  }
};

