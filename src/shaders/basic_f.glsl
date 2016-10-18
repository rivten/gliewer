#version 400 core

out vec4 Color;

uniform vec4 ObjectColor;

void main()
{
	Color = ObjectColor;
}
