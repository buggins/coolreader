// Based on: Galaxy Trip, by xbe, https://www.shadertoy.com/view/lslSDS, which is a relerp of the Start Tunnel shader by P_Malin adding background nebulea and different stars colors.

Shader "Unlit/Starscape"
{
  Properties
  {
    _MainTex ("Texture", 2D) = "white" {}
  }
  SubShader
  {
    Tags { "RenderType"="Opaque" }
    LOD 100
    Cull Front
    
    Pass
    {
      CGPROGRAM
      #pragma vertex vert
      #pragma fragment frag
      // make fog work
      #pragma multi_compile_fog
      
      #include "UnityCG.cginc"
      
      struct appdata
      {
        float4 vertex : POSITION;
        float2 uv : TEXCOORD0;
      };
      
      struct v2f
      {
        float2 uv : TEXCOORD0;
        UNITY_FOG_COORDS(1)
        float4 vertex : SV_POSITION;
        float4 orgvertex : POSITIONT;
      };
      
      sampler2D _MainTex;
      float4 _MainTex_ST;
      
      v2f vert (appdata v)
      {
        v2f o;
        o.vertex = UnityObjectToClipPos(v.vertex);
        o.orgvertex = v.vertex;
        o.uv = TRANSFORM_TEX(v.uv, _MainTex);
        UNITY_TRANSFER_FOG(o,o.vertex);
        return o;
      }
      
      #define PASS_COUNT 4
      #define fBrightness 2.5
      
      // Number of angular segments
      #define fSteps 2.0
      
      #define fBaseParticleSize 0.015
      static float fParticleLength = (0.5/60.0);
      
      // Min and Max star position radius. Min must be present to prevent stars too near camera
      #define fMinDist 0.8
      #define fMaxDist 5.0
      
      #define fRepeatMin 10.0
      #define fRepeatMax 12.0
      
      // fog density
      #define fDepthFade 0.8
      
      float Random(float x)
      {
        return frac(sin(x * 123.456) * 23.4567 + sin(x * 345.678) * 45.6789 + sin(x * 456.789) * 56.789);
      }
      
      float3 Starfield( const in float3 vRayDir, const in float fSeed )
      {	
        float3 sRayDir = normalize (vRayDir);
        
        float3 starDir = normalize ((floor (sRayDir * fSteps + fSeed) + 0.5 - fSeed) / fSteps);
        starDir += float3 (0.025 * (Random (starDir.x + starDir.y + starDir.z + 3.2354 * fSeed) - 0.5),
                           0.025 * (Random (starDir.x + starDir.y + starDir.z + 8.2313 * fSeed) - 0.5),
                           0.025 * (Random (starDir.x + starDir.y + starDir.z + 1.3453 * fSeed) - 0.5));
        float sRadius = fMinDist + Random(starDir.x + starDir.y + starDir.z + fSeed) * (fMaxDist - fMinDist);
        float sSize = 300.0 + Random(starDir.x + starDir.y + starDir.z + fSeed) * (900.0);

        float dot1 = dot (sRayDir, starDir);

        float dotd = length (sRayDir - starDir);
        float sShade = pow (1.0 - dotd, sSize);
        
        sShade = sShade * exp2(-dotd * fDepthFade) * fBrightness;
        return fixed4 (sShade, sShade, sShade,1);
      
//        float3 vParticlePos = GetParticlePos(vRayDir, fZPos, fSeed);

//        return GetParticleColour(vParticlePos, fBaseParticleSize, vRayDir);	
      }
      
      float3 RotateX( const in float3 vPos, const in float fAngle )
      {
        float s = sin(fAngle); float c = cos(fAngle);
        return float3( vPos.x, c * vPos.y + s * vPos.z, -s * vPos.y + c * vPos.z);
      }
      
      float3 RotateY( const in float3 vPos, const in float fAngle )
      {
        float s = sin(fAngle); float c = cos(fAngle);
        return float3( c * vPos.x + s * vPos.z, vPos.y, -s * vPos.x + c * vPos.z);
      }
      
      float3 RotateZ( const in float3 vPos, const in float fAngle )
      {
        float s = sin(fAngle); float c = cos(fAngle);
        return float3( c * vPos.x + s * vPos.y, -s * vPos.x + c * vPos.y, vPos.z);
      }
      
      // Simplex Noise by IQ
      float3 hash( float3 p )
      {
        p = float3( dot(p,float3(127.1,311.7,174.8)),
                    dot(p,float3(269.5,183.3,281.2)),
                    dot(p,float3(423.3,133.4,190.0)) );
        
        return -1.0 + 2.0*frac(sin(p)*43758.5453123);
      }
      
      float noise( in float3 p )
      {
        const float K1 = 0.366025404; // (sqrt(3)-1)/2;
        const float K2 = 0.211324865; // (3-sqrt(3))/6;
        
//        float3 i = floor( p + (p.x+p.y+p.z)*K1 );
        float3 i = 1.0;
        
        float3 a = p - i + (i.x+i.y+i.z)*K2;
        float3 o = (a.x>a.y) ? float3(1.0,0.0,0.0) : float3(0.0,1.0,0.0); //float2 of = 0.5 + 0.5*float2(sign(a.x-a.y), sign(a.y-a.x));
        float3 b = a - o + K2;
        float3 c = a - 1.0 + 2.0*K2;
        
        float3 h = max( 0.5-float3(dot(a,a), dot(b,b), dot(c,c) ), 0.0 );
        
        float3 n = h*h*h*h*float3( dot(a,hash(i+0.0)), dot(b,hash(i+o)), dot(c,hash(i+1.0)));
        
        return dot( n, float3(70.0, 70.0, 70.0) );
        
      }
      
      float fbm4( in float3 p )
      {
        float3x3 m = float3x3( 0.80,  0.60, 0.0, -0.60,  0.80, 0.0, 0.0, 0.0, 1.0 );
        float f = 0.0;
        f += 0.5000*noise( p ); p = mul(p,m)*2.02;
        f += 0.2500*noise( p ); p = mul(p,m)*2.03;
        f += 0.1250*noise( p ); p = mul(p,m)*2.01;
        f += 0.0625*noise( p );
        return f;
      }
      
      float marble(in float3 p)
      {
        return cos(p.x+fbm4(p));
      }
      
      float dowarp ( in float3 q, out float3 a, out float3 b )
      {
        float ang=0.;
        ang = 1.2345 * sin (33.33); //0.015*iTime);
        float3x3 m1 = float3x3(cos(ang), -sin(ang), 0.0, sin(ang), cos(ang), 0.0, 0.0, 0.0, 1.0);
        ang = 0.2345 * sin (66.66); //0.021*iTime);
        float3x3 m2 = float3x3(cos(ang), -sin(ang), 0.0, sin(ang), cos(ang), 0.0, 0.0, 0.0, 1.0);
        
        a = float3( marble(mul(q,m1)), marble(mul(q,m2)*float3(1.12,0.654,0.353)), marble(mul(q,m2)*float3(0.464,0.234,3.642)) );
        
        ang = 0.543 * cos (13.33); //0.011*iTime);
        m1 = float3x3(cos(ang), -sin(ang), 0.0, sin(ang), cos(ang), 0.0, 0.0, 0.0, 1.0);
        ang = 1.128 * cos (53.33); //0.018*iTime);
        m2 = float3x3(cos(ang), -sin(ang), 0.0, sin(ang), cos(ang), 0.0, 0.0, 0.0, 1.0);
        float3x3 m3 = float3x3(cos(ang), 0.0, -sin(ang), 0.0, 1.0, 0.0, sin(ang), 0.0, cos(ang));
        
        b = float3( marble( mul(q + a, m2)), marble( mul(q + a, m1) ), marble( mul(q + a, m3) ) );
        
        return marble( q + b +float3(0.32,1.654,.843));
      }
      
      
      fixed4 frag (v2f i) : SV_Target
      {
        float iTime = _Time.x;
        float2 uv = i.uv * 1.1;
//        float2 q = 2.*uv-1.;
        //q.y *= iResolution.y/iResolution.x;
        
        // camera	
//        float3 rd = normalize(float3( q.x, q.y, 1. ));
        float3 rd = normalize(i.orgvertex.xyz);
        float3 euler = float3(
          sin(iTime * 0.2) * 0.625,
                              cos(iTime * 0.1) * 0.625,
                              iTime * 0.1 + sin(iTime * 0.3) * 0.5);
        
        //	if(iMouse.z > 0.0)
        //	{
        //		euler.x = -((iMouse.y / iResolution.y) * 2.0 - 1.0);
        //		euler.y = -((iMouse.x / iResolution.x) * 2.0 - 1.0);
        //		euler.z = 0.0;
        //	}
//        rd = RotateX(rd, euler.x);
//        rd = RotateY(rd, euler.y);
//        rd = RotateZ(rd, euler.z);
        
        // Nebulae Background
        float pi = 3.141592654;
    //    q.x = 0.5 + atan2(rd.x, rd.z)/(2.*pi);
    //    q.y = 0.5 - asin(rd.y)/pi + 0.512 + 0.001*iTime;
   //     q = 2.*uv-1. + 0.01*iTime;
//        q *= 2.34;
 
        float3 neb = rd *2.34+ 0.01*iTime - float3 (1.0, 1.0, 1.0);
 
        float3 wa = float3(0.0, 0.0, 0.0);
        float3 wb = float3(0.0, 0.0, 0.0);
        float f = dowarp(neb, wa, wb);
        f = 0.5+0.5*f;
        f = pow (f, 10);
        
        float3 col = float3(f,f,f);
    //    return fixed4(col,1);
        float wc = 0.;
        wc = f;
        col = float3(wc, wc*wc, wc*wc*wc);
        wc = abs(wa.x);
        col -= float3(wc*wc, wc, wc*wc*wc);
        wc = abs(wb.x);
        col += float3(wc*wc*wc, wc*wc, wc);
        col *= 0.7;
        col.x = pow(col.x, 2.18);
        col.z = pow(col.z, 1.88);
        col = smoothstep(0., 1., col);
        col = 0.5 - (1.4*col-0.7)*(1.4*col-0.7);
        col = 0.75*sqrt(col);
        col *= 1. - 0.5*fbm4(8.*neb);
        col = clamp(col, 0., 1.);
//        return fixed4(col,1);

        // XX
       // col = float3(0.1, 0.1, 0.1);
        
        // StarField
        float fShade = 0.0;
        float a = 0.2;
        float b = 10.0;
        float c = 1.0;
//        float fZPos = 5.0;// + iTime * c + sin(iTime * a) * b;
        float fSpeed = 0.; //c + a * b * cos(a * iTime);
        
        fParticleLength = 0.25 * fSpeed / 60.0;
        
        float fSeed = 0.0;
        
        float3 vResult = float3(0.,0.,0.);
        
        float3 red = float3(0.7,0.4,0.3);
        float3 blue = float3(0.3,0.4,0.7);
        float3 tint = float3(0.,0.,0.);
        float ti = 1./float(PASS_COUNT-1);
        float t = 0.;
        for(int i=0; i<PASS_COUNT; i++)
        {
          tint = lerp(red,blue,t);
//          vResult += 1.1*tint*Starfield(rd, fZPos, fSeed);
          vResult += 1.1*tint*Starfield(rd, fSeed);
          t += ti;
          fSeed += 1.234;
          rd = RotateX(rd, 0.25*euler.x);
        }
        
        col += sqrt(vResult);
        
        // Vignetting
        float2 r = -1.0 + 2.0*(uv);
        float vb = max(abs(r.x), abs(r.y));
//        col *= (0.15 + 0.85*(1.0-exp(-(1.0-vb)*30.0)));
        fixed4 fragColor = fixed4( col, 1.0 );

        return fragColor;            
        
        // sample the texture
        //                fixed4 col = tex2D(_MainTex, i.uv);
        // apply fog
        //                UNITY_APPLY_FOG(i.fogCoord, col);
        //                return col;
      }
      ENDCG
    }
  }
}
