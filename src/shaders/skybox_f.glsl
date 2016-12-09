#version 400 core

in vec3 TextureCoordinates;
out vec4 Color;

uniform samplerCube Skybox;

void main()
{
	Color = texture(Skybox, TextureCoordinates);
}
