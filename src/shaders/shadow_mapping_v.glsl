#version 400 core

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 TextureCoord;

out vec3 VertexNormal;
out vec3 NormalWorldSpace;
out vec4 FragmentPositionInWorldSpace;
out vec4 FragmentPositionInLightSpace[4];
out vec2 TextureCoordinates;

uniform mat4 MVPMatrix;
uniform mat4 NormalMatrix;
uniform mat4 ModelObjectMatrix;
uniform mat4 LightSpaceMatrix[4];
uniform int LightCount;
uniform mat4 NormalWorldMatrix;
uniform int UseNormalMapping;

uniform sampler2D NormalMap;

void main()
{
	float Step = 4.5f;
	vec3 Offset = gl_InstanceID * Step * vec3(1.0f, 0.0f, 0.0f);
	vec4 ObjectPos = vec4(Position + Offset, 1.0f);

	// TODO(hugo) : What should really be done would be to add the offset to
	// the vertex in _world space_ (and not in _object space_ like I'm doing right now
	gl_Position = MVPMatrix * ObjectPos;
	FragmentPositionInWorldSpace = ModelObjectMatrix * ObjectPos;
	for(int LightIndex = 0; LightIndex < LightCount; ++LightIndex)
	{
		FragmentPositionInLightSpace[LightIndex] = LightSpaceMatrix[LightIndex] * ModelObjectMatrix * ObjectPos;
	}

	// TODO(hugo) : Do I want to negate everything here or do I want to load bitmap upside down ?
	TextureCoordinates = vec2(1.0f, 1.0f) - TextureCoord;

	if(UseNormalMapping == 1)
	{
		vec3 N = texture(NormalMap, TextureCoordinates).xyz;
		N = 2.0f * N - vec3(1.0f, 1.0f, 1.0f);
		VertexNormal = normalize(vec3(NormalMatrix * vec4(N, 1.0f)));
		NormalWorldSpace = normalize(vec3(NormalWorldMatrix * vec4(N, 1.0f)));
	}
	else
	{
		VertexNormal = normalize(vec3(NormalMatrix * vec4(Normal, 1.0)));
		NormalWorldSpace = normalize(vec3(NormalWorldMatrix * vec4(Normal, 1.0f)));
	}
}
