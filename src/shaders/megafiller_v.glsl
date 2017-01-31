#version 400 core

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 TextureCoord;

uniform sampler2D DepthMap;
uniform sampler2D NormalMap;

uniform int PatchX;
uniform int PatchY;
uniform int PatchSizeInPixels;

uniform mat4 MicroProjection;
uniform mat4 ObjectMatrix;
uniform vec3 WorldUp;

uniform float CameraNearPlane;
uniform float CameraFarPlane;
uniform float CameraFoV;
uniform float CameraAspect;

uniform mat4 InvLookAtCamera;

uniform int FaceIndex; // NOTE(hugo) : this uniform tells us which face of the hemicube we are considering

uniform mat4 NormalMatrix;
uniform int LightCount;
uniform mat4 LightSpaceMatrix[4];

out vec3 VertexNormal;
out vec3 NormalWorldSpace;
out vec4 FragmentPosInWorldSpace;
out vec4 FragmentPosInLightSpace[4];

float UnlinearizeDepth(float Depth, float NearPlane, float FarPlane)
{
	float Result = 2.0f * Depth - 1.0f;
	Result = 2.0f * NearPlane * FarPlane / (NearPlane + FarPlane - Result * (FarPlane - NearPlane));

	return(Result);
}

vec4 UnprojectPixel(float Depth, 
		float X, float Y, 
		float WindowWidth, float WindowHeight,
		float CameraFoV, float CameraAspect,
		mat4 InvLookAtCamera)
{
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

void main()
{
	//
	// NOTE(hugo) : Computing micro view matrix
	// {
	//

	// TODO(hugo) : Does not work for non-square patches. 
	// We should use PatchWidth and PatchHeight here
	float X = mod(gl_InstanceID, PatchSizeInPixels);
	float Y = floor(float(gl_InstanceID) / float(PatchSizeInPixels));
	vec2 PixelCoordInPatch = vec2(X, Y);

	vec2 PixelCoordInWindow = PixelCoordInPatch + 
		PatchSizeInPixels * vec2(PatchX, PatchY);

	vec2 WindowSize = textureSize(DepthMap, 0);
	vec2 WindowSizeToUV = 1.0f / WindowSize;
	vec2 UV = PixelCoordInWindow * WindowSizeToUV;

	float Depth = texture(DepthMap, UV).r;
	vec3 Normal = texture(NormalMap, UV).xyz;
	
	Depth = UnlinearizeDepth(Depth, CameraNearPlane, CameraFarPlane);
	Normal = normalize(2.0f * Normal - vec3(1.0f, 1.0f, 1.0f));

	vec4 UnprojectedPixelWorldSpace = UnprojectPixel(Depth,
			PixelCoordInWindow.x, PixelCoordInWindow.y,
			WindowSize.x, WindowSize.y,
			CameraFoV, CameraAspect, InvLookAtCamera);

	// TODO(hugo) : Divide by w component ?
	vec3 MicroEye = UnprojectedPixelWorldSpace.xyz / 
		UnprojectedPixelWorldSpace.w;

	vec3 LookDir = Normal;
	vec3 MicroUp = WorldUp;
	if(FaceIndex == 1)
	{
		LookDir = cross(Normal, WorldUp);
	}
	else if(FaceIndex == 2)
	{
		LookDir = - cross(Normal, WorldUp);
	}
	else if(FaceIndex == 3)
	{
		LookDir = cross(Normal, cross(Normal, WorldUp));
	}
	else if(FaceIndex == 4)
	{
		LookDir = - cross(Normal, cross(Normal, WorldUp));
	}

	if(FaceIndex != 0)
	{
		MicroUp = Normal;
	}

	vec3 MicroTarget = MicroEye + LookDir;

	mat4 MicroView = GetLookAt(MicroEye, MicroTarget, MicroUp);
	//
	// }
	//
	mat4 MicroMVP = MicroProjection * MicroView * ObjectMatrix;

	vec4 NDCPosition = MicroMVP * vec4(Position, 1.0f);
	NDCPosition = NDCPosition / NDCPosition.w;
	//NDCPosition.w = 1.0f;
	NDCPosition.xy *= (2.0f / float(PatchSizeInPixels));
	NDCPosition.xy -= (1.0f - (2.0f / float(PatchSizeInPixels))) * vec2(1.0f, 1.0f);
	NDCPosition.xy += PixelCoordInPatch * (2.0f / float(PatchSizeInPixels));

	gl_Position = NDCPosition;

	NormalWorldSpace = Normal;
	VertexNormal = normalize((NormalMatrix * vec4(Normal, 1.0f)).xyz);
	FragmentPosInWorldSpace = ObjectMatrix * vec4(Position, 1.0f);
	for(int LightIndex = 0; LightIndex < LightCount; ++LightIndex)
	{
		FragmentPosInLightSpace[LightIndex] = LightSpaceMatrix[LightIndex] * FragmentPosInWorldSpace;
	}
}
