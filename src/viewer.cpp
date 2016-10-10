#pragma once

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

void GameUpdateAndRender(thread_context* Thread, game_memory* Memory, game_input* Input, game_offscreen_buffer* Screenbuffer)
{
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	game_state* State = (game_state*)Memory->PermanentStorage;
	if(!Memory->IsInitialized)
	{
		State->Mesh = LoadOBJ("../models/teapot.obj");
		State->BasicShader = LoadShader("../src/shaders/basic_v.glsl", "../src/shaders/basic_f.glsl");
		State->Time = 0.0f;

		State->Camera = {};
		State->Camera.Pos = V3(0.0f, 0.0f, 5.0f);
		State->Camera.Target = V3(0.0f, 0.0f, 0.0f);
		State->Camera.FoV = Radians(45);
		State->Camera.Aspect = float(GlobalWindowWidth) / float(GlobalWindowHeight);
		State->Camera.NearPlane = 0.5f;
		State->Camera.FarPlane = 30.0f;

		// NOTE(hugo) : This must be the last command of the initialization of memory
		Memory->IsInitialized = true;
	}

	glEnable(GL_DEPTH_TEST);

	State->Time += Input->dtForFrame;

	Clear(V4(0.4f, 0.6f, 0.2f, 1.0f));

	State->Mesh.ModelMatrix = Scaling(V3(0.2f, 0.2f, 0.2f));
	v3 NewCameraPos = (Rotation(State->Time, V3(0.0f, 1.0f, 0.0f)) * ToV4(State->Camera.Pos)).xyz;
	mat4 ViewMatrix = LookAt(NewCameraPos, State->Camera.Target, V3(0.0f, 1.0f, 0.0f));
	mat4 ProjectionMatrix = Perspective(State->Camera.FoV, State->Camera.Aspect, State->Camera.NearPlane, State->Camera.FarPlane);

	mat4 MVPMatrix = ProjectionMatrix * ViewMatrix * State->Mesh.ModelMatrix;
	mat4 NormalMatrix = Transpose(Inverse(ViewMatrix * State->Mesh.ModelMatrix));

	UseShader(State->BasicShader);
	SetUniform(State->BasicShader, MVPMatrix, "MVPMatrix");
	SetUniform(State->BasicShader, NormalMatrix, "NormalMatrix");
	SetUniform(State->BasicShader, ViewMatrix, "ViewMatrix");

	DrawTrianglesMesh(&State->Mesh);

}
