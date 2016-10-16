#pragma once

/*
 * TODO(hugo)
 *   - shadow mapping
 *   - proper OBJ loading
 *   - mesh code cleanup
 *   - profiling
 *   - bloom
 *   - ambient occlusion (SSAO ?)
 *   - culling
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

struct game_state
{
	mesh Mesh;
	shader BasicShader;
	camera Camera;
	float Time;

	s32 MouseXInitial;
	s32 MouseYInitial;
	bool MouseDragging;
};

#include "viewer.cpp"

