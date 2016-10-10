#pragma once

struct camera
{
	v3 Pos;
	v3 Target;
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
};

#include "viewer.cpp"

