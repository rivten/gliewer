#version 400 core

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 TextureCoord;

uniform mat4 ObjectMatrix;

uniform vec3 LightPos;

uniform mat4 NormalMatrix;
uniform mat4 LightSpaceMatrix;
uniform mat4 ViewMatrix;

// NOTE(hugo) : This goes to the geometry shader
out VS_OUT
{
	int ViewportIndex;

	vec3 VertexNormal;
	vec4 FragmentPosInLightSpace;
	vec3 ViewDir;
	vec3 FragmentPos;
	vec3 LightDir;
	vec3 HalfDir;
} vs_out;

void main()
{
	vs_out.ViewportIndex = gl_InstanceID;

	gl_Position = vec4(Position, 1.0f);

	vs_out.VertexNormal = normalize((NormalMatrix * vec4(Normal, 1.0f)).xyz);
	vec4 FragmentPosInWorldSpace = ObjectMatrix * vec4(Position, 1.0f);
	vs_out.FragmentPos = (ViewMatrix * FragmentPosInWorldSpace).xyz;
	vs_out.ViewDir = normalize(-vs_out.FragmentPos);

	vs_out.FragmentPosInLightSpace = LightSpaceMatrix * FragmentPosInWorldSpace;
	vs_out.LightDir = normalize((ViewMatrix * vec4(LightPos, 1.0f)).xyz - vs_out.FragmentPos);
	vs_out.HalfDir = normalize(vs_out.ViewDir + vs_out.LightDir);
}
