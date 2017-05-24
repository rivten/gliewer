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
	ShaderType_FillMegaTexture,
	ShaderType_DEBUGMegaBuffer,

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
	char* GeometrySourceFilename; // NOTE(hugo) : If this pointer is NULL, then the shader has to geometry stage implemented.
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
	{"../src/shaders/megafiller_v.glsl", "../src/shaders/megafiller_f.glsl", "../src/shaders/megafiller_g.glsl"},
	{"../src/shaders/depth_debug_quad_v.glsl", "../src/shaders/debugmega_f.glsl"},
	//{"../src/shaders/megafiller_v.glsl", "../src/shaders/megafiller_f.glsl"},
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
		//"AmbientFactor",
	},

	// NOTE(hugo) : ShaderType_BRDFConvolutional
	{
		// NOTE(hugo) : Fragment shader
		"MegaTexture",
		"DepthMap",
		"NormalMap",
		"AlbedoMap",
		"DirectIlluminationMap",
		"PatchSizeInPixels",
		"PatchX",
		"PatchY",
		"MicrobufferWidth",
		"MicrobufferHeight",
		"CameraPos",
		"PixelSurfaceInMeters",
		"Alpha",
		"CookTorranceF0",
		"WorldUp",
		"MainCameraAspect",
		"MainCameraFoV",
		"MainCameraNearPlane",
		"MainCameraFarPlane",
		"InvLookAtCamera",
		"WindowWidth",
		"WindowHeight",
		"LayerCount",
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

	// NOTE(hugo) : ShaderType_FillMegaTexture
	{
		"DepthMap",
		"NormalMap",
		"PatchX",
		"PatchY",
		"PatchSizeInPixels",
		"ObjectMatrix",
		"WorldUp",
		"CameraNearPlane",
		"CameraFarPlane",
		"CameraFoV",
		"CameraAspect",
		"InvLookAtCamera",
		"NormalMatrix",
		"LightCount",
		"LightSpaceMatrix[0]",
		"LightSpaceMatrix[1]",
		"LightSpaceMatrix[2]",
		"LightSpaceMatrix[3]",
		"BaseTileID",

		"LayerCount",

		"ShadowMaps[0]",
		"ShadowMaps[1]",
		"ShadowMaps[2]",
		"ShadowMaps[3]",
		"LightPos[0]",
		"LightPos[1]",
		"LightPos[2]",
		"LightPos[3]",
		"LightColor[0]",
		"LightColor[1]",
		"LightColor[2]",
		"LightColor[3]",
		"ViewMatrix",
		"LightIntensity",
		"DiffuseColor",
		"SpecularColor",
		"Alpha",
		"CTF0",
		"Ks",
		"Kd",
	},

	// NOTE(hugo) : ShaderType_DEBUGMegaBuffer
	{
		"Texture",
		"LayerIndex",
	},
};

GLuint GetUniformLocation(shader Shader, const char* VariableName);

shader LoadShader(u32 ShaderType)
{
	const char* VertexPath = Sources[ShaderType].VertexSourceFilename;
	const char* FragmentPath = Sources[ShaderType].FragmentSourceFilename;

	const char* GeometryPath = Sources[ShaderType].GeometrySourceFilename;
	bool UseGeometry = (GeometryPath != 0);

	shader Result = {};
	Result.Type = (shader_type)ShaderType;

	char* VertexCode = ReadFileContent(VertexPath);
	char* FragmentCode = ReadFileContent(FragmentPath);

	char* GeometryCode = 0;
	if(UseGeometry)
	{
		GeometryCode = ReadFileContent(GeometryPath);
	}

	GLint CompileSuccess;
	GLchar InfoLog[512];

	GLuint Vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(Vertex, 1, &VertexCode, 0);
	glCompileShader(Vertex);
	glGetShaderiv(Vertex, GL_COMPILE_STATUS, &CompileSuccess);
	if (!CompileSuccess)
	{
		char* FilePath = Sources[ShaderType].VertexSourceFilename;
		glGetShaderInfoLog(Vertex, 512, 0, InfoLog);
		SDL_Log("FILE(%s)\n%s\n", FilePath, InfoLog);
		InvalidCodePath;
	}
	Free(VertexCode);

	GLuint Geometry = 0;
	if(UseGeometry)
	{
		Geometry = glCreateShader(GL_GEOMETRY_SHADER);;
		glShaderSource(Geometry, 1, &GeometryCode, 0);
		glCompileShader(Geometry);
		glGetShaderiv(Geometry, GL_COMPILE_STATUS, &CompileSuccess);
		if (!CompileSuccess)
		{
			char* FilePath = Sources[ShaderType].GeometrySourceFilename;
			glGetShaderInfoLog(Geometry, 512, 0, InfoLog);
			SDL_Log("FILE(%s)\n%s\n", FilePath, InfoLog);
			InvalidCodePath;
		}
		Free(GeometryCode);
	}

	GLuint Fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(Fragment, 1, &FragmentCode, 0);
	glCompileShader(Fragment);
	glGetShaderiv(Fragment, GL_COMPILE_STATUS, &CompileSuccess);
	if (!CompileSuccess)
	{
		char* FilePath = Sources[ShaderType].FragmentSourceFilename;
		glGetShaderInfoLog(Fragment, 512, 0, InfoLog);
		SDL_Log("FILE(%s)\n%s\n", FilePath, InfoLog);
		InvalidCodePath;
	}
	Free(FragmentCode);

	Result.Program = glCreateProgram();
	glAttachShader(Result.Program, Vertex);

	if(UseGeometry)
	{
		glAttachShader(Result.Program, Geometry);
	}

	glAttachShader(Result.Program, Fragment);

	glLinkProgram(Result.Program);
	glGetProgramiv(Result.Program, GL_LINK_STATUS, &CompileSuccess);
	if (!CompileSuccess)
	{
		glGetProgramInfoLog(Result.Program, 512, 0, InfoLog);
		SDL_Log("%s\n", InfoLog);
		InvalidCodePath;
	}

	glDeleteShader(Vertex);
	if(UseGeometry)
	{
		glDeleteShader(Geometry);
	}
	glDeleteShader(Fragment);

	// NOTE(hugo) : Setting uniform location
	for(u32 UniformIndex = 0; UniformIndex < ArrayCount(Uniforms[ShaderType]); ++UniformIndex)
	{
		char* UniformName = Uniforms[ShaderType][UniformIndex];
		if(UniformName && (!StringEmpty(UniformName)))
		{
			Result.Locations[UniformIndex] = glGetUniformLocation(Result.Program, UniformName);
			Assert(Result.Locations[UniformIndex] != -1);
		}
	}

	return(Result);
}
