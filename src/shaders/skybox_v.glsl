#version 400 core

layout (location = 0) in vec3 Position;
out vec3 TextureCoordinates;

uniform mat4 Projection;
uniform mat4 View;

void main()
{
	vec4 P = Projection * View * vec4(Position, 1.0f);
	P = P.xyww;
	gl_Position = P;
	TextureCoordinates = Position;
}
