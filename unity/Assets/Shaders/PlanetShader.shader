Shader "Custom/PlanetShader"
{
    Properties
    {
      CameraFOVH ("Camera field of view (horizontal)", Float) = 60.0
      CameraFOVV ("Camera field of view (vertical)", Float) = 60.0
      CameraNear ("Camera near clipping plane", Float) = 0.3
      CameraHeight ("Height of camera about planet", Float) = 2.0
      PlanetRadius ("Planet radius", Float) = 100.0
    
        _Color ("Color", Color) = (1,1,1,1)
        _MainTex ("Albedo (RGB)", 2D) = "white" {}
        _Glossiness ("Smoothness", Range(0,1)) = 0.5
        _Metallic ("Metallic", Range(0,1)) = 0.0
    }
    SubShader
    {
        Tags { "RenderType"="Opaque" }
        LOD 10000
        Cull Off

        CGPROGRAM
        // Physically based Standard lighting model, and enable shadows on all light types
        #pragma surface surf Standard fullforwardshadows vertex:vert

        // Use shader model 3.0 target, to get nicer looking lighting
        #pragma target 3.0

        sampler2D _MainTex;

        struct Input
        {
            float2 uv_MainTex;
        };

        half _Glossiness;
        half _Metallic;
        fixed4 _Color;

        float CameraFOVH;
        float CameraFOVV;
        float CameraNear;
        float CameraHeight;
        float PlanetRadius;
        
        void vert (inout appdata_full v, out Input o) {
          
          // Angles define the visible arc of the planet. 
          // 0 is vertically coming from directly below.
          // +ve angle represent the forward arc.
          
          // The nearest point on the planet is defined by the near clipping plane. 
          // Assuming terrain height is 0 (offset from radius).
          // nearDist = CameraHeight * tan (radians (CameraFOVV));
          float nearAngle = atan2 (CameraHeight * tan (radians (CameraFOVV / 2.0)), PlanetRadius);
          
          // Horizon is where eye ray is tangent to planet.        
          float farAngle = acos (PlanetRadius / (PlanetRadius + CameraHeight));
          
          float angle = nearAngle + (farAngle - nearAngle) * v.texcoord.y;
          float dist = PlanetRadius * sin (angle);
          v.vertex = float4 ((v.texcoord.x - 0.5) * dist * tan (radians (CameraFOVH / 2.0)), PlanetRadius * (cos (angle) - 1.0), dist, 1.0);
          
          v.vertex.y += 0.01 * sin (10.0 * v.texcoord.x + _Time.y) + 0.01 * sin (19.0 * v.texcoord.y + _Time.z);
          v.normal = float3 (0, 1, 0);
          o.uv_MainTex = v.texcoord;
        }
        
        // Add instancing support for this shader. You need to check 'Enable Instancing' on materials that use the shader.
        // See https://docs.unity3d.com/Manual/GPUInstancing.html for more information about instancing.
        // #pragma instancing_options assumeuniformscaling
       // UNITY_INSTANCING_BUFFER_START(Props)
            // put more per-instance properties here
       // UNITY_INSTANCING_BUFFER_END(Props)

        void surf (Input IN, inout SurfaceOutputStandard o)
        {
            // Albedo comes from a texture tinted by color
            fixed4 c = tex2D (_MainTex, IN.uv_MainTex) * _Color;
            o.Albedo = c.rgb + float3 (IN.uv_MainTex, 0);
            // Metallic and smoothness come from slider variables
        //    o.Metallic = _Metallic;
//            o.Smoothness = _Glossiness;
            o.Alpha = c.a;
        }
        ENDCG
    }
    FallBack "Diffuse"
}
