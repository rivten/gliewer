#version 400 core

layout (location = 0) in vec3 Position;
layout (location = 1) in vec2 TexCoords;

out vec2 TextureCoordinates;

void main()
{
	gl_Position = vec4(Position, 1.0f);
	TextureCoordinates = TexCoords;
}
