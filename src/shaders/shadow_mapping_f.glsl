#version 400 core

out vec4 Color;

in vec3 VertexNormal;
in vec4 FragmentPositionInWorldSpace;
in vec4 FragmentPositionInLightSpace;

uniform sampler2D ShadowMap;

uniform vec3 LightPos[4];
uniform vec4 LightColor[4];
uniform mat4 ViewMatrix;
uniform vec4 ObjectColor;
uniform int BlinnPhongShininess;
uniform float CTF0;
uniform float CTM;

float ShadowFactor(vec4 FragmentPositionInLightSpace)
{
	vec3 ProjectedCoordinates = FragmentPositionInLightSpace.xyz / FragmentPositionInLightSpace.w;
	ProjectedCoordinates = 0.5f * ProjectedCoordinates + 0.5f;
	float FragmentDepth = ProjectedCoordinates.z;

	float ShadowMappingBias = 0.01;
	float Result = 0.0f;
	vec2 TexelSize = 1.0f / textureSize(ShadowMap, 0);

	int PCFSize = 9;
	int K = PCFSize / 2;

	for(int X = -K; X <= K; ++X)
	{
		for(int Y = -K; Y <= K; ++Y)
		{
			float PCFDepthValue = texture(ShadowMap, ProjectedCoordinates.xy + vec2(X, Y) * TexelSize).r;
			Result += ((FragmentDepth - ShadowMappingBias) > PCFDepthValue) ? 1.0f : 0.0f;
		}
	}
	Result /= float(PCFSize * PCFSize);

	return(Result);
}

float FresnelSchlickFactor(float F0, float LightDirDotHalfDir)
{
	float Result = F0 + (1.0f - F0) * pow((1.0f - LightDirDotHalfDir), 5);
	return(Result);
}

float CookTorranceGeometricTerm(float NormalDotHalfDir, float NormalDotViewDir, float NormalDotLightDir, float ViewDirDotHalfDir)
{
	float A = 2 * NormalDotHalfDir * NormalDotViewDir / ViewDirDotHalfDir;
	float B = 2 * NormalDotHalfDir * NormalDotLightDir / ViewDirDotHalfDir;
	float Result = min(1.0f, min(A, B));

	return(Result);
}

float CookTorranceDistributionTerm(float NormalDotHalfDir, float M)
{
	float Alpha = acos(NormalDotHalfDir);
	float Result = exp(- (Alpha * Alpha) / (M * M));

	return(Result);
}

vec4 DiffuseColor(vec4 ObjectColor, vec4 LightColor, vec3 LightDir, vec3 Normal)
{
	vec4 Result = ObjectColor * LightColor * max(0.0, dot(LightDir, Normal));

	return(Result);
}

vec4 CookTorranceBRDF(vec4 ObjectColor, vec4 LightColor, vec3 Normal, vec3 LightDir, vec3 HalfDir, vec3 ViewDir, float F0, float M)
{
	float NormalDotHalfDir = max(0.0f, dot(Normal, HalfDir));
	float NormalDotViewDir = max(0.0f, dot(Normal, ViewDir));
	float NormalDotLightDir = max(0.0f, dot(Normal, LightDir));
	float ViewDirDotHalfDir = max(0.0f, dot(ViewDir, HalfDir));
	float LightDirDotHalfDir = max(0.0f, dot(LightDir, HalfDir));

	float F = FresnelSchlickFactor(F0, LightDirDotHalfDir);
	float G = CookTorranceGeometricTerm(NormalDotHalfDir, NormalDotViewDir, NormalDotLightDir, ViewDirDotHalfDir);
	float D = CookTorranceDistributionTerm(NormalDotHalfDir, M);
	vec4 SpecularColor = (1 / (4 * NormalDotLightDir * NormalDotViewDir)) * F * G * D * ObjectColor * LightColor;

	vec4 DiffColor = DiffuseColor(ObjectColor, LightColor, LightDir, Normal);

	return(DiffColor + SpecularColor);
}

vec4 BlinnPhongBRDF(vec4 ObjectColor, vec4 LightColor, vec4 SpecularColor, vec3 VertexNormal, vec3 LightDir, vec3 HalfDir, int Shininess, float SpecularIntensity)
{
	vec4 SpecularObjectColor = SpecularColor * pow(max(0.0, dot(HalfDir, LightDir)), Shininess);
	vec4 DiffColor = DiffuseColor(ObjectColor, LightColor, LightDir, VertexNormal);
	return(DiffColor + ObjectColor * SpecularIntensity * SpecularObjectColor);
}

void main()
{
	vec3 FragmentPos = vec3(ViewMatrix * FragmentPositionInWorldSpace);
	vec3 LightDir = normalize(vec3(ViewMatrix * vec4(LightPos[0], 1.0f)) - FragmentPos);
	vec3 ViewDir = normalize(-FragmentPos);
	vec3 HalfDir = normalize(ViewDir + LightDir);

	// NOTE(hugo) : Computations needs to happen in eye space
	// since we computed the Normal in that very space
	float BlinnPhongSpecularIntensity = 1.0f;
	//Color = BlinnPhongBRDF(ObjectColor, LightColor, vec4(1.0f), VertexNormal, LightDir, HalfDir, BlinnPhongShininess, BlinnPhongSpecularIntensity);
	Color = (1.0f - ShadowFactor(FragmentPositionInLightSpace)) * CookTorranceBRDF(ObjectColor, LightColor[0], VertexNormal, LightDir, HalfDir, ViewDir, CTF0, CTM);
}
