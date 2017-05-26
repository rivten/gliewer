#version 400 core

layout(triangles) in;

layout(triangle_strip, max_vertices = 3) out;

out vec4 LayerColor;

in VS_OUT
{
	int LayerIndex;
} gs_in[];

void main()
{
	int InLength = gl_in.length();

	for(int VertexIndex = 0; VertexIndex < InLength; ++VertexIndex)
	{
		int LayerIndex = gs_in[VertexIndex].LayerIndex;
		LayerColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
		if(LayerIndex == 0)
		{
			LayerColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
		}
		else if(LayerIndex == 1)
		{
			LayerColor = vec4(0.0f, 0.0f, 1.0f, 1.0f);
		}
		else if(LayerIndex == 2)
		{
			LayerColor = vec4(1.0f, 1.0f, 0.0f, 1.0f);
		}
		else if(LayerIndex == 3)
		{
			LayerColor = vec4(1.0f, 0.0f, 1.0f, 1.0f);
		}
		else if(LayerIndex == 4)
		{
			LayerColor = vec4(0.0f, 1.0f, 1.0f, 1.0f);
		}
		else if(LayerIndex == 5)
		{
			LayerColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);
		}
		else if(LayerIndex == 6)
		{
			LayerColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
		}
		else if(LayerIndex == 7)
		{
			LayerColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
		}
		gl_Position = gl_in[VertexIndex].gl_Position;
		gl_Layer = LayerIndex;
		EmitVertex();
	}
	EndPrimitive();

}
