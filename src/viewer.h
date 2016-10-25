#pragma once

/*
 * TODO(hugo)
 *   - different type of camera (FPS camera)
 *   - texture support
 *   - shadow mapping
 *   - mesh code cleanup
 *   - GL code cleanup (separate layer)
 *   - SDL code cleanup (cleaner layer, e.g. use of DLL for the main "game", etc...)
 *   - display triangles on the mesh
 *   - ambient occlusion (SSAO ?)
 *   - proper OBJ loading
 *   - profiling
 *   - bloom
 *   - cubemaps
 *   - instancing
 *   - gamma correction
 *   - normal mapping
 *   - post-processing
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
	camera Camera;
	float Time;

	s32 MouseXInitial;
	s32 MouseYInitial;
	bool MouseDragging;

	u32 BlinnPhongShininess;
	float CookTorranceF0;
	float CookTorranceM;

	GLuint FBO;
	GLuint Texture;

	GLuint RBO;
	GLuint QuadVAO;
	GLuint QuadVBO;

	GLuint DepthMapFBO;
	GLuint DepthMapTexture;
};

#include "viewer.cpp"

