#version 400 core

in vec2 TextureCoordinates;

out vec4 Color;

uniform sampler2DArray Texture;
uniform int LayerIndex;

void main()
{
	vec3 SampleCoords = vec3(TextureCoordinates, LayerIndex);
	Color = texture(Texture, SampleCoords);
}
