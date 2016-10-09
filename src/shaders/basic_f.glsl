#version 330 core

out vec4 Color;
in vec4 VertexNormal;
uniform mat4 ViewMatrix;

void main()
{
	// NOTE(hugo) : In eye space
	vec3 LightPosition = normalize((ViewMatrix * vec4(10.0, 0.0, 10.0, 1.0)).xyz); 

	Color = vec4(1.0, 0.0, 0.0, 1.0) * max(0.0, dot(LightPosition, VertexNormal.xyz));
}
