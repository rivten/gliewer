#version 330 core

out vec4 Color;
in vec4 VertexNormal;
uniform mat4 ViewMatrix;
uniform vec3 LightPos;
uniform vec4 LightColor;

void main()
{
	// NOTE(hugo) : In eye space
	vec3 LightPositionInEyeSpace = normalize((ViewMatrix * vec4(LightPos, 1.0f)).xyz); 

	Color = LightColor * max(0.0, dot(LightPositionInEyeSpace, VertexNormal.xyz));
}
