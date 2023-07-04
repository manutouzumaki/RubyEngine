struct DirectionalLight
{
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;
    float3 Direction;
    float pad;
};

struct PointLight
{
    float4 Ambient;
    float4 Diffuse;
    float4 Specular;

    float3 Position;
    float Range;

    float3 Att;
    float pad;
};

struct Material
{
    float4 Ambient;
    float4 Diffuse;
    float4 Specular; // w = SpecPower
    float4 Reflect;
};

cbuffer cbPerObject
{
    float4x4 gWorld;
    float4x4 gWorldInvTranspose;
	float4x4 gWorldViewProj;
    float4x4 gLightSpaceMatrix;
    Material gMaterial;
    DirectionalLight gDirLight;
    PointLight gPointLight;
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
	float3 PosL     : POSITION;
    float3 NormalL  : NORMAL;
    float3 Tangent  : TANGENT;
    float2 TexCoord : TEXCOORD;
};

struct VertexOut
{
	float4 PosH                 : SV_POSITION;
    float3 PosW                 : POSITION;
    float3 NormalW              : NORMAL;
    float2 TexCoord             : TEXCOORD0;
    float4 FragPosLightSpace    : TEXCOORD1;
};

void ComputeDirectionalLight(Material mat, DirectionalLight L,
    float3 normal, float3 toEye,
    out float4 ambient,
    out float4 diffuse,
    out float4 spec)
{
    // Initialize outputs.
    ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

    // The light vector aims opposite the direction the light rays travel.
    float3 lightDir = normalize( - L.Direction);
    float3 viewDir = toEye;
    float3 halfwayDir = normalize(lightDir + viewDir);

    // Add ambient term.
    ambient = mat.Ambient * L.Ambient;

    // Add diffuse and specular term, provided the surface is in 
    // the line of site of the light.

    float diffuseFactor = dot(lightDir, normal);

    // Flatten to avoid dynamic branching.
    [flatten]
    if (diffuseFactor > 0.0f)
    {
        float specFactor = pow(max(dot(normal, halfwayDir), 0.0f), mat.Specular.w);
        diffuse = diffuseFactor * mat.Diffuse * L.Diffuse;

        // gamma correction
        float gamma = 2.2;
        diffuse = pow(abs(diffuse), float4(gamma.xxxx));

        spec = specFactor * mat.Specular * L.Specular;
    }
}

void ComputePointLight(Material mat, PointLight L, float3 pos, float3 normal, float3 toEye,
    out float4 ambient, out float4 diffuse, out float4 spec)
{
    // Initialize outputs.
    ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

    // The vector from the surface to the light.
    float3 lightVec = L.Position - pos;

    // The distance from surface to light.
    float d = length(lightVec);

    // Range test.
    if (d > L.Range)
        return;

    // Normalize the light vector.
    lightVec /= d;

    // Ambient term.
    ambient = mat.Ambient * L.Ambient;

    // Add diffuse and specular term, provided the surface is in 
    // the line of site of the light.

    float diffuseFactor = dot(lightVec, normal);

    // Flatten to avoid dynamic branching.
    [flatten]
    if (diffuseFactor > 0.0f)
    {
        float3 v = reflect(-lightVec, normal);
        float specFactor = pow(max(dot(v, toEye), 0.0f), mat.Specular.w);

        diffuse = diffuseFactor * mat.Diffuse * L.Diffuse;

        // gamma correction
        float gamma = 2.2;
        diffuse = pow(abs(diffuse), float4(gamma.xxxx));

        spec = specFactor * mat.Specular * L.Specular;
    }

    // Attenuate
    float att = 1.0f / dot(L.Att, float3(1.0f, d, d));

    diffuse *= att;
    spec *= att;
}


VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	// Transform to homogeneous clip space.
    vout.PosW = mul(float4(vin.PosL, 1.0f), gWorld).xyz;
	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
    vout.NormalW = mul(vin.NormalL, (float3x3)gWorldInvTranspose);

    float4x4 model = mul(gWorld, gLightSpaceMatrix);
    vout.FragPosLightSpace = mul(float4(vin.PosL, 1.0f), model);
    vout.TexCoord = vin.TexCoord;

    return vout;
}

float ShadowCalculation(float4 fragPosLightSpace, DirectionalLight L, float3 normal)
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

struct PixelOut
{
    float4 Color: SV_Target0;
    float4 Bright: SV_Target1;
};

PixelOut PS(VertexOut pin)
{

    pin.NormalW = normalize(pin.NormalW);

    float3 toEyeW = normalize(gEyePosW - pin.PosW);

    
    float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

    float4 A, D, S;

    ComputeDirectionalLight(gMaterial, gDirLight, pin.NormalW, toEyeW, A, D, S);
    ambient += A;
    diffuse += D;
    spec += S;

    ComputePointLight(gMaterial, gPointLight, pin.PosW, pin.NormalW, toEyeW, A, D, S);
    ambient += A;
    diffuse += D;
    spec += S;

    float shadow = ShadowCalculation(pin.FragPosLightSpace, gDirLight, pin.NormalW);

    float2 uvs = float2(pin.TexCoord.x, 1.0f - pin.TexCoord.y);
    float4 depth = float4(gShadowMap.Sample(samLinear, uvs).xxx, 1);
    //return depth;


    float4 litColor = (ambient + (1.0f - shadow) * (diffuse + spec));
    litColor.a = gMaterial.Diffuse.a;

    if (gMaterial.Ambient.w == 0.0f)
    {
        litColor = gMaterial.Ambient;
    }


    PixelOut output;

    output.Color = litColor;
    
    float brightness = dot(litColor.rgb, float3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0)
        output.Bright = litColor;
    else
        output.Bright = float4(0.0, 0.0, 0.0, 1.0);
    
    
    return output;
}

technique11 ColorTech
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_5_0, VS() ) );
		SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_5_0, PS() ) );
    }
}
