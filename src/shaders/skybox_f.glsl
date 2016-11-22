#version 400 core

in vec3 TextureCoordinates;
out vec4 Color;

uniform samplerCube Skybox;

void main()
{
	Color = texture(Skybox, TextureCoordinates);
	//Color = vec4(1.0f, 0.0f, 1.0f, 1.0f);
}
