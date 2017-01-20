#version 400 core

in vec2 TextureCoordinates;

out vec4 Color;

uniform sampler2D Texture;

uniform float FXAAMultiplicationFactor;
uniform float FXAAMinimalReduction;
uniform float FXAASpanMax;

void main()
{
	vec2 TexelSize = 1.0f / textureSize(Texture, 0);

	vec3 EyeChroma = vec3(0.299, 0.587, 0.114);
	float ChromaTopLeft = dot(EyeChroma, 
			texture(Texture, TextureCoordinates + TexelSize * vec2(-1.0f, -1.0f)).xyz);
	float ChromaTopRight = dot(EyeChroma, 
			texture(Texture, TextureCoordinates + TexelSize * vec2(1.0f, -1.0f)).xyz);
	float ChromaBottomLeft = dot(EyeChroma, 
			texture(Texture, TextureCoordinates + TexelSize * vec2(-1.0f, 1.0f)).xyz);
	float ChromaBottomRight = dot(EyeChroma, 
			texture(Texture, TextureCoordinates + TexelSize * vec2(1.0f, 1.0f)).xyz);
	float ChromaMiddle = dot(EyeChroma, texture(Texture, TextureCoordinates).xyz);

	vec2 Dir;
	Dir.x = -((ChromaTopLeft + ChromaTopRight) - (ChromaBottomLeft + ChromaBottomRight));
	Dir.y = (ChromaTopLeft + ChromaBottomLeft) - (ChromaTopRight + ChromaBottomRight);

	float DirectionReduce = max((ChromaTopLeft + ChromaTopRight + ChromaBottomLeft + ChromaBottomRight) * 
			(FXAAMultiplicationFactor * 0.25f), FXAAMinimalReduction);
	float InverseDirectionAdjustment = 1.0f / (min(abs(Dir.x), abs(Dir.y)) + DirectionReduce);

	Dir = min(vec2(FXAASpanMax, FXAASpanMax),
			max(vec2(-FXAASpanMax, -FXAASpanMax), Dir * InverseDirectionAdjustment)) * TexelSize;

	vec3 Color0 = 0.5f * (
			texture(Texture, TextureCoordinates + (Dir * (1.0f / 3.0f - 0.5f))).xyz +
			texture(Texture, TextureCoordinates + (Dir * (2.0f / 3.0f - 0.5f))).xyz);
	vec3 Color1 = 0.5f * Color0 + 0.25f * (
			texture(Texture, TextureCoordinates + (Dir * (0.0f / 3.0f - 0.5f))).xyz +
			texture(Texture, TextureCoordinates + (Dir * (3.0f / 3.0f - 0.5f))).xyz);

	float ChromaMin = min(ChromaMiddle, 
			min(min(ChromaTopLeft, ChromaTopRight), 
				min(ChromaBottomLeft, ChromaBottomRight)));

	float ChromaMax = max(ChromaMiddle, 
			max(max(ChromaTopLeft, ChromaTopRight), 
				max(ChromaBottomLeft, ChromaBottomRight)));

	float Chroma1 = dot(EyeChroma, Color1);

	if((Chroma1 < ChromaMin) || (Chroma1 > ChromaMax))
	{
		Color = vec4(Color0, 1.0f);
	}
	else
	{
		Color = vec4(Color1, 1.0f);
	}
	//Color = vec4(Dir / TexelSize, 0.0f, 1.0f);
}
