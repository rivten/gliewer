#version 400 core

in vec2 TextureCoordinates;

layout (location = 0) out vec4 Color;

uniform sampler2D MegaTexture;
uniform sampler2D DepthMap;
uniform sampler2D NormalMap;
uniform sampler2D AlbedoMap;
uniform sampler2D DirectIlluminationMap;

uniform int PatchSizeInPixels;

uniform int PatchX;
uniform int PatchY;

uniform int MicrobufferWidth;
uniform int MicrobufferHeight;

uniform vec3 CameraPos;
uniform float PixelSurfaceInMeters;

uniform float Alpha;
uniform float CookTorranceF0;

uniform vec3 WorldUp;

uniform float MainCameraAspect;
uniform float MainCameraFoV;
uniform float MainCameraNearPlane;
uniform float MainCameraFarPlane;
uniform mat4 InvLookAtCamera;

uniform int WindowWidth;
uniform int WindowHeight;

const float Pi = 3.14159265f;
const float GammaCor = 2.2f;

float DotClamp(vec3 A, vec3 B)
{
	float Result = max(0.0f, dot(A, B));
	return(Result);
}

vec4 UnprojectPixel(float Depth, 
		float X, float Y, 
		float WindowWidth, float WindowHeight,
		float CameraFoV, float CameraAspect,
		mat4 InvLookAtCamera)
{
	// TODO(hugo) : I _think_ the coordinates (X,Y) needs to 
	// be from the LOWER-LEFT corner of the window, but
	// I need to check this
	vec4 PixelPos = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	PixelPos.z = -Depth;
	PixelPos.w = 1.0f;
	PixelPos.x = float(X) / float(WindowWidth);
	PixelPos.y = float(Y) / float(WindowHeight);

	PixelPos.xy = 2.0f * PixelPos.xy - vec2(1.0f, 1.0f);
	PixelPos.x = - CameraAspect * tan(0.5f * CameraFoV) * PixelPos.z * PixelPos.x;
	PixelPos.y = - tan(0.5f * CameraFoV) * PixelPos.z * PixelPos.y;

	PixelPos = InvLookAtCamera * PixelPos;

	PixelPos.w = 1.0f;
	return(PixelPos / PixelPos.w);
}

// TODO(hugo) : Do shader parsing in order to implement include
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

float GGXBRDF(vec3 Normal, vec3 LightDir, vec3 HalfDir, float NormalDotViewDir, float Alpha, float F0)
{
	float NormalDotHalfDir = DotClamp(Normal, HalfDir);
	float NormalDotLightDir = DotClamp(Normal, LightDir);
	float LightDirDotHalfDir = DotClamp(LightDir, HalfDir);

	float AlphaSqr = Alpha * Alpha;
	float F = FresnelSchlickFactor(F0, LightDirDotHalfDir);
	float D = GGXDistributionTerm(AlphaSqr, NormalDotHalfDir);
	float OneOverGL = NormalDotLightDir + sqrt(AlphaSqr + ((1.0f - AlphaSqr) * (NormalDotLightDir * NormalDotLightDir)));
	float OneOverGV = NormalDotViewDir + sqrt(AlphaSqr + ((1.0f - AlphaSqr) * (NormalDotViewDir * NormalDotViewDir)));

	float DiffuseFactor = 0.3f;
	float Result = DiffuseFactor + ((F * D) / (OneOverGL * OneOverGV));

	return(Result);
}

float LengthSqr(vec4 V)
{
	return(dot(V, V));
}

mat4 GetLookAt(vec3 Eye, vec3 Target, vec3 Up)
{
	vec3 ZAxis = normalize(Eye - Target);
	vec3 XAxis = normalize(cross(Up, ZAxis));
	vec3 YAxis = cross(ZAxis, XAxis);

	// TODO(hugo) : Check if this is this and not the transpose
	vec4 Col0 = vec4(XAxis.x, YAxis.x, ZAxis.x, 0.0f);
	vec4 Col1 = vec4(XAxis.y, YAxis.y, ZAxis.y, 0.0f);
	vec4 Col2 = vec4(XAxis.z, YAxis.z, ZAxis.z, 0.0f);
	vec4 Col3 = vec4(- dot(Eye, XAxis), - dot(Eye, YAxis), - dot(Eye, ZAxis), 1.0f);

	mat4 Result = mat4(Col0, Col1, Col2, Col3);
	return(Result);
}

float ParaboloidMap(vec2 UV)
{
	float Z = 0.5f * (1.0f - (UV.x * UV.x) - (UV.y * UV.y));
	return(Z);
}

void main()
{
	// NOTE(hugo) : These coords are from the LOWER-LEFT corner
	vec2 ScreenSize = vec2(WindowWidth, WindowHeight);
	//vec2 FragCoord = gl_FragCoord.xy + 1.0f * vec2(0.5f, 0.5f);
	vec2 FragCoord = gl_FragCoord.xy - vec2(0.5f, 0.5f);
	vec2 ScreenUV = FragCoord / ScreenSize;
	vec2 PixelCoordInPatch = FragCoord - vec2(PatchX * PatchSizeInPixels, PatchY * PatchSizeInPixels);

	// NOTE(hugo) : Unlinearize depth
	float Depth = texture(DepthMap, ScreenUV).r;
	float NearPlane = MainCameraNearPlane;
	float FarPlane = MainCameraFarPlane;
	Depth = 2.0f * Depth - 1.0f;
	Depth = 2.0f * NearPlane * FarPlane / (NearPlane + FarPlane - Depth * (FarPlane - NearPlane));

	// NOTE(hugo) : Unprojection the pixel
	vec4 PixelPos = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	PixelPos.z = -Depth;
	PixelPos.w = 1.0f;
	PixelPos.xy = FragCoord / ScreenSize;

	PixelPos.xy = 2.0f * PixelPos.xy - vec2(1.0f, 1.0f);
	PixelPos.x = - MainCameraAspect * tan(0.5f * MainCameraFoV) * PixelPos.z * PixelPos.x;
	PixelPos.y = - tan(0.5f * MainCameraFoV) * PixelPos.z * PixelPos.y;
	PixelPos = InvLookAtCamera * PixelPos;
	PixelPos.w = 1.0f;
	vec4 FragmentWorldPos = PixelPos;
	vec3 Normal = texture(NormalMap, ScreenUV).xyz;
	Normal = normalize(2.0f * Normal - vec3(1.0f, 1.0f, 1.0f));

	vec4 Albedo = texture(AlbedoMap, ScreenUV);

	vec2 MegaTextureTexelSize = 1.0f / textureSize(MegaTexture, 0);

	Color = texture(DirectIlluminationMap, ScreenUV);
	
	vec3 Wo = normalize(CameraPos - FragmentWorldPos.xyz);
	float NormalDotWo = DotClamp(Normal, Wo);

	vec3 ReferenceUp = WorldUp;
	if(length(cross(Normal, WorldUp)) < 0.001f)
	{
		ReferenceUp = vec3(0.44742f, 0.894427f, 0.0f);
	}

	// NOTE(hugo) : getting microcamera variables
	vec3 MicroCameraLookingDir = Normal;

	vec3 Up = WorldUp;
	vec3 MicroCameraRight = normalize(cross(MicroCameraLookingDir, Up));
	vec3 MicroCameraUp = Up;
	vec3 MicroCameraTarget = FragmentWorldPos.xyz + MicroCameraLookingDir;
	mat4 InvMicroCameraLookAt = inverse(GetLookAt(FragmentWorldPos.xyz, MicroCameraTarget, MicroCameraUp));

	vec2 MicrobufferSize = vec2(MicrobufferWidth, MicrobufferHeight);

	for(int Y = 0; Y < MicrobufferHeight; ++Y)
	{
		for(int X = 0; X < MicrobufferWidth; ++X)
		{
			//
			// NOTE(hugo) : Get world position of the micropixel we want to
			// convolute with.
			// {
			//
			vec4 MicroPixelWorldPos = vec4(0.0f, 0.0f, 0.0f, 0.0f);

			MicroPixelWorldPos.x = float(X) / float(MicrobufferWidth);
			MicroPixelWorldPos.y = float(Y) / float(MicrobufferHeight);
			MicroPixelWorldPos.xy = 2.0f * MicroPixelWorldPos.xy - vec2(1.0f, 1.0f);

			MicroPixelWorldPos.z = -ParaboloidMap(MicroPixelWorldPos.xy);
			MicroPixelWorldPos.w = 1.0f;

			MicroPixelWorldPos = InvMicroCameraLookAt * MicroPixelWorldPos;
			MicroPixelWorldPos.w = 1.0f;
			//
			// }
			//

			vec3 Wi = normalize(MicroPixelWorldPos.xyz - (FragmentWorldPos.xyz));
			if(DotClamp(Normal, Wi) > 0.0f)
			{
				vec2 SampleCoord = MicrobufferSize * PixelCoordInPatch + vec2(X, Y);
				SampleCoord = SampleCoord * MegaTextureTexelSize;
				vec4 SampleColor = texture(MegaTexture, SampleCoord);
				//if(LengthSqr(vec4(SampleColor.xyz, 0.0f)) > 0.0f)
				{
					vec3 H = normalize(0.5f * (Wi + Wo));
					float DistanceMicroCameraPixelSqr = LengthSqr(FragmentWorldPos - MicroPixelWorldPos);
					float SolidAngle = dot(Wi, MicroCameraLookingDir) * (PixelSurfaceInMeters / DistanceMicroCameraPixelSqr);
					float BRDF = GGXBRDF(Normal, Wi, H, NormalDotWo, Alpha, CookTorranceF0);
					// TODO(hugo) : Check earlier if Albedo.rgb == 0
					Color += BRDF * DotClamp(Normal, Wi) * SolidAngle * Albedo * SampleColor;
				}
			}
		}
	}

	// NOTE(hugo) : Reinhard Tone Mapping
	Color.xyz = Color.xyz / (Color.xyz + vec3(1.0f, 1.0f, 1.0f));
	Color.w = 1.0f;

	// NOTE(hugo) : Gamma correction
	Color.xyz = pow(Color.xyz, vec3(1.0f / GammaCor));
}
