float2 positions[6] = {
    float2(-1,  1),
    float2(1,  1),
    float2(-1, -1),
    float2(-1, -1),
    float2(1,  1),
    float2(1, -1)
};
float2 uvs[6] = {
    float2(0.0, 0.0),
    float2(1.0, 0.0),
    float2(0.0, 1.0),
    float2(0.0, 1.0),
    float2(1.0, 0.0),
    float2(1.0, 1.0)
};

struct VertexIn
{
};

struct VertexOut
{
    float4 PosH  : SV_POSITION;
    float2 Uvs   : TEXCOORD0;
};

cbuffer cpPerObject
{
    float gTimer;
};

Texture2D gBackBuffer;
Texture2D gBloomBuffer;

SamplerState samPoint
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

SamplerState samLinear
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

VertexOut VS(VertexIn vin, uint vertID : SV_VertexID)
{
    VertexOut vout;

    uint index = vertID % 6;

    vout.PosH = float4(positions[index].xy, 0, 1);
    vout.Uvs = uvs[index];

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{


    // Ripples     
    /*
    float distCenter = length(pin.Uvs - 0.5f);
    float d = sin(distCenter * 50.0f - gTimer * 4.0f);
    float2 dir = normalize(pin.Uvs - 0.5f);
    float2 rippleCoords = pin.Uvs + d * dir * 0.01f;
    
    // Pixelation
    float2 dims = float2(64.0, 64.0);
    float2 coord = floor(rippleCoords * dims) / dims;
    */
    float2 coord = pin.Uvs;
    
    // this is for 4xmsaa disable
    float4 color = gBackBuffer.Sample(samPoint, coord);
    float4 bloom = gBloomBuffer.Sample(samLinear, coord);

    color += bloom;
    
    // HDR
    float3 hdrColor = color.rgb;

    // tone mapping
    //float3 mapped = hdrColor / (hdrColor + float3(1.0f, 1.0f, 1.0f));

    // exposure tone mapping
    float exposure = 1.0f;
    float3 mapped = float3(1.0f, 1.0f, 1.0f) - exp(-hdrColor * exposure);

    // gama correction
    float invgamma = 1.0f / 2.2;
    mapped = pow(abs(mapped), invgamma.xxx);
    
    
    return float4(mapped, 1.0f);
}

technique11 HdrTech
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }
}