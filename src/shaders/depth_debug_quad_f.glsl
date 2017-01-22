#version 400 core

in vec2 TextureCoordinates;

out vec4 Color;

uniform sampler2D ScreenTexture;
uniform float Sigma;

uniform sampler2D DepthTexture;
uniform sampler2D NormalTexture;
uniform float NearPlane;
uniform float FarPlane;
uniform float FoV;
uniform float Aspect;
uniform mat4 InvView;
uniform int AOSamples;
uniform float AOIntensity;
uniform float AOScale;
uniform float AORadius;
uniform float AOBias;

uniform int WindowWidth;
uniform int WindowHeight;

uniform int DoMotionBlur;
uniform mat4 PreviousViewProj;
uniform int MotionBlurSampleCount;

const float Pi = 3.14159265f;
const float Epsilon = 0.001f;

float OcclusionFactor(vec3 Normal, vec3 V, float d)
{
	float Result = max(0.0f, dot(Normal, V) - AOBias) * (1.0f / (1.0f + d));
	return(Result);
}

vec4 UnprojectPixel(float Depth, 
		float X, float Y, 
		float WindowWidth, float WindowHeight,
		float CameraFoV, float CameraAspect)
{
	vec4 PixelPos = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	PixelPos.z = -Depth;
	PixelPos.w = 1.0f;
	PixelPos.x = float(X) / float(WindowWidth);
	PixelPos.y = float(Y) / float(WindowHeight);

	PixelPos.xy = 2.0f * PixelPos.xy - vec2(1.0f, 1.0f);
	PixelPos.x = - CameraAspect * tan(0.5f * CameraFoV) * PixelPos.z * PixelPos.x;
	PixelPos.y = - tan(0.5f * CameraFoV) * PixelPos.z * PixelPos.y;

	PixelPos = InvView * PixelPos;

	PixelPos.w = 1.0f;
	return(PixelPos / PixelPos.w);
}

float UnlinearizeDepth(float Depth, float NearPlane, float FarPlane)
{
	float Result = 2.0f * Depth - 1.0f;
	Result = 2.0f * NearPlane * FarPlane / (NearPlane + FarPlane - Result * (FarPlane - NearPlane));

	return(Result);
}

float ComputeAmbientOcclusion(vec2 SamplingOffset, vec3 PixelPos, vec3 Normal, vec2 TexelSize)
{
	float NeighbourDepth = texture(DepthTexture, TextureCoordinates + (SamplingOffset * TexelSize)).r;
	NeighbourDepth = UnlinearizeDepth(NeighbourDepth, NearPlane, FarPlane);
	float X = (TextureCoordinates.x + SamplingOffset.x * TexelSize.x) * WindowWidth;
	float Y = (TextureCoordinates.y + SamplingOffset.y * TexelSize.y) * WindowHeight;
	vec4 NeighbourPixel = UnprojectPixel(NeighbourDepth, 
			X, Y, float(WindowWidth), float(WindowHeight), FoV, Aspect);
	vec3 Diff = NeighbourPixel.xyz - PixelPos;
	float DiffLength = AOScale * length(Diff);
	Diff = normalize(Diff);

	float Result = AOIntensity * OcclusionFactor(Normal, Diff, DiffLength);
	return(Result);
}

void main()
{
	// TODO(hugo) : Sort out how to compose post-processes
	if(DoMotionBlur == 1)
	{
		vec2 TexelSize = 1.0f / textureSize(ScreenTexture, 0);
		vec2 SamplingCoord = TextureCoordinates;
		float Depth = texture(DepthTexture, SamplingCoord).r;
		Depth = UnlinearizeDepth(Depth, NearPlane, FarPlane);
		vec4 PixelPos = UnprojectPixel(Depth, SamplingCoord.x * WindowWidth, 
				SamplingCoord.y * WindowHeight,
				float(WindowWidth), float(WindowHeight), FoV, Aspect);
		vec4 PixelPreviousPosInClipSpace = PreviousViewProj * PixelPos;

		PixelPreviousPosInClipSpace.xyz = PixelPreviousPosInClipSpace.xyz / PixelPreviousPosInClipSpace.w;
		PixelPreviousPosInClipSpace.xy = 0.5f * PixelPreviousPosInClipSpace.xy + vec2(0.5f, 0.5f);

		vec2 BlurVector = PixelPreviousPosInClipSpace.xy - TextureCoordinates;

		Color = texture(ScreenTexture, TextureCoordinates);
		for(int SampleIndex = 0; SampleIndex < MotionBlurSampleCount; ++SampleIndex)
		{
			vec2 Offset = BlurVector * ((float(SampleIndex) / float(MotionBlurSampleCount - 1)) - 0.5f);
			Color += texture(ScreenTexture, TextureCoordinates + Offset);
		}
		Color = Color / float(MotionBlurSampleCount);
		Color.w = 1.0f;
	}
	else if(Sigma >= Epsilon)
	{
		// NOTE(hugo) : Code inspired by GPU Gems 3 - Incremental Computation of the Gaussian
		vec2 TexelSize = 1.0f / textureSize(ScreenTexture, 0);
		vec2 BlurOffset = vec2(1.0f, 0.0f);

		vec3 IncrementalGaussian;
		IncrementalGaussian.x = 1.0f / (sqrt(2.0f * Pi) * Sigma);
		IncrementalGaussian.y = exp(-0.5f / (Sigma * Sigma));
		IncrementalGaussian.z = IncrementalGaussian.y * IncrementalGaussian.y;

		vec4 BlurValue;
		float CoefficientSum;

		// NOTE(hugo) : Central sample
		BlurValue += texture(ScreenTexture, TextureCoordinates) * IncrementalGaussian.x;
		CoefficientSum += IncrementalGaussian.x;

		for(int PixelIndex = 1; PixelIndex <= 8; ++PixelIndex)
		{
			BlurValue += texture(ScreenTexture, TextureCoordinates - PixelIndex * TexelSize * BlurOffset) * IncrementalGaussian.x;
			BlurValue += texture(ScreenTexture, TextureCoordinates + PixelIndex * TexelSize * BlurOffset) * IncrementalGaussian.x;
			CoefficientSum += 2.0f * IncrementalGaussian.x;
			IncrementalGaussian.xy *= IncrementalGaussian.yz;
		}

		Color = BlurValue / CoefficientSum;
	}
	else if(AOSamples != 0)
	{
		vec2 TexelSize = 1.0f / textureSize(ScreenTexture, 0);
		vec2 SamplingCoord = TextureCoordinates;
		float Depth = texture(DepthTexture, SamplingCoord).r;
		Depth = UnlinearizeDepth(Depth, NearPlane, FarPlane);
		vec4 PixelPos = UnprojectPixel(Depth, SamplingCoord.x * WindowWidth, 
				SamplingCoord.y * WindowHeight,
				float(WindowWidth), float(WindowHeight), FoV, Aspect);
		vec3 Normal = texture(NormalTexture, SamplingCoord).xyz;

		vec2 SamplingDirs[4];
		SamplingDirs[0] = vec2(1.0f, 0.0f);
		SamplingDirs[1] = vec2(-1.0f, 0.0f);
		SamplingDirs[2] = vec2(0.0f, 1.0f);
		SamplingDirs[3] = vec2(0.0f, -1.0f);
		
		// NOTE(hugo) : Performing SSAO
		float AmbientOcclusionFactor = 0.0f;
		int IterationCount = 4;
		for(int IterationIndex = 0; IterationIndex < IterationCount; ++IterationIndex)
		{
			// TODO(hugo) : Use random texture to sample for uniformly
			vec2 CoordA = AORadius * SamplingDirs[IterationIndex] / PixelPos.z;
			vec2 CoordB = 0.707f * vec2(CoordA.x - CoordA.y, CoordA.x + CoordA.y);
			AmbientOcclusionFactor += ComputeAmbientOcclusion(0.25f * CoordA, PixelPos.xyz, Normal, TexelSize);
			AmbientOcclusionFactor += ComputeAmbientOcclusion(0.50f * CoordB, PixelPos.xyz, Normal, TexelSize);
			AmbientOcclusionFactor += ComputeAmbientOcclusion(0.75f * CoordA, PixelPos.xyz, Normal, TexelSize);
			AmbientOcclusionFactor += ComputeAmbientOcclusion(CoordB, PixelPos.xyz, Normal, TexelSize);
		}
		AmbientOcclusionFactor = AmbientOcclusionFactor / (4.0f * float(IterationCount));

		Color = (1.0f - AmbientOcclusionFactor) * texture(ScreenTexture, TextureCoordinates);
	}
	else
	{
		Color = texture(ScreenTexture, TextureCoordinates);
	}
}
