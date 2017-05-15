#version 400 core

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 TextureCoord;

uniform mat4 ObjectMatrix;

uniform vec3 LightPos[4];

uniform mat4 NormalMatrix;
uniform int LightCount;
uniform mat4 LightSpaceMatrix[4];
uniform mat4 ViewMatrix;

// NOTE(hugo) : This goes to the geometry shader
out VS_OUT
{
	int ViewportIndex;

	vec3 Position;
	vec3 VertexNormal;
	vec4 FragmentPosInLightSpace[4];
	vec3 ViewDir;
	vec3 FragmentPos;
	vec3 LightDir[4];
	vec3 HalfDir[4];
} vs_out;

void main()
{
	vs_out.ViewportIndex = gl_InstanceID;

	vs_out.Position = Position;

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
