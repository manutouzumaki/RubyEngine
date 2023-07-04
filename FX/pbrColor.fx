struct DirectionalLight
{
    float4 Color;
    float3 Direction;
    float Pad;
};

struct PointLight
{
    float4 Color;
    float3 Position;
    float Pad;
};


struct Material
{
    float4 Albedo;
    float Metallic;
    float Roughness;
    float Ao;
    float Pad;
};


cbuffer cbPerObject
{
    Material gMaterial;
    DirectionalLight gDirLight;
    PointLight gPointLight;
    
    float4x4 gWorld;
    float4x4 gWorldInvTranspose;
    float4x4 gWorldViewProj;
    float4x4 gLightSpaceMatrix;
    float3 gEyePosW;

};

Texture2D gShadowMap;

SamplerState samLinear
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float3 Tangent : TANGENT;
    float2 TexCoord : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 TexCoord : TEXCOORD0;
    float4 FragPosLightSpace : TEXCOORD1;
};

static const float PI = 3.14159265359;

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
	
    vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
    vout.PosW = mul(float4(vin.PosL, 1.0f), gWorld).xyz;
    vout.NormalW = mul(vin.NormalL, (float3x3) gWorldInvTranspose);
    vout.TexCoord = vin.TexCoord;
    vout.FragPosLightSpace = mul(float4(vin.PosL, 1.0f), mul(gWorld, gLightSpaceMatrix));

    return vout;
}

float ShadowCalculation(float4 fragPosLightSpace)
{
    float3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    float u = 0.5f * projCoords.x + 0.5f;
    float v = -0.5f * projCoords.y + 0.5f;
    float2 uvs = float2(u, v);

    // get closest depth value from light's perspective
    float4 depth = float4(gShadowMap.Sample(samLinear, uvs).xxx, 1);
    float closestDepth = depth.r;
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow
   
    float bias = 0.01f;
    float shadow = 0.0f;
    if ((currentDepth - bias) > closestDepth)
    {
        shadow = 1.0;
    }

    if (projCoords.z > 1.0)
        shadow = 0.0;

    return shadow;
}

// TODO: pbr fragemnt Shader ...
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

struct PixelOut
{
    float4 Color : SV_Target0;
    float4 Bright : SV_Target1;
};

PixelOut PS(VertexOut pin)
{
    float shadow = ShadowCalculation(pin.FragPosLightSpace);
    
    float3 Albedo = gMaterial.Albedo.rgb;
    float3 isBloom = gMaterial.Albedo.a;
    float Metallic = gMaterial.Metallic;
    float Roughness = gMaterial.Roughness;
    float Ao = gMaterial.Ao;
    
    
    float3 N = normalize(pin.NormalW);
    float3 V = normalize(gEyePosW - pin.PosW);
    
    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, Albedo, Metallic);
    
    float3 LightsDir[2] = { gDirLight.Direction, gPointLight.Position - pin.PosW };
    float3 LightsColor[2] = { gDirLight.Color.rgb, gPointLight.Color.rgb };
    
     // reflectance equation
    float3 Lo = float3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < 2; ++i)
    {
        // calculate per-light radiance
        float3 L = normalize(LightsDir[i]);
        float3 H = normalize(V + L);
        float distance = length(LightsDir[i]);
        float attenuation = 1.0f / (distance * distance);
        float3 radiance = LightsColor[i] * attenuation;
        if (i == 0)
        {
            radiance = LightsColor[i];
        }
        
        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, Roughness);
        float G = GeometrySmith(N, V, L, Roughness);
        float3 F = FresnelSchlick(clamp(dot(H, V), 0.0f, 1.0f), F0);
        
        float3 numerator = NDF * G * F;
        float denominator = 4.0f * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0f) + 0.0001f; // + 0.0001 to prevent divide by zero
        float3 specular = numerator / denominator;
        
        // kS is equal to Fresnel
        float3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - Metallic;
        
        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0f);
        
        Lo += (kD * Albedo / PI + specular) * radiance * NdotL; // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
        
    }
    
    // ambient lighting (note that the next IBL tutorial will replace 
    // this ambient lighting with environment lighting).
    float3 ambient = float3(0.03f, 0.03f, 0.03f) * Albedo * Ao;

    float3 color = ambient + ((1.0f - shadow) * Lo);
    //float3 color = ambient + Lo;

    if (gMaterial.Albedo.w == 0.0f)
    {
        color = gMaterial.Albedo.rgb;
    }
    
    PixelOut output;
    
    // HDR tonemapping
    //color = color / (color + float3(1.0f, 1.0f, 1.0f));
    // gamma correct
    //color = pow(abs(color), float3(1.0f / 2.2f, 1.0f / 2.2f, 1.0f / 2.2f));

    output.Color = float4(color, 1.0f);
    
    float brightness = dot(color, float3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0)
        output.Bright = float4(color, 1.0f);
    else
        output.Bright = float4(0.0, 0.0, 0.0, 1.0);
    
    
    return output;
}

technique11 ColorTech
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }
}