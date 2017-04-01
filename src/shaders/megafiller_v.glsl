#version 400 core

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 TextureCoord;

uniform sampler2D DepthMap;
uniform sampler2D NormalMap;

uniform int PatchX;
uniform int PatchY;
uniform int PatchSizeInPixels;

uniform mat4 ObjectMatrix;
uniform vec3 WorldUp;

uniform float CameraNearPlane;
uniform float CameraFarPlane;
uniform float CameraFoV;
uniform float CameraAspect;

uniform vec3 LightPos[4];

uniform mat4 InvLookAtCamera;

uniform mat4 NormalMatrix;
uniform int LightCount;
uniform mat4 LightSpaceMatrix[4];
uniform mat4 ViewMatrix;

uniform int BaseTileID;

// NOTE(hugo) : This goes to the geometry shader
out VS_OUT
{
	int ViewportIndex;

	vec3 VertexNormal;
	vec4 FragmentPosInLightSpace[4];
	vec3 ViewDir;
	vec3 FragmentPos;
	vec3 LightDir[4];
	vec3 HalfDir[4];
} vs_out;


// NOTE(hugo) : These functions helper are 
// here to avoid branching which is _very_ costly
// in shaders. These are inspired by
// http://theorangeduck.com/page/avoiding-shader-conditionals
// {

float WhenEquals(float A, float B)
{
	return(1.0f - abs(sign(A - B)));
}

float WhenNotEquals(float A, float B)
{
	return(abs(sign(A - B)));
}

float WhenGreaterThan(float A, float B)
{
	return(max(sign(A - B), 0.0f));
}

float WhenLesserThan(float A, float B)
{
	return(max(sign(B - A), 0.0f));
}

float WhenGreaterOrEqual(float A, float B)
{
	return(1.0f - WhenLesserThan(A, B));
}

float WhenLesserOrEqual(float A, float B)
{
	return(1.0f - WhenGreaterThan(A, B));
}

float And(float A, float B)
{
	return(A * B);
}

float Or(float A, float B)
{
	return(min(A + B, 1.0f));
}

float Not(float A)
{
	return(1.0f - A);
}

//
// }
//

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
	vs_out.ViewportIndex = gl_InstanceID;
	//
	// NOTE(hugo) : Computing micro view matrix
	// {
	//

	// TODO(hugo) : Dhhes not work for non-square patches. 
	// We should use PatchWidth and PatchHeight here
	// TODO(hugo) : This computation is already done in
	// the CPU. Just pass the result as uniform
	int RealInstanceID = gl_InstanceID + BaseTileID;
	float X = mod(RealInstanceID, PatchSizeInPixels);
	float Y = floor(float(RealInstanceID) / float(PatchSizeInPixels));
	vec2 PixelCoordInPatch = vec2(X, Y);

	vec2 PixelCoordInWindow = PixelCoordInPatch + 
		PatchSizeInPixels * vec2(PatchX, PatchY);

	vec2 WindowSize = textureSize(DepthMap, 0);
	vec2 WindowSizeToUV = 1.0f / WindowSize;
	vec2 UV = PixelCoordInWindow * WindowSizeToUV;

	float Depth = texture(DepthMap, UV).r;
	vec3 PatchNormal = texture(NormalMap, UV).xyz;
	
	Depth = UnlinearizeDepth(Depth, CameraNearPlane, CameraFarPlane);
	PatchNormal = normalize(2.0f * PatchNormal - vec3(1.0f, 1.0f, 1.0f));

	vec4 UnprojectedPixelWorldSpace = UnprojectPixel(Depth,
			PixelCoordInWindow.x, PixelCoordInWindow.y,
			WindowSize.x, WindowSize.y,
			CameraFoV, CameraAspect, InvLookAtCamera);

	// TODO(hugo) : Divide by w component ?
	vec3 MicroEye = UnprojectedPixelWorldSpace.xyz / 
		UnprojectedPixelWorldSpace.w;

	vec3 MicroUp = WorldUp;
	vec3 MicroTarget = MicroEye + PatchNormal;

	mat4 MicroView = GetLookAt(MicroEye, MicroTarget, MicroUp);
	//
	// }
	//

	// TODO(hugo): Apply paraboloid proj
	vec4 ViewPosition = MicroView * ObjectMatrix * vec4(Position, 1.0f);
	ViewPosition = normalize(ViewPosition);
	float Z = -ViewPosition.z;
	float ZPlusOne = Z + 1.0f;
	vec4 ParaboloidPosition = ViewPosition;
	ParaboloidPosition.xy = ParaboloidPosition.xy / ZPlusOne;
	ParaboloidPosition.z = (Z / CameraFarPlane) * 2.0f - 1.0f;
	ParaboloidPosition.w = 1.0f;

	gl_Position = ParaboloidPosition;

	vs_out.VertexNormal = normalize((NormalMatrix * vec4(Normal, 1.0f)).xyz);
	vec4 FragmentPosInWorldSpace = ObjectMatrix * vec4(Position, 1.0f);
	vs_out.FragmentPos = (ViewMatrix * FragmentPosInWorldSpace).xyz;
	vs_out.ViewDir = normalize(-vs_out.FragmentPos);
	for(int LightIndex = 0; LightIndex < LightCount; ++LightIndex)
	{
		vs_out.FragmentPosInLightSpace[LightIndex] = LightSpaceMatrix[LightIndex] * FragmentPosInWorldSpace;
		vs_out.LightDir[LightIndex] = normalize((ViewMatrix * vec4(LightPos[LightIndex], 1.0f)).xyz - vs_out.FragmentPos);
		vs_out.HalfDir[LightIndex] = normalize(vs_out.ViewDir[LightIndex] + vs_out.LightDir[LightIndex]);
	}
}
