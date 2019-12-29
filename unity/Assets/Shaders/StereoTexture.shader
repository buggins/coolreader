Shader "Unlit/StereoTexture"
{
  // Disable mipmaps on the textures used to remove gray stripe at the texture boundary.
  Properties
  {
    LeftTex ("TextureMainOrLeft", 2D) = "white" {}
    RightTex ("TextureRight", 2D) = "white" {}
    Mode ("0 - separate L/R, 1 - joined horizontally, 2 - joined vertically", Int) = 0
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
      
      sampler2D LeftTex;
      float4 LeftTex_ST;
      sampler2D RightTex;
      float4 RightTex_ST;
      
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
        float longitude = 3.14159 + atan2 (i.objvertex.z, i.objvertex.x);
        uv.y = 0.5 + latitude / 3.14159;
        uv.x = 1.0 - (longitude / (2 * 3.14159));
        
        // sample the texture
        fixed4 col;
        if (unity_StereoEyeIndex > 0)
        {
          col = tex2D(RightTex, uv);
        }
        else
        {
          col = tex2D (LeftTex, uv);
        }
        
        return col;
      }
      ENDCG
    }
  }
}
