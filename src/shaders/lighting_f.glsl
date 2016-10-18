#version 400 core

out vec4 Color;
in vec3 VertexNormal;
in vec4 FragmentPositionInWorldSpace;
uniform vec3 LightPos;
uniform vec4 LightColor;
uniform mat4 ViewMatrix;

void main()
{
	vec3 LightDirectionInEyeSpace = normalize(vec3(ViewMatrix * vec4(LightPos, 1.0f) - vec3(ViewMatrix * FragmentPositionInWorldSpace)));

	// NOTE(hugo) : Computations needs to happen in eye space
	// since we computed the Normal in that very space
	Color = LightColor * max(0.0, dot(LightDirectionInEyeSpace, VertexNormal));
}
