#version 400 core

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;

uniform mat4 MVPMatrix;

void main()
{
	gl_Position = MVPMatrix * vec4(Position, 1.0);
}
