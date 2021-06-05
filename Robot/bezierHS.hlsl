struct VS_CONTROL_POINT_OUTPUT
{
	float4 vPosition : SV_POSITION;
	float3 normal : NORMAL;
};

struct HS_CONTROL_POINT_OUTPUT
{
	float3 vPosition : WORLDPOS;
};

struct HS_CONSTANT_DATA_OUTPUT
{
	float EdgeTessFactor[4]			: SV_TessFactor;
	float InsideTessFactor[2]			: SV_InsideTessFactor;

	float2 tex[4] : TEXTURECOORD;
};

#define NUM_CONTROL_POINTS 16

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

float factor(float z) {
	return 0.5f*-16.0f * log10(z * 0.01);
}

HS_CONSTANT_DATA_OUTPUT CalcHSPatchConstants(
	InputPatch<VS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> ip,
	uint PatchID : SV_PrimitiveID)
{
	HS_CONSTANT_DATA_OUTPUT Output;

	//Output.EdgeTessFactor[0] = 
	//	Output.EdgeTessFactor[1] = 
	//	Output.EdgeTessFactor[2] = 
	//	Output.EdgeTessFactor[3] = 
	//	Output.InsideTessFactor[0] =
	//	Output.InsideTessFactor[1] = 15;

	float4 leftPoint = ip[5].vPosition + (ip[6].vPosition - ip[5].vPosition) / 2.0f;
	float4 rightPoint = ip[9].vPosition + (ip[10].vPosition - ip[9].vPosition) / 2.0f;
	float4 upPoint = ip[5].vPosition + (ip[9].vPosition - ip[5].vPosition) / 2.0f;
	float4 downPoint = ip[6].vPosition + (ip[10].vPosition - ip[6].vPosition) / 2.0f;

	Output.EdgeTessFactor[0] = factor(mul(viewMatrix, leftPoint).z);
	Output.EdgeTessFactor[3] = factor(mul(viewMatrix, downPoint).z);
	Output.EdgeTessFactor[2] = factor(mul(viewMatrix, rightPoint).z);
	Output.EdgeTessFactor[1] = factor(mul(viewMatrix, upPoint).z);
	Output.InsideTessFactor[0] = Output.EdgeTessFactor[0] + (Output.EdgeTessFactor[2] - Output.EdgeTessFactor[0]) / 2.0f;
	Output.InsideTessFactor[1] = Output.EdgeTessFactor[1] + (Output.EdgeTessFactor[3] - Output.EdgeTessFactor[1]) / 2.0f;


	Output.tex[0] = ip[0].normal.xy;
	Output.tex[1] = ip[3].normal.xy;
	Output.tex[2] = ip[12].normal.xy;
	Output.tex[3] = ip[15].normal.xy;
	return Output;
}

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(NUM_CONTROL_POINTS)]
[patchconstantfunc("CalcHSPatchConstants")]
HS_CONTROL_POINT_OUTPUT main( 
	InputPatch<VS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> ip, 
	uint i : SV_OutputControlPointID,
	uint PatchID : SV_PrimitiveID )
{
	HS_CONTROL_POINT_OUTPUT Output;

	Output.vPosition = ip[i].vPosition.xyz;

	return Output;
}