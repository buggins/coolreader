Shader "Custom/LittlePlanetShader"
{
  Properties {
    // The horizontal position of the player, used to set the centre of the grid.
    TerrainPosition ("Terrain Position", Vector) = (0, 0, 0, 0)
    TerrainForward ("Terrain Forward", Vector) = (0, 0, 0, 0)
    
    // The heightmap.
    TerrainHeightmapTexture ("Height map", 2D) = "white" {}
    
    // Textures, taken from the terrain.
    _Control ("SplatMap (RGBA)", 2D) = "red" {}
    _Splat0 ("Layer 0 (R)", 2D) = "white" {} 
    _Splat1 ("Layer 1 (G)", 2D) = "white" {} 
    _Splat2 ("Layer 2 (B)", 2D) = "white" {} 
    _Splat3 ("Layer 3 (A)", 2D) = "white" {}
  }
  
  SubShader {
    Tags {
      "Queue" = "Geometry-100"
      "RenderType" = "Opaque"
    }
    Cull Off
    
    CGPROGRAM
    #pragma surface surf Standard addshadow fullforwardshadows vertex:vert
    #pragma instancing_options assumeuniformscaling nomatrices nolightprobe nolightmap forwardadd
    #pragma multi_compile_fog // needed because finalcolor oppresses fog code generation.
    #pragma target 3.0
    #include "UnityPBSLighting.cginc"
    
    #pragma multi_compile_local __ _ALPHATEST_ON
    #pragma multi_compile_local __ _NORMALMAP
    
    #define TERRAIN_STANDARD_SHADER
    #define TERRAIN_INSTANCED_PERPIXEL_NORMAL
    #define TERRAIN_SURFACE_OUTPUT SurfaceOutputStandard
    //#include "TerrainSplatmapCommon.cginc"
    
    #define PI       3.14159
    
    struct Input
    {
      float4 tc;
      float3 vpos;
    };
    
    half _Metallic0;
    half _Metallic1;
    half _Metallic2;
    half _Metallic3;
    
    half _Smoothness0;
    half _Smoothness1;
    half _Smoothness2;
    half _Smoothness3;
    
    float4 TerrainPosition;
    float4 TerrainForward;
    sampler2D TerrainHeightmapTexture;
    
    sampler2D _Control;
    float4 _Control_ST;
    float4 _Control_TexelSize;
    sampler2D _Splat0, _Splat1, _Splat2, _Splat3;
    float4 _Splat0_ST, _Splat1_ST, _Splat2_ST, _Splat3_ST;
    
    //         void LittleSplatmapVert(inout appdata_full v, out Input data)
    //         {
    //             UNITY_INITIALIZE_OUTPUT(Input, data);
    //             
    //             // Work out where this 
    //             data.tc.xy = fmod (v.texcoord.xy + TerrainPosition.xz + float2 (1000, 1000), float2 (1, 1));
    //         //data.tc.zw = data.tc.xy;
    // 
    //             float hm = UnpackHeightmap(tex2Dlod(TerrainHeightmapTexture, float4(data.tc.xy, 0, 0)));
    //         //hm = tex2Dlod(TerrainHeightmapTexture, float4(data.tc.xy, 0, 0)).x;
    //         v.vertex.y = 100*hm;// + 10 * sin (10.0 * v.texcoord.x);
    // 
    //         v.vertex.y = hm;// + 10 * sin (10.0 * v.texcoord.x);
    //         //data.tc.xy *= 100;
    //         }
    
    //         // Take points that fall on the boundary of a square and reshape them to fall into a circle.
    //         float2 sqrToCir (float2 t)
    //         {
    //         }
    // 
    //         // https://stackoverflow.com/questions/2656899/mapping-a-sphere-to-a-cube
    //         const float isqrt2 = 0.70710676908493042;
    // 
    //         float3 cubify (float3 s)
    //         {
    //           float xx2 = s.x * s.x * 2.0;
    //           float yy2 = s.y * s.y * 2.0;
    // 
    //           float2 v = float2 (xx2 - yy2, yy2 - xx2);
    // 
    //           float ii = v.y - 3.0;
    //           ii *= ii;
    // 
    //           float isqrt = -sqrt(ii - 12.0 * xx2) + 3.0;
    // 
    //           v = sqrt(v + isqrt);
    //           v *= isqrt2;
    // 
    //           return sign (s) * float3 (v, 1.0);
    //         }
    // 
    //         // Move points on the boundary of a sphere to the boundary of a cube.
    //         float3 sphere2cubeB (float3 sphere)
    //         {
    //           float3 f = abs(sphere);
    // 
    //           bool a = f.y >= f.x && f.y >= f.z;
    //           bool b = f.x >= f.z;
    // 
    //           return a ? cubify(sphere.xzy).xzy : b ? cubify(sphere.yzx).zxy : cubify(sphere);
    //         }
    // 
    //         // http://mathproofs.blogspot.com/2005/07/mapping-cube-to-sphere.html
    //         float3 cube2sphere (float3 s)
    //         {
    //           float x2 = s.x * s.x / 2;
    //           float y2 = s.y * s.y / 2;
    //           float z2 = s.z * s.z / 2;
    //           
    //           return float3 (s.x * sqrt (1.0 - y2 - z2 + (4.0 / 3.0) * y2 * z2),
    //                          s.y * sqrt (1.0 - z2 - x2 + (4.0 / 3.0) * z2 * x2), 
    //                          s.z * sqrt (1.0 - x2 - y2 + (4.0 / 3.0) * x2 * y2));
    //         }
    //        
    //         // http://alexcpeterson.com/2010/04/23/sphere-to-cube-mapping/
    //         float3 sphere2cubeC (float3 s)
    //         {
    //           float x,y,z;
    //           float3 r = s; 
    //           x = s.x;
    //           y = s.y;
    //           z = s.z;
    //           float fx, fy, fz;
    //           fx = abs(x);
    //           fy = abs(y);
    //           fz = abs(z);
    //           const float inverseSqrt2 = 0.70710676908493042;
    //           if (fy >= fx && fy >= fz) {
    //             float a2 = x * x * 2.0;
    //             float b2 = z * z * 2.0;
    //             float inner = -a2 + b2 -3;
    //             float innersqrt = -sqrt((inner * inner) - 12.0 * a2);
    //             if(x == 0.0 || x == -0.0) {
    //               r.x = 0.0;
    //             }
    //             else {
    //               r.x = sqrt(innersqrt + a2 - b2 + 3.0) * inverseSqrt2;
    //             }
    //             if(z == 0.0 || z == -0.0) {
    //               r.z = 0.0;
    //             }
    //             else {
    //               r.z = sqrt(innersqrt - a2 + b2 + 3.0) * inverseSqrt2;
    //             }
    //             if(r.x > 1.0) r.x = 1.0;
    //             if(r.z > 1.0) r.z = 1.0;
    //             if(x < 0) r.x = -r.x;
    //             if(z < 0) r.z = -r.z;
    //             if (y > 0) {
    //               // top face
    //               r.y = 1.0;
    //             }
    //             else {
    //               // bottom face
    //               r.y = -1.0;
    //             }
    //           }
    //           else if (fx >= fy && fx >= fz) {
    //             float a2 = y * y * 2.0;
    //             float b2 = z * z * 2.0;
    //             float inner = -a2 + b2 -3;
    //             float innersqrt = -sqrt((inner * inner) - 12.0 * a2);
    //             if(y == 0.0 || y == -0.0) {
    //               r.y = 0.0;
    //             }
    //             else {
    //               r.y = sqrt(innersqrt + a2 - b2 + 3.0) * inverseSqrt2;
    //             }
    //             if(z == 0.0 || z == -0.0) {
    //               r.z = 0.0;
    //             }
    //             else {
    //               r.z = sqrt(innersqrt - a2 + b2 + 3.0) * inverseSqrt2;
    //             }
    //             if(r.y > 1.0) r.y = 1.0;
    //             if(r.z > 1.0) r.z = 1.0;
    //             if(y < 0) r.y = -r.y;
    //             if(z < 0) r.z = -r.z;
    //             if (x > 0) {
    //               // right face
    //               r.x = 1.0;
    //             }
    //             else {
    //               // left face
    //               r.x = -1.0;
    //             }
    //           }
    //           else {
    //             float a2 = x * x * 2.0;
    //             float b2 = y * y * 2.0;
    //             float inner = -a2 + b2 -3;
    //             float innersqrt = -sqrt((inner * inner) - 12.0 * a2);
    //             if(x == 0.0 || x == -0.0) {
    //               r.x = 0.0;
    //             }
    //             else {
    //               r.x = sqrt(innersqrt + a2 - b2 + 3.0) * inverseSqrt2;
    //             }
    //             if(y == 0.0 || y == -0.0) {
    //               r.y = 0.0;
    //             }
    //             else {
    //               r.y = sqrt(innersqrt - a2 + b2 + 3.0) * inverseSqrt2;
    //             }
    //             if(r.x > 1.0) r.x = 1.0;
    //             if(r.y > 1.0) r.y = 1.0;
    //             if(x < 0) r.x = -r.x;
    //             if(y < 0) r.y = -r.y;
    //             if (z > 0) {
    //               // front face
    //               r.z = 1.0;
    //             }
    //             else {
    //               // back face
    //               r.z = -1.0;
    //             }
    //           }
    //           return r;
    //         }  
    
    // Keep unit spacing on cube the same as angular spacing on sphere.
    float3 sphere2cube (float3 s, out float2 uv)
    {
      float3 r = float3 (0, 0, 0);
      uv = float2 (0, 0);
      float3 abss = abs (s);
      
      if ((abss.x > abss.y) && (abss.x > abss.z))
      {
        r = float3 (sign (s.x), atan2 (s.y, abss.x) / (PI / 4.0), atan2 (s.z, abss.x) / (PI / 4.0));
        uv = 0.5 * r.yz + float2 (0.5, 0.5);
      }
      else 
        if ((abss.y > abss.x) && ((abss.y > abss.z)))
        {
          r = float3 (atan2 (s.x, abss.y) / (PI / 4.0), sign (s.y), atan2 (s.z, abss.y) / (PI / 4.0));
          uv = 0.5 * r.zx + float2 (0.5, 0.5);
        }
        else
        {
          r = float3 (atan2 (s.x, abss.z) / (PI / 4.0), atan2 (s.y, abss.z) / (PI / 4.0), sign (s.z));
          uv = 0.5 * r.xy + float2 (0.5, 0.5);
        }
        return r;
    }
    
    float2 sphere2cubeTexCoord (float3 s)
    {
      float2 uv = float2 (0, 0);
      float3 abss = abs (s);
      float twodivpi = 2.0 / PI;
      if ((abss.x > abss.y) && (abss.x > abss.z))
      {
        uv = twodivpi * float2 (atan2 (s.y, abss.x), atan2 (s.z, abss.x)) + float2 (0.5, 0.5);
      }
      else 
        if ((abss.y > abss.x) && ((abss.y > abss.z)))
        {
          uv = twodivpi * float2 (atan2 (s.z, abss.y), atan2 (s.x, abss.y)) + float2 (0.5, 0.5);
        }
        else
        {
          uv = twodivpi * float2 (atan2 (s.x, abss.z), atan2 (s.y, abss.z)) + float2 (0.5, 0.5);
        }
        return uv;
    }
    
    
    void vert (inout appdata_full v, out Input data) {
      
      float2 patchVertex = v.vertex.xy;
      
      // The vertex comes from a unit grid. Texture coordinates range from 0 to 1, in x and y directions.
      UNITY_INITIALIZE_OUTPUT(Input, data);
      
      // Position in the terrain is relative to the player position, so the center of the grid falls there.
      //           data.tc.xy = fmod (v.texcoord.xy + TerrainPosition.xz + float2 (1000, 1000), float2 (1, 1));
      //data.tc.xy = v.texcoord.xy + TerrainPosition.xz;
      
      //          float hm = UnpackHeightmap(tex2Dlod(TerrainHeightmapTexture, float4 (data.tc.xy, 0, 0)));
      //          v.vertex.y = hm;// + 10 * sin (10.0 * v.texcoord.x);
      
      
      //         LittleSplatmapVert (v, data);
      //          v.vertex *= 100;
      //return;
      
      float region = 0.2;
      float r = 60;
      float tx = (v.texcoord.y - 0.5);
      float ty = (v.texcoord.x - 0.5);
      float tlen = length (float2 (tx, ty));
      float trad = 0.0;
      float theta = 0.0;
      float phi = 0.0;
      if (tlen > 0)
      {
        float2 sqrToCir = normalize (float2 (tx, ty));
        if (abs (sqrToCir.x) > abs (sqrToCir.y))
        {
          sqrToCir = float2 (sqrToCir.x / abs (sqrToCir.x), sqrToCir.y / abs (sqrToCir.x)); 
        }
        else
        {
          sqrToCir = float2 (sqrToCir.x / abs (sqrToCir.y), sqrToCir.y / abs (sqrToCir.y)); 
        }
        trad = 1 / length (sqrToCir);
        theta = 2.0 * region * PI * tlen * trad;
        phi = atan2 (tx, ty);
      }
      
      float3 vpos = float3 (sin (theta) * cos (phi), cos (theta), sin (theta) * sin (phi));
      
      //           float ptheta = (TerrainPosition.z) * PI;
      //           float pphi = (TerrainPosition.x) * 2.0 * PI;
      //           float3x3 m1 = float3x3 (cos (ptheta), -sin(ptheta), 0.0, sin(ptheta), cos(ptheta), 0.0, 0.0, 0.0, 1.0);
      //          // float3x3 m2 = float3x3 (cos (pphi), 0.0, -sin(pphi), 0.0, 1.0, 0.0, sin(pphi), 0.0, cos(pphi));
      //           float3x3 m2 = float3x3 (1.0, 0.0, 0.0, 0.0, cos (pphi), -sin(pphi), 0.0, sin(pphi), cos(pphi));
      // 
      //           vpos = mul (mul (vpos, m2), m1);
      float3 up = normalize (TerrainPosition.xyz);
      float3 forward = normalize (TerrainForward.xyz);
      // ensure forward is perpendicular to up.
      forward = forward - up * dot (up, forward);
      float3 side = cross (up, forward);
      float3x3 posrot = float3x3 (side, up, forward);
      vpos = mul (vpos, posrot);
      
      //data.tc.xy = float2 (sin (0.5 * acos (vpos.y)), 1.0 * (atan2 (vpos.x, vpos.z) / (1.0 * PI)));
      
      
      // So now we have vpos as a vector on the unit sphere, pointing in the direction of this vertex.
      // Can either map that back to a sphere, and find texture coordinates with all the distortion that involves.
      // data.tc.xy = float2 (sin (0.5 * acos (vpos.y)), 1.0 * (atan2 (vpos.x, vpos.z) / (1.0 * PI)));
      
      // Or map to the face of a cube. A cube map may have less distortion.
      
      data.tc = float4 (0,0,0,0);
      data.vpos = vpos;
      float3 sqrpos = sphere2cube (vpos, data.tc.xy);
      
      float hm = UnpackHeightmap(tex2Dlod (TerrainHeightmapTexture, float4 (data.tc.xy, 0, 0)));
      
      float relev = r + 100.0 * hm;
      
      v.vertex.xyz = relev * vpos - r * float3 (0, 1, 0);
      //          v.vertex.xyz = mul (vpos, m2) - r * float3 (0, 1, 0);
      
      //          v.vertex.xyz = relev * float3 (sin (theta) * cos (phi), cos (theta), sin (theta) * sin (phi)) - r * float3 (0, 1, 0);
      
    }
    
    void surf (Input IN, inout SurfaceOutputStandard o) {
      half4 splat_control;
      half weight;
      fixed4 mixedDiffuse;
      half4 defaultSmoothness = half4(_Smoothness0, _Smoothness1, _Smoothness2, _Smoothness3);
      //            SplatmapMix(IN, defaultSmoothness, splat_control, weight, mixedDiffuse, o.Normal);
      
      float2 datatcxy = sphere2cubeTexCoord (IN.vpos);
      
      
      float2 splatUV = (datatcxy * (_Control_TexelSize.zw - 1.0f) + 0.5f) * _Control_TexelSize.xy;
      splat_control = tex2D(_Control, splatUV);
      
      float2 uvSplat0 = TRANSFORM_TEX(datatcxy, _Splat0) * 100;
      float2 uvSplat1 = TRANSFORM_TEX(datatcxy, _Splat1) * 100;
      float2 uvSplat2 = TRANSFORM_TEX(datatcxy, _Splat2) * 100;
      float2 uvSplat3 = TRANSFORM_TEX(datatcxy, _Splat3) * 100;
      
      mixedDiffuse = 0.0f;
      mixedDiffuse += splat_control.r * tex2D(_Splat0, uvSplat0);
      mixedDiffuse += splat_control.g * tex2D(_Splat1, uvSplat1);
      mixedDiffuse += splat_control.b * tex2D(_Splat2, uvSplat2);
      mixedDiffuse += splat_control.a * tex2D(_Splat3, uvSplat3);
      
      //            float2 uvSplat0 = TRANSFORM_TEX(IN.tc.xy, _Splat0) * 100;
      //            mixedDiffuse = tex2D(_Splat0, uvSplat0);
      o.Albedo = mixedDiffuse.rgb;
      //o.Albedo = float3 ((IN.tc.xy * (_Control_TexelSize.zw - 1.0f) + 0.5f) * _Control_TexelSize.xy, 0);
      o.Alpha = 1.0;//weight;
      o.Smoothness = mixedDiffuse.a;
      //           o.Metallic = dot(splat_control, half4(_Metallic0, _Metallic1, _Metallic2, _Metallic3));
      o.Metallic = 0.0;
    }
    ENDCG
    
    //  UsePass "Hidden/Nature/Terrain/Utilities/PICKING"
    //  UsePass "Hidden/Nature/Terrain/Utilities/SELECTION"
  }
  
  //  Dependency "AddPassShader"    = "Hidden/TerrainEngine/Splatmap/Standard-AddPass"
  //  Dependency "BaseMapShader"    = "Hidden/TerrainEngine/Splatmap/Standard-Base"
  //  Dependency "BaseMapGenShader" = "Hidden/TerrainEngine/Splatmap/Standard-BaseGen"
  
  Fallback "Nature/Terrain/Diffuse"
}
