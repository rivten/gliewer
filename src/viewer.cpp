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

		// NOTE(hugo) : This must be the last command of the initialization of memory
		Memory->IsInitialized = true;
	}

	glEnable(GL_DEPTH_TEST);

	State->Time += Input->dtForFrame;

	Clear(V4(0.4f, 0.6f, 0.2f, 1.0f));

#if 0
	v3 Vertices[] = 
	{
	    V3(-1.0f, -1.0f,  1.0f), V3(1.0f, 0.0f, 0.0f),
	    V3( 1.0f, -1.0f,  1.0f), V3(1.0f, 1.0f, 0.0f),
	    V3(-1.0f,  1.0f,  1.0f), V3(0.0f, 1.0f, 0.0f),
	    V3( 1.0f,  1.0f,  1.0f), V3(0.0f, 1.0f, 1.0f),
	    V3(-1.0f, -1.0f, -1.0f), V3(0.0f, 0.0f, 1.0f),
	    V3( 1.0f, -1.0f, -1.0f), V3(1.0f, 0.0f, 1.0f),
	    V3(-1.0f,  1.0f, -1.0f), V3(1.0f, 1.0f, 1.0f),
	    V3( 1.0f,  1.0f, -1.0f), V3(0.0f, 0.0f, 0.0f),
	};

	mat4 ModelMatrix = Translation(V3(0.5f, 0.0f, 0.0f)) * Rotation(GlobalTime, V3(0.0f, 1.0f, 0.0f)) * Scaling(V3(0.2f, 0.2f, 0.2f));
	mat4 ViewMatrix = LookAt(V3(0.0f, 1.0f, 0.0f), V3(0.2f, 0.0f, 0.0f), V3(0.0f, 0.0f, 1.0f));
	mat4 ProjectionMatrix = Perspective(Radians(60), GlobalWindowWidth / GlobalWindowHeight, 0.1f, 10.0f);

	for(u32 VertexIndex = 0; VertexIndex < ArrayCount(Vertices); VertexIndex++)
	{
		if(VertexIndex % 2 == 0)
		{
			Vertices[VertexIndex] = (ProjectionMatrix * ViewMatrix * ModelMatrix * ToV4(Vertices[VertexIndex])).xyz;
		}
	}

	GLuint Indices[] =
	{
		0, 1, 3,
		0, 3, 2,
		1, 5, 7,
		1, 7, 3,
		5, 4, 6,
		5, 6, 7,
		4, 0, 2,
		4, 2, 6,
		3, 6, 7,
		3, 2, 6,
		1, 4, 5,
		1, 0, 4,
	};

	GLuint TriangleVAO;
	glGenVertexArrays(1, &TriangleVAO);
	GLuint TriangleVBO;
	glGenBuffers(1, &TriangleVBO);
	GLuint TriangleEBO;
	glGenBuffers(1, &TriangleEBO);

	glBindVertexArray(TriangleVAO);

	glBindBuffer(GL_ARRAY_BUFFER, TriangleVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, TriangleEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
	shader BasicShader = shader("shaders/basic_v.glsl", "shaders/basic_f.glsl");

	UseShader(BasicShader);
	glBindVertexArray(TriangleVAO);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
#endif
	//State->Mesh.ModelMatrix = Rotation(State->Time, V3(0.0f, 1.0f, 0.0f)) * Scaling(V3(0.2f, 0.2f, 0.2f));
	State->Mesh.ModelMatrix = Scaling(V3(0.2f, 0.2f, 0.2f));
	v3 CameraPos = V3(0.0f, 0.0f, 5.0f);
	CameraPos = (Rotation(State->Time, V3(0.0f, 1.0f, 0.0f)) * ToV4(CameraPos)).xyz;
	v3 CameraTarget = V3(0.0f, 0.0f, 0.0f);
	mat4 ViewMatrix = LookAt(CameraPos, CameraTarget, V3(0.0f, 1.0f, 0.0f));
	mat4 ProjectionMatrix = Perspective(Radians(45), float(GlobalWindowWidth) / float(GlobalWindowHeight), 0.5f, 30.0f);

	mat4 MVPMatrix = ProjectionMatrix * ViewMatrix * State->Mesh.ModelMatrix;
	mat4 NormalMatrix = Transpose(Inverse(ViewMatrix * State->Mesh.ModelMatrix));

	UseShader(State->BasicShader);
	SetUniform(State->BasicShader, MVPMatrix, "MVPMatrix");
	SetUniform(State->BasicShader, NormalMatrix, "NormalMatrix");
	SetUniform(State->BasicShader, ViewMatrix, "ViewMatrix");

	DrawTrianglesMesh(&State->Mesh);

}
