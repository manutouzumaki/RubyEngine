cbuffer cbPerObject
{
    float4x4 gWorld;
    float4x4 gWorldViewProj;
};

struct VertexIn
{
	float3 PosL     : POSITION;
    float3 NormalL  : NORMAL;
    float4 Tangent  : TANGENT;
    float2 TexCoord : TEXCOORD;
};

struct VertexOut
{
	float4 PosH     : SV_POSITION;
    float3 NormalW  : NORMAL;
    float3 NormalL : TEXCOORD0;

};


VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
    vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);
    vout.NormalL = abs(vin.NormalL);
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    float3 lightDir = normalize(float3(0.2, 0.5, -1));
    
    float3 color = pin.NormalL * max(dot(lightDir, normalize(pin.NormalW)), 0.1f);
    
    return float4(color, 1.0f);
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
