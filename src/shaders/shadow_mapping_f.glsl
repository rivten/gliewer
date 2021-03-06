#version 400 core

layout (location = 0) out vec4 Color;

in vec2 TextureCoordinates;

uniform sampler2D ShadowMap[4];
uniform sampler2D DepthTexture;
uniform sampler2D NormalTexture;
uniform sampler2D AlbedoTexture;
uniform sampler2D SpecularTexture;

// TODO(hugo) : GLSL struct for lights
uniform vec3 LightPos[4];
uniform vec4 LightColor[4];
uniform int LightCount;
uniform mat4 LightViews[4];

// TODO(hugo) : Consider getting rid of this uniform : I could pass the light pos in view space !
uniform vec3 CameraPos;
uniform float CTF0;
uniform float Alpha;
uniform float LightIntensity;
uniform float Ks;
uniform float Kd;
uniform mat4 InvView;

uniform float CameraNearPlane;
uniform float CameraFarPlane;
uniform float CameraFoV;
uniform float CameraAspect;

//uniform float AmbientFactor;

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
// NOTE(hugo) : Blinn Phong BRDF
// ---------------------------------------
vec4 DiffuseComponent(vec4 ObjectColor, vec4 LightColor, vec3 LightDir, vec3 Normal)
{
	vec4 Result = ObjectColor * LightColor * max(0.0, dot(LightDir, Normal));

	return(Result);
}

vec4 BlinnPhongBRDF(vec4 ObjectColor, vec4 LightColor, vec4 SpecularColor, vec3 VertexNormal, vec3 LightDir, vec3 HalfDir, int Shininess, float SpecularIntensity)
{
	vec4 SpecularObjectColor = SpecularColor * pow(max(0.0, dot(HalfDir, LightDir)), Shininess);
	vec4 DiffColor = DiffuseComponent(ObjectColor, LightColor, LightDir, VertexNormal);
	return(DiffColor + ObjectColor * SpecularIntensity * SpecularObjectColor);
}


float FresnelSchlickFactor(float F0, float LightDirDotHalfDir)
{
	float Result = F0 + (1.0f - F0) * pow((1.0f - LightDirDotHalfDir), 5);
	return(Result);
}

// ---------------------------------------
// NOTE(hugo) : Cook-Torrance BRDF
// ---------------------------------------
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

	vec4 DiffColor = DiffuseComponent(ObjectColor, LightColor, LightDir, Normal);

	// TODO(hugo) : Investigate why I need to use clamp
	// I think I could get negative color otherwise
	return(clamp(DiffColor + SpecularColor, 0.0f, 1.0f));
}

// ---------------------------------------
// NOTE(hugo) : GGX BRDF
// ---------------------------------------
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

vec4 UnprojectPixelToViewSpace(float Depth, 
		float X, float Y, 
		float WindowWidth, float WindowHeight,
		float CameraFoV, float CameraAspect)
{
	vec4 PixelPos = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	PixelPos.z = -Depth;
	PixelPos.w = 1.0f;
	PixelPos.x = float(X) / float(WindowWidth);
	PixelPos.y = float(Y) / float(WindowHeight);

	PixelPos.xy = 2.0f * PixelPos.xy - vec2(1.0f, 1.0f);
	PixelPos.x = - CameraAspect * tan(0.5f * CameraFoV) * PixelPos.z * PixelPos.x;
	PixelPos.y = - tan(0.5f * CameraFoV) * PixelPos.z * PixelPos.y;

	PixelPos.w = 1.0f;
	return(PixelPos / PixelPos.w);
}

float UnlinearizeDepth(float Depth, float NearPlane, float FarPlane)
{
	float Result = 2.0f * Depth - 1.0f;
	Result = 2.0f * NearPlane * FarPlane / (NearPlane + FarPlane - Result * (FarPlane - NearPlane));

	return(Result);
}

void main()
{
	float FragmentDepth = texture(DepthTexture, TextureCoordinates).r;
	vec3 VertexNormal = texture(NormalTexture, TextureCoordinates).xyz;
	VertexNormal = normalize(2.0f * VertexNormal - vec3(1.0f, 1.0f, 1.0f));
	vec4 SpecularColor = texture(SpecularTexture, TextureCoordinates);
	vec4 DiffuseColor = texture(AlbedoTexture, TextureCoordinates);

	FragmentDepth = UnlinearizeDepth(FragmentDepth, CameraNearPlane, CameraFarPlane);
	vec4 FragmentPosition = UnprojectPixelToViewSpace(FragmentDepth, 
			TextureCoordinates.x, TextureCoordinates.y,
			1.0f, 1.0f,
			CameraFoV, CameraAspect);
	vec4 FragmentPositionInWorldSpace = InvView * FragmentPosition;
	vec3 ViewDir = normalize(CameraPos - FragmentPositionInWorldSpace.xyz);

	Color = vec4(0.0f, 0.0f, 0.0f, 1.0f);

	// NOTE(hugo) : Computations needs to happen in eye space
	// since we computed the Normal in that very space
	for(int LightIndex = 0; LightIndex < LightCount; ++LightIndex)
	{
		vec4 FragmentPositionInLightSpace = LightViews[LightIndex] * FragmentPositionInWorldSpace;
		vec3 LightDir = normalize(LightPos[LightIndex] - FragmentPositionInWorldSpace.xyz);
		vec3 HalfDir = normalize(ViewDir + LightDir);

		float ShadowMappingBias = max(0.01f * (1.0f - dot(VertexNormal, LightDir)), 0.005f);
		float Shadow = ShadowFactor(FragmentPositionInLightSpace, ShadowMap[LightIndex], ShadowMappingBias);
		vec4 BRDFLambert = DiffuseColor / Pi;
		vec4 BRDFSpec = SpecularColor * GGXBRDF(VertexNormal, LightDir, HalfDir, ViewDir, Alpha, CTF0);
		vec4 Li = LightIntensity * LightColor[LightIndex];
		Color += (1.0f - Shadow) * (Ks * BRDFLambert + Kd * BRDFSpec) * Li * DotClamp(VertexNormal, LightDir);
	}

	//Color = max(Color, 0.0f * AmbientFactor * DiffuseColor);
	Color.w = 1.0f;
}
