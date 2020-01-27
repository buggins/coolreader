// Derived from: Shadertober 29 Carpet -jaburns 
// https://www.shadertoy.com/view/Ws3Szl

Shader "Custom/CarpetShader"
{
  Properties
  {
    _Color ("Color", Color) = (1,1,1,1)
    Entropy0 ("Albedo (RGB)", 2D) = "white" {}
    Entropy1 ("Albedo (RGB)", 2D) = "white" {}
    _Glossiness ("Smoothness", Range(0,1)) = 0.5
    _Metallic ("Metallic", Range(0,1)) = 0.0
  }
  SubShader
  {
    Tags { "RenderType"="Opaque" }
    LOD 200
    
    CGPROGRAM
    // Physically based Standard lighting model, and enable shadows on all light types
    #pragma surface surf Standard fullforwardshadows
    
    // Use shader model 3.0 target, to get nicer looking lighting
    #pragma target 3.0
    
    sampler2D Entropy0;
    sampler2D Entropy1;
    
    struct Input
    {
      float2 uvEntropy0;
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
    
    
    #define PI       3.14159
    #define sqrt3    1.73205
    #define invSqrt3 0.57735
    
    #define COLORA (float3(73, 43, 123)/float3(255.,255.,255.))
    #define COLORB (float3(237, 138, 10)/float3(255.,255.,255.))
    #define COLORC (float3(247, 217, 20)/float3(255.,255.,255.))
    #define FOG    (.3*float3(58, 113, 140)/float3(255.,255.,255.))
    
    float2 Resolution = float2 (100, 100);
    
    float2x2 rot( float theta )
    {
      float c = cos(theta);
      float s = sin(theta);
      return float2x2(c, s, -s, c);
    }
    
    struct HexCoord
    {
      float2 id;
      float2 xy;
      float dist;
    };
    
    float hash21( float2 p )
    {
      return frac(sin(dot(p ,float2(12.9898,78.233))) * 43758.5453);
    }
    
    HexCoord hexCoord( float2 uv )
    {
      const float2 hexDim = float2(1, sqrt3);
      const float2 normHexDim = normalize(hexDim);
      
      float2 uva = fmod(uv, hexDim) - .5*hexDim;
      float2 uvb = fmod(uv + .5*hexDim, hexDim) - .5*hexDim;
      
      float2 guv;
      float idx;
      
      if (length(uva) < length(uvb)) {
        guv = uva;
        idx = .5;
      } else {
        guv = uvb;
        uv += .5*hexDim;
        idx = 0.;
      }
      
      float2 q = abs(guv);
      
      HexCoord result;
      result.id = float2(floor(uv.x), 2.*(floor(uv.y / sqrt3)+idx));
      result.xy = 2. * guv;
      result.dist = max(dot(q, normHexDim), q.x);
      
      return result;
    }
    
    float2 minx( float2 a, float2 b )
    {
      return a.x < b.x ? a : b;
    }
    
    float3 rawCarpet( float2 uv, float t, float level )
    {
      float mixer = lerp(.5, clamp(3.*tex2D (Entropy0, -.02 * uv + 0.0025*t).r - .75, 0., 1.), level);
      
      float3 result = mixer*tex2D (Entropy0, uv).rgb;
      uv = mul (uv, rot(.1));
      result += (1.-mixer)*tex2D (Entropy0, 2.*uv).rrr;
      return .5+.75*result + .4*mixer;
    }
    
    float3 carpetPattern( float2 uv, float level, float T, bool top )
    {
      if( top ) uv.x += sin(.15*T);
      
      HexCoord hx = hexCoord(uv);
      
      float slide = level*level*10.*cos(0.3*T + uv.x + uv.y);
      float spin = floor(3.*hash21(hx.id));
      
      hx.xy = mul (hx.xy, rot( 2. * spin * PI / 3. ));
      
      float R = .15 + level*.05*sin(1.*T + uv.x*.25) * cos(1.3*T + uv.y*.5);
      float P = 10.;
      
      float2 p1 = hx.xy - float2(1., invSqrt3);
      float2 l1 = float2(
        abs(length(p1) - invSqrt3),
                         sin(P*atan2(p1.y, p1.x) + slide + 2. * spin * PI / 3.)
      );
      
      float2 p2 = hx.xy + float2(1., invSqrt3);
      float2 l2 = float2(
        abs(length(p2) - invSqrt3),
                         sin(P*atan2(p2.x, p2.y) + slide - 2. * spin * PI / 3.)
      );
      
      float2 l3 = float2(
        abs(dot(hx.xy, normalize(float2(sqrt3,1)))),
                         sin(-slide + .5*P*PI*dot(hx.xy, normalize(float2(1,-sqrt3))))
      );
      
      float2 l = minx(minx(l2, l1), l3);
      
      float band = 1. - smoothstep(R, R+0.05, l.x);
      float middle = smoothstep(.4, .6, max(l.y * max(1. - 6.*l.x, 0.0), 0.0));
      
      if (top) return l.x*(0.5 + 0.5*cos(_Time.y+uv.xyx+float3(0,2,4)));
      
      float3 base = rawCarpet( uv, T, level );
      float3 color = base * lerp (COLORA, lerp (COLORB, COLORC, middle), band);
      
      return color;
    }
    
    float3 carpet( float2 uv, float level, float T )
    {
      float2 displacement = .1*level * float2(sin(.4*uv.x+T), cos(2.*uv.y));
      float3 a = carpetPattern( uv += displacement, level, T, false );
      a += level * level * level * .3 * carpetPattern( .75*uv, 1., T, true );
      a = carpetPattern( uv, level, T, false );
      return a;
    }

    float3 ripples (float2 p)
    {
      float3 nor = float3 (0, 0, 0);
      
      float speed = 0.0005;
      
      float rippleID;
      float3 rippleDir;
      float ripplePhase;
      float3 rippleParam;
      float rippleSpatialFreq;
      float rippleTemporalFreq;
      float rippleAmplitude;
      
      rippleID = 1.0 * tex2D (Entropy0, frac (0.1293427 * (0.3522) * _Time.x));
      rippleDir = tex2D (Entropy0, float2 (frac ((3.4353 * rippleID + 0.231 *_Time.x) * speed), frac ((4.3424 * (rippleID + 1) + 0.2345 *_Time.x) * speed))) - float3 (0.5, 0.5, 0);
      ripplePhase = 3.342 * rippleDir.z;
      rippleParam = tex2D (Entropy0, float2 (frac ((8.34221 * rippleID + 0.59823 *_Time.x) * speed), frac ((2.98453 * rippleID + 0.978234 *_Time.x) * speed)));
      rippleSpatialFreq = 100.0 * rippleParam.x;
      rippleTemporalFreq = 0.05 * rippleParam.y;
      rippleAmplitude = 0.3 * rippleParam.z;
      
      nor += rippleAmplitude * sin (dot (rippleDir, float3 (p, 0)) * rippleSpatialFreq + rippleTemporalFreq * _Time.y + ripplePhase);
      
      rippleID = 1.0 * tex2D (Entropy0, frac (0.1293427 * (0.231) * _Time.x));
      rippleDir = (tex2D (Entropy0, float2 (frac ((3.4353 * rippleID + 0.231 *_Time.x) * speed), frac ((4.3424 * (rippleID + 1) + 0.2345 *_Time.x) * speed))) - float3 (0.5, 0.5, 0)).xzy;
      ripplePhase = 3.342 * rippleDir.z;
      rippleParam = tex2D (Entropy0, float2 (frac ((8.34221 * rippleID + 0.59823 *_Time.x) * speed), frac ((2.98453 * rippleID + 0.978234 *_Time.x) * speed)));
      rippleSpatialFreq = 100.0 * rippleParam.x;
      rippleTemporalFreq = 0.05 * rippleParam.y;
      rippleAmplitude = 0.3 * rippleParam.z;
      
      nor += rippleAmplitude * sin (dot (rippleDir, float3 (p, 0)) * rippleSpatialFreq + rippleTemporalFreq * _Time.y + ripplePhase);
      
      rippleID = 1.0 * tex2D (Entropy0, frac (0.1293427 * (3.7534) * _Time.x));
      rippleDir = (tex2D (Entropy0, float2 (frac ((3.4353 * rippleID + 0.231 *_Time.x) * speed), frac ((4.3424 * (rippleID + 1) + 0.2345 *_Time.x) * speed))) - float3 (0.5, 0.5, 0)).zyx;
      ripplePhase = 3.342 * rippleDir.z;
      rippleParam = tex2D (Entropy0, float2 (frac ((8.34221 * rippleID + 0.59823 *_Time.x) * speed), frac ((2.98453 * rippleID + 0.978234 *_Time.x) * speed)));
      rippleSpatialFreq = 100.0 * rippleParam.x;
      rippleTemporalFreq = 0.05 * rippleParam.y;
      rippleAmplitude = 0.3 * rippleParam.z;
      
      nor += rippleAmplitude * sin (dot (rippleDir, float3 (p, 0)) * rippleSpatialFreq + rippleTemporalFreq * _Time.y + ripplePhase);
      
      
      return nor;
    }
    
    
    void surf (Input IN, inout SurfaceOutputStandard o)
    {
      // Albedo comes from a texture tinted by color
      fixed4 c = tex2D (Entropy0, IN.uvEntropy0) * _Color;
      c = fixed4 (1, 0, 0, 1);
      float T  = fmod(_Time.y,15.);
      float level = min(T, 10.) / 10.;
      c = fixed4 (carpet (IN.uvEntropy0 * 20.0, level, T), 1);
      //            c = fixed4 (ripples (IN.uvEntropy0),1);
      o.Albedo = c.rgb;
      o.Normal += ripples (IN.uvEntropy0);
      // Metallic and smoothness come from slider variables
      o.Metallic = _Metallic;
      o.Smoothness = _Glossiness;
      o.Alpha = c.a;
    }
    ENDCG
  }
  FallBack "Diffuse"
}
