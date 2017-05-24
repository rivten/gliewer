#pragma once

struct mega_buffer
{
	u32 ID;
	u32 Width;
	u32 Height;

	texture TextureArray;
};

global_variable u32 GlobalLayerCount = 8;

mega_buffer CreateMegaBuffer(render_state* State, u32 BufferWidth, u32 BufferHeight)
{
	mega_buffer Result = {};

	Result.Width = BufferWidth;
	Result.Height = BufferHeight;

	glGenFramebuffers(1, &Result.ID);
	BindFramebuffer(State, GL_FRAMEBUFFER, Result.ID);
	GL_CHECK("BindFramebuffer");
	GLenum DrawBuffer[1] = {};

	// NOTE(hugo): Generating the 3D texture
	Result.TextureArray = CreateTexture(GL_TEXTURE_2D_ARRAY);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, Result.TextureArray.ID);
	GL_CHECK("BindTexture");

	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA16F, 
			Result.Width, Result.Height, GlobalLayerCount,
			0, GL_RGB, GL_FLOAT, 0);
	GL_CHECK("TexImage3D");

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	GL_CHECK("TexParameteri");

#if 0
	GLint MaxColorAttachment = 0;
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &MaxColorAttachment);
	SDL_Log("MaxColorAttachment: %i", MaxColorAttachment);

	GLint MaxFramebufferLayer = 0;
	glGetIntegerv(GL_MAX_FRAMEBUFFER_LAYERS, &MaxFramebufferLayer);
	SDL_Log("Max Framebuffer Layer : %i", MaxFramebufferLayer);
#endif

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, Result.TextureArray.ID, 0);
	GL_CHECK("FramebufferTexture");

	DrawBuffer[0] = GL_COLOR_ATTACHMENT0;
	glDrawBuffers(1, DrawBuffer);

	GLenum FramebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(FramebufferStatus != GL_FRAMEBUFFER_COMPLETE)
	{
		if(FramebufferStatus == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT)
		{
			SDL_Log("Incomplete Attachment");
		}
		//else if(FramebufferStatus == GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS)
		//{
			//SDL_Log("Incomplete Dimensions");
		//}
		else if(FramebufferStatus == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT)
		{
			SDL_Log("Incomplete Missing Attachment");
		}
		else if(FramebufferStatus == GL_FRAMEBUFFER_UNSUPPORTED)
		{
			SDL_Log("Unsupporter");
		}
		else
		{
			InvalidCodePath;
		}
	}

	Assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
	BindFramebuffer(State, GL_FRAMEBUFFER, 0);

	GL_CHECK("End");
	return(Result);
}

void UpdateMegaBuffer(render_state* State, mega_buffer* Framebuffer, u32 Width, u32 Height)
{
	Assert((Framebuffer->Width != Width) 
			|| (Framebuffer->Height != Height));

	glDeleteFramebuffers(1, &Framebuffer->ID);
	DeleteTexture(&Framebuffer->TextureArray);

	*Framebuffer = CreateMegaBuffer(State, Width, Height);
}
