Texture2D heighMap : register(t0);
SamplerState colorSampler : register(s0);

struct DS_OUTPUT
{
	float4 vPosition  : SV_POSITION;
	float2 tex : TEXCORD;
	float3 tangent: TANGENT;
	float3 normal: NORMAL;
	float3 binormal: BINORMAL;
	float3 worldPos : POSITION;
	float3 view : VIEW;
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

float3 drawBrezier4(float t, float3 B0_, float3 B1_, float3 B2_, float3 B3_) {
	float T0 = -1.0f;
	float T1 = 0.0f;
	float T2 = 1.0f;
	float T3 = 2.0f;
	float T4 = 3.0f;
	float Tm1 = -2.0f;

	float A1 = T2 - t;
	float A2 = T3 - t;
	float A3 = T4 - t;
	float B1 = t - T1;
	float B2 = t - T0;
	float B3 = t - Tm1;

	float N1 = 1;
	float N2 = 0;
	float N3 = 0;
	float N4 = 0;

	float saved = 0.0f;
	float term = 0.0f;

	term = N1 / (A1 + B1);
	N1 = saved + A1 * term;
	saved = B1 * term;

	N2 = saved;
	saved = 0.0f;

	term = N1 / (A1 + B2);
	N1 = saved + A1 * term;
	saved = B2 * term;

	term = N2 / (A2 + B1);
	N2 = saved + A2 * term;
	saved = B1 * term;

	N3 = saved;
	saved = 0.0f;

	term = N1 / (A1 + B3);
	N1 = saved + A1 * term;
	saved = B3 * term;

	term = N2 / (A2 + B2);
	N2 = saved + A2 * term;
	saved = B2 * term;

	term = N3 / (A3 + B1);
	N3 = saved + A3 * term;
	saved = B1 * term;

	N4 = saved;

	return N1 * B0_ + N2 * B1_ + N3 * B2_ + N4 * B3_;
}


float3 TangentVec(float t, float3 B0_, float3 B1_, float3 B2_, float3 B3_) {
	float T0 = -1.0f;
	float T1 = 0.0f;
	float T2 = 1.0f;
	float T3 = 2.0f;
	float T4 = 3.0f;
	float Tm1 = -2.0f;

	float A1 = T2 - t;
	float A2 = T3 - t;
	float A3 = T4 - t;
	float B1 = t - T1;
	float B2 = t - T0;
	float B3 = t - Tm1;

	float N1 = 1;
	float N2 = 0;
	float N3 = 0;
	float N4 = 0;

	float saved = 0.0f;
	float term = 0.0f;

	term = N1 / (A1 + B1);
	N1 = saved + A1 * term;
	saved = B1 * term;

	N2 = saved;
	saved = 0.0f;

	term = N1 / (A1 + B2);
	N1 = saved + A1 * term;
	saved = B2 * term;

	term = N2 / (A2 + B1);
	N2 = saved + A2 * term;
	saved = B1 * term;

	N3 = saved;

	float3 f1 = 3.0f * ((B1_ - B0_) / (T2 - Tm1));
	float3 f2 = 3.0f * ((B2_ - B1_) / (T3 - T0));
	float3 f3 = 3.0f * ((B3_ - B2_) / (T4 - T1));

	return N1 * f1 + N2 * f2 + N3 * f3;
}

#define NUM_CONTROL_POINTS 16

float factor(float z) {
	return -16.0f * log10(z * 0.1);
}

float mipLevel(float z) {
	return 6 - log2(factor(z));
}

[domain("quad")]
DS_OUTPUT main(
	HS_CONSTANT_DATA_OUTPUT input,
	float2 domain : SV_DomainLocation,
	const OutputPatch<HS_CONTROL_POINT_OUTPUT, NUM_CONTROL_POINTS> patch)
{
	DS_OUTPUT Output;
	float3 p00 = patch[0].vPosition.xyz;
	float3 p10 = patch[1].vPosition.xyz;
	float3 p20 = patch[2].vPosition.xyz;
	float3 p30 = patch[3].vPosition.xyz;
	float3 p01 = patch[4].vPosition.xyz;
	float3 p11 = patch[5].vPosition.xyz;
	float3 p21 = patch[6].vPosition.xyz;
	float3 p31 = patch[7].vPosition.xyz;
	float3 p02 = patch[8].vPosition.xyz;
	float3 p12 = patch[9].vPosition.xyz;
	float3 p22 = patch[10].vPosition.xyz;
	float3 p32 = patch[11].vPosition.xyz;
	float3 p03 = patch[12].vPosition.xyz;
	float3 p13 = patch[13].vPosition.xyz;
	float3 p23 = patch[14].vPosition.xyz;
	float3 p33 = patch[15].vPosition.xyz;

	float u = domain.x;
	float v = domain.y;

	float3 bu0 = drawBrezier4(v, p00, p10, p20, p30);
	float3 bu1 = drawBrezier4(v, p01, p11, p21, p31);
	float3 bu2 = drawBrezier4(v, p02, p12, p22, p32);
	float3 bu3 = drawBrezier4(v, p03, p13, p23, p33);


	float3 position = drawBrezier4(u, bu0, bu1, bu2, bu3);
	Output.worldPos = position;

	

	float3 bv0 = drawBrezier4(u, p00, p01, p02, p03);
	float3 bv1 = drawBrezier4(u, p10, p11, p12, p13);
	float3 bv2 = drawBrezier4(u, p20, p21, p22, p23);
	float3 bv3 = drawBrezier4(u, p30, p31, p32, p33);

	Output.tangent = normalize((TangentVec(u, bu0, bu1, bu2, bu3) + TangentVec(v, bv0, bv1, bv2, bv3)) / 2.0f);
	Output.binormal = normalize(cross(float3(0.0f, 1.0f, 0.0f), Output.tangent));
	Output.normal = normalize(cross(Output.tangent, Output.binormal));
	//Output.binormal = normalize(cross(Output.tangent, Output.normal));

	Output.tex.x = patch[5].tex.x + (patch[9].tex.x - patch[5].tex.x) * domain.x;
	Output.tex.y = patch[5].tex.y + (patch[6].tex.y - patch[5].tex.y) * domain.y;

	float h = heighMap.SampleLevel(colorSampler, Output.tex, mipLevel(mul(viewMatrix, float4(position, 1.0f)).z)).x;
	h *= 0.05f;

	Output.worldPos = Output.worldPos + Output.normal * h;

	float3 camPos = mul(invViewMatrix, float4(0.0f, 0.0f, 0.0f, 1.0f)).xyz;
	Output.view = camPos - Output.worldPos;

	Output.vPosition = mul(viewMatrix, float4(Output.worldPos, 1.0f));
	Output.vPosition = mul(projMatrix, Output.vPosition);
	//Output.normal = float3(input.z_val[0], input.z_val[1], input.z_val[2]);
	return Output;
}
