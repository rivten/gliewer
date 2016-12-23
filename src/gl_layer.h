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

static GLfloat SkyboxVertices[] = {
    // Positions          
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
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

struct gl_depth_framebuffer
{
	u32 FBO;
	u32 Texture;
};

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

struct gl_screen_normal_framebuffer
{
	u32 FBO;
	u32 ScreenTexture;
	u32 NormalTexture;
	u32 RBO;
};

gl_screen_normal_framebuffer CreateScreenNormalFramebuffer(int BufferWidth, int BufferHeight)
{
	gl_screen_normal_framebuffer Result = {};
	glGenFramebuffers(1, &Result.FBO);

	glGenTextures(1, &Result.ScreenTexture);
	glGenTextures(1, &Result.NormalTexture);

	glBindFramebuffer(GL_FRAMEBUFFER, Result.FBO);

	glBindTexture(GL_TEXTURE_2D, Result.ScreenTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, BufferWidth, BufferHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Result.ScreenTexture, 0);

	glBindTexture(GL_TEXTURE_2D, Result.NormalTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, BufferWidth, BufferHeight, 0, GL_RGB, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, Result.NormalTexture, 0);

	glGenRenderbuffers(1, &Result.RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, Result.RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, BufferWidth, BufferHeight);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, Result.RBO);

	GLuint Attachements[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
	glDrawBuffers(2, Attachements);

	Assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return(Result);
}

// NOTE(hugo) : If we wanted to do a real GBuffer, we
// would need to store the world position as well. Not need for that
// here at the moment.
struct gl_geometry_framebuffer
{
	GLuint FBO;
	GLuint ScreenTexture;
	GLuint NormalTexture;
	GLuint AlbedoTexture;
	GLuint RBO;

	u32 Width;
	u32 Height;
};

gl_geometry_framebuffer CreateGeometryFramebuffer(u32 BufferWidth, u32 BufferHeight)
{
	gl_geometry_framebuffer Result = {};

	Result.Width = BufferWidth;
	Result.Height = BufferHeight;

	glGenFramebuffers(1, &Result.FBO);

	glGenTextures(1, &Result.ScreenTexture);
	glGenTextures(1, &Result.NormalTexture);
	glGenTextures(1, &Result.AlbedoTexture);

	glBindFramebuffer(GL_FRAMEBUFFER, Result.FBO);

	glBindTexture(GL_TEXTURE_2D, Result.ScreenTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, BufferWidth, BufferHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Result.ScreenTexture, 0);

	glBindTexture(GL_TEXTURE_2D, Result.NormalTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, BufferWidth, BufferHeight, 0, GL_RGB, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, Result.NormalTexture, 0);

	glBindTexture(GL_TEXTURE_2D, Result.AlbedoTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, BufferWidth, BufferHeight, 0, GL_RGB, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, Result.AlbedoTexture, 0);

	glGenRenderbuffers(1, &Result.RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, Result.RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, BufferWidth, BufferHeight);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, Result.RBO);

	GLuint Attachements[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
	glDrawBuffers(3, Attachements);

	Assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return(Result);
}

void UpdateGeometryFramebuffer(gl_geometry_framebuffer* Framebuffer,
		u32 Width, u32 Height)
{
	Assert((Framebuffer->Width != Width) 
			|| (Framebuffer->Height != Height));

	glDeleteFramebuffers(1, &Framebuffer->FBO);
	glDeleteTextures(1, &Framebuffer->ScreenTexture);
	glDeleteTextures(1, &Framebuffer->NormalTexture);
	glDeleteTextures(1, &Framebuffer->AlbedoTexture);

	glDeleteRenderbuffers(1, &Framebuffer->RBO);

	*Framebuffer = CreateGeometryFramebuffer(Width, Height);
}

struct gl_hemicube_framebuffer
{
	union
	{
		struct
		{
			gl_geometry_framebuffer FrontMicroBuffers;
			gl_geometry_framebuffer LeftMicroBuffers;
			gl_geometry_framebuffer RightMicroBuffers;
			gl_geometry_framebuffer TopMicroBuffers;
			gl_geometry_framebuffer BottomMicroBuffers;
		};
		gl_geometry_framebuffer MicroBuffers[5];
	};
};

gl_hemicube_framebuffer CreateHemicubeScreenFramebuffer(int BufferWidth, int BufferHeight)
{
	gl_hemicube_framebuffer Result = {};
	for(u32 FramebufferIndex = 0; FramebufferIndex < ArrayCount(Result.MicroBuffers); ++FramebufferIndex)
	{
		u32 Width = BufferWidth;
		u32 Height = BufferHeight;
#if 0
		if(FramebufferIndex > 0)
		{
			Height /= 2;
		}
#endif
		Result.MicroBuffers[FramebufferIndex] = CreateGeometryFramebuffer(Width, Height);
	}

	return(Result);
}
