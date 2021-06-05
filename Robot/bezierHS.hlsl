struct VS_CONTROL_POINT_OUTPUT
{
	float4 vPosition : SV_POSITION;
	float3 normal : NORMAL;
};

struct HS_CONTROL_POINT_OUTPUT
{
	float3 vPosition : WORLDPOS;
	float2 tex : TEXTURECOORD;
};

struct HS_CONSTANT_DATA_OUTPUT
{
	float EdgeTessFactor[4]			: SV_TessFactor;
	float InsideTessFactor[2]			: SV_InsideTessFactor;
	float z_val[4] : ZVAL;
	
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

cbuffer cbdivisions : register(b3) //Pixel Shader constant buffer slot 0 - matches slot in psBilboard.hlsl
{
	float4 divisions;
}

float factor(float z) {
	return divisions .x*-16.0f * log10(z * 0.1);
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
	Output.EdgeTessFactor[0] = Output.EdgeTessFactor[0] < 3 ? 3 : Output.EdgeTessFactor[0];
	Output.EdgeTessFactor[1] = Output.EdgeTessFactor[1] < 3 ? 3 : Output.EdgeTessFactor[1];
	Output.EdgeTessFactor[2] = Output.EdgeTessFactor[2] < 3 ? 3 : Output.EdgeTessFactor[2];
	Output.EdgeTessFactor[3] = Output.EdgeTessFactor[3] < 3 ? 3 : Output.EdgeTessFactor[3];
	
	Output.InsideTessFactor[0] = Output.EdgeTessFactor[0] + (Output.EdgeTessFactor[2] - Output.EdgeTessFactor[0]) / 2.0f;
	Output.InsideTessFactor[1] = Output.EdgeTessFactor[1] + (Output.EdgeTessFactor[3] - Output.EdgeTessFactor[1]) / 2.0f;

	Output.z_val[0] = mul(viewMatrix, leftPoint).z;
	Output.z_val[1] = mul(viewMatrix, downPoint).z;
	Output.z_val[2] = mul(viewMatrix, rightPoint).z;
	Output.z_val[3] = mul(viewMatrix, upPoint).z;

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
	Output.tex = ip[i].normal.xy;

	return Output;
}
