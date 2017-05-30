#pragma once

static u32 GlobalMicrobufferWidth = 64;
static u32 GlobalMicrobufferHeight = 64;

static u32 MaxViewportCount = 16;
static u32 MaxInstanceDrawn = MaxViewportCount * GlobalLayerCount;

// NOTE(hugo): IMPORTANT(hugo): All those values must be powers of two
v2 GetMegaBufferSize(u32 PatchSizeInPixels,
		u32 MicrobufferWidth, u32 MicrobufferHeight,
		u32 LayerCount)
{
	v2 TileCount = {};

#if 0
	u32 NumberOfTiles = PatchSizeInPixels * PatchSizeInPixels;
	u32 NumberOfTilesPerLayer = NumberOfTiles / LayerCount;
	// TODO(hugo) : Find the powers of two which match the total number of tiles.
#else
	// TODO(hugo): Wow ! Much flexibility...
	Assert(PatchSizeInPixels == 32);
	Assert(LayerCount == 8);
	TileCount.x = 16;
	TileCount.y = 8;
#endif

	v2 Result = Hadamard(V2(MicrobufferWidth, MicrobufferHeight), TileCount);

	return(Result);
}

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
		v3 WorldUp,
		u32 PatchXCount)
{
	//
	// NOTE(hugo) : Lighting with the mega texture
	// {
	//
	shader ConvShader = State->Shaders[ShaderType_BRDFConvolutional];
	UseShader(State->RenderState, ConvShader);

	SetUniformTexture(State->RenderState, ConvShader,
			State->GBuffer.DepthTexture.ID, 0, "DepthMap");
	SetUniformTexture(State->RenderState, ConvShader,
			State->GBuffer.NormalTexture.ID, 1, "NormalMap");
	SetUniformTexture(State->RenderState, ConvShader,
			State->GBuffer.AlbedoTexture.ID, 2, "AlbedoMap");
	SetUniformTexture(State->RenderState, ConvShader,
			State->PreProcess.Texture.ID, 3, "DirectIlluminationMap");
	SetUniformTexture(State->RenderState, ConvShader,
			State->MegaBuffer.TextureArray.ID, 4, "MegaTexture", GL_TEXTURE_2D_ARRAY);

	SetUniform(ConvShader, PatchSizeInPixels, "PatchSizeInPixels");
	GL_CHECK();
	SetUniform(ConvShader, PatchX, "PatchX");
	GL_CHECK();
	SetUniform(ConvShader, PatchY, "PatchY");
	GL_CHECK();
	SetUniform(ConvShader, State->HemicubeFramebuffer.Width, "MicrobufferWidth");
	GL_CHECK();
	SetUniform(ConvShader, State->HemicubeFramebuffer.Height, "MicrobufferHeight");
	GL_CHECK();
	SetUniform(ConvShader, Camera.P, "CameraPos");
	GL_CHECK();
	SetUniform(ConvShader, PixelSurfaceInMeters, "PixelSurfaceInMeters");
	GL_CHECK();

	SetUniform(ConvShader, State->Alpha, "Alpha");
	GL_CHECK();
	SetUniform(ConvShader, State->CookTorranceF0, "CookTorranceF0");
	GL_CHECK();
	SetUniform(ConvShader, WorldUp, "WorldUp");
	GL_CHECK();
	SetUniform(ConvShader, Camera.Aspect, "MainCameraAspect");
	GL_CHECK();
	SetUniform(ConvShader, Camera.FoV, "MainCameraFoV");
	GL_CHECK();
	SetUniform(ConvShader, Camera.NearPlane, "MainCameraNearPlane");
	GL_CHECK();
	SetUniform(ConvShader, Camera.FarPlane, "MainCameraFarPlane");
	GL_CHECK();
	SetUniform(ConvShader, InvLookAtCamera, "InvLookAtCamera");
	GL_CHECK();
	SetUniform(ConvShader, GlobalWindowWidth, "WindowWidth");
	GL_CHECK();
	SetUniform(ConvShader, GlobalWindowHeight, "WindowHeight");
	GL_CHECK();
	SetUniform(ConvShader, GlobalLayerCount, "LayerCount");
	GL_CHECK();
	SetUniform(ConvShader, PatchXCount, "PatchXCount");
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
	Assert(PatchSizeInPixels == 32);
	Assert(GlobalLayerCount == 8);
	u32 TileCountX = 16;
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

	shader FillShader = State->Shaders[ShaderType_FillMegaTexture];
	// NOTE(hugo) : Calling the ShaderType_FillMegaTexture
	UseShader(State->RenderState, FillShader);

	SetUniformTexture(State->RenderState, FillShader,
			State->GBuffer.DepthTexture.ID, 4, "DepthMap");
	SetUniformTexture(State->RenderState, FillShader,
			State->GBuffer.NormalTexture.ID, 5, "NormalMap");

	SetUniform(FillShader, PatchX, "PatchX");
	SetUniform(FillShader, PatchY, "PatchY");
	SetUniform(FillShader, PatchSizeInPixels, "PatchSizeInPixels");
	SetUniform(FillShader, State->ObjectModelMatrix, "ObjectMatrix");
	SetUniform(FillShader, WorldUp, "WorldUp");
	SetUniform(FillShader, State->Camera.NearPlane, "CameraNearPlane");
	SetUniform(FillShader, State->Camera.FarPlane, "CameraFarPlane");
	SetUniform(FillShader, State->Camera.FoV, "CameraFoV");
	SetUniform(FillShader, State->Camera.Aspect, "CameraAspect");
	SetUniform(FillShader, InvLookAtCamera, "InvLookAtCamera");
	SetUniform(FillShader, NormalMatrix, "NormalMatrix");
	SetUniform(FillShader, State->LightIntensity, "LightIntensity");
	SetUniform(FillShader, State->Alpha, "Alpha");
	SetUniform(FillShader, State->CookTorranceF0, "CTF0");
	SetUniform(FillShader, State->Ks, "Ks");
	SetUniform(FillShader, State->Kd, "Kd");
	SetUniform(FillShader, GlobalLayerCount, "LayerCount");
	mat4 ViewMatrix = LookAt(State->Camera);
	SetUniform(FillShader, ViewMatrix, "ViewMatrix");
	//SetUniform(State->Shaders[ShaderType_FillMegaTexture], MicrobufferSize, "MicrobufferSize");
	Assert(State->LightCount == 1);
	u32 LightIndex = 0;
	light* Light = State->Lights + LightIndex;
	SetUniform(FillShader, Light->Pos, "LightPos");
	SetUniform(FillShader, Light->Color, "LightColor");

	v3 LightYAxis = V3(0.0f, 1.0f, 0.0f);
	v3 LightZAxis = Normalized(Light->Pos - Light->Target);
	v3 LightXAxis = Cross(LightYAxis, LightZAxis);
	mat4 LightViewProj = State->LightProjectionMatrix * LookAt(Light->Pos, LightXAxis, LightZAxis);
	SetUniform(FillShader, LightViewProj, "LightSpaceMatrix");

	SetUniformTexture(State->RenderState, FillShader,
			Light->DepthFramebuffer.Texture.ID, LightIndex, "ShadowMap");

#if 0
	s32 MaxGeometryOutputVertices = 0;
	s32 MaxGeometryOutputComponents = 0;
	s32 MaxGeometryTotalOutputComponents = 0;
	glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES, &MaxGeometryOutputVertices);
	glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_COMPONENTS, &MaxGeometryOutputComponents);
	glGetIntegerv(GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS, &MaxGeometryTotalOutputComponents);
	SDL_Log("MaxGeometryOutputVertices = %i -- MaxGeometryOutputComponent = %i -- MaxGeometryTotalOutputComponents = %i", MaxGeometryOutputVertices, MaxGeometryOutputComponents, MaxGeometryTotalOutputComponents);
#endif

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
			SetUniform(FillShader, 
					ToV4(Object->Material.SpecularColor), "SpecularColor");
			Assert(!DetectErrors("SetUniform"));
			SetUniform(FillShader, 
					ToV4(Object->Material.DiffuseColor), "DiffuseColor");
			Assert(!DetectErrors("SetUniform"));

			// NOTE(hugo) : Split the draw call by patch of sixteen
			u32 InstanceDrawnCount = 0;
			while(InstanceDrawnCount < InstanceCount)
			{
				Assert(InstanceDrawnCount % GlobalLayerCount == 0);
				u32 DrawLeft = InstanceCount - InstanceDrawnCount;
				u32 DrawCount = Minu(MaxInstanceDrawn, DrawLeft);

#if 0
				// TODO(hugo) : Here I recompute the viewports for each object.
				// Maybe I could, for each viewport sets, draw all the objects... ?
				float* FirstViewport = Viewports + 4 * InstanceDrawnCount;
#else
				u32 BaseTileID = InstanceDrawnCount / GlobalLayerCount;
				float* FirstViewport = AllocateArray(float, 4 * DrawCount);
				Assert(DrawCount % GlobalLayerCount == 0);
				u32 ViewportCount = DrawCount / GlobalLayerCount;
				for(u32 ViewportIndex = 0; ViewportIndex < ViewportCount; ++ViewportIndex)
				{
					u32 TileID = (InstanceDrawnCount / GlobalLayerCount) + ViewportIndex;
					u32 TileX = TileID % TileCountX;
					u32 TileY = (TileID - TileX) / TileCountX;

					v2 ViewportMin = Hadamard(ViewportSize, V2(TileX, TileY));
					FirstViewport[4 * ViewportIndex + 0] = ViewportMin.x;
					FirstViewport[4 * ViewportIndex + 1] = ViewportMin.y;
					FirstViewport[4 * ViewportIndex + 2] = ViewportSize.x;
					FirstViewport[4 * ViewportIndex + 3] = ViewportSize.y;
				}
#endif
				SetUniform(FillShader,
						BaseTileID, "BaseTileID");
				GL_CHECK("SetUniform");

				glViewportArrayv(0, ViewportCount, FirstViewport);
				GL_CHECK("ViewportArrayv");
				Assert(DrawCount % GlobalLayerCount == 0);
				u32 Instance = DrawCount / GlobalLayerCount;
				DrawTriangleObjectInstances(State->RenderState, Object, Instance);
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
#if 0
		for(u32 LayerIndex = 0; LayerIndex < LAYER_COUNT; ++LayerIndex)
		{
			char Buffer[64];
			sprintf(Buffer, "Megatexture_%i.png", LayerIndex);
			ScreenshotBufferAttachment(Buffer,
				State->RenderState, State->MegaBuffer.ID,
				LayerIndex, State->MegaBuffer.Width,
				State->MegaBuffer.Height,
				GL_RGBA, GL_RGBA16F);
			GL_CHECK();
		}
#else
		ScreenshotBufferAttachment("Megatexture_0.png",
			State->RenderState, State->MegaBuffer.ID,
			0, State->MegaBuffer.Width,
			State->MegaBuffer.Height,
			GL_RGBA, GL_UNSIGNED_BYTE);
#endif
	}

	MegaConvolution(State, Camera, PatchSizeInPixels,
			PixelSurfaceInMeters,
			PatchX, PatchY,
			PatchWidth, PatchHeight,
			InvLookAtCamera, WorldUp, PatchXCount);

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

		v2 MegaBufferSize = GetMegaBufferSize(State->PatchSizeInPixels, GlobalMicrobufferWidth, GlobalMicrobufferHeight, GlobalLayerCount);
		UpdateMegaBuffer(State->RenderState, &State->MegaBuffer, MegaBufferSize.x, MegaBufferSize.y);
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

#if 0
	ScreenshotBufferAttachment("GI_result.png",
		State->RenderState, State->IndirectIlluminationFramebuffer.ID,
		0, State->IndirectIlluminationFramebuffer.Width, 
		State->IndirectIlluminationFramebuffer.Height,
		GL_RGBA, GL_UNSIGNED_BYTE);
#endif
	State->MegaBufferComputed = true;
}

void DEBUGComputeOnePatchOfGI(game_state* State, 
		camera Camera, 
		mat4 LightProjectionMatrix,
		u32 PatchSizeInPixels,
		bool SaveFirstMegaTexture = false)
{
	u32 PatchXCount = Ceil(GlobalWindowWidth / (float)PatchSizeInPixels);
	u32 PatchYCount = Ceil(GlobalWindowHeight / (float)PatchSizeInPixels);

	// NOTE(hugo) : This works because all microbuffers have the same width (but different heights)
	// TODO(hugo) : This changes in the paraboloid setup
	float MicrobufferWidthInMeters = 2.0f;
	float PixelsToMeters = MicrobufferWidthInMeters / float(State->HemicubeFramebuffer.Width);
	float PixelSurfaceInMeters = PixelsToMeters * PixelsToMeters;

	mat4 InvLookAtCamera = Inverse(LookAt(Camera));
	v2 MicrobufferSize = V2(GlobalMicrobufferWidth, GlobalMicrobufferHeight);

	Assert(PatchSizeInPixels == 32);
	Assert(GlobalLayerCount == 8);
	u32 TileCountX = 16;
	u32 PatchX = 4;
	u32 PatchY = 4;
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
	v3 WorldUp = V3(0.0f, 1.0f, 0.0f);

	mat4 NormalMatrix = Identity4();

	shader FillShader = State->Shaders[ShaderType_FillMegaTexture];
	// NOTE(hugo) : Calling the ShaderType_FillMegaTexture
	UseShader(State->RenderState, FillShader);

	SetUniformTexture(State->RenderState, FillShader,
			State->GBuffer.DepthTexture.ID, 4, "DepthMap");
	SetUniformTexture(State->RenderState, FillShader,
			State->GBuffer.NormalTexture.ID, 5, "NormalMap");

	SetUniform(FillShader, PatchX, "PatchX");
	SetUniform(FillShader, PatchY, "PatchY");
	SetUniform(FillShader, PatchSizeInPixels, "PatchSizeInPixels");
	SetUniform(FillShader, State->ObjectModelMatrix, "ObjectMatrix");
	SetUniform(FillShader, WorldUp, "WorldUp");
	SetUniform(FillShader, State->Camera.NearPlane, "CameraNearPlane");
	SetUniform(FillShader, State->Camera.FarPlane, "CameraFarPlane");
	SetUniform(FillShader, State->Camera.FoV, "CameraFoV");
	SetUniform(FillShader, State->Camera.Aspect, "CameraAspect");
	SetUniform(FillShader, InvLookAtCamera, "InvLookAtCamera");
	SetUniform(FillShader, NormalMatrix, "NormalMatrix");
	SetUniform(FillShader, State->LightIntensity, "LightIntensity");
	SetUniform(FillShader, State->Alpha, "Alpha");
	SetUniform(FillShader, State->CookTorranceF0, "CTF0");
	SetUniform(FillShader, State->Ks, "Ks");
	SetUniform(FillShader, State->Kd, "Kd");
	SetUniform(FillShader, GlobalLayerCount, "LayerCount");
	mat4 ViewMatrix = LookAt(State->Camera);
	SetUniform(FillShader, ViewMatrix, "ViewMatrix");

	Assert(State->LightCount == 1);
	u32 LightIndex = 0;
	light* Light = State->Lights + LightIndex;

	SetUniform(FillShader, Light->Pos, "LightPos");

	SetUniform(FillShader, Light->Color, "LightColor");

	v3 LightYAxis = V3(0.0f, 1.0f, 0.0f);
	v3 LightZAxis = Normalized(Light->Pos - Light->Target);
	v3 LightXAxis = Cross(LightYAxis, LightZAxis);
	mat4 LightViewProj = State->LightProjectionMatrix * LookAt(Light->Pos, LightXAxis, LightZAxis);
	SetUniform(FillShader, LightViewProj, "LightSpaceMatrix");

	SetUniformTexture(State->RenderState, FillShader,
			Light->DepthFramebuffer.Texture.ID, LightIndex, "ShadowMap");

#if 0
	s32 MaxGeometryOutputVertices = 0;
	s32 MaxGeometryOutputComponents = 0;
	s32 MaxGeometryTotalOutputComponents = 0;
	glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES, &MaxGeometryOutputVertices);
	glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_COMPONENTS, &MaxGeometryOutputComponents);
	glGetIntegerv(GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS, &MaxGeometryTotalOutputComponents);
	SDL_Log("MaxGeometryOutputVertices = %i -- MaxGeometryOutputComponent = %i -- MaxGeometryTotalOutputComponents = %i", MaxGeometryOutputVertices, MaxGeometryOutputComponents, MaxGeometryTotalOutputComponents);
#endif

	rect2 MegaViewport = RectFromMinSize(V2(0.0f, 0.0f), 
			V2(State->MegaBuffer.Width, State->MegaBuffer.Height));
	SetViewport(State->RenderState, MegaViewport);
	Assert(!DetectErrors("SetViewport"));
	BindFramebuffer(State->RenderState, GL_FRAMEBUFFER, State->MegaBuffer.ID);
	ClearColor(State->RenderState, V4(0.0f, 0.0f, 0.0f, 1.0f));
	Assert(!DetectErrors("BindFramebuffer"));
	for(u32 ObjectIndex = 0; ObjectIndex < State->ObjectCount; ++ObjectIndex)
	{
		object* Object = State->Objects + ObjectIndex;
		// TODO(hugo) : Should I do frustum culling here ?
		if(Object->Visible)
		{
			SetUniform(FillShader, 
					ToV4(Object->Material.SpecularColor), "SpecularColor");
			Assert(!DetectErrors("SetUniform"));
			SetUniform(FillShader, 
					ToV4(Object->Material.DiffuseColor), "DiffuseColor");
			Assert(!DetectErrors("SetUniform"));

			// NOTE(hugo) : Split the draw call by patch of sixteen
			u32 InstanceDrawnCount = 0;
			while(InstanceDrawnCount < InstanceCount)
			{
				Assert(InstanceDrawnCount % GlobalLayerCount == 0);
				u32 DrawLeft = InstanceCount - InstanceDrawnCount;
				u32 DrawCount = Minu(MaxInstanceDrawn, DrawLeft);

				u32 BaseTileID = InstanceDrawnCount / GlobalLayerCount;
				float* FirstViewport = AllocateArray(float, 4 * DrawCount);
				Assert(DrawCount % GlobalLayerCount == 0);
				u32 ViewportCount = DrawCount / GlobalLayerCount;
				for(u32 ViewportIndex = 0; ViewportIndex < ViewportCount; ++ViewportIndex)
				{
					u32 TileID = BaseTileID + ViewportIndex;
					u32 TileX = TileID % TileCountX;
					u32 TileY = (TileID - TileX) / TileCountX;

					v2 ViewportMin = Hadamard(ViewportSize, V2(TileX, TileY));
					FirstViewport[4 * ViewportIndex + 0] = ViewportMin.x;
					FirstViewport[4 * ViewportIndex + 1] = ViewportMin.y;
					FirstViewport[4 * ViewportIndex + 2] = ViewportSize.x;
					FirstViewport[4 * ViewportIndex + 3] = ViewportSize.y;
				}
				SetUniform(FillShader,
						BaseTileID, "BaseTileID");
				GL_CHECK("SetUniform");

				glViewportArrayv(0, ViewportCount, FirstViewport);
				GL_CHECK("ViewportArrayv");
				Assert(DrawCount % GlobalLayerCount == 0);
				u32 Instance = DrawCount / GlobalLayerCount;
				DrawTriangleObjectInstances(State->RenderState, Object, Instance);
				Assert(!DetectErrors("Draw"));

				Free(FirstViewport);

				InstanceDrawnCount += DrawCount;
			}
		}
	}
	// 
	// }
	//
	State->MegaBufferComputed = true;
	BindFramebuffer(State->RenderState, GL_FRAMEBUFFER, 0);
}

void DEBUGComputeDummyLayeredFramebuffer(game_state* State)
{
	shader DEBUGFillShader = State->Shaders[ShaderType_DEBUGLayerFiller];
	UseShader(State->RenderState, DEBUGFillShader);

	BindFramebuffer(State->RenderState, GL_FRAMEBUFFER, State->DEBUGBuffer.ID);
	Assert(!DetectErrors("BindFramebuffer"));
	ClearColorAndDepth(State->RenderState, V4(0.0f, 0.0f, 0.0f, 1.0f));

	BindVertexArray(State->RenderState, State->QuadVAO);
	Disable(State->RenderState, GL_DEPTH_TEST);

	glDrawArraysInstanced(GL_TRIANGLES, 0, 6, GlobalLayerCount);

	Enable(State->RenderState, GL_DEPTH_TEST);
	BindVertexArray(State->RenderState, 0);

	State->MegaBufferComputed = true;
	BindFramebuffer(State->RenderState, GL_FRAMEBUFFER, 0);
}

void DEBUGDisplayMegabufferLayer(game_state* State, u32 LayerIndex)
{
	Assert(State->MegaBufferComputed);
	shader DEBUGMegaBufferShader = State->Shaders[ShaderType_DEBUGMegaBuffer];

	UseShader(State->RenderState, DEBUGMegaBufferShader);

	SetUniform(DEBUGMegaBufferShader, LayerIndex, "LayerIndex");
	SetUniformTexture(State->RenderState, DEBUGMegaBufferShader,
			State->MegaBuffer.TextureArray.ID, 0, "Texture", GL_TEXTURE_2D_ARRAY);
			//State->DEBUGBuffer.TextureArray.ID, 0, "Texture", GL_TEXTURE_2D_ARRAY);

	BindVertexArray(State->RenderState, State->QuadVAO);
	Disable(State->RenderState, GL_DEPTH_TEST);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	Enable(State->RenderState, GL_DEPTH_TEST);
	BindVertexArray(State->RenderState, 0);
}
