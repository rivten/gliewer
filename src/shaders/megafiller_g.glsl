#version 400 core

#extension GL_ARB_viewport_array : enable

layout(triangles) in;

layout(triangle_strip, max_vertices = 3) out;

uniform int LightCount;

in VS_OUT
{
	int ViewportIndex;

	vec3 VertexNormal;
	vec4 FragmentPosInLightSpace[4];
	vec3 ViewDir;
	vec3 FragmentPos;
	vec3 LightDir[4];
	vec3 HalfDir[4];
} gs_in[];

out GS_OUT
{
	vec3 VertexNormal;
	vec4 FragmentPosInLightSpace[4];
	vec3 FragmentPos;
	vec3 ViewDir;
	vec3 LightDir[4];
	vec3 HalfDir[4];
} gs_out;

void main()
{
	int InLength = gl_in.length();
	for(int VertexIndex = 0; VertexIndex < InLength; ++VertexIndex)
	{
		gl_ViewportIndex = gs_in[VertexIndex].ViewportIndex;
		gl_Position = gl_in[VertexIndex].gl_Position;

		gs_out.VertexNormal = gs_in[VertexIndex].VertexNormal;
		gs_out.FragmentPos = gs_in[VertexIndex].FragmentPos;
		gs_out.ViewDir = gs_in[VertexIndex].ViewDir;
		for(int LightIndex = 0; LightIndex < LightCount; ++LightIndex)
		{
			gs_out.FragmentPosInLightSpace[LightIndex] = gs_in[VertexIndex].FragmentPosInLightSpace[LightIndex];
			gs_out.LightDir[LightIndex] = gs_in[VertexIndex].LightDir[LightIndex];
			gs_out.HalfDir[LightIndex] = gs_in[VertexIndex].HalfDir[LightIndex];
		}
		EmitVertex();
	}

	EndPrimitive();
}
