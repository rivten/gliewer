#pragma once

/*
   TODO(hugo)
     * Basic GLiewer todos
		- get rid of ImGui / camera interference
		- clean Camera code
	    - different type of camera (FPS camera)
		- proper SDL-ImGui layer
	    - texture support
		- asset streaming
		- shader as assets (having a structure, uniforms, etc.. ==> glsl parsing ??)
	    - split the viewer.cpp code into several files
		- light attenuation
		- material handling when OBJ parsing
		- normal mapping
		- motion blur
	    - mesh code cleanup
	    - ambient occlusion (SSAO ?)
		- possibility to see light depth buffer from GUI
		- light handling has objects having an emissivity
		- check memory footprint
		- sorting objects by bounding box for rendering (to avoid too many
			fragment computations) ?
		- logging system
		- ray tracing ability
	    - SDL code cleanup (cleaner layer, e.g. use of DLL for the main "game", etc...)
		- clean OpenGL layer (no more gl_..., get rid of glew ??)
	    - proper OBJ loading
	    - profiling
		- Mouse Wheel clean up
	    - bloom
	    - automatic reloading of shaders (as in http://swarminglogic.com/jotting/2013_10_gamedev01)
		- automatic shader gestion I/O to chain easily shaders
		- BHV if ever necessary
		- BRDF visualisation
		- GBuffer pipeline
		- cascade shadow mapping
		- voxelisation
		- lighting shader code cleanup (use structure)
		- SIMD BRDF integration
		- subsurface scattering
		- mesh parametrization
		- mesh simplification

	* Some ideas
		- framebuffer have an OpenGL structure that I could exploit (with color attachment). For example, 
			I know that if I want to read into the normal map, I could read a mapping that says that the
			normal map is the nth color attachment of this framebuffer.

	* PBGI todos
		- do smarter stuff
		- understand the long call in the profiler
 */

struct camera
{
	v3 Pos;
	v3 Target;
	v3 Right;
	float FoV;
	float Aspect;
	float NearPlane;
	float FarPlane;
};

enum light_type
{
	LightType_Orthographic,
	LightType_Perspective,
	LightType_PointLight,
};

struct projection_parameters
{
	union
	{
		struct
		{
			float FoV;
			float Aspect;
		};
		struct
		{
			float Width;
			float Height;
		};
	};

	float NearPlane;
	float FarPlane;
};

struct light
{
	// NOTE(hugo) : If Object = NULL, then the light must not be rendered
	object* Object;
	v3 Pos;
	v4 Color;
	v3 Target;

	gl_depth_framebuffer DepthFramebuffer;
};

struct game_state
{
	object Objects[512];
	u32 ObjectCount;
	mat4 ObjectModelMatrix;

	light Lights[4];
	u32 LightCount;

	light_type LightType;
	projection_parameters ProjectionParams;

	shader BasicShader;
	shader DepthDebugQuadShader;
	shader ShadowMappingShader;
	shader SkyboxShader;
	shader BRDFConvShader;

	camera Camera;
	float Time;

	s32 MouseXInitial;
	s32 MouseYInitial;
	bool MouseDragging;

	// NOTE(hugo) : Render parameters
	// {
	u32 BlinnPhongShininess;
	float CookTorranceF0;
	float CookTorranceM;
	float Alpha;
	float Sigma;
	float LightIntensity;
	// }

	gl_geometry_framebuffer ScreenFramebuffer;
	gl_hemicube_framebuffer HemicubeFramebuffer;

	GLuint RBO;
	GLuint QuadVAO;
	GLuint QuadVBO;

	GLuint SkyboxVAO;
	GLuint SkyboxVBO;
	GLuint SkyboxTexture;

	u32 MicroFoVInDegrees;

	texture IndirectIlluminationTexture;
	u32* IndirectIlluminationBuffer;

	opengl_state* GLState;
	rect3 FrustumBoundingBox;
};

#include "viewer.cpp"

