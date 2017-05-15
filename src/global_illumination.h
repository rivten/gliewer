#pragma once

struct mega_buffer
{
	u32 ID;
	u32 Width;
	u32 Height;

	texture TextureArray;
};

#define LAYER_COUNT 8
mega_buffer CreateMegaBuffer(render_state* State, u32 BufferWidth, u32 BufferHeight)
{
	mega_buffer Result = {};

	Result.Width = BufferWidth;
	Result.Height = BufferHeight;

	glGenFramebuffers(1, &Result.ID);
	BindFramebuffer(State, GL_FRAMEBUFFER, Result.ID);
	GL_CHECK("BindFramebuffer");
	GLenum DrawBuffer[LAYER_COUNT] = {};

	// NOTE(hugo): Generating the 3D texture
	Result.TextureArray = CreateTexture(GL_TEXTURE_2D_ARRAY);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, Result.TextureArray.ID);
	GL_CHECK("BindTexture");

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	GL_CHECK("TexParameteri");

	glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA16F, 
			Result.Width, Result.Height, LAYER_COUNT,
			0, GL_RGB, GL_FLOAT, 0);
	GL_CHECK("TexImage3D");

#if 0
	GLint MaxColorAttachment = 0;
	glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &MaxColorAttachment);
	SDL_Log("%i", MaxColorAttachment);
#endif

	for(u32 LayerIndex = 0; LayerIndex < LAYER_COUNT; ++LayerIndex)
	{
		glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + LayerIndex, 
				Result.TextureArray.ID, 0, LayerIndex);
		DrawBuffer[LayerIndex] = GL_COLOR_ATTACHMENT0 + LayerIndex;
		GL_CHECK("FramebufferTextureLayer");
	}

	glDrawBuffers(LAYER_COUNT, DrawBuffer);

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
