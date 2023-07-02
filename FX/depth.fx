//***************************************************************************************
// color.fx by Frank Luna (C) 2011 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************


cbuffer cbPerObject
{
    float4x4 gWorld;
	float4x4 gLightSpaceMatrix;

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
	float4 PosH    : SV_POSITION;
};


VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
    float4x4 model = mul(gWorld, gLightSpaceMatrix);
	vout.PosH = mul(float4(vin.PosL, 1.0f), model);
	    
    return vout;
}


float4 PS(VertexOut pin) : SV_Target
{
}


technique11 DepthTech
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_5_0, VS() ) );
		SetGeometryShader( NULL );
        SetPixelShader(NULL);
        //SetPixelShader( CompileShader( ps_5_0, PS() ) );
    }
}
