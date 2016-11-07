#version 400 core

in vec2 TextureCoordinates;

out vec4 Color;

uniform sampler2D ScreenTexture;
uniform float Sigma;

const float Pi = 3.14159265f;
const float Epsilon = 0.001f;

void main()
{
	if(Sigma < Epsilon)
	{
		Color = texture(ScreenTexture, TextureCoordinates);
		//Color = vec4(dot(texture(ScreenTexture, TextureCoordinates), vec4(0.3f, 0.59, 0.11f, 0.0f)) * vec3(1.0f, 1.0f, 1.0f), 1.0f);
	}
	else
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
}
