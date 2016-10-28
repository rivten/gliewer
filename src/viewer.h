#pragma once

/*
 * TODO(hugo)
 *   - different type of camera (FPS camera)
 *   - texture support
 *   - shadow mapping code cleanup
 *   - mesh code cleanup
 *   - GL code cleanup (separate layer)
 *   - SDL code cleanup (cleaner layer, e.g. use of DLL for the main "game", etc...)
 *   - ambient occlusion (SSAO ?)
 *   - proper OBJ loading
 *   - profiling
 *   - bloom
 *   - cubemaps
 *   - gamma correction
 *   - normal mapping
 *   - deferred shading
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

struct light
{
	mesh* Mesh;
	v3 Pos;
	v4 Color;
	mat4 ModelMatrix;
};

struct gl_depth_framebuffer
{
	u32 FBO;
	u32 Texture;
};

struct gl_screen_framebuffer
{
	u32 FBO;
	u32 Texture;
	u32 RBO;
};


struct game_state
{
	mesh ObjectMesh;
	mat4 ObjectModelMatrix;
	v4 ObjectColor;

	mesh CubeMesh;

	light Light;

	shader BasicShader;
	shader LightingShader;
	shader DepthDebugQuadShader;
	shader ShadowMappingShader;

	camera Camera;
	float Time;

	s32 MouseXInitial;
	s32 MouseYInitial;
	bool MouseDragging;

	u32 BlinnPhongShininess;
	float CookTorranceF0;
	float CookTorranceM;

	gl_screen_framebuffer ScreenFramebuffer;

	GLuint RBO;
	GLuint QuadVAO;
	GLuint QuadVBO;

	gl_depth_framebuffer DepthFramebuffer;
};

#include "viewer.cpp"

