Shader "Unlit/ProgressiveMaterialShader"
{
  Properties
  {
    SceneTexture ("Texture", 2D) = "white" {}
  }
  SubShader
  {
    Tags { "RenderType"="Opaque" }
    LOD 100
    Cull Off
    
    Pass
    {
      CGPROGRAM
      #pragma vertex vert
      #pragma fragment frag
      
      #include "UnityCG.cginc"
      
      struct appdata
      {
        float4 vertex : POSITION;
      };
      
      struct v2f
      {
        float4 vertex : SV_POSITION;
        float4 objvertex : TEXCOORD1;
      };
      
      sampler2D SceneTexture;
      float4 SceneTexture_ST;
      
      v2f vert (appdata v)
      {
        v2f o;
        o.objvertex = v.vertex;
        o.vertex = UnityObjectToClipPos(v.vertex);
        return o;
      }
      
      fixed4 frag (v2f i) : SV_Target
      {
        fixed2 uv;
        float xz = sqrt (i.objvertex.x * i.objvertex.x + i.objvertex.z * i.objvertex.z);
        float latitude = atan2 (i.objvertex.y, xz);
        float longitude = UNITY_PI + atan2 (i.objvertex.z, i.objvertex.x);
        uv.y = 0.5 + latitude / UNITY_PI;
        uv.x = 1.0 - (longitude / (2.0 * UNITY_PI));
        
        fixed4 col = tex2D (SceneTexture, uv);
        return col;
      }
      ENDCG
    }
  }
}
