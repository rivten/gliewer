#version 400 core

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 TextureCoord;

uniform mat4 ObjectMatrix;

uniform vec3 LightPos;

uniform mat4 NormalMatrix;
uniform mat4 LightSpaceMatrix;
uniform mat4 ViewMatrix;

// NOTE(hugo) : This goes to the geometry shader
out VS_OUT
{
	int ViewportIndex;

	vec3 VertexNormal;
	//vec4 FragmentPosInLightSpace;
	float Shadow;
	vec3 FragmentPos;

	vec3 ViewDir;
	vec3 LightDir;
	vec3 HalfDir;
} vs_out;

uniform sampler2D ShadowMap;
// ---------------------------------------
// NOTE(hugo) : Shadow Map Computation
// ---------------------------------------
float ShadowFactor(vec4 FragmentPositionInLightSpace, sampler2D ShadowMap, float Bias)
{
	vec3 ProjectedCoordinates = FragmentPositionInLightSpace.xyz / FragmentPositionInLightSpace.w;
	ProjectedCoordinates = 0.5f * ProjectedCoordinates + 0.5f;
	float FragmentDepth = ProjectedCoordinates.z;

	float Result = 0.0f;
	vec2 TexelSize = 1.0f / textureSize(ShadowMap, 0);

	int PCFSize = 3;
	int K = PCFSize / 2;

	for(int X = -K; X <= K; ++X)
	{
		for(int Y = -K; Y <= K; ++Y)
		{
			float PCFDepthValue = texture(ShadowMap, ProjectedCoordinates.xy + vec2(X, Y) * TexelSize).r;
			Result += ((FragmentDepth - Bias) > PCFDepthValue) ? 1.0f : 0.0f;
		}
	}
	Result /= float(PCFSize * PCFSize);

	return(Result);
}

void main()
{
	vs_out.ViewportIndex = gl_InstanceID;

	gl_Position = vec4(Position, 1.0f);

	vs_out.VertexNormal = normalize((NormalMatrix * vec4(Normal, 1.0f)).xyz);
	vec4 FragmentPosInWorldSpace = ObjectMatrix * vec4(Position, 1.0f);
	vs_out.FragmentPos = (ViewMatrix * FragmentPosInWorldSpace).xyz;

	vec3 ViewDir = normalize(-vs_out.FragmentPos);
	vec3 LightDir = normalize((ViewMatrix * vec4(LightPos, 1.0f)).xyz - vs_out.FragmentPos);
	vec3 HalfDir = normalize(vs_out.ViewDir + vs_out.LightDir);

	vs_out.ViewDir = ViewDir;
	vs_out.LightDir = LightDir;
	vs_out.HalfDir = HalfDir;

	vec4 FragmentPosInLightSpace = LightSpaceMatrix * FragmentPosInWorldSpace;
	float ShadowMappingBias = max(0.01f * (1.0f - dot(vs_out.VertexNormal, LightDir)), 0.005f);
	vs_out.Shadow = ShadowFactor(FragmentPosInLightSpace, ShadowMap, ShadowMappingBias);

}
