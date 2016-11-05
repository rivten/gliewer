#pragma once

#include <imgui_demo.cpp>

static int GlobalShadowWidth = 2 * 1024;
static int GlobalShadowHeight = 2 * 1024;
static int GlobalTeapotInstanceCount = 10;

mat4 GetLightModelMatrix(light Light)
{
		mat4 ModelMatrix = Translation(Light.Pos) * Scaling(V3(0.2f, 0.2f, 0.2f));

		return(ModelMatrix);
}

void RenderShadowedScene(game_state* State, v3 CameraPos, v3 CameraTarget, v3 CameraUp, mat4 ProjectionMatrix, mat4 LightSpaceMatrix)
{
	mat4 ViewMatrix = LookAt(CameraPos, CameraTarget, CameraUp);

	// NOTE(hugo) : Drawing Object Mesh
	mat4 MVPObjectMatrix = ProjectionMatrix * ViewMatrix * State->ObjectModelMatrix;
	mat4 NormalObjectMatrix = Transpose(Inverse(ViewMatrix * State->ObjectModelMatrix));

	UseShader(State->ShadowMappingShader);

	glBindTexture(GL_TEXTURE_2D, State->DepthFramebuffer.Texture);

	SetUniform(State->ShadowMappingShader, MVPObjectMatrix, "MVPMatrix");
	SetUniform(State->ShadowMappingShader, NormalObjectMatrix, "NormalMatrix");
	SetUniform(State->ShadowMappingShader, ViewMatrix, "ViewMatrix");
	SetUniform(State->ShadowMappingShader, State->ObjectModelMatrix, "ModelObjectMatrix");
	SetUniform(State->ShadowMappingShader, State->ObjectColor, "ObjectColor");
	SetUniform(State->ShadowMappingShader, LightSpaceMatrix, "LightSpaceMatrix");

	for(u32 LightIndex = 0; LightIndex < State->LightCount; ++LightIndex)
	{
		char Buffer[80];
		char StringNum[2];
		sprintf(StringNum, "%i", LightIndex);
		strcpy(Buffer, "LightPos[");
		strcat(Buffer, StringNum);
		strcat(Buffer, "]");

		SetUniform(State->ShadowMappingShader, State->Lights[LightIndex].Pos, Buffer);

		memset(Buffer, 0, ArrayCount(Buffer));
		strcpy(Buffer, "LightColor[");
		strcat(Buffer, StringNum);
		strcat(Buffer, "]");

		SetUniform(State->ShadowMappingShader, State->Lights[LightIndex].Color, Buffer);
	}

	SetUniform(State->ShadowMappingShader, State->CookTorranceF0, "CTF0");
	SetUniform(State->ShadowMappingShader, State->CookTorranceM, "CTM");


	//DrawTriangleMeshInstances(&State->ObjectMesh, GlobalTeapotInstanceCount);
	DrawTriangleMesh(&State->ObjectMesh);


	// NOTE(hugo) : Drawing ground based on the cube mesh
	mat4 GroundModelMatrix = Translation(V3(0.0f, -1.0f, 0.0f)) * Scaling(V3(10.0f, 0.01f, 10.0f));
	mat4 MVPGroundMatrix = ProjectionMatrix * ViewMatrix * GroundModelMatrix;
	mat4 NormalGroundMatrix = Transpose(Inverse(ViewMatrix * GroundModelMatrix));
	v4 GroundColor = V4(0.5f, 0.5f, 0.5f, 1.0f);
	SetUniform(State->ShadowMappingShader, MVPGroundMatrix, "MVPMatrix");
	SetUniform(State->ShadowMappingShader, NormalGroundMatrix, "NormalMatrix");
	SetUniform(State->ShadowMappingShader, GroundModelMatrix, "ModelObjectMatrix");
	SetUniform(State->ShadowMappingShader, GroundColor, "ObjectColor");
	DrawTriangleMesh(&State->CubeMesh);

	// NOTE(hugo) : Drawing Light Mesh
	mat4 MVPLightMatrix = ProjectionMatrix * ViewMatrix * GetLightModelMatrix(State->Lights[0]);
	UseShader(State->BasicShader);
	SetUniform(State->BasicShader, MVPLightMatrix, "MVPMatrix");
	SetUniform(State->BasicShader, State->Lights[0].Color, "ObjectColor");
	DrawTriangleMesh(State->Lights[0].Mesh);

#if 0
	SetUniform(State->BasicShader, MVPObjectMatrix, "MVPMatrix");
	SetUniform(State->BasicShader, V4(0.0f, 1.0f, 1.0f, 1.0f), "ObjectColor");
	DrawWiredTriangleMeshInstances(&State->ObjectMesh, GlobalTeapotInstanceCount);
#endif

}

void RenderSimpleScene(game_state* State, v3 CameraPos, v3 CameraTarget, v3 CameraUp, mat4 ProjectionMatrix)
{
	mat4 ViewMatrix = LookAt(CameraPos, CameraTarget, CameraUp);
	mat4 MVPObjectMatrix = ProjectionMatrix * ViewMatrix * State->ObjectModelMatrix;

	UseShader(State->BasicShader);
	SetUniform(State->BasicShader, MVPObjectMatrix, "MVPMatrix");
	//DrawTriangleMeshInstances(&State->ObjectMesh, GlobalTeapotInstanceCount);
	DrawTriangleMesh(&State->ObjectMesh);

	mat4 GroundModelMatrix = Translation(V3(0.0f, -1.0f, 0.0f)) * Scaling(V3(10.0f, 0.01f, 10.0f));
	mat4 MVPGroundMatrix = ProjectionMatrix * ViewMatrix * GroundModelMatrix;
	SetUniform(State->BasicShader, MVPGroundMatrix, "MVPMatrix");
	DrawTriangleMesh(&State->CubeMesh);
}

void RenderLightedScene(game_state* State, v3 CameraPos, v3 CameraTarget, v3 CameraUp, mat4 ProjectionMatrix)
{
	mat4 ViewMatrix = LookAt(CameraPos, CameraTarget, CameraUp);

	// NOTE(hugo) : Drawing Object Mesh
	mat4 MVPObjectMatrix = ProjectionMatrix * ViewMatrix * State->ObjectModelMatrix;
	mat4 NormalObjectMatrix = Transpose(Inverse(ViewMatrix * State->ObjectModelMatrix));

	UseShader(State->LightingShader);
	SetUniform(State->LightingShader, MVPObjectMatrix, "MVPMatrix");
	SetUniform(State->LightingShader, NormalObjectMatrix, "NormalMatrix");
	SetUniform(State->LightingShader, ViewMatrix, "ViewMatrix");
	SetUniform(State->LightingShader, State->ObjectModelMatrix, "ModelObjectMatrix");
	SetUniform(State->LightingShader, State->ObjectColor, "ObjectColor");

	SetUniform(State->LightingShader, State->Lights[0].Pos, "LightPos");
	SetUniform(State->LightingShader, State->Lights[0].Color, "LightColor");

	SetUniform(State->LightingShader, State->CookTorranceF0, "CTF0");
	SetUniform(State->LightingShader, State->CookTorranceM, "CTM");


	//DrawTriangleMeshInstances(&State->ObjectMesh, GlobalTeapotInstanceCount);
	DrawTriangleMesh(&State->ObjectMesh);
	
	// NOTE(hugo) : Drawing ground based on the cube mesh
	mat4 GroundModelMatrix = Translation(V3(0.0f, -1.0f, 0.0f)) * Scaling(V3(10.0f, 0.01f, 10.0f));
	mat4 MVPGroundMatrix = ProjectionMatrix * ViewMatrix * GroundModelMatrix;
	mat4 NormalGroundMatrix = Transpose(Inverse(ViewMatrix * GroundModelMatrix));
	v4 GroundColor = V4(0.5f, 0.5f, 0.5f, 1.0f);
	SetUniform(State->LightingShader, MVPGroundMatrix, "MVPMatrix");
	SetUniform(State->LightingShader, NormalGroundMatrix, "NormalMatrix");
	SetUniform(State->LightingShader, GroundModelMatrix, "ModelObjectMatrix");
	SetUniform(State->LightingShader, GroundColor, "ObjectColor");
	DrawTriangleMesh(&State->CubeMesh);

	// NOTE(hugo) : Drawing Light Mesh
	mat4 MVPLightMatrix = ProjectionMatrix * ViewMatrix * GetLightModelMatrix(State->Lights[0]);
	UseShader(State->BasicShader);
	SetUniform(State->BasicShader, MVPLightMatrix, "MVPMatrix");
	SetUniform(State->BasicShader, State->Lights[0].Color, "ObjectColor");
	DrawTriangleMesh(State->Lights[0].Mesh);

}

void PushLight(game_state* State, light Light)
{
	Assert(State->LightCount < ArrayCount(State->Lights));

	State->Lights[State->LightCount] = Light;
	State->LightCount++;
}

void GameUpdateAndRender(thread_context* Thread, game_memory* Memory, game_input* Input, game_offscreen_buffer* Screenbuffer)
{
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	game_state* State = (game_state*)Memory->PermanentStorage;
	if(!Memory->IsInitialized)
	{
		State->ObjectMesh = LoadOBJ("../models/teapot.obj");
		State->ObjectModelMatrix = Scaling(V3(0.2f, 0.2f, 0.2f));
		State->ObjectColor = V4(0.6f, 0.1f, 0.0f, 1.0f);

		State->CubeMesh = LoadOBJ("../models/cube.obj");

		State->BasicShader = LoadShader("../src/shaders/basic_v.glsl", "../src/shaders/basic_f.glsl");
		State->LightingShader = LoadShader("../src/shaders/lighting_v.glsl", "../src/shaders/lighting_f.glsl");
		State->DepthDebugQuadShader = LoadShader("../src/shaders/depth_debug_quad_v.glsl", "../src/shaders/depth_debug_quad_f.glsl");
		State->ShadowMappingShader = LoadShader("../src/shaders/shadow_mapping_v.glsl", "../src/shaders/shadow_mapping_f.glsl");

		light Light = {&State->CubeMesh, V3(3.0f, 0.0f, 3.0f), V4(1.0f, 1.0f, 1.0f, 1.0f), V3(0.0f, 0.0f, 0.0f)};
		PushLight(State, Light);

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

		State->BlinnPhongShininess = 32;
		State->CookTorranceF0 = 0.5f;
		State->CookTorranceM = 0.5f;

		State->ScreenFramebuffer = CreateScreenFramebuffer(GlobalWindowWidth, GlobalWindowHeight);

		// NOTE(hugo) : Initializing Quad data 
		// {
		glGenVertexArrays(1, &State->QuadVAO);
		glGenBuffers(1, &State->QuadVBO);
		glBindVertexArray(State->QuadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, State->QuadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(QuadVertices), &QuadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)(2 * sizeof(GLfloat)));
		glBindVertexArray(0);
		// }

		State->DepthFramebuffer = CreateDepthFramebuffer(GlobalShadowWidth, GlobalShadowHeight);

		// NOTE(hugo) : This must be the last command of the initialization of memory
		Memory->IsInitialized = true;
	}
	// TODO(hugo) : I should not enable the depth test each frame. This is a hack that fixes some issues on Linux (Ubuntu 16.04).
	// For more information, go to http://stackoverflow.com/questions/24990637/opengl-radeon-driver-seems-to-mess-with-depth-testing
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);

	State->Camera.Aspect = float(GlobalWindowWidth) / float(GlobalWindowHeight);

	State->Time += Input->dtForFrame;

	ClearColorAndDepth(V4(0.4f, 0.6f, 0.2f, 1.0f));

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

	State->Lights[0].Pos.y = Sin(State->Time) + 2.0f;

#if 0
	// NOTE(hugo) : Rendering on quads
	// {
	mat4 ProjectionMatrix = Perspective(State->Camera.FoV, State->Camera.Aspect, State->Camera.NearPlane, State->Camera.FarPlane);
	glBindFramebuffer(GL_FRAMEBUFFER, State->ScreenFramebuffer.FBO);
	ClearColorAndDepth(V4(1.0f, 0.0f, 0.5f, 1.0f));
	glEnable(GL_DEPTH_TEST);
	RenderLightedScene(State, NextCamera.Pos, NextCamera.Target, NextCameraUp, ProjectionMatrix);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	UseShader(State->DepthDebugQuadShader);
	glBindVertexArray(State->QuadVAO);
	glDisable(GL_DEPTH_TEST);
	glBindTexture(GL_TEXTURE_2D, State->ScreenFramebuffer.Texture);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
	// }
#else
	// NOTE(hugo) : Shadow mapping rendering
	// {
	// TODO(hugo) : Get rid of OpenGL in here
	mat4 LightProjectionMatrix = Orthographic(10.0f, 10.0f, 1.0f, 20.0f);
	SetViewport(GlobalShadowWidth, GlobalShadowHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, State->DepthFramebuffer.FBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	glCullFace(GL_FRONT);
	RenderSimpleScene(State, State->Lights[0].Pos, State->Lights[0].Target, V3(0.0f, 1.0f, 0.0f), LightProjectionMatrix);
	glCullFace(GL_BACK);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	SetViewport(GlobalWindowWidth, GlobalWindowHeight);
	ClearColorAndDepth(V4(1.0f, 0.0f, 0.5f, 1.0f));

	mat4 LightLookAt = LookAt(State->Lights[0].Pos, State->Lights[0].Target, V3(0.0f, 1.0f, 0.0f));
	mat4 LightSpaceMatrix = LightProjectionMatrix * LightLookAt;
	mat4 ProjectionMatrix = Perspective(State->Camera.FoV, State->Camera.Aspect, State->Camera.NearPlane, State->Camera.FarPlane);
	RenderShadowedScene(State, NextCamera.Pos, NextCamera.Target, NextCameraUp, ProjectionMatrix, LightSpaceMatrix);

#if 0
	UseShader(State->DepthDebugQuadShader);
	glBindVertexArray(State->QuadVAO);
	glBindTexture(GL_TEXTURE_2D, State->DepthFramebuffer.Texture);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
#endif
	// }

#endif

	//ImGui::SliderInt("Blinn-Phong Shininess", (int*)&State->BlinnPhongShininess, 1, 256);
	ImGui::SliderFloat("Cook-Torrance F0", (float*)&State->CookTorranceF0, 0.0f, 1.0f);
	ImGui::SliderFloat("Cook-Torrance M", (float*)&State->CookTorranceM, 0.0f, 1.0f);

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
