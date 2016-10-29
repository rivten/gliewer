#pragma once

static GLfloat QuadVertices[] = { 
	// Positions   // TexCoords
	-1.0f,  1.0f,  0.0f, 1.0f,
	-1.0f, -1.0f,  0.0f, 0.0f,
	 1.0f, -1.0f,  1.0f, 0.0f,

	-1.0f,  1.0f,  0.0f, 1.0f,
	 1.0f, -1.0f,  1.0f, 0.0f,
	 1.0f,  1.0f,  1.0f, 1.0f
};	

void ClearColorAndDepth(v4 ClearColor)
{
	glClearColor(ClearColor.r, ClearColor.g, ClearColor.b, ClearColor.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void ClearColor(v4 ClearColor)
{
	glClearColor(ClearColor.r, ClearColor.g, ClearColor.b, ClearColor.a);
	glClear(GL_COLOR_BUFFER_BIT);
}

void ClearDepth(void)
{
	glClear(GL_DEPTH_BUFFER_BIT);
}

void SetViewport(int Width, int Height)
{
	glViewport(0, 0, Width, Height);
}

GLuint GetUniformLocation(shader Shader, const char* VariableName)
{
	GLuint Location = glGetUniformLocation(Shader.Program, VariableName);
	//Assert(Location != -1);
	return(Location);
}

void SetUniform(shader Shader, mat4 Matrix, const char* VariableName)
{
	GLuint Location = GetUniformLocation(Shader, VariableName);
	glUniformMatrix4fv(Location, 1, GL_FALSE, Matrix.Data_); 
}

void SetUniform(shader Shader, u32 UnsignedInteger, const char* VariableName)
{
	GLuint Location = GetUniformLocation(Shader, VariableName);
	glUniform1i(Location, UnsignedInteger); 
}

void SetUniform(shader Shader, float Value, const char* VariableName)
{
	GLuint Location = GetUniformLocation(Shader, VariableName);
	glUniform1f(Location, Value); 
}

void SetUniform(shader Shader, v2 V, const char* VariableName)
{
	GLuint Location = GetUniformLocation(Shader, VariableName);
	glUniform2f(Location, V.x, V.y); 
}

void SetUniform(shader Shader, v3 V, const char* VariableName)
{
	GLuint Location = GetUniformLocation(Shader, VariableName);
	glUniform3f(Location, V.x, V.y, V.z); 
}

void SetUniform(shader Shader, v4 V, const char* VariableName)
{
	GLuint Location = GetUniformLocation(Shader, VariableName);
	glUniform4f(Location, V.x, V.y, V.z, V.w); 
}

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

gl_screen_framebuffer CreateScreenFramebuffer(int BufferWidth, int BufferHeight)
{
	gl_screen_framebuffer Result = {};
	glGenFramebuffers(1, &Result.FBO);

	glGenTextures(1, &Result.Texture);

	glBindFramebuffer(GL_FRAMEBUFFER, Result.FBO);

	glBindTexture(GL_TEXTURE_2D, Result.Texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, BufferWidth, BufferHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Result.Texture, 0);

	glGenRenderbuffers(1, &Result.RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, Result.RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, BufferWidth, BufferHeight);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, Result.RBO);

	Assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return(Result);
}

gl_depth_framebuffer CreateDepthFramebuffer(int BufferWidth, int BufferHeight)
{
	gl_depth_framebuffer Result = {};
	glGenFramebuffers(1, &Result.FBO);
	glGenTextures(1, &Result.Texture);

	glBindFramebuffer(GL_FRAMEBUFFER, Result.FBO);
	glBindTexture(GL_TEXTURE_2D, Result.Texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, BufferWidth, BufferHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, Result.Texture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	Assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return(Result);

}

