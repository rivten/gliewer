#pragma once

#include <imgui_demo.cpp>

void Clear(v4 ClearColor)
{
	glClearColor(ClearColor.r, ClearColor.g, ClearColor.b, ClearColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void SetUniform(shader Shader, mat4 Matrix, const char* VariableName)
{
	GLuint Location = glGetUniformLocation(Shader.Program, VariableName);
	glUniformMatrix4fv(Location, 1, GL_FALSE, Matrix.Data_); 
}

void SetUniform(shader Shader, v2 V, const char* VariableName)
{
	GLuint Location = glGetUniformLocation(Shader.Program, VariableName);
	glUniform2f(Location, V.x, V.y); 
}

void SetUniform(shader Shader, v3 V, const char* VariableName)
{
	GLuint Location = glGetUniformLocation(Shader.Program, VariableName);
	glUniform3f(Location, V.x, V.y, V.z); 
}

void SetUniform(shader Shader, v4 V, const char* VariableName)
{
	GLuint Location = glGetUniformLocation(Shader.Program, VariableName);
	glUniform4f(Location, V.x, V.y, V.z, V.w); 
}

void GameUpdateAndRender(thread_context* Thread, game_memory* Memory, game_input* Input, game_offscreen_buffer* Screenbuffer)
{
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	game_state* State = (game_state*)Memory->PermanentStorage;
	if(!Memory->IsInitialized)
	{
		State->ObjectMesh = LoadOBJ("../models/teapot.obj");
		State->ObjectMesh.ModelMatrix = Scaling(V3(0.2f, 0.2f, 0.2f));

		State->CubeMesh = LoadOBJ("../models/cube.obj");
		State->CubeMesh.ModelMatrix = Scaling(V3(0.2f, 0.2f, 0.2f));

		State->BasicShader = LoadShader("../src/shaders/basic_v.glsl", "../src/shaders/basic_f.glsl");
		State->LightingShader = LoadShader("../src/shaders/lighting_v.glsl", "../src/shaders/lighting_f.glsl");

		// TODO(hugo) : Maybe I should need to split a mesh and its Model Matrix 
		// (here several lights would need the same mesh (vertex infos) 
		// but could be placed at different positions (different model matrix))
		State->Light = {&State->CubeMesh, V3(3.0f, 0.0f, 3.0f), V4(0.0f, 0.0f, 1.0f, 0.0f)};
		State->CubeMesh.ModelMatrix = Translation(State->Light.Pos) * State->CubeMesh.ModelMatrix;

		State->Time = 0.0f;

		State->Camera = {};
		State->Camera.Pos = V3(0.0f, 0.0f, 5.0f);
		State->Camera.Target = V3(0.0f, 0.0f, 0.0f);
		v3 LookingDir = State->Camera.Target - State->Camera.Pos;
		v3 WorldUp = V3(0.0f, 1.0f, 0.0f);
		State->Camera.Right = Normalized(Cross(LookingDir, WorldUp));
		State->Camera.FoV = Radians(45);
		State->Camera.Aspect = float(GlobalWindowWidth) / float(GlobalWindowHeight);
		State->Camera.NearPlane = 0.5f;
		State->Camera.FarPlane = 30.0f;

		State->MouseXInitial = 0;
		State->MouseYInitial = 0;
		State->MouseDragging = false;

		// NOTE(hugo) : This must be the last command of the initialization of memory
		Memory->IsInitialized = true;
	}

	State->Time += Input->dtForFrame;

	Clear(V4(0.4f, 0.6f, 0.2f, 1.0f));

	// TODO(hugo) : Smooth MouseWheel camera movement
	float DeltaMovement = 0.5f;
	v3 LookingDir = Normalized(State->Camera.Target - State->Camera.Pos);
	State->Camera.Pos += Input->MouseZ * DeltaMovement * LookingDir;

	if(Input->MouseButtons[0].EndedDown)
	{
		if(!State->MouseDragging)
		{
			State->MouseDragging = true;
			State->MouseXInitial = Input->MouseX;
			State->MouseYInitial = Input->MouseY;
		}
	}

	camera NextCamera = State->Camera;
	v3 NextCameraUp = Cross(State->Camera.Right, Normalized(State->Camera.Target - State->Camera.Pos));
	if(State->MouseDragging)
	{
		s32 DeltaX = Input->MouseX - State->MouseXInitial;
		s32 DeltaY = Input->MouseY - State->MouseYInitial;
		v3 WorldUp = V3(0.0f, 1.0f, 0.0f);
		NextCamera.Pos = (Rotation(-Sign(Dot(WorldUp, NextCameraUp)) * Radians(DeltaX), WorldUp) * Rotation(-Radians(DeltaY), NextCamera.Right) * ToV4(State->Camera.Pos)).xyz;
		NextCamera.Right = (Rotation(-Sign(Dot(WorldUp, NextCameraUp)) * Radians(DeltaX), V3(0.0f, 1.0f, 0.0f)) * Rotation(-Radians(DeltaY), NextCamera.Right) * ToV4(State->Camera.Right)).xyz;
		v3 LookingDir = Normalized(State->Camera.Target - NextCamera.Pos);
		NextCameraUp = Cross(NextCamera.Right, LookingDir);
	}

	if(State->MouseDragging && !(Input->MouseButtons[0].EndedDown))
	{
		State->Camera.Pos = NextCamera.Pos;
		State->Camera.Right = NextCamera.Right;
		State->MouseDragging = false;
	}

	mat4 ViewMatrix = LookAt(NextCamera.Pos, NextCamera.Target, NextCameraUp);
	mat4 ProjectionMatrix = Perspective(State->Camera.FoV, State->Camera.Aspect, State->Camera.NearPlane, State->Camera.FarPlane);

	// NOTE(hugo) : Drawing Object Mesh
	mat4 MVPObjectMatrix = ProjectionMatrix * ViewMatrix * State->ObjectMesh.ModelMatrix;
	mat4 NormalObjectMatrix = Transpose(Inverse(ViewMatrix * State->ObjectMesh.ModelMatrix));

	UseShader(State->LightingShader);
	SetUniform(State->LightingShader, MVPObjectMatrix, "MVPMatrix");
	SetUniform(State->LightingShader, NormalObjectMatrix, "NormalMatrix");
	SetUniform(State->LightingShader, ViewMatrix, "ViewMatrix");
	SetUniform(State->LightingShader, State->Light.Pos, "LightPos");
	SetUniform(State->LightingShader, State->Light.Color, "LightColor");

	DrawTrianglesMesh(&State->ObjectMesh);
	
	// NOTE(hugo) : Drawing Light Mesh
	mat4 MVPLightMatrix = ProjectionMatrix * ViewMatrix * State->Light.Mesh->ModelMatrix;
	mat4 NormalLightMatrix = Transpose(Inverse(ViewMatrix * State->Light.Mesh->ModelMatrix));
	UseShader(State->BasicShader);
	SetUniform(State->BasicShader, MVPLightMatrix, "MVPMatrix");
	DrawTrianglesMesh(State->Light.Mesh);
	

	if(ImGui::BeginMainMenuBar())
	{
		if(ImGui::BeginMenu("Menu"))
		{
			ImGui::MenuItem("(dummy menu)", NULL, false, true);
			ImGui::EndMenu();
		}
		if(ImGui::BeginMenu("About"))
		{
            ImGui::MenuItem("Main menu bar", NULL, false, true);
			ImGui::EndMenu();
		}
        ImGui::EndMainMenuBar();
	}
}
