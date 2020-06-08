Shader "Custom/LittlePlanetShader"
{
    Properties {
        // used in fallback on old cards & base map
        //[HideInInspector] 
        _MainTex ("BaseMap (RGB)", 2D) = "white" {}
        //[HideInInspector] 
        _Color ("Main Color", Color) = (1,1,1,1)
        //[HideInInspector] 
        _TerrainHolesTexture("Holes Map (RGB)", 2D) = "white" {}
        
        TerrainPosition ("Terrain Position", Vector) = (0, 0, 0, 0)
        TerrainHeightmapTexture ("Height map", 2D) = "white" {}
    }

    SubShader {
        Tags {
            "Queue" = "Geometry-100"
            "RenderType" = "Opaque"
        }
        Cull Off

        CGPROGRAM
        #pragma surface surf Standard finalcolor:SplatmapFinalColor finalgbuffer:SplatmapFinalGBuffer addshadow fullforwardshadows vertex:vert
        #pragma instancing_options assumeuniformscaling nomatrices nolightprobe nolightmap forwardadd
        #pragma multi_compile_fog // needed because finalcolor oppresses fog code generation.
        #pragma target 3.0
        #include "UnityPBSLighting.cginc"

        #pragma multi_compile_local __ _ALPHATEST_ON
        #pragma multi_compile_local __ _NORMALMAP

        #define TERRAIN_STANDARD_SHADER
        #define TERRAIN_INSTANCED_PERPIXEL_NORMAL
        #define TERRAIN_SURFACE_OUTPUT SurfaceOutputStandard
        #include "TerrainSplatmapCommon.cginc"

        #define PI       3.14159

        half _Metallic0;
        half _Metallic1;
        half _Metallic2;
        half _Metallic3;

        half _Smoothness0;
        half _Smoothness1;
        half _Smoothness2;
        half _Smoothness3;
        
        float4 TerrainPosition;
        sampler2D TerrainHeightmapTexture;
        
        void LittleSplatmapVert(inout appdata_full v, out Input data)
{
    UNITY_INITIALIZE_OUTPUT(Input, data);
    data.tc.xy = fmod (v.texcoord.xy + TerrainPosition.xz + float2 (1000, 1000), float2 (1, 1));

    float hm = UnpackHeightmap(tex2Dlod(TerrainHeightmapTexture, float4(data.tc.xy, 0, 0)));
//hm = tex2Dlod(TerrainHeightmapTexture, float4(data.tc.xy, 0, 0)).x;
v.vertex.y = 100*hm;// + 10 * sin (10.0 * v.texcoord.x);
}

        
        void vert (inout appdata_full v, out Input data) {
      
          float2 patchVertex = v.vertex.xy;

    
          LittleSplatmapVert (v, data);
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
            theta = PI * tlen * trad;
            phi = atan2 (tx, ty);
          }
          
//          v.vertex.xyz = r * float3 (trad * tx, 0.0 - sin (tlen), trad * ty);
          float relev = r + 1.0 * v.vertex.y;
//r += 10.0 * sin (12.5 * sin (theta) * cos (phi));
          v.vertex.xyz = relev * float3 (sin (theta) * cos (phi), cos (theta), sin (theta) * sin (phi)) - r * float3 (0, 1, 0);
      //    v.vertex.xyz = r * float3 (theta, 0, phi);
          v.vertex.xyz += float3 (50, 0, 50);
        }
        
        void surf (Input IN, inout SurfaceOutputStandard o) {
            half4 splat_control;
            half weight;
            fixed4 mixedDiffuse;
            half4 defaultSmoothness = half4(_Smoothness0, _Smoothness1, _Smoothness2, _Smoothness3);
            SplatmapMix(IN, defaultSmoothness, splat_control, weight, mixedDiffuse, o.Normal);
            o.Albedo = mixedDiffuse.rgb;
            //o.Albedo = float3 ((IN.tc.xy * (_Control_TexelSize.zw - 1.0f) + 0.5f) * _Control_TexelSize.xy, 0);
            o.Alpha = weight;
            o.Smoothness = mixedDiffuse.a;
            o.Metallic = dot(splat_control, half4(_Metallic0, _Metallic1, _Metallic2, _Metallic3));
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
