Shader "Custom/LuggageShader"
{
  Properties
  {
    _Color ("Color", Color) = (1,1,1,1)
    PlankTex ("Plank Texture", 2D) = "white" {}
    PlankNorm ("Plank Normal Map", 2D) = "white" {}
    FootTex ("Foot Texture", 2D) = "white" {}
    FootNorm ("Foot Normal Map", 2D) = "white" {}
    _Glossiness ("Smoothness", Range(0,1)) = 0.5
    _Metallic ("Metallic", Range(0,1)) = 0.0
    
    Hinge ("Position of hinge (runs along z axis)", Vector) = (-1, 1, 0, 1)
  }
  SubShader
  {
    Tags { "RenderType"="Opaque" }
    LOD 200
    
    CGPROGRAM
    // Physically based Standard lighting model, and enable shadows on all light types
    #pragma surface surf Standard fullforwardshadows vertex:vert
    
    // Use shader model 3.0 target, to get nicer looking lighting
    #pragma target 3.0
    
    sampler2D PlankTex;
    sampler2D PlankNorm;
    sampler2D FootTex;
    sampler2D FootNorm;
    
    struct Input
    {
      float2 texcoord;
      float metal;
      float foot;
    };
    
    half _Glossiness;
    half _Metallic;
    fixed4 _Color;
    
    // Add instancing support for this shader. You need to check 'Enable Instancing' on materials that use the shader.
    // See https://docs.unity3d.com/Manual/GPUInstancing.html for more information about instancing.
    // #pragma instancing_options assumeuniformscaling
    UNITY_INSTANCING_BUFFER_START(Props)
    // put more per-instance properties here
    UNITY_INSTANCING_BUFFER_END(Props)
    
    fixed4 Hinge;
    
    void vert (inout appdata_full v, out Input o) {
      float lidAngle = _Time.y;
      
      lidAngle = clamp (fmod (lidAngle, 2 * UNITY_PI), 0.0, 0.5 * UNITY_PI);
      
      o.texcoord = 0.2 * float2 (v.vertex.y, v.vertex.x + v.vertex.z);
      o.metal = 0;
      o.foot = 0;
      
      if (((v.vertex.y > 0.7) && (v.vertex.y < 1.0)) ||
        ((v.vertex.y > 2.0) && (v.vertex.y < 2.3)))
      {
        o.metal = 1;
      }
      
      if (v.vertex.y > Hinge.y)
      {
        float sina, cosa;
        sincos (lidAngle, sina, cosa);
        float2x2 m = float2x2 (cosa, sina, -sina, cosa);
        v.vertex.xy = mul (m, (v.vertex.xy - Hinge.xy)) + Hinge.xy;
      }
      
      float footy = 0.0;
      float alpha = 0.3;
      float speed = 3.0;
      if (v.vertex.y < footy)
      {
        v.vertex.y = v.vertex.y * (1.0 * (1.0 - alpha) + alpha * 0.5 * (1.0 + sin (speed * _Time.y + v.vertex.z)));
        o.foot = 1;
      }
    }
    
    
    void surf (Input IN, inout SurfaceOutputStandard o)
    {
      // Albedo comes from a texture tinted by color
      if (IN.metal > 0)
      {
        o.Metallic = 1.0;
        o.Albedo = fixed4 (0.95, 1, 0.7, 1);
        o.Normal = UnpackNormal (tex2D (PlankNorm, 0.2 * IN.texcoord));
        o.Smoothness = 0.5;
      }
      else
      {
        if (IN.foot > 0)
        {
          fixed4 c = tex2D (FootTex, 10.0 * IN.texcoord) * _Color;
          o.Albedo = 0.8 * c.rgb * float3 (1.0, 0.8, 0.95);
          o.Normal = UnpackNormal (tex2D (FootNorm, 10.0 * IN.texcoord));
          // Metallic and smoothness come from slider variables
          o.Metallic = _Metallic;
          o.Smoothness = 0.0;
        }
        else
        {
          fixed4 c = tex2D (PlankTex, IN.texcoord) * _Color;
          o.Albedo = c.rgb;
          o.Normal = UnpackNormal (tex2D (PlankNorm, IN.texcoord));
          // Metallic and smoothness come from slider variables
          o.Metallic = _Metallic;
          o.Smoothness = _Glossiness;
        }
      }
      o.Alpha = 1.0;
    }
    ENDCG
  }
  FallBack "Diffuse"
}
