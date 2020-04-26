// Derived from: Elevated, https://www.shadertoy.com/view/MdX3Rr

// Original: Created by inigo quilez - iq/2013
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// on the derivatives based noise: http://iquilezles.org/www/articles/morenoise/morenoise.htm
// on the soft shadow technique: http://iquilezles.org/www/articles/rmshadows/rmshadows.htm
// on the fog calculations: http://iquilezles.org/www/articles/fog/fog.htm
// on the lighting: http://iquilezles.org/www/articles/outdoorslighting/outdoorslighting.htm
// on the raymarching: http://iquilezles.org/www/articles/terrainmarching/terrainmarching.htm

// Thanks also: https://alastaira.wordpress.com/2015/08/07/unity-shadertoys-a-k-a-converting-glsl-shaders-to-cghlsl/

Shader "Unlit/ElevatedShader"
{
  Properties
  {
    NoiseTex ("Texture", 2D) = "white" {}
    UXStart ("UXStart", Float) = 0
    UYStart ("UYStart", Float) = 0
    UXWidth ("UXWidth", Float) = 1
    UYHeight ("UYHeight", Float) = 1
    PathTime ("PathTime", Float) = 0
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
        float2 uv : TEXCOORD0;
      };
      
      struct v2f
      {
        float2 uv : TEXCOORD0;
        float4 vertex : SV_POSITION;
      };
      
      sampler2D NoiseTex;
      float4 NoiseTex_ST;
      float UXStart;
      float UYStart;
      float UXWidth;
      float UYHeight;
      float PathTime;
      
      v2f vert (appdata v)
      {
        v2f o;
        o.vertex = UnityObjectToClipPos(v.vertex);
        o.uv = TRANSFORM_TEX(v.uv, NoiseTex);
        return o;
      }
      
      #define SC (250.0)
      
      // value noise, and its analytical derivatives
      float3 noised (in float2 x)
      {
        // Retrieve fractional component of x.
        float2 f = frac (x);
        float2 u = f * f * (3.0 - 2.0 * f);
        
        // Use texture to determine neighbouring grid points from noise texture.
        float2 p = floor(x);
        float a = tex2Dlod (NoiseTex, float4 ((p + float2 (0.5, 0.5)) / 256.0, 0.0, 0.0)).x;
        float b = tex2Dlod (NoiseTex, float4 ((p + float2 (1.5, 0.5)) / 256.0, 0.0, 0.0)).x;
        float c = tex2Dlod (NoiseTex, float4 ((p + float2 (0.5, 1.5)) / 256.0, 0.0, 0.0)).x;
        float d = tex2Dlod (NoiseTex, float4 ((p + float2 (1.5, 1.5)) / 256.0, 0.0, 0.0)).x;
        
        // Interpolate between grid points, based on fractional position.
        return float3 (a + (b-a) * u.x + (c-a) * u.y + (a-b-c+d) *u.x * u.y,
                       6.0 * f * (1.0-f) * (float2 (b-a, c-a) + (a-b-c+d) * u.yx));
      }
      
      // fBm based terrain. Select number of layers.
      float terrain (in float2 x, int layers)
      {
        float2x2 m2 = float2x2 (0.8, -0.6, 0.6, 0.8);
        float2 p = x * 0.003 / SC;
        float a = 0.0;
        float b = 1.0;
        float2 d = float2 (0.0, 0.0);
        for (int i = 0; i < layers; i++)
        {
          float3 n = 2.0 * noised (p);
          d += n.yz;
          a += b * n.x / (1.0 + dot (d,d));
          b *= 0.5;
          p = mul (m2, p * 2.0);
        }
        
        return SC * 120.0 * a;
      }
      
      // Ray marching to find intersection of ray with terrain, returning
      // ray parameter t of point of intersection, or terminating at given
      // maximum distance along ray.
      float intersect( in float3 ro, in float3 rd, in float tmin, in float tmax )
      {
        float t = tmin;
        for( int i=0; i<300; i++ )
        {
          float3 pos = ro + t*rd;
          float h = pos.y - terrain (pos.xz, 9);
          if( abs(h)<(0.002*t) || t>tmax ) break;
          t += 0.4*h;
        }
        
        return t;
      }
      
      // Compute shadowing in terrain.
      float softShadow(in float3 ro, in float3 rd )
      {
        float res = 1.0;
        // Position along ray, start at origin point.
        float t = 0.1 * SC;
        
        for( int i=0; i<80; i++ )
        {
          // Calculate current position on ray, determined by position t.
          float3  p = ro + t*rd;
          // Calculate height above terrain of point on ray.
          float h = p.y - terrain (p.xz, 9);
          
          // Getting some artefacts from soft shadows so limit
          // maximum darkness.
          if (h < 0.0)
            h = 0.0;
          
          // Find the minimum distance between the ray and the
          // terrain. The further along the ray the closest
          // point is, the more light is allowed.
          res = min (res, 1.6 * h / t);
          
          // Take another sameple along the ray.
          t += h;
          
          // Quit once shadow established, or well about terrain.
          if (res < 0.3 || p.y > (SC * 200.0))
            break;
        }
        return clamp (res, 0.0, 1.0);
      }
      
      float3 calcNormal( in float3 pos, float t )
      {
        // small epsilon offset.
        float2 eps = float2 (0.002 * t, 0.0);
        return normalize (float3 (terrain (pos.xz - eps.xy, 16) - terrain (pos.xz + eps.xy, 16),
                                  2.0 * eps.x,
                                  terrain (pos.xz - eps.yx, 16) - terrain (pos.xz + eps.yx, 16)));
      }
      
      float fbm( float2 p )
      {
        float2x2 m2 = float2x2 (0.8,-0.6,0.6,0.8);
        float f = 0.0;
        f += 0.5000 * tex2D (NoiseTex, p / 256.0).x; p = mul (m2, p * 2.02);
        f += 0.2500 * tex2D (NoiseTex, p / 256.0 ).x; p = mul (m2, p * 2.03);
        f += 0.1250 * tex2D (NoiseTex, p / 256.0 ).x; p = mul (m2, p * 2.01);
        f += 0.0625 * tex2D (NoiseTex, p / 256.0 ).x;
        return f / 0.9375;
      }
      
      float4 render( in float3 ro, in float3 rd )
      {
        float kMaxT = 500.0*SC;
        float3 light1 = normalize (float3 (-0.8, 1.0, -0.3));
        // bounding plane
        float tmin = 0.1;
        float tmax = kMaxT;
        #if 1
        float maxh = 3000.0 * SC;
        float tp = (maxh - ro.y) / rd.y;
        if (tp > 0.0)
        {
          if (ro.y > maxh)
            tmin = max (tmin, tp);
          else    
            tmax = min (tmax, tp);
        }
        #endif
        float sundot = clamp (0.22 * dot (rd, light1), 0.0, 1.0);
        float3 col;
        float t = intersect (ro, rd, tmin, tmax);
        
        if (t > tmax)
        {
          // No terrain intersection, so just draw the sky.
          // sky		
          float altitudeFactor = rd.y*rd.y*0.25;
          col = clamp (float3(0.2,0.5,0.85)*1.1 - float3 (altitudeFactor, altitudeFactor, altitudeFactor), 0.0, 1.0);
          col = lerp (col, 0.85 * float3 (0.7, 0.75, 0.85), pow (1.0 - max (rd.y, 0.0), 4.0));
          // sun
          col += 0.25*float3(1.0,0.7,0.4)*pow( sundot,5.0 );
          col += 0.25*float3(1.0,0.8,0.6)*pow( sundot,64.0 );
          col += 0.2*float3(1.0,0.8,0.6)*pow( sundot,512.0 );
          // clouds
          float2 sc = ro.xz + rd.xz*(SC*1000.0-ro.y)/rd.y;
          col = lerp (col, float3 (1.0, 0.95, 1.0), 0.5 * smoothstep (0.5, 0.8, fbm (0.005 * sc / SC)));
          // horizon
          col = lerp (col, 0.68 * float3 (0.4, 0.65, 1.0), pow (1.0 - max (rd.y, 0.0), 16.0));
          t = -1.0;
        }
        else
        {
          // mountains		
          // Assuming ray hit the mountain, then find position of intersection.
          float3 pos = ro + t * rd;
          // Normal vector to the terrain surface.
          float3 nor = calcNormal (pos, t);
          
          float3 ref = reflect (rd, nor);
          float fre = clamp (1.0 + dot (rd, nor), 0.0, 1.0);
          float3 hal = normalize (light1 - rd);
          
          // rock
          float r = tex2D (NoiseTex, (7.0 / SC) * pos.xz / 256.0).x;
          col = (r * 0.25 + 0.75) * 0.9 * lerp (float3 (0.08, 0.05, 0.03), float3 (0.10, 0.09, 0.08), 
                                                tex2D (NoiseTex, 0.00007 * float2 (pos.x, pos.y * 48.0) / SC).x);
          col = lerp (col, 0.20 * float3 (0.45, .30, 0.15) * (0.50 + 0.50 * r), smoothstep (0.70, 0.9, nor.y));
          col = lerp (col, 0.15 * float3 (0.30, .30, 0.10) * (0.25 + 0.75 * r), smoothstep (0.95, 1.0, nor.y));
          col *= 0.1+1.8*sqrt(fbm(pos.xz*0.04)*fbm(pos.xz*0.005));
          
          // snow
          float h = smoothstep (55.0, 80.0, pos.y / SC + 25.0 * fbm (0.1 * pos.xz / SC));
          float e = smoothstep (1.0 - 0.9 * h, 1.0 - 0.1 * h, nor.y);
          float o = 0.3 + 0.7 * smoothstep (0.0, 0.1, nor.x + h * h);
          float s = h * e * o;
          col = lerp (col, 0.29 * float3 (0.62, 0.65, 0.7), smoothstep (0.1, 0.9, s));
          
          // lighting		
          // Ambient lighting term.
          float amb = clamp(0.5+0.5*nor.y,0.0,1.0);
          // Diffuse lighting term.
          float dif = clamp( dot( light1, nor ), 0.0, 1.0 );
          float bac = clamp( 0.2 + 0.8*dot( normalize( float3(-light1.x, 0.0, light1.z ) ), nor ), 0.0, 1.0 );
          float sh = 1.0; 
          
          // If area getting some light, then calculate soft shadow term.
          if (dif >= 0.0001)
          {
            // Cast ray from current position (small offset towards light) in the light direction.
            sh = softShadow (pos + light1 * SC * 0.05, light1);
          }
          
          float3 lin = float3 (0.0, 0.0, 0.0);
          lin += dif*float3(7.00,5.00,3.00)*1.3*float3( sh, sh*sh*0.5+0.5*sh, sh*sh*0.8+0.2*sh );
          lin += amb*float3(0.40,0.60,1.00)*1.2;
          lin += bac*float3(0.40,0.50,0.60);
          col *= lin;
          
          col += (0.7+0.3*s)*(0.04+0.96*pow(clamp(1.0+dot(hal,rd),0.0,1.0),5.0))*
          float3(7.0,5.0,3.0)*dif*sh*
          pow( clamp(dot(nor,hal), 0.0, 1.0),16.0);
          
          col += s*0.65*pow(fre,4.0)*float3(0.3,0.5,0.6)*smoothstep(0.0,0.6,ref.y);
          
          // fog
          float fo = 1.0-exp(-pow(0.0005*t/SC,1.5) );
          float3 fco = 0.65*float3(0.4,0.65,1.0);// + 0.1*float3(1.0,0.8,0.5)*pow( sundot, 4.0 );
          col = lerp (col, fco, fo);
        }
        // sun scatter
        col += 0.3*float3(1.0,0.7,0.3)*pow( sundot, 8.0 );
        
        // gamma
        col = sqrt(col);
        
        return float4( col, 1 );
      }
      
      float3 camPath( float time )
      {
        return SC*1100.0*float3( cos(0.0+0.23*time), 0.0, cos(1.5+0.21*time) );
      }
      
      fixed4 frag (v2f i) : SV_Target
      {
        float ux = (i.uv.x) * UXWidth + UXStart;
        float uy = (i.uv.y) * UYHeight + UYStart;
        float latitude = (uy - 0.5) * UNITY_PI;
        float longitude = (1.0 - ux) * 2 * UNITY_PI;
        
        fixed4 fragColor;
        
        float time = PathTime * 0.1 - 0.1 + 0.3;
        
        // camera position
        float3 ro; // current camera position
        ro = camPath (time);
        ro.y = terrain (ro.xz, 9) + 5.0 * SC;
        
        float3 rd = normalize (float3 (sin (longitude) * cos(latitude), sin (latitude), cos (longitude) * cos (latitude))); // ray direction.
        
        float4 res = render( ro, rd );
        fragColor = float4( res);
        return fragColor;
      }
      
      ENDCG
    }
  }
}
