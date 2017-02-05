static u32 GlobalMicrobufferWidth = 128;
static u32 GlobalMicrobufferHeight = 128;

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

	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], PatchSizeInPixels, "PatchSizeInPixels");
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], PatchX, "PatchX");
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], PatchY, "PatchY");
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], State->HemicubeFramebuffer.Width, "MicrobufferWidth");
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], State->HemicubeFramebuffer.Height, "MicrobufferHeight");
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], Camera.P, "CameraPos");
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], PixelSurfaceInMeters, "PixelSurfaceInMeters");

	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], State->Alpha, "Alpha");
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], State->CookTorranceF0, "CookTorranceF0");
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], MicroCameraNearPlane, "MicroCameraNearPlane");
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], WorldUp, "WorldUp");
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], Camera.Aspect, "MainCameraAspect");
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], Camera.FoV, "MainCameraFoV");
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], Camera.NearPlane, "MainCameraNearPlane");
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], Camera.FarPlane, "MainCameraFarPlane");
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], InvLookAtCamera, "InvLookAtCamera");
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], GlobalWindowWidth, "WindowWidth");
	SetUniform(State->Shaders[ShaderType_BRDFConvolutional], GlobalWindowHeight, "WindowHeight");

	rect2 ViewportRect = RectFromMinSize(PatchSizeInPixels * V2(PatchX, PatchY), V2(PatchWidth, PatchHeight));
	SetViewport(State->RenderState, ViewportRect);
	BindFramebuffer(State->RenderState, GL_FRAMEBUFFER, State->IndirectIlluminationFramebuffer.ID);
	BindVertexArray(State->RenderState, State->QuadVAO);
	Disable(State->RenderState, GL_DEPTH_TEST);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	Enable(State->RenderState, GL_DEPTH_TEST);
	BindVertexArray(State->RenderState, 0);

	//
	// }
	//

	SetViewport(State->RenderState, GlobalWindowWidth, GlobalWindowHeight);
	BindFramebuffer(State->RenderState, GL_FRAMEBUFFER, 0);
	RenderTextureOnQuadScreen(State, State->IndirectIlluminationFramebuffer.Texture);
	Assert(!DetectErrors("GI2"));
	SDL_GL_SwapWindow(GlobalWindow);

}

void ComputeGlobalIlluminationWithPatch(game_state* State, 
		camera Camera, 
		mat4 LightProjectionMatrix,
		u32 PatchSizeInPixels)
{
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

	for(u32 PatchY = 0; PatchY < PatchYCount; ++PatchY)
	{
		for(u32 PatchX = 0; PatchX < PatchXCount; ++PatchX)
		{
			ComputeOnePatchOfGI(State, Camera, LightProjectionMatrix, PatchSizeInPixels,
					PatchX, PatchY, PatchXCount, PatchYCount,
					InvLookAtCamera,
					PixelSurfaceInMeters, MicroCameraNearPlane);
		}
	}
}


