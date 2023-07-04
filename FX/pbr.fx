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
    PointLight gPointLight0;
    PointLight gPointLight1;
    PointLight gPointLight2;
    PointLight gPointLight3;
    
    float4x4 gWorld;
    float4x4 gWorldViewProj;
    float4x4 gWorldInvTranspose;   
    float3 gCamPos;
};

struct VertexIn
{
    float3 Pos : POSITION;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
    float2 TexCoord : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 WorldPos : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD0;
};

static const float PI = 3.14159265359;

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
	
    vout.PosH = mul(float4(vin.Pos, 1.0f), gWorldViewProj);
    vout.WorldPos = mul(float4(vin.Pos, 1.0f), gWorld).xyz;
    vout.Normal = mul(vin.Normal, (float3x3) gWorldInvTranspose);
    vout.TexCoord = vin.TexCoord;

    return vout;
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

float4 PS(VertexOut pin) : SV_Target
{

    float3 Albedo = gMaterial.Albedo.rgb;
    float Metallic = gMaterial.Metallic;
    float Roughness = gMaterial.Roughness;
    float Ao = gMaterial.Ao;
    
    float3 N = normalize(pin.Normal);
    float3 V = normalize(gCamPos - pin.WorldPos);

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0
    // of 0.04 and if it's a metal, use the albedo color as F0 (metalic workflow)
    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, Albedo, Metallic);
    
    float3 LightsPos[4] = { 
        gPointLight0.Position,
        gPointLight1.Position,
        gPointLight2.Position,
        gPointLight3.Position
    };
    float3 LightsColor[4] = {
        gPointLight0.Color.rgb,
        gPointLight1.Color.rgb,
        gPointLight2.Color.rgb,
        gPointLight3.Color.rgb 
    };
    
    // reflectance equation
    float3 Lo = float3(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < 4; ++i)
    {
        // calculate per-light radiance
        float3 L = normalize(LightsPos[i] - pin.WorldPos);
        float3 H = normalize(V + L);
        float distance = length(LightsPos[i] - pin.WorldPos);
        float attenuation = 1.0f / (distance * distance);
        float3 radiance = LightsColor[i] * attenuation;
        
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

    float3 color = ambient + Lo;

    // HDR tonemapping
    color = color / (color + float3(1.0f, 1.0f, 1.0f));
    // gamma correct
    color = pow(abs(color), float3(1.0f / 2.2f, 1.0f / 2.2f, 1.0f / 2.2f));
    
    return float4(color, 1.0f);

}

technique11 PbrTech
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }
}
    
