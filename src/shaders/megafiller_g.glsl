#version 400 core

#extension GL_ARB_viewport_array : enable

layout(triangles) in;

layout(triangle_strip, max_vertices = 3) out;

uniform mat4 ObjectMatrix;
uniform vec3 WorldUp;

uniform int LightCount;
uniform int LayerCount;

uniform sampler2D DepthMap;
uniform sampler2D NormalMap;

uniform int PatchX;
uniform int PatchY;
uniform int PatchSizeInPixels;

uniform float CameraNearPlane;
uniform float CameraFarPlane;
uniform float CameraFoV;
uniform float CameraAspect;

uniform mat4 InvLookAtCamera;

uniform int BaseTileID;

in VS_OUT
{
	int ViewportIndex;

	vec3 Position;
	vec3 VertexNormal;
	vec4 FragmentPosInLightSpace[4];
	vec3 ViewDir;
	vec3 FragmentPos;
	vec3 LightDir[4];
	vec3 HalfDir[4];
} gs_in[];

out GS_OUT
{
	vec3 VertexNormal;
	vec4 FragmentPosInLightSpace[4];
	vec3 FragmentPos;
	vec3 ViewDir;
	vec3 LightDir[4];
	vec3 HalfDir[4];
} gs_out;

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
	int InLength = gl_in.length();
	for(int LayerIndex = 0; LayerIndex < LayerCount; ++LayerIndex)
	{
		//
		// NOTE(hugo) : Computing micro view matrix
		// {
		//

		// TODO(hugo) : Dhhes not work for non-square patches. 
		// We should use PatchWidth and PatchHeight here
		// TODO(hugo) : This computation is already done in
		// the CPU. Just pass the result as uniform
		int RealInstanceID = LayerIndex + LayerCount * gs_in[0].ViewportIndex + BaseTileID;
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

		for(int VertexIndex = 0; VertexIndex < InLength; ++VertexIndex)
		{
			//
			// NOTE(hugo) : Performing paraboloid transformation
			// {
			//

			vec4 ViewPosition = MicroView * ObjectMatrix * vec4(gs_in[VertexIndex].Position, 1.0f);
			ViewPosition = normalize(ViewPosition);
			float Z = -ViewPosition.z;
			float ZPlusOne = Z + 1.0f;
			vec4 ParaboloidPosition = ViewPosition;
			ParaboloidPosition.xy = ParaboloidPosition.xy / ZPlusOne;
			ParaboloidPosition.z = (Z / CameraFarPlane) * 2.0f - 1.0f;
			ParaboloidPosition.w = 1.0f;

			//
			// }
			//

			gl_Position = ParaboloidPosition;
			gl_ViewportIndex = gs_in[VertexIndex].ViewportIndex;
			gl_Layer = LayerIndex;

			gs_out.VertexNormal = gs_in[VertexIndex].VertexNormal;
			gs_out.FragmentPos = gs_in[VertexIndex].FragmentPos;
			gs_out.ViewDir = gs_in[VertexIndex].ViewDir;
			for(int LightIndex = 0; LightIndex < LightCount; ++LightIndex)
			{
				gs_out.FragmentPosInLightSpace[LightIndex] =
					gs_in[VertexIndex].FragmentPosInLightSpace[LightIndex];
				gs_out.LightDir[LightIndex] = gs_in[VertexIndex].LightDir[LightIndex];
				gs_out.HalfDir[LightIndex] = gs_in[VertexIndex].HalfDir[LightIndex];
			}
			EmitVertex();
		}

		EndPrimitive();
	}
}
