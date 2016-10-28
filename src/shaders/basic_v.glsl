#version 400 core

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;

uniform mat4 MVPMatrix;

void main()
{
	float Step = 4.5f;
	vec3 Offset = gl_InstanceID * Step * vec3(1.0f, 0.0f, 0.0f);
	vec4 ObjectPos = vec4(Position + Offset, 1.0f);

	// TODO(hugo) : What should really be done would be to add the offset to
	// the vertex in _world space_ (and not in _object space_ like I'm doing right now
	gl_Position = MVPMatrix * ObjectPos;
}
