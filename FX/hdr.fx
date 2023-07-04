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
    // this is for 4xmsaa enable
    /*
    float4 color = float4(0, 0, 0, 1.0f);
    uint width = 0;
    uint height = 0;
    uint sampleCount = 0;
    gBackBuffer.GetDimensions(width, height, sampleCount);

    for (uint i = 0; i < sampleCount; i++)
    {
        int2 coords = int2(pin.Uvs * float2(width, height));
        color += gBackBuffer.Load(coords, i);
    }

    color /= sampleCount;
    */

    // this is for 4xmsaa disable
    float4 color = gBackBuffer.Sample(samPoint, pin.Uvs);
    float4 bloom = gBloomBuffer.Sample(samLinear, pin.Uvs);

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

    //return color;
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