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

float weight[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };

cbuffer cbPerObject
{
    bool horizontal;
};

struct VertexIn
{
};

struct VertexOut
{
    float4 PosH  : SV_POSITION;
    float2 Uvs   : TEXCOORD0;
};

Texture2D gImage;

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
    uint Width = 0;
    uint Height = 0;
    gImage.GetDimensions(Width, Height);
    float2 texOffset = 1.0f / float2(Width, Height);

    float3 result = gImage.Sample(samLinear, pin.Uvs).rgb * weight[0];
    if (horizontal)
    {
        for (int i = 1; i < 5; ++i)
        {
            result += gImage.Sample(samLinear, pin.Uvs + float2(texOffset.x * i, 0.0f)).rgb * weight[i];
            result += gImage.Sample(samLinear, pin.Uvs - float2(texOffset.x * i, 0.0f)).rgb * weight[i];
        }
    }
    else 
    {
        for (int i = 1; i < 5; ++i)
        {
            result += gImage.Sample(samLinear, pin.Uvs + float2(0.0f, texOffset.y * i)).rgb * weight[i];
            result += gImage.Sample(samLinear, pin.Uvs - float2(0.0f, texOffset.y * i)).rgb * weight[i];
        }
    }
 
    return float4(result, 1.0f);
}

technique11 BlurTech
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }
}