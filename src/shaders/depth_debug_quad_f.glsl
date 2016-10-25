#version 400 core

in vec2 TextureCoordinates;

out vec4 Color;

uniform sampler2D ScreenTexture;

void main()
{
	//float DepthValue = texture(ScreenTexture, TextureCoordinates).r;
	//Color = vec4(vec3(DepthValue), 1.0f);
	Color = texture(ScreenTexture, TextureCoordinates);
}
