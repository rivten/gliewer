#version 400 core

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 UV;

out vec3 NormalWorldSpace;
out vec2 TextureCoordinates;

uniform mat4 MVPMatrix;
uniform mat4 NormalWorldMatrix;
uniform int UseNormalMapping;
uniform sampler2D NormalMap;

void main()
{
	gl_Position = MVPMatrix * vec4(Position, 1.0f);

	// TODO(hugo) : Do I want to negate everything here or do I want to load bitmap upside down ?
	TextureCoordinates = vec2(1.0f, 1.0f) - UV;

	if(UseNormalMapping == 1)
	{
		vec3 N = texture(NormalMap, TextureCoordinates).xyz;
		N = 2.0f * N - vec3(1.0f, 1.0f, 1.0f);
		NormalWorldSpace = normalize(vec3(NormalWorldMatrix * vec4(N, 1.0f)));
	}
	else
	{
		NormalWorldSpace = normalize(vec3(NormalWorldMatrix * vec4(Normal, 1.0f)));
	}
}
