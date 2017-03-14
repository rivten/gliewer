#version 400 core

#extension GL_ARB_viewport_array : enable

layout(triangles) in;

layout(triangle_strip, max_vertices = 3) out;

in VS_OUT
{
	int ViewportIndex;

	vec3 VertexNormal;
	vec3 NormalWorldSpace;
	vec4 FragmentPosInWorldSpace;
	vec4 FragmentPosInLightSpace[4];

	vec4 DEBUGColor;
} gs_in[];

out GS_OUT
{
	vec3 VertexNormal;
	vec3 NormalWorldSpace;
	vec4 FragmentPosInWorldSpace;
	vec4 FragmentPosInLightSpace[4];

	vec4 DEBUGColor;
} gs_out;

void main()
{
	gl_ViewportIndex = gs_in[0].ViewportIndex;
	gl_Position = gl_in[0].gl_Position;
	gs_out.VertexNormal = gs_in[0].VertexNormal;
	gs_out.NormalWorldSpace = gs_in[0].NormalWorldSpace;
	gs_out.FragmentPosInWorldSpace = gs_in[0].FragmentPosInWorldSpace;
	gs_out.FragmentPosInLightSpace[0] = gs_in[0].FragmentPosInLightSpace[0];
	gs_out.FragmentPosInLightSpace[1] = gs_in[0].FragmentPosInLightSpace[1];
	gs_out.FragmentPosInLightSpace[2] = gs_in[0].FragmentPosInLightSpace[2];
	gs_out.FragmentPosInLightSpace[3] = gs_in[0].FragmentPosInLightSpace[3];
	gs_out.DEBUGColor = gs_in[0].DEBUGColor;
	EmitVertex();

	gl_ViewportIndex = gs_in[1].ViewportIndex;
	gl_Position = gl_in[1].gl_Position;
	gs_out.VertexNormal = gs_in[1].VertexNormal;
	gs_out.NormalWorldSpace = gs_in[1].NormalWorldSpace;
	gs_out.FragmentPosInWorldSpace = gs_in[1].FragmentPosInWorldSpace;
	gs_out.FragmentPosInLightSpace[0] = gs_in[1].FragmentPosInLightSpace[0];
	gs_out.FragmentPosInLightSpace[1] = gs_in[1].FragmentPosInLightSpace[1];
	gs_out.FragmentPosInLightSpace[2] = gs_in[1].FragmentPosInLightSpace[2];
	gs_out.FragmentPosInLightSpace[3] = gs_in[1].FragmentPosInLightSpace[3];
	gs_out.DEBUGColor = gs_in[1].DEBUGColor;
	EmitVertex();

	gl_ViewportIndex = gs_in[2].ViewportIndex;
	gl_Position = gl_in[2].gl_Position;
	gs_out.VertexNormal = gs_in[2].VertexNormal;
	gs_out.NormalWorldSpace = gs_in[2].NormalWorldSpace;
	gs_out.FragmentPosInWorldSpace = gs_in[2].FragmentPosInWorldSpace;
	gs_out.FragmentPosInLightSpace[0] = gs_in[2].FragmentPosInLightSpace[0];
	gs_out.FragmentPosInLightSpace[1] = gs_in[2].FragmentPosInLightSpace[1];
	gs_out.FragmentPosInLightSpace[2] = gs_in[2].FragmentPosInLightSpace[2];
	gs_out.FragmentPosInLightSpace[3] = gs_in[2].FragmentPosInLightSpace[3];
	gs_out.DEBUGColor = gs_in[2].DEBUGColor;
	EmitVertex();

	EndPrimitive();

}
