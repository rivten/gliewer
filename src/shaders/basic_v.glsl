#version 330 core

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;
out vec4 VertexNormal;

uniform mat4 MVPMatrix;
uniform mat4 NormalMatrix;

void main()
{
	gl_Position = MVPMatrix * vec4(Position, 1.0);
	VertexNormal = normalize(NormalMatrix * vec4(Normal, 1.0));
}
