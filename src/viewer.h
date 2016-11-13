#pragma once

/*
 * TODO(hugo)
 *   - different type of camera (FPS camera)
 *   - texture support
 *   - mesh code cleanup
 *   - SDL code cleanup (cleaner layer, e.g. use of DLL for the main "game", etc...)
 *   - ambient occlusion (SSAO ?)
 *   - proper OBJ loading
 *   - profiling
 *   - bloom
 *   - cubemaps
 *   - gamma correction
 *   - automatic reloading of shaders (as in http://swarminglogic.com/jotting/2013_10_gamedev01)
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
	// NOTE(hugo) : If Mesh = NULL, then the light must not be rendered
	mesh* Mesh;
	v3 Pos;
	v4 Color;
	v3 Target;

	gl_depth_framebuffer DepthFramebuffer;
};

struct game_state
{
	mesh Meshes[10];
	u32 MeshCount;
	mat4 ObjectModelMatrix;

	mesh CubeMesh;

	light Lights[4];
	u32 LightCount;

	light_type LightType;
	projection_parameters ProjectionParams;

	shader BasicShader;
	shader DepthDebugQuadShader;
	shader ShadowMappingShader;

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

	gl_screen_normal_framebuffer ScreenFramebuffer;

	GLuint RBO;
	GLuint QuadVAO;
	GLuint QuadVBO;
};

#include "viewer.cpp"

