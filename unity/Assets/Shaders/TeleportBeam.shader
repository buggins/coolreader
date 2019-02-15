Shader "Unlit/TeleportBeam"
{
	Properties
	{
		Col1 ("Band Colour 1", Color) = (1, 1, 0, 1)
		Col2 ("Band Colour 2", Color) = (0, 0, 0, 1)
		Frequency ("BandFreq", float) = 10.0
	}
	SubShader
	{
                Tags {"Queue"="Transparent" "RenderType"="Transparent" }
		LOD 100
                ZWrite Off
                Blend SrcAlpha OneMinusSrcAlpha
                
		Pass
		{
			CGPROGRAM
			#pragma vertex vert
			#pragma fragment frag
			
			#include "UnityCG.cginc"

			struct appdata
			{
				float4 vertex : POSITION;
				float2 uv : TEXCOORD0;
			};

			struct v2f
			{
				float2 uv : TEXCOORD0;
				float4 vertex : SV_POSITION;
			};

			float4 Col1;
			float4 Col2;
			float Frequency;
			
			v2f vert (appdata v)
			{
				v2f o;
				o.vertex = UnityObjectToClipPos(v.vertex);
				o.uv = v.uv;
				return o;
			}
			
			fixed4 frag (v2f i) : SV_Target
			{
                                float factor = 0.5 + 0.5 * (sin (Frequency * i.uv.y 
                                - 10 * _Time.y) + 0.3 * sin (40 * i.uv.x));
				fixed4 col = Col1 * factor + Col2 * (1 - factor);
				return col;
			}
			ENDCG
		}
	}
}
