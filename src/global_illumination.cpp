static u32 GlobalMicrobufferWidth = 128;
static u32 GlobalMicrobufferHeight = 128;

#define GL_CHECK Assert(!DetectErrors(""))

void MegaConvolution(game_state* State,
		camera Camera,
		u32 PatchSizeInPixels,
		float PixelSurfaceInMeters,
		u32 PatchX, u32 PatchY,
		u32 PatchWidth, u32 PatchHeight,
		float MicroCameraNearPlane,
		mat4 InvLookAtCamera,
		v3 WorldUp)
{
	//
	// NOTE(hugo) : Lighting with the mega texture
	// {
	//
	UseShader(State->RenderState, State->Shaders[ShaderType_BRDFConvolutional]);

	for(u32 TextureIndex = 0; TextureIndex < ArrayCount(State->MegaBuffers); ++TextureIndex)
	{
		ActiveTexture(State->RenderState, GL_TEXTURE0 + TextureIndex);
		char Buffer[80];
		sprintf(Buffer, "MegaTextures[%d]", TextureIndex);
		SetUniform(State->Shaders[ShaderType_BRDFConvolutional], TextureIndex, Buffer);
		BindTexture(State->RenderState, GL_TEXTURE_2D, State->MegaBuffers[TextureIndex].Texture.ID);
	}
	
	ActiveTexture(State->RenderState, GL_TEXTURE5);
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], (u32)5, "DepthMap");
	BindTexture(State->RenderState, GL_TEXTURE_2D, State->GBuffer.DepthTexture.ID);	

	ActiveTexture(State->RenderState, GL_TEXTURE6);
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], (u32)6, "NormalMap");
	BindTexture(State->RenderState, GL_TEXTURE_2D, State->GBuffer.NormalTexture.ID);

	ActiveTexture(State->RenderState, GL_TEXTURE7);
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], (u32)7, "AlbedoMap");
	BindTexture(State->RenderState, GL_TEXTURE_2D, State->GBuffer.AlbedoTexture.ID);

	ActiveTexture(State->RenderState, GL_TEXTURE8);
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], (u32)8, "DirectIlluminationMap");
	// TODO(hugo) : maybe PreFXAA framebuffer ??
	BindTexture(State->RenderState, GL_TEXTURE_2D, State->PreProcess.Texture.ID);
	GL_CHECK;

	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], PatchSizeInPixels, "PatchSizeInPixels");
	GL_CHECK;
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], PatchX, "PatchX");
	GL_CHECK;
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], PatchY, "PatchY");
	GL_CHECK;
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], State->HemicubeFramebuffer.Width, "MicrobufferWidth");
	GL_CHECK;
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], State->HemicubeFramebuffer.Height, "MicrobufferHeight");
	GL_CHECK;
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], Camera.P, "CameraPos");
	GL_CHECK;
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], PixelSurfaceInMeters, "PixelSurfaceInMeters");
	GL_CHECK;

	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], State->Alpha, "Alpha");
	GL_CHECK;
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], State->CookTorranceF0, "CookTorranceF0");
	GL_CHECK;
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], MicroCameraNearPlane, "MicroCameraNearPlane");
	GL_CHECK;
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], WorldUp, "WorldUp");
	GL_CHECK;
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], Camera.Aspect, "MainCameraAspect");
	GL_CHECK;
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], Camera.FoV, "MainCameraFoV");
	GL_CHECK;
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], Camera.NearPlane, "MainCameraNearPlane");
	GL_CHECK;
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], Camera.FarPlane, "MainCameraFarPlane");
	GL_CHECK;
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], InvLookAtCamera, "InvLookAtCamera");
	GL_CHECK;
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], GlobalWindowWidth, "WindowWidth");
	GL_CHECK;
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], GlobalWindowHeight, "WindowHeight");
	GL_CHECK;

	rect2 ViewportRect = RectFromMinSize(PatchSizeInPixels * V2(PatchX, PatchY), V2(PatchWidth, PatchHeight));
	SetViewport(State->RenderState, ViewportRect);
	BindFramebuffer(State->RenderState, GL_FRAMEBUFFER, State->IndirectIlluminationFramebuffer.ID);
	GL_CHECK;
	BindVertexArray(State->RenderState, State->QuadVAO);
	GL_CHECK;
	Disable(State->RenderState, GL_DEPTH_TEST);
	GL_CHECK;
	glDrawArrays(GL_TRIANGLES, 0, 6);
	GL_CHECK;
	Enable(State->RenderState, GL_DEPTH_TEST);
	GL_CHECK;
	BindVertexArray(State->RenderState, 0);
	GL_CHECK;

	//
	// }
	//

}

void ComputeOnePatchOfGIWithoutInstancing(game_state* State,
		camera Camera,
		mat4 LightProjectionMatrix,
		u32 PatchSizeInPixels,
		u32 PatchX, u32 PatchY,
		u32 PatchXCount, u32 PatchYCount,
		mat4 InvLookAtCamera,
		float PixelSurfaceInMeters,
		float MicroCameraNearPlane,
		float* Depths, v3* Normals)
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

	ReadBufferDepth(State->RenderState, State->GBuffer.ID,
			PatchX * PatchSizeInPixels, PatchY * PatchSizeInPixels,
			PatchWidth, PatchHeight, Depths);

	ReadBufferAttachement(State->RenderState, State->GBuffer.ID, 0,
			PatchX * PatchSizeInPixels, PatchY * PatchSizeInPixels,
			PatchWidth, PatchHeight, GL_RGB, GL_FLOAT, Normals);

	v3 WorldUp = V3(0.0f, 1.0f, 0.0f);

	float NearPlane = Camera.NearPlane;
	float FarPlane = Camera.FarPlane;
	for(u32 Y = 0; Y < PatchHeight; ++Y)
	{
		for(u32 X = 0; X < PatchWidth; ++X)
		{
			u32 PixelIndex = X + PatchWidth * Y;
			float PixelDepth = Depths[PixelIndex];
			v3 Normal = Normals[PixelIndex];

			PixelDepth = 2.0f * PixelDepth - 1.0f;
			// TODO(hugo) : I don't think this next computation works if I am using an orthographic projection
			PixelDepth = 2.0f * NearPlane * FarPlane / (NearPlane + FarPlane - PixelDepth * (FarPlane - NearPlane));


			Normal = Normalized(2.0f * Normal - V3(1.0f, 1.0f, 1.0f));

			// NOTE(hugo) : Render directions are, in order : FRONT / LEFT / RIGHT / TOP / BOTTOM
			// with FRONT being the direction of the normal previously found;
			camera MicroCameras[5];
			mat4 MicroCameraProjections[5];
			{
				v4 PixelPos = UnprojectPixel(PixelDepth, 
						X + PatchX * PatchSizeInPixels, Y + PatchY * PatchSizeInPixels,
						GlobalWindowWidth, GlobalWindowHeight,
						Camera, InvLookAtCamera);
				v3 MicroCameraPos = PixelPos.xyz;

				v3 MicroRenderDir[5];
				MicroRenderDir[0] = Normal;
				MicroRenderDir[1] = Cross(Normal, WorldUp);
				MicroRenderDir[2] = -1.0f * MicroRenderDir[1];
				MicroRenderDir[3] = Cross(Normal, MicroRenderDir[1]);
				MicroRenderDir[4] = -1.0f * MicroRenderDir[3];
				for(u32 MicroCameraIndex = 0; MicroCameraIndex < ArrayCount(MicroCameras); ++MicroCameraIndex)
				{
					MicroCameras[MicroCameraIndex].P = MicroCameraPos;
					MicroCameras[MicroCameraIndex].ZAxis = -1.0f * MicroRenderDir[MicroCameraIndex];

					v3 MicroCameraUp = WorldUp;
					if(MicroCameraIndex != 0)
					{
						MicroCameraUp = Normal;
					}

					MicroCameras[MicroCameraIndex].XAxis = Normalized(Cross(MicroRenderDir[MicroCameraIndex], MicroCameraUp));
					MicroCameras[MicroCameraIndex].FoV = Radians(State->MicroFoVInDegrees);
					MicroCameras[MicroCameraIndex].Aspect = float(State->HemicubeFramebuffer.MicroBuffers[MicroCameraIndex].Width) / float(State->HemicubeFramebuffer.MicroBuffers[MicroCameraIndex].Height);
					// TODO(hugo) : Make the micro near/far plane parametrable
					MicroCameras[MicroCameraIndex].NearPlane = MicroCameraNearPlane;
					//MicroCameras[MicroCameraIndex].FarPlane = 2.2f;
					// TODO(hugo) : Why does changing the FarPlane value change the size of the
					// result in the mega texture ?
					MicroCameras[MicroCameraIndex].FarPlane = 5.0f;

					MicroCameraProjections[MicroCameraIndex] = GetCameraPerspective(MicroCameras[MicroCameraIndex]);
				}
			}

			for(u32 FaceIndex = 0; 
					FaceIndex < ArrayCount(State->HemicubeFramebuffer.MicroBuffers); 
					++FaceIndex)
			{
				camera MicroCamera = MicroCameras[FaceIndex];
				SetViewport(State->RenderState, State->HemicubeFramebuffer.MicroBuffers[FaceIndex].Width, 
						State->HemicubeFramebuffer.MicroBuffers[FaceIndex].Height);

				FillGBuffer(State, 
						State->HemicubeFramebuffer.MicroBuffers[FaceIndex],
						MicroCamera,
						MicroCameraProjections[FaceIndex]);
				rect2 ViewportRect = RectFromMinSize(Hadamard(V2(X, Y), V2(GlobalMicrobufferWidth, GlobalMicrobufferHeight)),
						V2(GlobalMicrobufferWidth, GlobalMicrobufferHeight));
				SetViewport(State->RenderState, ViewportRect);
				LightGBuffer(State, 
						State->HemicubeFramebuffer.MicroBuffers[FaceIndex], 
						State->MegaBuffers[FaceIndex],
						MicroCamera, false);
			}

		}
	}

#if 0
	SetViewport(State->RenderState, GlobalWindowWidth, GlobalWindowHeight);
	BindFramebuffer(State->RenderState, GL_FRAMEBUFFER, 0);
	RenderTextureOnQuadScreen(State, State->MegaBuffers[0].Texture);
	SDL_GL_SwapWindow(GlobalWindow);
#endif

#if 0
	if(PatchX == 0 && PatchY == 0)
	{
		ScreenshotBufferAttachment("MegatextureFace0.png",
			State->RenderState, State->MegaBuffers[0].ID,
			0, State->MegaBuffers[0].Width, 
			State->MegaBuffers[0].Height,
			GL_RGBA, GL_UNSIGNED_BYTE);
	}
#endif

	MegaConvolution(State, Camera, PatchSizeInPixels,
			PixelSurfaceInMeters,
			PatchX, PatchY,
			PatchWidth, PatchHeight,
			MicroCameraNearPlane,
			InvLookAtCamera, WorldUp);

	SetViewport(State->RenderState, GlobalWindowWidth, GlobalWindowHeight);
	BindFramebuffer(State->RenderState, GL_FRAMEBUFFER, 0);
	RenderTextureOnQuadScreen(State, State->IndirectIlluminationFramebuffer.Texture);
	Assert(!DetectErrors("GI2"));
	SDL_GL_SwapWindow(GlobalWindow);

}

void ComputeOnePatchOfGI(game_state* State,
		camera Camera,
		mat4 LightProjectionMatrix,
		u32 PatchSizeInPixels,
		u32 PatchX, u32 PatchY,
		u32 PatchXCount, u32 PatchYCount,
		mat4 InvLookAtCamera,
		float PixelSurfaceInMeters,
		float MicroCameraNearPlane)
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

	v3 WorldUp = V3(0.0f, 1.0f, 0.0f);

	mat4 MicroProjection = Perspective(Radians(45), 1.0f, 0.2f, 2.2f);
	mat4 NormalMatrix = Identity4();

	// NOTE(hugo) : Calling the ShaderType_FillMegaTexture
	UseShader(State->RenderState, State->Shaders[ShaderType_FillMegaTexture]);

	ActiveTexture(State->RenderState, GL_TEXTURE4);
	SetUniform(State->Shaders[ShaderType_FillMegaTexture], (u32)4, "DepthMap");
	BindTexture(State->RenderState, GL_TEXTURE_2D, State->GBuffer.DepthTexture.ID);

	ActiveTexture(State->RenderState, GL_TEXTURE5);
	SetUniform(State->Shaders[ShaderType_FillMegaTexture], (u32)5, "NormalMap");
	BindTexture(State->RenderState, GL_TEXTURE_2D, State->GBuffer.NormalTexture.ID);

	SetUniform(State->Shaders[ShaderType_FillMegaTexture], PatchX, "PatchX");
	SetUniform(State->Shaders[ShaderType_FillMegaTexture], PatchY, "PatchY");
	SetUniform(State->Shaders[ShaderType_FillMegaTexture], PatchSizeInPixels, "PatchSizeInPixels");
	SetUniform(State->Shaders[ShaderType_FillMegaTexture], MicroProjection, "MicroProjection");
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

	for(u32 FaceIndex = 0; FaceIndex < ArrayCount(State->MegaBuffers); ++FaceIndex)
	{
		SetUniform(State->Shaders[ShaderType_FillMegaTexture], FaceIndex, "FaceIndex");
		Assert(!DetectErrors("SetUniform"));
		rect2 MegaViewport = RectFromMinSize(V2(0.0f, 0.0f), 
				V2(State->MegaBuffers[FaceIndex].Width, State->MegaBuffers[FaceIndex].Height));
				//V2(256.0f, 256.0f));
		SetViewport(State->RenderState, MegaViewport);
		Assert(!DetectErrors("SetViewport"));
		BindFramebuffer(State->RenderState, GL_FRAMEBUFFER, State->MegaBuffers[FaceIndex].ID);
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

				DrawTriangleObjectInstances(State->RenderState, Object, InstanceCount);
				Assert(!DetectErrors("Draw"));
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

	MegaConvolution(State, Camera, PatchSizeInPixels,
			PixelSurfaceInMeters,
			PatchX, PatchY,
			PatchWidth, PatchHeight,
			MicroCameraNearPlane,
			InvLookAtCamera, WorldUp);

	SetViewport(State->RenderState, GlobalWindowWidth, GlobalWindowHeight);
	BindFramebuffer(State->RenderState, GL_FRAMEBUFFER, 0);
	RenderTextureOnQuadScreen(State, State->IndirectIlluminationFramebuffer.Texture);
	Assert(!DetectErrors("GI2"));
	SDL_GL_SwapWindow(GlobalWindow);

}

void ComputeGlobalIlluminationWithPatch(game_state* State, 
		camera Camera, 
		mat4 LightProjectionMatrix,
		u32 PatchSizeInPixels,
		bool UseInstancing = false)
{
	u32 BeginTicks = SDL_GetTicks();
	if((State->HemicubeFramebuffer.Width != GlobalMicrobufferWidth) ||
			(State->HemicubeFramebuffer.Height != GlobalMicrobufferHeight))
	{
		UpdateHemicubeScreenFramebuffer(State->RenderState, &State->HemicubeFramebuffer, GlobalMicrobufferWidth, GlobalMicrobufferHeight);
		for(u32 BufferIndex = 0; BufferIndex < ArrayCount(State->MegaBuffers); ++BufferIndex)
		{
			UpdateBasicFramebuffer(State->RenderState, State->MegaBuffers + BufferIndex, GlobalMicrobufferWidth * State->PatchSizeInPixels, GlobalMicrobufferHeight * State->PatchSizeInPixels);
		}
	}

	BindFramebuffer(State->RenderState, GL_FRAMEBUFFER, State->IndirectIlluminationFramebuffer.ID);
	ClearColorAndDepth(State->RenderState, V4(0.0f, 0.0f, 0.0f, 1.0f));

	u32 PatchXCount = Ceil(GlobalWindowWidth / (float)PatchSizeInPixels);
	u32 PatchYCount = Ceil(GlobalWindowHeight / (float)PatchSizeInPixels);

	float MicroCameraNearPlane = 0.2f;
	// NOTE(hugo) : This works because all microbuffers have the same width (but different heights)
	float MicrobufferWidthInMeters = 2.0f * MicroCameraNearPlane * Tan(0.5f * Radians(90));
	float PixelsToMeters = MicrobufferWidthInMeters / float(State->HemicubeFramebuffer.Width);
	float PixelSurfaceInMeters = PixelsToMeters * PixelsToMeters;

	mat4 InvLookAtCamera = Inverse(LookAt(Camera));
	v2 MicrobufferSize = V2(GlobalMicrobufferWidth, GlobalMicrobufferHeight);

	float* Depths = 0;
	v3* Normals = 0;

	if(!UseInstancing)
	{
		Depths = AllocateArray(float, PatchSizeInPixels * PatchSizeInPixels);
		Normals = AllocateArray(v3, PatchSizeInPixels * PatchSizeInPixels);
	}

	for(u32 PatchY = 0; PatchY < PatchYCount; ++PatchY)
	{
		for(u32 PatchX = 0; PatchX < PatchXCount; ++PatchX)
		{
			if(UseInstancing)
			{
				ComputeOnePatchOfGI(State, Camera, LightProjectionMatrix, PatchSizeInPixels,
						PatchX, PatchY, PatchXCount, PatchYCount,
						InvLookAtCamera,
						PixelSurfaceInMeters, MicroCameraNearPlane);
			}
			else
			{
				ComputeOnePatchOfGIWithoutInstancing(State,
						Camera, LightProjectionMatrix,
						PatchSizeInPixels,
						PatchX, PatchY,
						PatchXCount, PatchYCount,
						InvLookAtCamera,
						PixelSurfaceInMeters,
						MicroCameraNearPlane,
						Depths, Normals);
			}
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

	if(!UseInstancing)
	{
		Free(Depths);
		Free(Normals);
	}
}


