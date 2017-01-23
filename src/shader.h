#pragma once

enum shader_type
{
	ShaderType_DirectLighting,
	ShaderType_BRDFConvolutional,
	ShaderType_Skybox,
	ShaderType_PostProcess,
	ShaderType_LowCost,
	ShaderType_FXAA,
	ShaderType_FillGBuffer,

	ShaderType_Count,
};

#define MAX_UNIFORM_COUNT 64
struct shader
{
	GLuint Program;
	shader_type Type;

	// NOTE(hugo) : Uniform and Location indices are in a one-to-one correspondance
	u32 Locations[MAX_UNIFORM_COUNT];
};

struct shader_source
{
	char* VertexSourceFilename;
	char* FragmentSourceFilename;
};

static shader_source Sources[ShaderType_Count] = 
{
	{"../src/shaders/depth_debug_quad_v.glsl", "../src/shaders/shadow_mapping_f.glsl"},
	{"../src/shaders/depth_debug_quad_v.glsl", "../src/shaders/brdf_conv_f.glsl"},
	{"../src/shaders/skybox_v.glsl", "../src/shaders/skybox_f.glsl"},
	{"../src/shaders/depth_debug_quad_v.glsl", "../src/shaders/depth_debug_quad_f.glsl"},
	{"../src/shaders/basic_v.glsl", "../src/shaders/basic_f.glsl"},
	{"../src/shaders/depth_debug_quad_v.glsl", "../src/shaders/fxaa_f.glsl"},
	{"../src/shaders/fillg_v.glsl", "../src/shaders/fillg_f.glsl"},
};

static char* Uniforms[ShaderType_Count][MAX_UNIFORM_COUNT] = 
{
	// NOTE(hugo) : ShaderType_DirectLighting
	{

		"ShadowMap[0]",
		"ShadowMap[1]",
		"ShadowMap[2]",
		"ShadowMap[3]",
		"DepthTexture",
		"NormalTexture",
		"AlbedoTexture",
		"SpecularTexture",
		"LightPos[0]",
		"LightPos[1]",
		"LightPos[2]",
		"LightPos[3]",
		"LightColor[0]",
		"LightColor[1]",
		"LightColor[2]",
		"LightColor[3]",
		"LightCount",
		"LightViews[0]",
		"LightViews[1]",
		"LightViews[2]",
		"LightViews[3]",
		"CameraPos",
		"CTF0",
		"Alpha",
		"LightIntensity",
		"Ks",
		"Kd",
		"InvView",
		"CameraNearPlane",
		"CameraFarPlane",
		"CameraFoV",
		"CameraAspect",
		"AmbientFactor",
	},

	// NOTE(hugo) : ShaderType_BRDFConvolutional
	{
		// NOTE(hugo) : Fragment shader
		"MegaTextures[0]",
		"MegaTextures[1]",
		"MegaTextures[2]",
		"MegaTextures[3]",
		"MegaTextures[4]",
		"DepthMap",
		"NormalMap",
		"AlbedoMap",
		"DirectIlluminationMap",
		"PatchSizeInPixels",
		"PatchWidth",
		"PatchHeight",
		"PatchX",
		"PatchY",
		"MicrobufferWidth",
		"MicrobufferHeight",
		"CameraPos",
		"PixelSurfaceInMeters",
		"Alpha",
		"CookTorranceF0",
		"MicroCameraNearPlane",
		"WorldUp",
		"MainCameraAspect",
		"MainCameraFoV",
		"MainCameraNearPlane",
		"MainCameraFarPlane",
		"InvLookAtCamera",
		"WindowWidth",
		"WindowHeight",
	},

	// NOTE(hugo) : ShaderType_Skybox
	{
		"Projection",
		"View",

		"Skybox",
	},

	// NOTE(hugo) : ShaderType_PostProcess
	{

		"ScreenTexture",
		"Sigma",
		"DepthTexture", 
		"NormalTexture",
		"NearPlane",
		"FarPlane", 
		"FoV",
		"Aspect", 
		"InvView",
		"AOSamples",
		"AOIntensity",
		"AOScale",
		"AORadius",
		"AOBias",
		"WindowWidth",
		"WindowHeight",
		"DoMotionBlur",
		"PreviousViewProj",
		"MotionBlurSampleCount",
		"Skybox",
		"UntranslatedInvView",
	},

	// NOTE(hugo) : ShaderType_LowCost
	{
		"MVPMatrix",

		"ObjectColor",
	},
	
	// NOTE(hugo) : ShaderType_FXAA
	{
		"Texture",
		"FXAAMultiplicationFactor",
		"FXAAMinimalReduction",
		"FXAASpanMax",
	},
	// NOTE(hugo) : ShaderType_FillGBuffer
	{
		"MVPMatrix",
		"NormalWorldMatrix",
		"UseNormalMapping",
		"NormalMap",

		"SpecularColor",
		"DiffuseColor",
		"UseTextureMapping",
		"TextureMap",
	},
	
};

GLuint GetUniformLocation(shader Shader, const char* VariableName);
shader LoadShader(u32 ShaderType)
{
	const char* VertexPath = Sources[ShaderType].VertexSourceFilename;
	const char* FragmentPath = Sources[ShaderType].FragmentSourceFilename;
	shader Result = {};
	Result.Type = (shader_type)ShaderType;

	char* VertexCode = ReadFileContent(VertexPath);
	char* FragmentCode = ReadFileContent(FragmentPath);

	GLint Success;
	GLchar InfoLog[512];

	GLuint Vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(Vertex, 1, &VertexCode, 0);
	glCompileShader(Vertex);
	glGetShaderiv(Vertex, GL_COMPILE_STATUS, &Success);
	if (!Success)
	{
		glGetShaderInfoLog(Vertex, 512, 0, InfoLog);
		SDL_Log("%s\n", InfoLog);
		InvalidCodePath;
	}
	Free(VertexCode);

	GLuint Fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(Fragment, 1, &FragmentCode, 0);
	glCompileShader(Fragment);
	glGetShaderiv(Fragment, GL_COMPILE_STATUS, &Success);
	if (!Success)
	{
		glGetShaderInfoLog(Fragment, 512, 0, InfoLog);
		SDL_Log("%s\n", InfoLog);
		InvalidCodePath;
	}
	Free(FragmentCode);

	Result.Program = glCreateProgram();
	glAttachShader(Result.Program, Vertex);
	glAttachShader(Result.Program, Fragment);
	glLinkProgram(Result.Program);
	glGetProgramiv(Result.Program, GL_LINK_STATUS, &Success);
	if (!Success)
	{
		glGetProgramInfoLog(Result.Program, 512, 0, InfoLog);
		SDL_Log("%s\n", InfoLog);
		InvalidCodePath;
	}

	glDeleteShader(Vertex);
	glDeleteShader(Fragment);

	// NOTE(hugo) : Setting uniform location
	for(u32 UniformIndex = 0; UniformIndex < ArrayCount(Uniforms[ShaderType]); ++UniformIndex)
	{
		char* UniformName = Uniforms[ShaderType][UniformIndex];
		if(UniformName && (!IsEmptyString(UniformName)))
		{
			Result.Locations[UniformIndex] = glGetUniformLocation(Result.Program, UniformName);
			Assert(Result.Locations[UniformIndex] != -1);
		}
	}

	return(Result);
}

shader LoadShader(const char* VertexPath, const char* GeometryPath, const char* FragmentPath)
{
	shader Result = {};

	char* VertexCode = ReadFileContent(VertexPath);
	char* GeometryCode = ReadFileContent(FragmentPath);
	char* FragmentCode = ReadFileContent(FragmentPath);

	GLint Success;
	GLchar InfoLog[512];

	GLuint Vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(Vertex, 1, &VertexCode, 0);
	glCompileShader(Vertex);
	glGetShaderiv(Vertex, GL_COMPILE_STATUS, &Success);
	if (!Success)
	{
		glGetShaderInfoLog(Vertex, 512, 0, InfoLog);
		SDL_Log("%s\n", InfoLog);
	}
	Free(VertexCode);

	GLuint Geometry = glCreateShader(GL_GEOMETRY_SHADER);
	glShaderSource(Geometry, 1, &GeometryCode, 0);
	glCompileShader(Geometry);
	glGetShaderiv(Geometry, GL_COMPILE_STATUS, &Success);
	if (!Success)
	{
		glGetShaderInfoLog(Geometry, 512, 0, InfoLog);
		SDL_Log("%s\n", InfoLog);
	}
	Free(GeometryCode);

	GLuint Fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(Fragment, 1, &FragmentCode, 0);
	glCompileShader(Fragment);
	glGetShaderiv(Fragment, GL_COMPILE_STATUS, &Success);
	if (!Success)
	{
		glGetShaderInfoLog(Fragment, 512, 0, InfoLog);
		SDL_Log("%s\n", InfoLog);
	}
	Free(FragmentCode);

	Result.Program = glCreateProgram();
	glAttachShader(Result.Program, Vertex);
	glAttachShader(Result.Program, Geometry);
	glAttachShader(Result.Program, Fragment);
	glLinkProgram(Result.Program);
	glGetProgramiv(Result.Program, GL_LINK_STATUS, &Success);
	if (!Success)
	{
		glGetProgramInfoLog(Result.Program, 512, 0, InfoLog);
		SDL_Log("%s\n", InfoLog);
	}

	glDeleteShader(Vertex);
	glDeleteShader(Geometry);
	glDeleteShader(Fragment);

	return(Result);
}

