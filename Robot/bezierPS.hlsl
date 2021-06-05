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

static const float3 ambientColor = float3(0.2f, 0.1f, 0.1f);
static const float3 lightColor = float3(1.0f, 1.0f, 1.0f);
static const float kd = 0.5, ks = 0.2f, m = 100.0f;
static const float4 lightPos = float4(1.0f, 2.0f, 0.0f, 1.0f);

float4 main(DS_OUTPUT i) : SV_TARGET
{

	float3 V = normalize(i.view);
	float3 N = normalize(i.normal);

	//float3 col = envMap.Sample(samp, i.tex).rgb;
	//col = pow(col, 0.4545f);

	float3 col = ambientColor;
	
	float3 L = normalize(lightPos.xyz - i.worldPos);
	float3 H = normalize(V + L);
	col += lightColor.xyz * kd * clamp(dot(N, L), 0.0f, 1.0f);
	float nh = dot(N, H);
	nh = clamp(nh, 0.0f, 1.0f);
	nh = ks * pow(nh, m);
	col += lightColor.xyz * nh;

	return saturate(float4(col,1.0f));
	return float4(((i.normal /10.0f) + 1.0f) / 2.0f, 1.0f);
}