#version 400 core

in vec2 TextureCoordinates;

layout (location = 0) out vec4 Color;
layout (location = 1) out vec3 NormalMap;
layout (location = 2) out vec3 AlbedoMap;

uniform sampler2D MegaTexture;
uniform sampler2D DepthPatch;
uniform sampler2D NormalPatch;
uniform sampler2D AlbedoPatch;

uniform int PatchSizeInPixels;

uniform int PatchX;
uniform int PatchY;

uniform int MicrobufferWidth;
uniform int MicrobufferHeight;

uniform vec3 CameraPos;
uniform float PixelSurfaceInMeters;
uniform float PixelsToMeters;

uniform float Alpha;
uniform float CookTorranceF0;

uniform float MicroCameraNearPlane;
uniform vec3 WorldUp;

uniform float MainCameraAspect;
uniform float MainCameraFoV;
uniform float MainCameraNearPlane;
uniform float MainCameraFarPlane;
uniform mat4 InvLookAtCamera;

uniform int WindowWidth;
uniform int WindowHeight;

const float Pi = 3.14159265f;

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

vec3 ComputePositionOfPixel(float MicroCameraNearPlane, 
		int X, int Y, float PixelsToMeters, 
		mat4 InvMicroCameraLookAt,
		int MicrobufferWidth, int MicrobufferHeight)
{
	int PixelX = X - int(0.5f * MicrobufferWidth);
	int PixelY = Y - int(0.5f * MicrobufferHeight);
	vec4 PixelCameraPos = vec4(PixelX * PixelsToMeters,
			PixelY * PixelsToMeters, - MicroCameraNearPlane, 1.0f);
	vec4 PixelWorldPos = InvMicroCameraLookAt * PixelCameraPos;

	return(PixelWorldPos.xyz / PixelWorldPos.w);
	//return(PixelWorldPos.xyz);
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

// TODO(hugo): This is highly dependant on how I chose the 
// orientations of the microcameras in the cpp code. One change
// in a file has to impact the other
vec3 GetLookingDir(vec3 Normal, vec3 WorldUp, int FaceIndex)
{
	vec3 Result = vec3(0.0f, 0.0f, 0.0f);
	if(FaceIndex == 0)
	{
		Result = Normal;
	}
	else if(FaceIndex == 1)
	{
		Result = cross(Normal, WorldUp);
	}
	else if(FaceIndex == 2)
	{
		Result = -cross(Normal, WorldUp);
	}
	else if(FaceIndex == 3)
	{
		Result = cross(Normal, cross(Normal, WorldUp));
	}
	else if(FaceIndex == 4)
	{
		Result = -cross(Normal, cross(Normal, WorldUp));
	}

	return(Result);
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

vec4 Hadamard(vec4 A, vec4 B)
{
	vec4 Result = vec4(A.x * B.x,
			A.y * B.y,
			A.z * B.z,
			A.w * B.w);

	return(Result);
}

void main()
{
	// NOTE(hugo) : These coords are from the LOWER-LEFT corner
	// NOTE(hugo) : If my viewport is the size of the patch, then
	// first I need to find which pixel in the patch I am in, and then divides
	// the result by the patch size in pixels to get in the texture range [0, 1]
	// If my viewport is the size of the whole screen, then I just need to
	// divide the fragment coord by the size of the screen to be in the proper
	// texture range.
	//vec2 PixelCoordInScreen = gl_FragCoord.xy - vec2(0.5f, 0.5f);
	vec2 PixelCoordInScreen = gl_FragCoord.xy;
	vec2 PixelCoordInPatch = PixelCoordInScreen  - vec2(PatchX * PatchSizeInPixels, PatchY * PatchSizeInPixels);
	vec2 TextureCoord = PixelCoordInPatch / PatchSizeInPixels;

	float Depth = texture(DepthPatch, TextureCoord).r;
	float NearPlane = MainCameraNearPlane;
	float FarPlane = MainCameraNearPlane;
	Depth = 2.0f * Depth - 1.0f;
	Depth = 2.0f * NearPlane * FarPlane / (NearPlane + FarPlane - Depth * (FarPlane - NearPlane));

	vec4 PixelPos = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	PixelPos.z = -Depth;
	PixelPos.w = 1.0f;
	PixelPos.x = float(PixelCoordInScreen.x);
	PixelPos.y = float(PixelCoordInScreen.y);

	PixelPos.xy = 2.0f * PixelPos.xy - vec2(1.0f, 1.0f);
	PixelPos.x = - MainCameraAspect * tan(0.5f * MainCameraFoV) * PixelPos.z * PixelPos.x;
	PixelPos.y = - tan(0.5f * MainCameraFoV) * PixelPos.z * PixelPos.y;
	PixelPos = InvLookAtCamera * PixelPos;
	PixelPos.w = 1.0f;
	//vec4 FragmentWorldPos = UnprojectPixel(Depth, 
			//PixelCoordInScreen.x, PixelCoordInScreen.y, 
			//float(WindowWidth), float(WindowHeight), 
			//MainCameraFoV, MainCameraAspect, 
			//InvLookAtCamera);
	vec4 FragmentWorldPos = PixelPos;
	vec3 Normal = texture(NormalPatch, TextureCoord).xyz;
	Normal = normalize(2.0f * Normal - vec3(1.0f, 1.0f, 1.0f));

	vec4 Albedo = texture(AlbedoPatch, TextureCoord);
	//if(length(Albedo.xyz) == 0.0f)
	//{
		//discard;
	//}

	int MegaTexturePixelIndex = 5 * 
		MicrobufferHeight * MicrobufferWidth * 
		int(PixelCoordInPatch.x + PatchSizeInPixels * PixelCoordInPatch.y);
	float MegaTextureTexelSize = 1.0f / textureSize(MegaTexture, 0).x;

	Color = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	vec3 Wo = normalize(CameraPos - FragmentWorldPos.xyz);
	float NormalDotWo = DotClamp(Normal, Wo);

	float DEBUGSolidAngle = 0.0f;
	float DEBUGRedError = 0.0f;
	float DEBUGCounter = 0.0f;
	for(int FaceIndex = 0; FaceIndex < 5; ++FaceIndex)
	{
		// NOTE(hugo) : getting microcamera variables
		//vec3 MicroCameraLookingDir = GetLookingDir(Normal, WorldUp, FaceIndex);
		vec3 MicroCameraLookingDir = vec3(0.0f, 0.0f, 0.0f);
		if(FaceIndex == 0)
		{
			MicroCameraLookingDir = Normal;
		}
		if(FaceIndex == 1)
		{
			MicroCameraLookingDir = cross(Normal, WorldUp);
		}
		if(FaceIndex == 2)
		{
			MicroCameraLookingDir = -1.0f * cross(Normal, WorldUp);
		}
		if(FaceIndex == 3)
		{
			MicroCameraLookingDir = cross(Normal, cross(Normal, WorldUp));
		}
		if(FaceIndex == 4)
		{
			MicroCameraLookingDir = -1.0f * cross(Normal, cross(Normal, WorldUp));
		}
		if(length(MicroCameraLookingDir) == 0.0f)
		{
			DEBUGRedError = 1.0f;
		}

		vec3 Up = WorldUp;
		int StartingY = 0;
		if(FaceIndex != 0)
		{
			Up = Normal;
			StartingY = int(0.5f * MicrobufferHeight);
			MegaTexturePixelIndex += StartingY * MicrobufferWidth;
		}
		vec3 MicroCameraRight = normalize(cross(MicroCameraLookingDir, Up));
		// TODO(hugo) : Can't I do a more straightforward computation ?
		// this works in the non-shader code as well
		vec3 MicroCameraUp = Up;
		vec3 MicroCameraTarget = FragmentWorldPos.xyz + MicroCameraLookingDir;
		mat4 InvMicroCameraLookAt = inverse(GetLookAt(FragmentWorldPos.xyz, MicroCameraTarget, MicroCameraUp));

		for(int Y = StartingY; Y < MicrobufferHeight; ++Y)
		{
			for(int X = 0; X < MicrobufferWidth; ++X)
			{
				vec3 MicroPixelWorldPos = ComputePositionOfPixel(MicroCameraNearPlane, 
						X, Y, PixelsToMeters, InvMicroCameraLookAt,
						MicrobufferWidth, MicrobufferHeight);
				vec3 Wi = normalize(MicroPixelWorldPos.xyz  - (FragmentWorldPos.xyz));
				if(DotClamp(Normal, Wi) > 0.0f)
				{
					DEBUGCounter++;
					vec4 SampleColor = texture(MegaTexture, MegaTextureTexelSize * vec2(MegaTexturePixelIndex, 0) + vec2(0.5f, 0.5f));
					if(LengthSqr(vec4(SampleColor.xyz, 0.0f)) > 0.0f)
					{
						vec3 H = normalize(0.5f * (Wi + Wo));
						float DistanceMicroCameraPixelSqr = LengthSqr(FragmentWorldPos - MicroPixelWorldPos);
						float SolidAngle = dot(Wi, MicroCameraLookingDir) * (PixelSurfaceInMeters / DistanceMicroCameraPixelSqr);
						float BRDF = GGXBRDF(Normal, Wi, H, NormalDotWo, Alpha, CookTorranceF0);
						// TODO(hugo) : Check earlier if Albedo.rgb == 0
						Color += BRDF * DotClamp(Normal, Wi) * SolidAngle * Hadamard(Albedo, SampleColor);
					}
				}
				float DistanceMicroCameraPixelSqr = LengthSqr(FragmentWorldPos - MicroPixelWorldPos);
				float SolidAngle = dot(Wi, MicroCameraLookingDir) * (PixelSurfaceInMeters / DistanceMicroCameraPixelSqr);
				DEBUGSolidAngle = DEBUGSolidAngle + SolidAngle;
				MegaTexturePixelIndex++;
			}
		}
	}
	Color.w = 1.0f;
	AlbedoMap = vec3(0.0f, 0.0f, 0.0f);
	NormalMap = vec3(0.0f, 0.0f, 0.0f);
	float Fraction = DEBUGSolidAngle / (2.0f * Pi);
	Color = vec4(DEBUGRedError, 1.0f * Fraction, 0.0f * DEBUGCounter / (float(MicrobufferWidth) * float(MicrobufferHeight) * 3.0f), 1.0f);
}
