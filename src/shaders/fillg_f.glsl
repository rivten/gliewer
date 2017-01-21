#version 400 core

layout (location = 0) out vec3 Normal;
layout (location = 1) out vec3 Albedo;
layout (location = 2) out vec3 Specular;

in vec3 NormalWorldSpace;
in vec2 TextureCoordinates;

uniform int UseTextureMapping;
uniform sampler2D TextureMap;
uniform vec3 DiffuseColor;
uniform vec3 SpecularColor;

void main()
{
	Albedo = DiffuseColor;
	if(UseTextureMapping == 1)
	{
		Albedo = texture(TextureMap, TextureCoordinates).xyz;
	}

	// NOTE(hugo) : Packing the normal into [0,1]^3
	Normal = 0.5f * NormalWorldSpace + vec3(0.5f, 0.5f, 0.5f);
	Specular = SpecularColor;
}

