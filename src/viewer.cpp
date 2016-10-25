#pragma once

#include <imgui_demo.cpp>

static GLfloat QuadVertices[] = { 
	// Positions   // TexCoords
	-1.0f,  1.0f,  0.0f, 1.0f,
	-1.0f, -1.0f,  0.0f, 0.0f,
	 1.0f, -1.0f,  1.0f, 0.0f,

	-1.0f,  1.0f,  0.0f, 1.0f,
	 1.0f, -1.0f,  1.0f, 0.0f,
	 1.0f,  1.0f,  1.0f, 1.0f
};	

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

void SetUniform(shader Shader, u32 UnsignedInteger, const char* VariableName)
{
	GLuint Location = glGetUniformLocation(Shader.Program, VariableName);
	glUniform1i(Location, UnsignedInteger); 
}

void SetUniform(shader Shader, float Value, const char* VariableName)
{
	GLuint Location = glGetUniformLocation(Shader.Program, VariableName);
	glUniform1f(Location, Value); 
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
		State->ObjectModelMatrix = Scaling(V3(0.2f, 0.2f, 0.2f));
		State->ObjectColor = V4(0.6f, 0.1f, 0.0f, 1.0f);

		State->CubeMesh = LoadOBJ("../models/cube.obj");

		State->BasicShader = LoadShader("../src/shaders/basic_v.glsl", "../src/shaders/basic_f.glsl");
		State->LightingShader = LoadShader("../src/shaders/lighting_v.glsl", "../src/shaders/lighting_f.glsl");
		State->DepthDebugQuadShader = LoadShader("../src/shaders/depth_debug_quad_v.glsl", "../src/shaders/depth_debug_quad_f.glsl");

		// TODO(hugo) : Maybe I should need to split a mesh and its Model Matrix 
		// (here several lights would need the same mesh (vertex infos) 
		// but could be placed at different positions (different model matrix))
		State->Light = {&State->CubeMesh, V3(3.0f, 0.0f, 3.0f), V4(1.0f, 1.0f, 1.0f, 1.0f), Scaling(V3(0.2f, 0.2f, 0.2f))};
		State->Light.ModelMatrix = Translation(State->Light.Pos) * State->Light.ModelMatrix;

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

		glGenFramebuffers(1, &State->FBO);

		glGenTextures(1, &State->Texture);

		glBindFramebuffer(GL_FRAMEBUFFER, State->FBO);

		glBindTexture(GL_TEXTURE_2D, State->Texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, GlobalWindowWidth, GlobalWindowHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, State->Texture, 0);

		glGenRenderbuffers(1, &State->RBO);
		glBindRenderbuffer(GL_RENDERBUFFER, State->RBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, GlobalWindowWidth, GlobalWindowHeight);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, State->RBO);

		Assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

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
		// NOTE(hugo) : This must be the last command of the initialization of memory
		Memory->IsInitialized = true;
	}
	State->Camera.Aspect = float(GlobalWindowWidth) / float(GlobalWindowHeight);

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

	glBindFramebuffer(GL_FRAMEBUFFER, State->FBO);
	Clear(V4(1.0f, 0.0f, 0.5f, 1.0f));
	glEnable(GL_DEPTH_TEST);
	// NOTE(hugo): RENDERING
	// NOTE(hugo) : Drawing Object Mesh
	mat4 MVPObjectMatrix = ProjectionMatrix * ViewMatrix * State->ObjectModelMatrix;
	mat4 NormalObjectMatrix = Transpose(Inverse(ViewMatrix * State->ObjectModelMatrix));

	UseShader(State->LightingShader);
	SetUniform(State->LightingShader, MVPObjectMatrix, "MVPMatrix");
	SetUniform(State->LightingShader, NormalObjectMatrix, "NormalMatrix");
	SetUniform(State->LightingShader, ViewMatrix, "ViewMatrix");
	SetUniform(State->LightingShader, State->ObjectModelMatrix, "ModelObjectMatrix");
	SetUniform(State->LightingShader, State->ObjectColor, "ObjectColor");

	SetUniform(State->LightingShader, State->Camera.Pos, "CameraPos");
	SetUniform(State->LightingShader, State->Light.Pos, "LightPos");
	SetUniform(State->LightingShader, State->Light.Color, "LightColor");

	SetUniform(State->LightingShader, State->BlinnPhongShininess, "BlinnPhongShininess");
	SetUniform(State->LightingShader, State->CookTorranceF0, "CTF0");
	SetUniform(State->LightingShader, State->CookTorranceM, "CTM");


	DrawTrianglesMesh(&State->ObjectMesh);
	
	// NOTE(hugo) : Drawing ground based on the cube mesh
#if 1
	mat4 GroundModelMatrix = Translation(V3(0.0f, -2.0f, 0.0f)) * Scaling(V3(10.0f, 0.01f, 10.0f));
	mat4 MVPGroundMatrix = ProjectionMatrix * ViewMatrix * GroundModelMatrix;
	mat4 NormalGroundMatrix = Transpose(Inverse(ViewMatrix * GroundModelMatrix));
	v4 GroundColor = V4(0.5f, 0.5f, 0.5f, 1.0f);
	SetUniform(State->LightingShader, MVPGroundMatrix, "MVPMatrix");
	SetUniform(State->LightingShader, NormalGroundMatrix, "NormalMatrix");
	SetUniform(State->LightingShader, GroundModelMatrix, "ModelObjectMatrix");
	SetUniform(State->LightingShader, GroundColor, "ObjectColor");
	DrawTrianglesMesh(&State->CubeMesh);
#endif

	// NOTE(hugo) : Drawing Light Mesh
	mat4 MVPLightMatrix = ProjectionMatrix * ViewMatrix * State->Light.ModelMatrix;
	UseShader(State->BasicShader);
	SetUniform(State->BasicShader, MVPLightMatrix, "MVPMatrix");
	SetUniform(State->BasicShader, State->Light.Color, "ObjectColor");
	DrawTrianglesMesh(State->Light.Mesh);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	UseShader(State->DepthDebugQuadShader);
	glBindVertexArray(State->QuadVAO);
	glDisable(GL_DEPTH_TEST);
	glBindTexture(GL_TEXTURE_2D, State->Texture);
	glDrawArrays(GL_LINES, 0, 6);
	glBindVertexArray(0);

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
