
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
			UpdateBasicFramebuffer(State->RenderState, State->MegaBuffers + BufferIndex, GlobalMicrobufferWidth, GlobalMicrobufferHeight);
		}
	}

	BindFramebuffer(State->RenderState, GL_FRAMEBUFFER, State->IndirectIlluminationFramebuffer.ID);
	ClearColor(State->RenderState, V4(0.0f, 0.0f, 0.0f, 1.0f));

	u32 PatchXCount = Ceil(GlobalWindowWidth / (float)PatchSizeInPixels);
	u32 PatchYCount = Ceil(GlobalWindowHeight / (float)PatchSizeInPixels);

	u32 PatchSizeInPixelsSqr = PatchSizeInPixels * PatchSizeInPixels;

	float MicroCameraNearPlane = 0.2f;
	// NOTE(hugo) : This works because all microbuffers have the same width (but different heights)
	float MicrobufferWidthInMeters = 2.0f * MicroCameraNearPlane * Tan(0.5f * Radians(90));
	float PixelsToMeters = MicrobufferWidthInMeters / float(State->HemicubeFramebuffer.Width);
	float PixelSurfaceInMeters = PixelsToMeters * PixelsToMeters;

	v3* Normals = AllocateArray(v3, PatchSizeInPixelsSqr);
	float* Depths = AllocateArray(float, PatchSizeInPixelsSqr);

	mat4 InvLookAtCamera = Inverse(LookAt(Camera));

	for(u32 PatchY = 0; PatchY < PatchYCount; ++PatchY)
	{
		for(u32 PatchX = 0; PatchX < PatchXCount; ++PatchX)
		{
			for(u32 BufferIndex = 0; BufferIndex < ArrayCount(State->MegaBuffers); ++BufferIndex)
			{
				BindFramebuffer(State->RenderState, GL_FRAMEBUFFER, State->MegaBuffers[BufferIndex].ID);
				ClearColor(State->RenderState, V4(0.0f, 0.0f, 0.0f, 1.0f));
			}
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
							MicroCameras[MicroCameraIndex].FarPlane = 2.2f;

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

			// NOTE(hugo) : The megatexture is full ! Now we apply the shader
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
			SetUniform(State->Shaders[ShaderType_BRDFConvolutional], PatchWidth, "PatchWidth");
			SetUniform(State->Shaders[ShaderType_BRDFConvolutional], PatchHeight, "PatchHeight");
			SetUniform(State->Shaders[ShaderType_BRDFConvolutional], PatchX, "PatchX");
			SetUniform(State->Shaders[ShaderType_BRDFConvolutional], PatchY, "PatchY");
			SetUniform(State->Shaders[ShaderType_BRDFConvolutional], State->HemicubeFramebuffer.Width, "MicrobufferWidth");
			SetUniform(State->Shaders[ShaderType_BRDFConvolutional], State->HemicubeFramebuffer.Height, "MicrobufferHeight");
			SetUniform(State->Shaders[ShaderType_BRDFConvolutional], Camera.P, "CameraPos");
			SetUniform(State->Shaders[ShaderType_BRDFConvolutional], PixelSurfaceInMeters, "PixelSurfaceInMeters");
			//SetUniform(State->Shaders[ShaderType_BRDFConvolutional], PixelsToMeters, "PixelsToMeters");
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

			SetViewport(State->RenderState, GlobalWindowWidth, GlobalWindowHeight);
			BindFramebuffer(State->RenderState, GL_FRAMEBUFFER, 0);
			RenderTextureOnQuadScreen(State, State->IndirectIlluminationFramebuffer.Texture);
			Assert(!DetectErrors("GI2"));
			SDL_GL_SwapWindow(GlobalWindow);

		}
	}

	Free(Depths);
	Free(Normals);
}


