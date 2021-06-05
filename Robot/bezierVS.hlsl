cbuffer cbWorld : register(b0) //Vertex Shader constant buffer slot 0 - matches slot in vsBilboard.hlsl
{
	matrix worldMatrix;
};

cbuffer cbView : register(b1) //Vertex Shader constant buffer slot 1 - matches slot in vsBilboard.hlsl
{
	matrix viewMatrix;
	matrix invViewMatrix;
};

cbuffer cbProj : register(b2) //Vertex Shader constant buffer slot 2 - matches slot in vsBilboard.hlsl
{
	matrix projMatrix;
};


struct VSInput
{
	float3 pos : POSITION;
	float3 norm : NORMAL;
};

struct VSOutput
{
	float4 vPosition : SV_POSITION;
	float3 normal : NORMAL;
};

VSOutput main(VSInput i )
{
	VSOutput o;
	o.vPosition = mul(worldMatrix, float4(i.pos, 1.0f));
	o.normal = i.norm;
	return o;
}