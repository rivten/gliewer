#pragma once

static u32 GlobalMicrobufferWidth = 64;
static u32 GlobalMicrobufferHeight = 64;

static u32 MaxInstanceDrawn = 16;

void SetUniformTexture(render_state* RenderState, shader Shader,
		u32 BufferID, u32 UniformID, char* UniformName,
		u32 Target = GL_TEXTURE_2D)
{
	ActiveTexture(RenderState, GL_TEXTURE0 + UniformID);
	SetUniform(Shader, UniformID, UniformName);
	BindTexture(RenderState, Target, BufferID);
	GL_CHECK("BindTexture");
}

void MegaConvolution(game_state* State,
		camera Camera,
		u32 PatchSizeInPixels,
		float PixelSurfaceInMeters,
		u32 PatchX, u32 PatchY,
		u32 PatchWidth, u32 PatchHeight,
		mat4 InvLookAtCamera,
		v3 WorldUp)
{
	//
	// NOTE(hugo) : Lighting with the mega texture
	// {
	//
	UseShader(State->RenderState, State->Shaders[ShaderType_BRDFConvolutional]);

	SetUniformTexture(State->RenderState, State->Shaders[ShaderType_BRDFConvolutional],
			State->MegaBuffer.TextureArray.ID, 0, "MegaTexture", GL_TEXTURE_2D_ARRAY);
	SetUniformTexture(State->RenderState, State->Shaders[ShaderType_BRDFConvolutional],
			State->GBuffer.DepthTexture.ID, 5, "DepthMap");
	SetUniformTexture(State->RenderState, State->Shaders[ShaderType_BRDFConvolutional],
			State->GBuffer.NormalTexture.ID, 6, "NormalMap");
	SetUniformTexture(State->RenderState, State->Shaders[ShaderType_BRDFConvolutional],
			State->GBuffer.AlbedoTexture.ID, 7, "AlbedoMap");
	SetUniformTexture(State->RenderState, State->Shaders[ShaderType_BRDFConvolutional],
			State->PreProcess.Texture.ID, 8, "DirectIlluminationMap");

	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], PatchSizeInPixels, "PatchSizeInPixels");
	GL_CHECK();
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], PatchX, "PatchX");
	GL_CHECK();
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], PatchY, "PatchY");
	GL_CHECK();
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], State->HemicubeFramebuffer.Width, "MicrobufferWidth");
	GL_CHECK();
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], State->HemicubeFramebuffer.Height, "MicrobufferHeight");
	GL_CHECK();
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], Camera.P, "CameraPos");
	GL_CHECK();
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], PixelSurfaceInMeters, "PixelSurfaceInMeters");
	GL_CHECK();

	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], State->Alpha, "Alpha");
	GL_CHECK();
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], State->CookTorranceF0, "CookTorranceF0");
	GL_CHECK();
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], WorldUp, "WorldUp");
	GL_CHECK();
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], Camera.Aspect, "MainCameraAspect");
	GL_CHECK();
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], Camera.FoV, "MainCameraFoV");
	GL_CHECK();
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], Camera.NearPlane, "MainCameraNearPlane");
	GL_CHECK();
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], Camera.FarPlane, "MainCameraFarPlane");
	GL_CHECK();
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], InvLookAtCamera, "InvLookAtCamera");
	GL_CHECK();
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], GlobalWindowWidth, "WindowWidth");
	GL_CHECK();
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], GlobalWindowHeight, "WindowHeight");
	GL_CHECK();

	rect2 ViewportRect = RectFromMinSize(PatchSizeInPixels * V2(PatchX, PatchY), V2(PatchWidth, PatchHeight));
	SetViewport(State->RenderState, ViewportRect);
	BindFramebuffer(State->RenderState, GL_FRAMEBUFFER, State->IndirectIlluminationFramebuffer.ID);
	GL_CHECK("BindFramebuffer");
	BindVertexArray(State->RenderState, State->QuadVAO);
	GL_CHECK("BindVertexArray");
	Disable(State->RenderState, GL_DEPTH_TEST);
	GL_CHECK("DisableDepthTest");
	glDrawArrays(GL_TRIANGLES, 0, 6);
	GL_CHECK("DrawArrays");
	Enable(State->RenderState, GL_DEPTH_TEST);
	GL_CHECK("EnableDepthTest");
	BindVertexArray(State->RenderState, 0);
	GL_CHECK("BindVertexArray");

	//
	// }
	//

}

void ComputeOnePatchOfGI(game_state* State,
		camera Camera,
		mat4 LightProjectionMatrix,
		u32 PatchSizeInPixels,
		u32 PatchX, u32 PatchY,
		u32 PatchXCount, u32 PatchYCount,
		mat4 InvLookAtCamera,
		float PixelSurfaceInMeters,
		bool SaveFirstMegaTexture)
{
	//
	// NOTE(hugo) : Filling the megatexture
	//
	// {

	u32 PatchWidth = PatchSizeInPixels;
	u32 PatchHeight = PatchSizeInPixels;
	// NOTE(hugo) : Only the last patches can have a different size
	if(PatchX == PatchXCount - 1)
	{
		PatchWidth = GlobalWindowWidth - (PatchXCount - 1) * PatchSizeInPixels;
	}
	if(PatchY == PatchYCount - 1)
	{
		PatchHeight = GlobalWindowHeight - (PatchYCount - 1) * PatchSizeInPixels;
	}
	Assert(PatchWidth <= PatchSizeInPixels);
	Assert(PatchHeight <= PatchSizeInPixels);

	u32 InstanceCount = PatchWidth * PatchHeight;

	// NOTE(hugo) : Compute Viewport Array
	// There is as many viewports as there are pixels in the patch
	v2 ViewportSize = V2(GlobalMicrobufferWidth, GlobalMicrobufferHeight);
#if 0
	//
	// TODO(hugo) : If I re-use this, do not forget to turn on the Free(Viewports)
	//
	float* Viewports = AllocateArray(float, 4 * InstanceCount);

	for(u32 Y = 0; Y < PatchHeight; ++Y)
	{
		for(u32 X = 0; X < PatchWidth; ++X)
		{
			u32 ViewportIndex = X + Y * PatchWidth;
			v2 ViewportMin = Hadamard(ViewportSize, V2(X, Y));
			Viewports[4 * ViewportIndex + 0] = ViewportMin.x;
			Viewports[4 * ViewportIndex + 1] = ViewportMin.y;
			Viewports[4 * ViewportIndex + 2] = ViewportSize.x;
			Viewports[4 * ViewportIndex + 3] = ViewportSize.y;
		}
	}
#endif

	v3 WorldUp = V3(0.0f, 1.0f, 0.0f);

	mat4 NormalMatrix = Identity4();

	// NOTE(hugo) : Calling the ShaderType_FillMegaTexture
	UseShader(State->RenderState, State->Shaders[ShaderType_FillMegaTexture]);

	SetUniformTexture(State->RenderState, State->Shaders[ShaderType_FillMegaTexture],
			State->GBuffer.DepthTexture.ID, 4, "DepthMap");
	SetUniformTexture(State->RenderState, State->Shaders[ShaderType_FillMegaTexture],
			State->GBuffer.NormalTexture.ID, 5, "NormalMap");

	SetUniform(State->Shaders[ShaderType_FillMegaTexture], PatchX, "PatchX");
	SetUniform(State->Shaders[ShaderType_FillMegaTexture], PatchY, "PatchY");
	SetUniform(State->Shaders[ShaderType_FillMegaTexture], PatchSizeInPixels, "PatchSizeInPixels");
	SetUniform(State->Shaders[ShaderType_FillMegaTexture], State->ObjectModelMatrix, "ObjectMatrix");
	SetUniform(State->Shaders[ShaderType_FillMegaTexture], WorldUp, "WorldUp");
	SetUniform(State->Shaders[ShaderType_FillMegaTexture], State->Camera.NearPlane, "CameraNearPlane");
	SetUniform(State->Shaders[ShaderType_FillMegaTexture], State->Camera.FarPlane, "CameraFarPlane");
	SetUniform(State->Shaders[ShaderType_FillMegaTexture], State->Camera.FoV, "CameraFoV");
	SetUniform(State->Shaders[ShaderType_FillMegaTexture], State->Camera.Aspect, "CameraAspect");
	SetUniform(State->Shaders[ShaderType_FillMegaTexture], InvLookAtCamera, "InvLookAtCamera");
	SetUniform(State->Shaders[ShaderType_FillMegaTexture], NormalMatrix, "NormalMatrix");
	SetUniform(State->Shaders[ShaderType_FillMegaTexture], State->LightCount, "LightCount");
	SetUniform(State->Shaders[ShaderType_FillMegaTexture], State->LightIntensity, "LightIntensity");
	SetUniform(State->Shaders[ShaderType_FillMegaTexture], State->Alpha, "Alpha");
	SetUniform(State->Shaders[ShaderType_FillMegaTexture], State->CookTorranceF0, "CTF0");
	SetUniform(State->Shaders[ShaderType_FillMegaTexture], State->Ks, "Ks");
	SetUniform(State->Shaders[ShaderType_FillMegaTexture], State->Kd, "Kd");
	mat4 ViewMatrix = LookAt(State->Camera);
	SetUniform(State->Shaders[ShaderType_FillMegaTexture], ViewMatrix, "ViewMatrix");
	//SetUniform(State->Shaders[ShaderType_FillMegaTexture], MicrobufferSize, "MicrobufferSize");
	for(u32 LightIndex = 0; LightIndex < State->LightCount; ++LightIndex)
	{
		light* Light = State->Lights + LightIndex;

		char Buffer[64];
		sprintf(Buffer, "LightPos[%i]", LightIndex);
		SetUniform(State->Shaders[ShaderType_FillMegaTexture], Light->Pos, Buffer);

		memset(Buffer, 0, ArrayCount(Buffer));
		sprintf(Buffer, "LightColor[%i]", LightIndex);
		SetUniform(State->Shaders[ShaderType_FillMegaTexture], Light->Color, Buffer);

		memset(Buffer, 0, ArrayCount(Buffer));
		sprintf(Buffer, "LightSpaceMatrix[%i]", LightIndex);
		v3 LightYAxis = V3(0.0f, 1.0f, 0.0f);
		v3 LightZAxis = Normalized(Light->Pos - Light->Target);
		v3 LightXAxis = Cross(LightYAxis, LightZAxis);
		mat4 LightViewProj = State->LightProjectionMatrix * LookAt(Light->Pos, LightXAxis, LightZAxis);
		SetUniform(State->Shaders[ShaderType_FillMegaTexture], LightViewProj, Buffer);

		memset(Buffer, 0, ArrayCount(Buffer));
		sprintf(Buffer, "ShadowMaps[%i]", LightIndex);
		ActiveTexture(State->RenderState, GL_TEXTURE0 + LightIndex);
		SetUniform(State->Shaders[ShaderType_FillMegaTexture], LightIndex, Buffer);
		BindTexture(State->RenderState, GL_TEXTURE_2D, Light->DepthFramebuffer.Texture.ID);
	}

	rect2 MegaViewport = RectFromMinSize(V2(0.0f, 0.0f), 
			V2(State->MegaBuffer.Width, State->MegaBuffer.Height));
			//V2(256.0f, 256.0f));
	SetViewport(State->RenderState, MegaViewport);
	Assert(!DetectErrors("SetViewport"));
	BindFramebuffer(State->RenderState, GL_FRAMEBUFFER, State->MegaBuffer.ID);
	ClearColor(State->RenderState, V4(0.0f, 0.0f, 0.0f, 1.0f));
	Assert(!DetectErrors("BindFramebuffer"));
	//ClearColorAndDepth(State->RenderState, V4(1.0f, 0.0f, 0.5f, 1.0f));
	for(u32 ObjectIndex = 0; ObjectIndex < State->ObjectCount; ++ObjectIndex)
	{
		object* Object = State->Objects + ObjectIndex;
		// TODO(hugo) : Should I do frustum culling here ?
		if(Object->Visible)
		{
			SetUniform(State->Shaders[ShaderType_FillMegaTexture], 
					ToV4(Object->Material.SpecularColor), "SpecularColor");
			Assert(!DetectErrors("SetUniform"));
			SetUniform(State->Shaders[ShaderType_FillMegaTexture], 
					ToV4(Object->Material.DiffuseColor), "DiffuseColor");
			Assert(!DetectErrors("SetUniform"));

			// NOTE(hugo) : Split the draw call by patch of sixteen
			u32 InstanceDrawnCount = 0;
			while(InstanceDrawnCount < InstanceCount)
			{
				u32 DrawCount = Minu(MaxInstanceDrawn, InstanceCount - InstanceDrawnCount);

#if 0
				// TODO(hugo) : Here I recompute the viewports for each object.
				// Maybe I could, for each viewport sets, draw all the objects... ?
				float* FirstViewport = Viewports + 4 * InstanceDrawnCount;
#else
				u32 BaseTileID = InstanceDrawnCount;
				float* FirstViewport = AllocateArray(float, 4 * DrawCount);
				for(u32 ViewportIndex = 0; ViewportIndex < DrawCount; ++ViewportIndex)
				{
					u32 TileID = InstanceDrawnCount + ViewportIndex;
					u32 TileX = TileID % PatchWidth;
					u32 TileY = (TileID - TileX) / PatchWidth;

					v2 ViewportMin = Hadamard(ViewportSize, V2(TileX, TileY));
					FirstViewport[4 * ViewportIndex + 0] = ViewportMin.x;
					FirstViewport[4 * ViewportIndex + 1] = ViewportMin.y;
					FirstViewport[4 * ViewportIndex + 2] = ViewportSize.x;
					FirstViewport[4 * ViewportIndex + 3] = ViewportSize.y;
				}
#endif
				SetUniform(State->Shaders[ShaderType_FillMegaTexture],
						BaseTileID, "BaseTileID");
				GL_CHECK("SetUniform");

				glViewportArrayv(0, DrawCount, FirstViewport);
				GL_CHECK("ViewportArrayv");
				DrawTriangleObjectInstances(State->RenderState, Object, DrawCount);
				Assert(!DetectErrors("Draw"));

				Free(FirstViewport);

				InstanceDrawnCount += DrawCount;
			}
		}
	}
	// 
	// }
	//

#if 0
	SetViewport(State->RenderState, GlobalWindowWidth, GlobalWindowHeight);
	BindFramebuffer(State->RenderState, GL_FRAMEBUFFER, 0);
	RenderTextureOnQuadScreen(State, State->MegaBuffers[0].Texture);
	SDL_GL_SwapWindow(GlobalWindow);
#endif

	if(SaveFirstMegaTexture && (PatchX == 0) && (PatchY == 0))
	{
		char Filename[64];
		ScreenshotBufferAttachment("Megatexture",
			State->RenderState, State->MegaBuffer.ID,
			0, State->MegaBuffer.Width, 
			State->MegaBuffer.Height,
			GL_RGBA, GL_UNSIGNED_BYTE);
	}

	MegaConvolution(State, Camera, PatchSizeInPixels,
			PixelSurfaceInMeters,
			PatchX, PatchY,
			PatchWidth, PatchHeight,
			InvLookAtCamera, WorldUp);

#if 1
	if(PatchX == 0)
	{
		SetViewport(State->RenderState, GlobalWindowWidth, GlobalWindowHeight);
		BindFramebuffer(State->RenderState, GL_FRAMEBUFFER, 0);
		RenderTextureOnQuadScreen(State, State->IndirectIlluminationFramebuffer.Texture);
		Assert(!DetectErrors("GI2"));
		SDL_GL_SwapWindow(GlobalWindow);
	}
#endif

	//Free(Viewports);
}

void ComputeGlobalIlluminationWithPatch(game_state* State, 
		camera Camera, 
		mat4 LightProjectionMatrix,
		u32 PatchSizeInPixels,
		bool SaveFirstMegaTexture = false)
{
	u32 BeginTicks = SDL_GetTicks();
	if((State->HemicubeFramebuffer.Width != GlobalMicrobufferWidth) ||
			(State->HemicubeFramebuffer.Height != GlobalMicrobufferHeight))
	{
		UpdateHemicubeScreenFramebuffer(State->RenderState, &State->HemicubeFramebuffer, GlobalMicrobufferWidth, GlobalMicrobufferHeight);
		UpdateMegaBuffer(State->RenderState, &State->MegaBuffer, GlobalMicrobufferWidth * State->PatchSizeInPixels, GlobalMicrobufferHeight * State->PatchSizeInPixels);
	}

	BindFramebuffer(State->RenderState, GL_FRAMEBUFFER, State->IndirectIlluminationFramebuffer.ID);
	ClearColorAndDepth(State->RenderState, V4(0.0f, 0.0f, 0.0f, 1.0f));

	u32 PatchXCount = Ceil(GlobalWindowWidth / (float)PatchSizeInPixels);
	u32 PatchYCount = Ceil(GlobalWindowHeight / (float)PatchSizeInPixels);

	// NOTE(hugo) : This works because all microbuffers have the same width (but different heights)
	// TODO(hugo) : This changes in the paraboloid setup
	float MicrobufferWidthInMeters = 2.0f;
	float PixelsToMeters = MicrobufferWidthInMeters / float(State->HemicubeFramebuffer.Width);
	float PixelSurfaceInMeters = PixelsToMeters * PixelsToMeters;

	mat4 InvLookAtCamera = Inverse(LookAt(Camera));
	v2 MicrobufferSize = V2(GlobalMicrobufferWidth, GlobalMicrobufferHeight);

	float* Depths = 0;
	v3* Normals = 0;

	for(u32 PatchY = 0; PatchY < PatchYCount; ++PatchY)
	{
		for(u32 PatchX = 0; PatchX < PatchXCount; ++PatchX)
		{
			ComputeOnePatchOfGI(State, Camera, LightProjectionMatrix, PatchSizeInPixels,
					PatchX, PatchY, PatchXCount, PatchYCount,
					InvLookAtCamera,
					PixelSurfaceInMeters,
					SaveFirstMegaTexture);
		}
	}
	u32 EndTicks = SDL_GetTicks();
	float Duration = float(EndTicks - BeginTicks);
	printf("%fms to compute the frame.\n", Duration);
	float MillisecondsPerPatch = Duration / float(PatchXCount * PatchYCount);
	printf("%fms on average per patch.\n", MillisecondsPerPatch);

	ScreenshotBufferAttachment("GI_result.png",
		State->RenderState, State->IndirectIlluminationFramebuffer.ID,
		0, State->IndirectIlluminationFramebuffer.Width, 
		State->IndirectIlluminationFramebuffer.Height,
		GL_RGBA, GL_UNSIGNED_BYTE);
}


