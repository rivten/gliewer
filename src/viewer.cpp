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

void RenderShadowedScene(game_state* State, v3 CameraPos, v3 CameraTarget, v3 CameraUp, mat4 ProjectionMatrix, mat4 LightProjectionMatrix)
{
	mat4 LightSpaceMatrix[4];

	mat4 ViewMatrix = LookAt(CameraPos, CameraTarget, CameraUp);

	// NOTE(hugo) : Drawing Object Mesh
	mat4 MVPObjectMatrix = ProjectionMatrix * ViewMatrix * State->ObjectModelMatrix;
	mat4 NormalObjectMatrix = Transpose(Inverse(ViewMatrix * State->ObjectModelMatrix));
	mat4 NormalWorldMatrix = Transpose(Inverse(State->ObjectModelMatrix));

	UseShader(State->ShadowMappingShader);


	SetUniform(State->ShadowMappingShader, MVPObjectMatrix, "MVPMatrix");
	SetUniform(State->ShadowMappingShader, NormalObjectMatrix, "NormalMatrix");
	SetUniform(State->ShadowMappingShader, ViewMatrix, "ViewMatrix");
	SetUniform(State->ShadowMappingShader, State->ObjectModelMatrix, "ModelObjectMatrix");
	SetUniform(State->ShadowMappingShader, State->LightCount, "LightCount");
	SetUniform(State->ShadowMappingShader, State->Alpha, "Alpha");
	SetUniform(State->ShadowMappingShader, State->LightIntensity, "LightIntensity");
	SetUniform(State->ShadowMappingShader, NormalWorldMatrix, "NormalWorldMatrix");


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

		mat4 LightLookAt = LookAt(State->Lights[LightIndex].Pos, State->Lights[LightIndex].Target, V3(0.0f, 1.0f, 0.0f));
		LightSpaceMatrix[LightIndex] = LightProjectionMatrix * LightLookAt;
		memset(Buffer, 0, ArrayCount(Buffer));
		strcpy(Buffer, "LightSpaceMatrix[");
		strcat(Buffer, StringNum);
		strcat(Buffer, "]");
		SetUniform(State->ShadowMappingShader, LightSpaceMatrix[LightIndex], Buffer);

		memset(Buffer, 0, ArrayCount(Buffer));
		strcpy(Buffer, "ShadowMap[");
		strcat(Buffer, StringNum);
		strcat(Buffer, "]");
		glActiveTexture(GL_TEXTURE0 + LightIndex);
		SetUniform(State->ShadowMappingShader, LightIndex, Buffer);
		glBindTexture(GL_TEXTURE_2D, State->Lights[LightIndex].DepthFramebuffer.Texture);
	}

	SetUniform(State->ShadowMappingShader, State->CookTorranceF0, "CTF0");
	SetUniform(State->ShadowMappingShader, State->CookTorranceM, "CTM");


	for(u32 MeshIndex = 0; MeshIndex < State->MeshCount; ++MeshIndex)
	{
		SetUniform(State->ShadowMappingShader, State->Meshes[MeshIndex].Color, "ObjectColor");
		DrawTriangleMesh(&State->Meshes[MeshIndex]);
	}
	glActiveTexture(GL_TEXTURE0);

	// NOTE(hugo) : Drawing Light Mesh
	for(u32 LightIndex = 0; LightIndex < State->LightCount; ++LightIndex)
	{
		if(State->Lights[LightIndex].Mesh)
		{
			mat4 MVPLightMatrix = ProjectionMatrix * ViewMatrix * GetLightModelMatrix(State->Lights[LightIndex]);
			UseShader(State->BasicShader);
			SetUniform(State->BasicShader, MVPLightMatrix, "MVPMatrix");
			SetUniform(State->BasicShader, State->Lights[LightIndex].Color, "ObjectColor");
			DrawTriangleMesh(State->Lights[LightIndex].Mesh);
		}
	}
}

void RenderSimpleScene(game_state* State, v3 CameraPos, v3 CameraTarget, v3 CameraUp, mat4 ProjectionMatrix)
{
	mat4 ViewMatrix = LookAt(CameraPos, CameraTarget, CameraUp);
	mat4 MVPObjectMatrix = ProjectionMatrix * ViewMatrix * State->ObjectModelMatrix;

	UseShader(State->BasicShader);
	SetUniform(State->BasicShader, MVPObjectMatrix, "MVPMatrix");
	//DrawTriangleMeshInstances(&State->ObjectMesh, GlobalTeapotInstanceCount);
	for(u32 MeshIndex = 0; MeshIndex < State->MeshCount; ++MeshIndex)
	{
		DrawTriangleMesh(&State->Meshes[MeshIndex]);
	}

}

void PushMesh(game_state* State, mesh* Mesh)
{
	Assert(State->MeshCount < ArrayCount(State->Meshes));

	State->Meshes[State->MeshCount] = *Mesh;
	State->MeshCount++;
}

void PushLight(game_state* State, light Light)
{
	Assert(State->LightCount < ArrayCount(State->Lights));

	State->Lights[State->LightCount] = Light;
	State->LightCount++;
}

void GameUpdateAndRender(thread_context* Thread, game_memory* Memory, game_input* Input)
{
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	game_state* State = (game_state*)Memory->PermanentStorage;
	if(!Memory->IsInitialized)
	{
		{
			std::vector<mesh> Meshes = LoadOBJ("../models/cornell_box/", "CornellBox-Original.obj");
			for(u32 MeshIndex = 0; MeshIndex < Meshes.size(); ++MeshIndex)
			{
				PushMesh(State, &Meshes[MeshIndex]);
			}
		}
		State->ObjectModelMatrix = Identity4();
		State->BasicShader = LoadShader("../src/shaders/basic_v.glsl", "../src/shaders/basic_f.glsl");
		State->LightingShader = LoadShader("../src/shaders/lighting_v.glsl", "../src/shaders/lighting_f.glsl");
		State->DepthDebugQuadShader = LoadShader("../src/shaders/depth_debug_quad_v.glsl", "../src/shaders/depth_debug_quad_f.glsl");
		State->ShadowMappingShader = LoadShader("../src/shaders/shadow_mapping_v.glsl", "../src/shaders/shadow_mapping_f.glsl");

		light Light = {&State->CubeMesh, V3(0.0f, 1.0f, 3.0f), V4(1.0f, 1.0f, 1.0f, 1.0f), V3(0.0f, 1.0f, 0.0f)};
		Light.DepthFramebuffer = CreateDepthFramebuffer(GlobalShadowWidth, GlobalShadowHeight);
		PushLight(State, Light);
		
		State->LightType = LightType_Perspective;

		switch(State->LightType)
		{
			case LightType_Orthographic:
				{
					State->ProjectionParams = {5.0f, 5.0f, 1.0f, 5.5f};
				} break;
			case LightType_Perspective:
				{
					State->ProjectionParams = {Radians(45), float(GlobalWindowWidth) / float(GlobalWindowHeight), 1.0f, 5.5f};
				} break;
			case LightType_PointLight:
				{
					InvalidCodePath;
				} break;
				InvalidDefaultCase;
		}

		State->Time = 0.0f;

		State->Camera = {};
		State->Camera.Pos = V3(0.0f, 0.0f, 5.0f);
		State->Camera.Target = V3(0.0f, 1.0f, 0.0f);
		v3 LookingDir = State->Camera.Target - State->Camera.Pos;
		v3 WorldUp = V3(0.0f, 1.0f, 0.0f);
		State->Camera.Right = Normalized(Cross(LookingDir, WorldUp));
		State->Camera.FoV = Radians(45);
		State->Camera.Aspect = float(GlobalWindowWidth) / float(GlobalWindowHeight);
		State->Camera.NearPlane = 0.5f;
		State->Camera.FarPlane = 10.0f;

		State->MouseXInitial = 0;
		State->MouseYInitial = 0;
		State->MouseDragging = false;

		State->BlinnPhongShininess = 32;
		State->CookTorranceF0 = 0.5f;
		State->CookTorranceM = 0.5f;
		State->Alpha = 0.5f;
		State->Sigma = 0.0f;
		State->LightIntensity = 4.5f;

		// TODO(hugo) : If the window size changes, then this screenbuffer will have wrong dimensions.
		// Maybe I need to see each frame if the window dim changes. If so, update the screenbuffer.
		State->ScreenFramebuffer = CreateScreenNormalFramebuffer(GlobalWindowWidth, GlobalWindowHeight);

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

	//State->Lights[0].Pos.y = Sin(State->Time) + 2.0f;

	// NOTE(hugo) : Shadow mapping rendering
	// {
	// TODO(hugo) : Get rid of OpenGL in here
	mat4 LightProjectionMatrix;
	switch(State->LightType)
	{
		case LightType_Orthographic:
			{
				LightProjectionMatrix = Orthographic(State->ProjectionParams.Width, State->ProjectionParams.Height, State->ProjectionParams.NearPlane, State->ProjectionParams.FarPlane);
			} break;
		case LightType_Perspective:
			{
				LightProjectionMatrix = Perspective(State->ProjectionParams.FoV, State->ProjectionParams.Aspect, State->ProjectionParams.NearPlane, State->ProjectionParams.FarPlane);
			} break;
		case LightType_PointLight:
			{
				InvalidCodePath;
			} break;
			InvalidDefaultCase;
	};
	SetViewport(GlobalShadowWidth, GlobalShadowHeight);
	for(u32 LightIndex = 0; LightIndex < State->LightCount; ++LightIndex)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, State->Lights[LightIndex].DepthFramebuffer.FBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		glCullFace(GL_FRONT);
		RenderSimpleScene(State, State->Lights[LightIndex].Pos, State->Lights[LightIndex].Target, V3(0.0f, 1.0f, 0.0f), LightProjectionMatrix);
		glCullFace(GL_BACK);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	SetViewport(GlobalWindowWidth, GlobalWindowHeight);
	ClearColorAndDepth(V4(1.0f, 0.0f, 0.5f, 1.0f));

	mat4 ProjectionMatrix = Perspective(State->Camera.FoV, State->Camera.Aspect, State->Camera.NearPlane, State->Camera.FarPlane);

	glBindFramebuffer(GL_FRAMEBUFFER, State->ScreenFramebuffer.FBO);
	ClearColorAndDepth(V4(1.0f, 0.0f, 0.5f, 1.0f));
	glEnable(GL_DEPTH_TEST);

	RenderShadowedScene(State, NextCamera.Pos, NextCamera.Target, NextCameraUp, ProjectionMatrix, LightProjectionMatrix);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	UseShader(State->DepthDebugQuadShader);
	SetUniform(State->DepthDebugQuadShader, State->Sigma, "Sigma");
	glBindVertexArray(State->QuadVAO);
	glDisable(GL_DEPTH_TEST);
	glBindTexture(GL_TEXTURE_2D, State->ScreenFramebuffer.ScreenTexture);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

#if 0
	UseShader(State->DepthDebugQuadShader);
	glBindVertexArray(State->QuadVAO);
	glBindTexture(GL_TEXTURE_2D, State->Lights[0].DepthFramebuffer.Texture);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
#endif
	// }

	if(Input->MouseButtons[2].EndedDown)
	{
		s32 MouseX = Input->MouseX;
		s32 MouseY = GlobalWindowHeight - Input->MouseY;
		u8 Color[3];
		float PixelDepth;
		glBindFramebuffer(GL_FRAMEBUFFER, State->ScreenFramebuffer.FBO);
		// TODO(hugo) : How to read the three color components at once ?

		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glReadPixels(MouseX, MouseY, 1, 1, GL_RED, GL_UNSIGNED_BYTE, Color + 0);
		glReadPixels(MouseX, MouseY, 1, 1, GL_GREEN, GL_UNSIGNED_BYTE, Color + 1);
		glReadPixels(MouseX, MouseY, 1, 1, GL_BLUE, GL_UNSIGNED_BYTE, Color + 2);

		glReadPixels(MouseX, MouseY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &PixelDepth);

		glReadBuffer(GL_COLOR_ATTACHMENT1);
		v3 Normal = {};
		glReadPixels(MouseX, MouseY, 1, 1, GL_RED, GL_FLOAT, &Normal.x);
		glReadPixels(MouseX, MouseY, 1, 1, GL_GREEN, GL_FLOAT, &Normal.y);
		glReadPixels(MouseX, MouseY, 1, 1, GL_BLUE, GL_FLOAT, &Normal.z);
		Assert(glGetError() != GL_INVALID_OPERATION);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		float NearPlane = State->Camera.NearPlane;
		float FarPlane = State->Camera.FarPlane;
		PixelDepth = 2.0f * PixelDepth - 1.0f;
		PixelDepth = 2.0f * NearPlane * FarPlane / (NearPlane + FarPlane - PixelDepth * (FarPlane - NearPlane));
		float ColorFloat[3];
		for(u32 i = 0; i < ArrayCount(Color); ++i)
		{
			ColorFloat[i] = (float)(Color[i]) / 255.0f;
		}

		v4 PixelPos = {};
		PixelPos.z = -PixelDepth;
		PixelPos.w = 1.0f;

		PixelPos.x = (float)(MouseX) / float(GlobalWindowWidth);
		PixelPos.y = (float)(MouseY) / float(GlobalWindowHeight);

		PixelPos.xy = 2.0f * PixelPos.xy - V2(1.0f, 1.0f);

		PixelPos.x = - State->Camera.Aspect * Tan(0.5f * State->Camera.FoV) * PixelPos.z * PixelPos.x;
		PixelPos.y = - Tan(0.5f * State->Camera.FoV) * PixelPos.z * PixelPos.y;

		mat4 InvLookAtCamera = Inverse(LookAt(NextCamera.Pos, NextCamera.Target, NextCameraUp));

		PixelPos = InvLookAtCamera * PixelPos;

		ImGui::ColorEdit3("Color Picked", ColorFloat);
		ImGui::Value("Depth @ Pixel", PixelDepth);
		ImGui::Text("Position pointed at screen : (%f, %f, %f, %f)", PixelPos.x, PixelPos.y, PixelPos.z, PixelPos.w);
		Normal = 2.0f * Normal - V3(1.0f, 1.0f, 1.0f);
		ImGui::Text("Normal pointed at screen : (%f, %f, %f)", Normal.x, Normal.y, Normal.z);

#if 0
		SDL_Window* DEBUGDataWindow = SDL_CreateWindow("DisplayData", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 100, 100, SDL_WINDOW_SHOWN);
		Assert(DEBUGDataWindow);
		SDL_Surface* DEBUGScreen = SDL_GetWindowSurface(DEBUGDataWindow);
		Assert(DEBUGScreen);
		u8* Pixel = (u8*)DEBUGScreen->pixels;
		for(u32 Y = 0; Y < DEBUGScreen->h; ++Y)
		{
			for(u32 X = 0; X < DEBUGScreen->w; ++X)
			{
				*Pixel = 0; // Blue
				Pixel++;
				*Pixel = 0; // Green
				Pixel++;
				*Pixel = Color; // Red
				Pixel++;
				*Pixel = 0xFF; // Alpha
				Pixel++;
			}
		}
		SDL_UpdateWindowSurface(DEBUGDataWindow);
		SDL_Delay(1000);
		SDL_DestroyWindow(DEBUGDataWindow);
#endif
	}

	ImGui::SliderFloat("Light Intensity", (float*)&State->LightIntensity, 0.0f, 10.0f);
#if 0
	//ImGui::SliderInt("Blinn-Phong Shininess", (int*)&State->BlinnPhongShininess, 1, 256);
	ImGui::SliderFloat("Cook-Torrance F0", (float*)&State->CookTorranceF0, 0.0f, 1.0f);
	ImGui::SliderFloat("Cook-Torrance M", (float*)&State->CookTorranceM, 0.0f, 1.0f);
	ImGui::SliderFloat("Blur Sigma", (float*)&State->Sigma, 0.0f, 50.0f);
	ImGui::SliderFloat("Alpha", (float*)&State->Alpha, 0.0f, 1.0f);

	if(ImGui::CollapsingHeader("Light Data"))
	{
		ImGui::SliderFloat3("Light Position", State->Lights[0].Pos.E, -3.0f, 3.0f);
		switch(State->LightType)
		{
			case LightType_Orthographic:
				{
					ImGui::SliderFloat("Orthographic Width", &State->ProjectionParams.Width, 0.0f, 5.0f);
					ImGui::SliderFloat("Orthographic Height", &State->ProjectionParams.Height, 0.0f, 5.0f);
					ImGui::SliderFloat("Orthographic Near Plane", &State->ProjectionParams.NearPlane, 0.0f, 1.0f);
					ImGui::SliderFloat("Orthographic Far Plane", &State->ProjectionParams.FarPlane, 1.0f, 8.0f);
				} break;
			case LightType_Perspective:
				{
					ImGui::SliderFloat("Perspective FoV", &State->ProjectionParams.FoV, 0.01f, 3.14f);
					ImGui::SliderFloat("Perspective Aspect", &State->ProjectionParams.Aspect, 0.01f, 5.0f);
					ImGui::SliderFloat("Perspective Near Plane", &State->ProjectionParams.NearPlane, 0.0f, 1.0f);
					ImGui::SliderFloat("Perspective Far Plane", &State->ProjectionParams.FarPlane, 1.0f, 8.0f);
				} break;
			case LightType_PointLight:
				{
					InvalidCodePath;
				} break;
				InvalidDefaultCase;
		}
	}

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
#endif
}
