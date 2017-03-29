#version 400 core

layout (location = 0) out vec4 Color;

in GS_OUT
{
	vec3 VertexNormal;
	vec4 FragmentPosInLightSpace[4];
	vec3 FragmentPos;
	vec3 ViewDir;
	vec3 LightDir[4];
	vec3 HalfDir[4];
} fs_in;

uniform sampler2D ShadowMaps[4];
uniform vec4 LightColor[4];
uniform int LightCount;
uniform mat4 ViewMatrix;
uniform float LightIntensity;

uniform vec4 DiffuseColor;
uniform vec4 SpecularColor;
uniform float Alpha;
uniform float CTF0;
uniform float Ks;
uniform float Kd;

//uniform vec2 MicrobufferSize;

const float Pi = 3.14159265f;

float DotClamp(vec3 A, vec3 B)
{
	float Result = max(0.0f, dot(A, B));
	return(Result);
}

// ---------------------------------------
// NOTE(hugo) : Shadow Map Computation
// ---------------------------------------
float ShadowFactor(vec4 FragmentPositionInLightSpace, sampler2D ShadowMap, float Bias)
{
	vec3 ProjectedCoordinates = FragmentPositionInLightSpace.xyz / FragmentPositionInLightSpace.w;
	ProjectedCoordinates = 0.5f * ProjectedCoordinates + 0.5f;
	float FragmentDepth = ProjectedCoordinates.z;

	float Result = 0.0f;
	vec2 TexelSize = 1.0f / textureSize(ShadowMap, 0);

	int PCFSize = 3;
	int K = PCFSize / 2;

	for(int X = -K; X <= K; ++X)
	{
		for(int Y = -K; Y <= K; ++Y)
		{
			float PCFDepthValue = texture(ShadowMap, ProjectedCoordinates.xy + vec2(X, Y) * TexelSize).r;
			Result += ((FragmentDepth - Bias) > PCFDepthValue) ? 1.0f : 0.0f;
		}
	}
	Result /= float(PCFSize * PCFSize);

	return(Result);
}

// ---------------------------------------
// NOTE(hugo) : GGX BRDF
// ---------------------------------------
float FresnelSchlickFactor(float F0, float LightDirDotHalfDir)
{
	float Result = F0 + (1.0f - F0) * pow((1.0f - LightDirDotHalfDir), 5);
	return(Result);
}

float GGXDistributionTerm(float AlphaSqr, float NormalDotHalfDir)
{
	float Denominator = ((NormalDotHalfDir * NormalDotHalfDir) * (AlphaSqr - 1.0f)) + 1.0f;
	Denominator = Pi * Denominator * Denominator;
	float Result = AlphaSqr / Denominator;

	return(Result);
}

float GGXBRDF(vec3 Normal, vec3 LightDir, vec3 HalfDir, vec3 ViewDir, float Alpha, float F0)
{
	float NormalDotHalfDir = DotClamp(Normal, HalfDir);
	float NormalDotLightDir = DotClamp(Normal, LightDir);
	float NormalDotViewDir = DotClamp(Normal, ViewDir);
	float ViewDirDotHalfDir = DotClamp(ViewDir, HalfDir);
	float LightDirDotHalfDir = DotClamp(LightDir, HalfDir);

	float AlphaSqr = Alpha * Alpha;
	float F = FresnelSchlickFactor(F0, LightDirDotHalfDir);
	float D = GGXDistributionTerm(AlphaSqr, NormalDotHalfDir);
	float OneOverGL = NormalDotLightDir + sqrt(AlphaSqr + ((1.0f - AlphaSqr) * (NormalDotLightDir * NormalDotLightDir)));
	float OneOverGV = NormalDotViewDir + sqrt(AlphaSqr + ((1.0f - AlphaSqr) * (NormalDotViewDir * NormalDotViewDir)));

	float Result = ((F * D) / (OneOverGL * OneOverGV));

	return(Result);
}

void main()
{
	vec3 FragmentPos = fs_in.FragmentPos;
	vec3 ViewDir = fs_in.ViewDir;

	Color = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	for(int LightIndex = 0; LightIndex < LightCount; ++LightIndex)
	{
		vec3 LightDir = fs_in.LightDir[LightIndex];
		vec3 HalfDir = fs_in.HalfDir[LightIndex];

		float ShadowMappingBias = max(0.01f * (1.0f - dot(fs_in.VertexNormal, LightDir)), 0.005f);
		float Shadow = ShadowFactor(fs_in.FragmentPosInLightSpace[LightIndex], ShadowMaps[LightIndex], ShadowMappingBias);
		vec4 BRDFLambert = DiffuseColor / Pi;
		vec4 BRDFSpec = SpecularColor * GGXBRDF(fs_in.VertexNormal, LightDir, HalfDir, ViewDir, Alpha, CTF0);
		vec4 Li = LightIntensity * LightColor[LightIndex];
		Color += (1.0f - Shadow) * (Ks * BRDFLambert + Kd * BRDFSpec) * Li * DotClamp(fs_in.VertexNormal, LightDir);
	}
	Color.w = 1.0f;
}
