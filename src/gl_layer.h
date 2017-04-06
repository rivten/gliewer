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

struct texture
{
	u32 ID;
	GLenum RenderTarget;
	bool IsValid;
	char Name[128];
};

struct render_state
{
	u32 ShaderID;
	u32 Texture2ID;
	u32 TextureCubeMapID;
	GLenum ActiveTexture;

	u32 VertexArrayID;
	u32 ArrayBufferID;

	u32 FramebufferID;

	GLenum DepthFunc;
	GLenum CullFaceMode;
	GLenum FrontFace;

	bool DepthMaskMode;
	bool DepthTestEnabled;
	bool CullFaceEnabled;
	bool MultisampleEnabled;
	bool GammaCorrectionOutputEnabled;

	v4 ClearColor;

	GLenum ColorAttachmentRead;

	rect2 Viewport;

	texture Textures[64];
	u32 TextureCount;
};

render_state CreateDefaultRenderState(void)
{
	// TODO(hugo) : Get all the default parameters of OpenGL;
	render_state Result = {};

	return(Result);
}

void ClearColorAndDepth(render_state* GLState, v4 ClearColor)
{
	if(GLState->ClearColor != ClearColor)
	{
		GLState->ClearColor = ClearColor;
		glClearColor(ClearColor.r, ClearColor.g, ClearColor.b, ClearColor.a);
		++DEBUGGLCurrentFrameStateChangeCount;
	}
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void ClearColor(render_state* GLState, v4 ClearColor)
{
	if(GLState->ClearColor != ClearColor)
	{
		GLState->ClearColor = ClearColor;
		glClearColor(ClearColor.r, ClearColor.g, ClearColor.b, ClearColor.a);
		++DEBUGGLCurrentFrameStateChangeCount;
	}
	glClear(GL_COLOR_BUFFER_BIT);
}

void ClearDepth(void)
{
	glClear(GL_DEPTH_BUFFER_BIT);
}

void SetViewport(render_state* State, rect2 Viewport)
{
	if((State->Viewport.Min.x != Viewport.Min.x) ||
			(State->Viewport.Min.y != Viewport.Min.y) ||
			(State->Viewport.Max.x != Viewport.Max.x) ||
			(State->Viewport.Max.y != Viewport.Max.y))
	{
		State->Viewport = Viewport;
		v2 Size = RectSize(Viewport);
		glViewport(Viewport.Min.x, Viewport.Min.y, Size.x, Size.y);
		++DEBUGGLCurrentFrameStateChangeCount;
	}
}

void SetViewport(render_state* GLState, int Width, int Height)
{
	rect2 ViewportRect = {V2(0, 0), V2(Width, Height)};
	SetViewport(GLState, ViewportRect);
}

void BindFramebuffer(render_state* State, GLenum Target, u32 FramebufferID)
{
	switch(Target)
	{
		case GL_FRAMEBUFFER:
			{
				if(State->FramebufferID != FramebufferID)
				{
					State->FramebufferID = FramebufferID;
					glBindFramebuffer(Target, State->FramebufferID);
					++DEBUGGLCurrentFrameStateChangeCount;
				}
			} break;

		InvalidDefaultCase;
	}
}

void CullFace(render_state* State, GLenum CullFaceMode)
{
	if(State->CullFaceMode != CullFaceMode)
	{
		State->CullFaceMode = CullFaceMode;
		glCullFace(CullFaceMode);
		++DEBUGGLCurrentFrameStateChangeCount;
	}
}

void FrontFace(render_state* State, GLenum FrontFaceMode)
{
	if(State->FrontFace != FrontFaceMode)
	{
		State->FrontFace = FrontFaceMode;
		glFrontFace(FrontFaceMode);
		++DEBUGGLCurrentFrameStateChangeCount;
	}
}

void BindTexture(render_state* State, GLenum TextureTarget, u32 TextureID)
{
	switch(TextureTarget)
	{
		case GL_TEXTURE_CUBE_MAP:
			{
				if(State->TextureCubeMapID != TextureID)
				{
					State->TextureCubeMapID = TextureID;
					glBindTexture(TextureTarget, State->TextureCubeMapID);
					++DEBUGGLCurrentFrameStateChangeCount;
				}
			} break;
		case GL_TEXTURE_2D:
			{
				if(State->Texture2ID != TextureID)
				{
					State->Texture2ID = TextureID;
					glBindTexture(TextureTarget, State->Texture2ID);
					++DEBUGGLCurrentFrameStateChangeCount;
				}
			} break;
		InvalidDefaultCase;
	}
}

void DepthMask(render_state* State, bool DepthMaskMode)
{
	if(State->DepthMaskMode != DepthMaskMode)
	{
		State->DepthMaskMode = DepthMaskMode;
		if(DepthMaskMode)
		{
			glDepthMask(GL_TRUE);
		}
		else
		{
			glDepthMask(GL_FALSE);
		}
		++DEBUGGLCurrentFrameStateChangeCount;
	}
}

void BindVertexArray(render_state* State, u32 VertexArrayID)
{
	if(State->VertexArrayID != VertexArrayID)
	{
		State->VertexArrayID = VertexArrayID;
		glBindVertexArray(State->VertexArrayID);
		++DEBUGGLCurrentFrameStateChangeCount;
	}
}

void ActiveTexture(render_state* State, GLenum Texture)
{
	if(State->ActiveTexture != Texture)
	{
		State->ActiveTexture = Texture;
		glActiveTexture(State->ActiveTexture);
		++DEBUGGLCurrentFrameStateChangeCount;
	}
}

void Enable(render_state* State, GLenum Cap)
{
	switch(Cap)
	{
		case GL_DEPTH_TEST:
			{
				if(!State->DepthTestEnabled)
				{
					State->DepthTestEnabled = true;
					glEnable(Cap);
					++DEBUGGLCurrentFrameStateChangeCount;
				}
			} break;
		case GL_CULL_FACE:
			{
				if(!State->CullFaceEnabled)
				{
					State->CullFaceEnabled = true;
					glEnable(Cap);
					++DEBUGGLCurrentFrameStateChangeCount;
				}
			} break;
		case GL_MULTISAMPLE:
			{
				if(!State->MultisampleEnabled)
				{
					State->MultisampleEnabled = true;
					glEnable(Cap);
					++DEBUGGLCurrentFrameStateChangeCount;
				}
			} break;
		case GL_FRAMEBUFFER_SRGB:
			{
				if(!State->GammaCorrectionOutputEnabled)
				{
					State->GammaCorrectionOutputEnabled = true;
					glEnable(Cap);
					++DEBUGGLCurrentFrameStateChangeCount;
				}
			} break;
		InvalidDefaultCase;
	}
}

void Disable(render_state* State, GLenum Cap)
{
	switch(Cap)
	{
		case GL_DEPTH_TEST:
			{
				if(State->DepthTestEnabled)
				{
					State->DepthTestEnabled = false;
					glDisable(Cap);
					++DEBUGGLCurrentFrameStateChangeCount;
				}
			} break;
		case GL_CULL_FACE:
			{
				if(State->CullFaceEnabled)
				{
					State->CullFaceEnabled = false;
					glDisable(Cap);
					++DEBUGGLCurrentFrameStateChangeCount;
				}
			} break;
		case GL_MULTISAMPLE:
			{
				if(State->MultisampleEnabled)
				{
					State->MultisampleEnabled = false;
					glDisable(Cap);
					++DEBUGGLCurrentFrameStateChangeCount;
				}
			} break;
		case GL_FRAMEBUFFER_SRGB:
			{
				if(State->GammaCorrectionOutputEnabled)
				{
					State->GammaCorrectionOutputEnabled = false;
					glDisable(Cap);
					++DEBUGGLCurrentFrameStateChangeCount;
				}
			} break;
		InvalidDefaultCase;
	}
}

void ReadBuffer(render_state* State, GLenum Mode)
{
	if(State->ColorAttachmentRead != Mode)
	{
		State->ColorAttachmentRead = Mode;
		glReadBuffer(State->ColorAttachmentRead);
		++DEBUGGLCurrentFrameStateChangeCount;
	}
}

void BindBuffer(render_state* State, GLenum Target, u32 BufferID)
{
	switch(Target)
	{
		case GL_ARRAY_BUFFER:
			{
				if(State->ArrayBufferID != BufferID)
				{
					State->ArrayBufferID = BufferID;
					glBindBuffer(Target, State->ArrayBufferID);
					++DEBUGGLCurrentFrameStateChangeCount;
				}
			} break;
		InvalidDefaultCase;
	}
}

void DepthFunc(render_state* State, GLenum Func)
{
	if(State->DepthFunc != Func)
	{
		State->DepthFunc = Func;
		glDepthFunc(State->DepthFunc);
		++DEBUGGLCurrentFrameStateChangeCount;
	}
}

inline void UseShader(render_state* State, shader Shader)
{
	if(State->ShaderID != Shader.Program)
	{
		State->ShaderID = Shader.Program;
		glUseProgram(State->ShaderID);
		++DEBUGGLCurrentFrameStateChangeCount;
	}
}

texture CreateTexture(GLenum RenderTarget = GL_TEXTURE_2D)
{
	texture Result = {};
	glGenTextures(1, &Result.ID);
	Result.IsValid = true;
	Result.RenderTarget = RenderTarget;

	return(Result);
}

void DeleteTexture(texture* Texture)
{
	glDeleteTextures(1, &Texture->ID);
	Texture->IsValid = false;
}

struct image_texture_loading_params
{
	u32 Width;
	u32 Height;
	GLint InternalFormat;
	GLint ExternalFormat;
	GLint ExternalType;

	GLfloat MinFilter;
	GLfloat MagFilter;
	GLfloat WrapS;
	GLfloat WrapT;

	void* Data;
};

image_texture_loading_params DefaultImageTextureLoadingParams(u32 Width, u32 Height, void* Data)
{
	image_texture_loading_params Result = {};
	Result.Width = Width;
	Result.Height = Height;
	Result.InternalFormat = GL_RGB;
	Result.ExternalFormat = GL_RGBA;
	Result.ExternalType = GL_UNSIGNED_BYTE;

	Result.MinFilter = GL_LINEAR;
	Result.MagFilter = GL_LINEAR;
	Result.WrapS = GL_CLAMP_TO_EDGE;
	Result.WrapT = GL_CLAMP_TO_EDGE;

	Result.Data = Data;

	return(Result);
}

void LoadImageToTexture(render_state* State, texture* Texture, image_texture_loading_params Params)
{
	BindTexture(State, Texture->RenderTarget, Texture->ID);
	glTexImage2D(Texture->RenderTarget, 0, Params.InternalFormat, 
			Params.Width, Params.Height, 0, 
			Params.ExternalFormat, Params.ExternalType, 
			Params.Data);

	glTexParameteri(Texture->RenderTarget, GL_TEXTURE_MIN_FILTER, Params.MinFilter);
	glTexParameteri(Texture->RenderTarget, GL_TEXTURE_MAG_FILTER, Params.MagFilter);
	glTexParameteri(Texture->RenderTarget, GL_TEXTURE_WRAP_S, Params.WrapS);
	glTexParameteri(Texture->RenderTarget, GL_TEXTURE_WRAP_T, Params.WrapT);
	BindTexture(State, Texture->RenderTarget, 0);
}

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

struct bitmap
{
	s32 Width;
	s32 Height;
	u32 BitsPerPixel;
	u8* Data;
};

bitmap LoadBitmap(const char* Filename, u32 OutputChannel = 3)
{
	bitmap Result = {};
	s32 BitsPerPixel;
	Result.Data = stbi_load(Filename, &Result.Width, &Result.Height, &BitsPerPixel, OutputChannel);
	Result.BitsPerPixel = OutputChannel;
	Assert(Result.Data);

	return(Result);
}

void FreeBitmap(bitmap* Bitmap)
{
	stbi_image_free(Bitmap->Data);
}

texture CreateTextureFromFile(render_state* State, const char* Filename)
{
	// TODO(hugo) : Maybe I can only load it as RGB not RGBA
	bitmap Bitmap = LoadBitmap(Filename, 4);
	texture Result = CreateTexture();
	image_texture_loading_params Params = 
		DefaultImageTextureLoadingParams(Bitmap.Width, Bitmap.Height, Bitmap.Data);
	Params.InternalFormat = GL_SRGB; // NOTE(hugo) : Loading gamma corrected
	LoadImageToTexture(State, &Result, Params);
	FreeBitmap(&Bitmap);

	return(Result);
}

u32 GetUniformLocation(shader Shader, char* VariableName)
{
	u32 ShaderType = Shader.Type;
	u32 Result = 0;
	bool Found = false;
	for(u32 UniformIndex = 0; !Found && (UniformIndex < ArrayCount(Uniforms[ShaderType])); ++UniformIndex)
	{
		char* UniformName = Uniforms[ShaderType][UniformIndex];
		if(UniformName && StringMatch(VariableName, UniformName))
		{
			Result = Shader.Locations[UniformIndex];
			Found = true;
		}
	}
	Assert(Found);
	return(Result);
}

void SetUniform(shader Shader, mat4 Matrix, char* VariableName)
{
	GLuint Location = GetUniformLocation(Shader, VariableName);
	glUniformMatrix4fv(Location, 1, GL_FALSE, Matrix.Data_); 
}

void SetUniform(shader Shader, u32 UnsignedInteger, char* VariableName)
{
	GLuint Location = GetUniformLocation(Shader, VariableName);
	glUniform1i(Location, UnsignedInteger); 
}

void SetUniform(shader Shader, float Value, char* VariableName)
{
	GLuint Location = GetUniformLocation(Shader, VariableName);
	glUniform1f(Location, Value); 
}

void SetUniform(shader Shader, v2 V, char* VariableName)
{
	GLuint Location = GetUniformLocation(Shader, VariableName);
	glUniform2f(Location, V.x, V.y); 
}

void SetUniform(shader Shader, v3 V, char* VariableName)
{
	GLuint Location = GetUniformLocation(Shader, VariableName);
	glUniform3f(Location, V.x, V.y, V.z); 
}

void SetUniform(shader Shader, v4 V, char* VariableName)
{
	GLuint Location = GetUniformLocation(Shader, VariableName);
	glUniform4f(Location, V.x, V.y, V.z, V.w); 
}

void SetUniform(shader Shader, bool B, char* VariableName)
{
	u32 IntegerBoolean = (u32)B;
	SetUniform(Shader, IntegerBoolean, VariableName);
}

struct screen_framebuffer
{
	u32 ID;
	texture Texture;
	u32 RBO;
};

screen_framebuffer CreateScreenFramebuffer(render_state* State, int BufferWidth, int BufferHeight)
{
	screen_framebuffer Result = {};
	glGenFramebuffers(1, &Result.ID);

	Result.Texture = CreateTexture();

	glBindFramebuffer(GL_FRAMEBUFFER, Result.ID);

	image_texture_loading_params Params = DefaultImageTextureLoadingParams(BufferWidth, BufferHeight, 0);
	LoadImageToTexture(State, &Result.Texture, Params);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Result.Texture.ID, 0);

	glGenRenderbuffers(1, &Result.RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, Result.RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, BufferWidth, BufferHeight);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, Result.RBO);

	Assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return(Result);
}

struct depth_framebuffer
{
	u32 ID;
	texture Texture;
};

depth_framebuffer CreateDepthFramebuffer(render_state* State, int BufferWidth, int BufferHeight)
{
	depth_framebuffer Result = {};
	glGenFramebuffers(1, &Result.ID);

	Result.Texture = CreateTexture();

	BindFramebuffer(State, GL_FRAMEBUFFER, Result.ID);
	image_texture_loading_params Params = DefaultImageTextureLoadingParams(BufferWidth, BufferHeight, 0);
	Params.InternalFormat = GL_DEPTH_COMPONENT;
	Params.ExternalFormat = GL_DEPTH_COMPONENT;
	Params.ExternalType = GL_FLOAT;
	Params.MinFilter = GL_NEAREST;
	Params.MagFilter = GL_NEAREST;

	LoadImageToTexture(State, &Result.Texture, Params);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, Result.Texture.ID, 0);
	glDrawBuffer(GL_NONE);
	ReadBuffer(State, GL_NONE);
	Assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	BindFramebuffer(State, GL_FRAMEBUFFER, 0);

	return(Result);
}

struct basic_framebuffer
{
	u32 Width;
	u32 Height;
	u32 ID;
	texture Texture;

	bool IsHDR;
};

basic_framebuffer CreateBasicFramebuffer(render_state* State, 
		u32 BufferWidth, u32 BufferHeight,
		bool IsHDR = false)
{
	basic_framebuffer Result = {};

	Result.Width = BufferWidth;
	Result.Height = BufferHeight;
	Result.IsHDR = IsHDR;

	glGenFramebuffers(1, &Result.ID);

	Result.Texture = CreateTexture();

	BindFramebuffer(State, GL_FRAMEBUFFER, Result.ID);

	image_texture_loading_params Params = DefaultImageTextureLoadingParams(BufferWidth, BufferHeight, 0);
	if(IsHDR)
	{
		Params.InternalFormat = GL_RGBA16F;
		Params.ExternalType = GL_FLOAT;
	}

	LoadImageToTexture(State, &Result.Texture, Params);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Result.Texture.ID, 0);

	GLuint Attachements[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, Attachements);

	Assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	BindFramebuffer(State, GL_FRAMEBUFFER, 0);

	return(Result);
}

void UpdateBasicFramebuffer(render_state* State, basic_framebuffer* Framebuffer, u32 Width, u32 Height)
{
	Assert((Framebuffer->Width != Width) 
			|| (Framebuffer->Height != Height));

	glDeleteFramebuffers(1, &Framebuffer->ID);
	DeleteTexture(&Framebuffer->Texture);

	*Framebuffer = CreateBasicFramebuffer(State, Width, Height, Framebuffer->IsHDR);
}

// NOTE(hugo) : If we wanted to do a real GBuffer, we
// would need to store the world position as well. Not need for that
// here at the moment.
struct geometry_framebuffer
{
	u32 ID;
	texture DepthTexture; // NOTE(hugo) : Depth Attachement
	texture NormalTexture; // NOTE(hugo) : Attachement 0
	texture AlbedoTexture; // NOTE(hugo) : Attachement 1
	texture SpecularTexture; // NOTE(hugo) : Attachement 2

	u32 Width;
	u32 Height;
};

geometry_framebuffer CreateGeometryFramebuffer(render_state* State, u32 BufferWidth, u32 BufferHeight)
{
	geometry_framebuffer Result = {};

	Result.Width = BufferWidth;
	Result.Height = BufferHeight;

	glGenFramebuffers(1, &Result.ID);

	Result.DepthTexture = CreateTexture();
	Result.NormalTexture = CreateTexture();
	Result.AlbedoTexture = CreateTexture();
	Result.SpecularTexture = CreateTexture();

	BindFramebuffer(State, GL_FRAMEBUFFER, Result.ID);

	image_texture_loading_params Params = DefaultImageTextureLoadingParams(BufferWidth, BufferHeight, 0);
	LoadImageToTexture(State, &Result.SpecularTexture, Params);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, Result.SpecularTexture.ID, 0);

	Params.ExternalType = GL_FLOAT;
	LoadImageToTexture(State, &Result.NormalTexture, Params);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, Result.NormalTexture.ID, 0);

	Params.ExternalType = GL_UNSIGNED_BYTE;
	LoadImageToTexture(State, &Result.AlbedoTexture, Params);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, Result.AlbedoTexture.ID, 0);

	glBindTexture(GL_TEXTURE_2D, Result.DepthTexture.ID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, BufferWidth, BufferHeight, 0, 
			GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); 
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, Result.DepthTexture.ID, 0);

	GLuint Attachements[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
	glDrawBuffers(3, Attachements);

	Assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	BindFramebuffer(State, GL_FRAMEBUFFER, 0);

	return(Result);
}

void UpdateGeometryFramebuffer(render_state* State, geometry_framebuffer* Framebuffer,
		u32 Width, u32 Height)
{
	Assert((Framebuffer->Width != Width) 
			|| (Framebuffer->Height != Height));

	glDeleteFramebuffers(1, &Framebuffer->ID);
	DeleteTexture(&Framebuffer->SpecularTexture);
	DeleteTexture(&Framebuffer->NormalTexture);
	DeleteTexture(&Framebuffer->AlbedoTexture);
	DeleteTexture(&Framebuffer->DepthTexture);

	*Framebuffer = CreateGeometryFramebuffer(State, Width, Height);
}

struct hemicube_framebuffer
{
	union
	{
		struct
		{
			geometry_framebuffer FrontMicroBuffers;
			geometry_framebuffer LeftMicroBuffers;
			geometry_framebuffer RightMicroBuffers;
			geometry_framebuffer TopMicroBuffers;
			geometry_framebuffer BottomMicroBuffers;
		};
		geometry_framebuffer MicroBuffers[5];
	};
	union
	{
		struct
		{
			basic_framebuffer BasicFrontMicroBuffers;
			basic_framebuffer BasicLeftMicroBuffers;
			basic_framebuffer BasicRightMicroBuffers;
			basic_framebuffer BasicTopMicroBuffers;
			basic_framebuffer BasicBottomMicroBuffers;
		};
		basic_framebuffer BasicMicroBuffers[5];
	};
	u32 Width;
	u32 Height;
};

hemicube_framebuffer CreateHemicubeScreenFramebuffer(render_state* State, int BufferWidth, int BufferHeight)
{
	hemicube_framebuffer Result = {};
	Result.Width = BufferWidth;
	Result.Height = BufferHeight;
	for(u32 FramebufferIndex = 0; FramebufferIndex < ArrayCount(Result.MicroBuffers); ++FramebufferIndex)
	{
		u32 Width = BufferWidth;
		u32 Height = BufferHeight;
		Result.MicroBuffers[FramebufferIndex] = CreateGeometryFramebuffer(State, Width, Height);
	}

	return(Result);
}

void UpdateHemicubeScreenFramebuffer(render_state* State, hemicube_framebuffer* Framebuffer,
		u32 Width, u32 Height)
{
	Assert((Framebuffer->Width != Width) || (Framebuffer->Height != Height));
	for(u32 FaceIndex = 0; FaceIndex < ArrayCount(Framebuffer->MicroBuffers); ++FaceIndex)
	{
		geometry_framebuffer* FaceBuffer = Framebuffer->MicroBuffers + FaceIndex;
		UpdateGeometryFramebuffer(State, FaceBuffer, Width, Height);
	}
	Framebuffer->Width = Width;
	Framebuffer->Height = Height;
}

// TODO(hugo) : Can be even further improved by giving the whole
// framebuffer and getting the ID and the attachement from there
void ReadBufferAttachement(render_state* State, u32 FramebufferID, u32 AttachementIndex, 
		u32 X, u32 Y, u32 Width, u32 Height, GLenum Format, GLenum Type, void* Data)
{
	BindFramebuffer(State, GL_FRAMEBUFFER, FramebufferID);
	ReadBuffer(State, GL_COLOR_ATTACHMENT0 + AttachementIndex);
	glReadPixels(X, Y, Width, Height, Format, Type, Data);
}

void ReadBufferDepth(render_state* State, u32 FramebufferID, 
		u32 X, u32 Y, u32 Width, u32 Height, void* Data)
{
	BindFramebuffer(State, GL_FRAMEBUFFER, FramebufferID);
	glReadPixels(X, Y, Width, Height, GL_DEPTH_COMPONENT, GL_FLOAT, Data);
}

void PushTexture(render_state* RenderState, texture Texture)
{
	Assert(RenderState->TextureCount < ArrayCount(RenderState->Textures));
	RenderState->Textures[RenderState->TextureCount] = Texture;
	++RenderState->TextureCount;
}

bool TextureExists(render_state* RenderState, char* TextureName, u32* Location)
{
	Assert(TextureName);
	Assert(!StringEmpty(TextureName));

	bool Found = false;
	for(u32 TextureIndex = 0; (!Found) && (TextureIndex < RenderState->TextureCount); ++TextureIndex)
	{
		texture* Texture = RenderState->Textures + TextureIndex;
		if(StringMatch(Texture->Name, TextureName))
		{
			Found = true;
			if(Location)
			{
				*Location = TextureIndex;
			}
		}
	}

	return(Found);
}

// --------------------------------
//  NOTE(hugo) : Error handling
// --------------------------------

bool DetectErrors(char* Tag)
{
	bool ErrorFound = false;
	for(GLenum Error; (Error = glGetError()) != GL_NO_ERROR;)
	{
		ErrorFound = true;
		switch(Error)
		{
			case GL_INVALID_ENUM:
				{
					SDL_Log("GL_INVALID_ENUM (%s)", Tag);
				} break;
			case GL_INVALID_VALUE:
				{
					SDL_Log("GL_INVALID_VALUE (%s)", Tag);
				} break;
			case GL_INVALID_OPERATION:
				{
					SDL_Log("GL_INVALID_OPERATION (%s)", Tag);
				} break;
			case GL_STACK_OVERFLOW:
				{
					SDL_Log("GL_STACK_OVERFLOW (%s)", Tag);
				} break;
			case GL_STACK_UNDERFLOW:
				{
					SDL_Log("GL_STACK_UNDERFLOW (%s)", Tag);
				} break;
			case GL_OUT_OF_MEMORY:
				{
					SDL_Log("GL_OUT_OF_MEMORY (%s)", Tag);
				} break;
			case GL_INVALID_FRAMEBUFFER_OPERATION:
				{
					SDL_Log("GL_INVALID_FRAMEBUFFER_OPERATION (%s)", Tag);
				} break;
			case GL_CONTEXT_LOST:
				{
					SDL_Log("GL_CONTEXT_LOST (%s)", Tag);
				} break;

			InvalidDefaultCase;
		}
	}

	return(ErrorFound);
}

u32 ColorV4ToU32(v4 Color)
{
	u32 Result = 0;

	u8 Red   = Floor(255.0f * Color.r) & 0x000000FF;
	u8 Green = Floor(255.0f * Color.g) & 0x000000FF;
	u8 Blue  = Floor(255.0f * Color.b) & 0x000000FF;
	u8 Alpha = Floor(255.0f * Color.a) & 0x000000FF;

	Result = (Red << 0) | (Green << 8) | (Blue << 16) | (Alpha << 24);

	return(Result);
}

v4 ColorU32ToV4(u32 Color)
{
	v4 Result = {};

	Result.r = float(((Color >>  0) & 0x000000FF)) / 255.0f;
	Result.g = float(((Color >>  8) & 0x000000FF)) / 255.0f;
	Result.b = float(((Color >> 16) & 0x000000FF)) / 255.0f;
	Result.a = float(((Color >> 24) & 0x000000FF)) / 255.0f;

	return(Result);
}

void SaveScreenshot(char* OutputFilename,
		u32 Width, u32 Height,
		u32* Buffer)
{
	u32* ScreenshotBuffer = AllocateArray(u32, Width * Height);
	for(u32 Y = 0; Y < Height; ++Y)
	{
		for(u32 X = 0; X < Width; ++X)
		{
			v4 UncorrectedColor = ColorU32ToV4(Buffer[Width * (Height - Y - 1) + X]);
			UncorrectedColor = SquareRoot4(UncorrectedColor);
			ScreenshotBuffer[Width * Y + X] = ColorV4ToU32(UncorrectedColor);
		}
	}

	u32 Stride = sizeof(u32) * Width;
	stbi_write_png(OutputFilename, Width, Height, 4, ScreenshotBuffer, Stride);

	Free(ScreenshotBuffer);
}

void ScreenshotBufferAttachment(char* OutputFilename,
		render_state* State, u32 FramebufferID,
		u32 AttachmentID, u32 Width, u32 Height,
		GLenum Format, GLenum Type)
{
	u32* Buffer = AllocateArray(u32, Width * Height);
	ReadBufferAttachement(State, FramebufferID, AttachmentID,
			0, 0, Width, Height, Format, Type, Buffer);
	SaveScreenshot(OutputFilename, Width, Height, Buffer);
	Free(Buffer);
}
