cbuffer cbPerFrame
{
    float4x4 gWorldViewProj;
    float gTimer;
};

TextureCube gCubeMap;

SamplerState samTrilinearSam
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

struct VerterxIn
{
    float3 PosL : POSITION;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosL : POSITIONT;
};

VertexOut VS(VerterxIn vin)
{
    VertexOut vout;
    
    vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj).xyww;
    vout.PosL = vin.PosL;
    
    return vout;   
}

float4 PS(VertexOut pin) : SV_Target
{
    float3 localPos = normalize(pin.PosL);
        
    float4 color = gCubeMap.Sample(samTrilinearSam, localPos);

    return color;
}

RasterizerState NoCull
{
    CullMode = None;
};

DepthStencilState LessEqualDSS
{
    DepthFunc = LESS_EQUAL;
};

technique11 SkyTech
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS()));
        
        SetRasterizerState(NoCull);
        SetDepthStencilState(LessEqualDSS, 0);

    }
}