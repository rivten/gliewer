#version 400 core

#extension GL_ARB_viewport_array : enable

layout(triangles) in;

layout(triangle_strip, max_vertices = 3) out;

uniform int LightCount;

in VS_OUT
{
	int ViewportIndex;

	vec3 VertexNormal;
	vec4 FragmentPosInWorldSpace;
	vec4 FragmentPosInLightSpace[4];

} gs_in[];

out GS_OUT
{
	vec3 VertexNormal;
	vec4 FragmentPosInWorldSpace;
	vec4 FragmentPosInLightSpace[4];
} gs_out;

void main()
{
	int InLength = gl_in.length();
	for(int VertexIndex = 0; VertexIndex < InLength; ++VertexIndex)
	{
		gl_ViewportIndex = gs_in[VertexIndex].ViewportIndex;
		gl_Position = gl_in[VertexIndex].gl_Position;

		gs_out.VertexNormal = gs_in[VertexIndex].VertexNormal;
		gs_out.FragmentPosInWorldSpace = gs_in[VertexIndex].FragmentPosInWorldSpace;
		for(int LightIndex = 0; LightIndex < LightCount; ++LightIndex)
		{
			gs_out.FragmentPosInLightSpace[LightIndex] = gs_in[VertexIndex].FragmentPosInLightSpace[LightIndex];
		}
		EmitVertex();
	}

	EndPrimitive();
}
