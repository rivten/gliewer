#version 400 core

layout (location = 0) in vec3 Position;

uniform mat4 LightMVPMatrix;

void main()
{
	gl_Position = LightMVPMatrix * vec4(Position, 1.0f);
}
