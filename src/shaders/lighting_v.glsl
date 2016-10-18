#version 400 core

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;
out vec3 VertexNormal;
out vec4 FragmentPositionInWorldSpace;

uniform mat4 MVPMatrix;
uniform mat4 NormalMatrix;
uniform mat4 ModelObjectMatrix;

void main()
{
	gl_Position = MVPMatrix * vec4(Position, 1.0);
	VertexNormal = normalize(vec3(NormalMatrix * vec4(Normal, 1.0)));
	FragmentPositionInWorldSpace = ModelObjectMatrix * vec4(Position, 1.0);
}
