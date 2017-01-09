#pragma once

#include <imgui_demo.cpp>

static int GlobalShadowWidth = 2 * 1024;
static int GlobalShadowHeight = 2 * 1024;
static int GlobalTeapotInstanceCount = 10;
static u32 GlobalMicrobufferWidth = 128;
static u32 GlobalMicrobufferHeight = 128;

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

GLuint LoadCubemap(game_state* State, const char** Filenames)
{
	GLuint Texture;
	glGenTextures(1, &Texture);

	BindTexture(State->GLState, GL_TEXTURE_CUBE_MAP, Texture);

	for(u32 FaceIndex = 0; FaceIndex < 6; ++FaceIndex)
	{
		bitmap Bitmap = LoadBitmap(Filenames[FaceIndex]);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + FaceIndex, 0, GL_SRGB, Bitmap.Width, Bitmap.Height, 0, GL_RGB, GL_UNSIGNED_BYTE, Bitmap.Data);
		FreeBitmap(&Bitmap);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	BindTexture(State->GLState, GL_TEXTURE_CUBE_MAP, 0);

	return(Texture);
}

mat4 GetLightModelMatrix(light Light)
{
		mat4 ModelMatrix = Translation(Light.Pos) * Scaling(V3(0.2f, 0.2f, 0.2f));

		return(ModelMatrix);
}

mat4 GetCameraPerspective(camera Camera)
{
	mat4 Result = Perspective(Camera.FoV, Camera.Aspect, Camera.NearPlane, Camera.FarPlane);

	return(Result);
}

// TODO(hugo) : A lot of render call recompute the LookAt matrix. Factorize this to compute it only once
void RenderSkybox(game_state* State, v3 CameraPos, v3 CameraTarget, v3 CameraUp, mat4 ProjectionMatrix)
{
	DepthMask(State->GLState, false);
	UseShader(State->GLState, State->SkyboxShader);

	mat4 UntranslatedView = RemoveTranslationPart(LookAt(CameraPos, CameraTarget, CameraUp));
	SetUniform(State->SkyboxShader, ProjectionMatrix, "Projection");
	SetUniform(State->SkyboxShader, UntranslatedView, "View");

	BindVertexArray(State->GLState, State->SkyboxVAO);
	BindTexture(State->GLState, GL_TEXTURE_CUBE_MAP, State->SkyboxTexture);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	BindVertexArray(State->GLState, 0);
	DepthMask(State->GLState, true);
}

// NOTE(hugo) : In order for this function to work, the light depth buffers must have been previously computed
void RenderShadowedScene(game_state* State, 
		v3 CameraPos, v3 CameraTarget, v3 CameraUp, 
		mat4 ProjectionMatrix, 
		mat4 LightProjectionMatrix, 
		rect3* FrustumBoundingBox = 0)
{
	mat4 LightSpaceMatrix[4];

	mat4 ViewMatrix = LookAt(CameraPos, CameraTarget, CameraUp);

	// NOTE(hugo) : Drawing Object Mesh
	mat4 MVPObjectMatrix = ProjectionMatrix * ViewMatrix * State->ObjectModelMatrix;
	mat4 NormalObjectMatrix = Transpose(Inverse(ViewMatrix * State->ObjectModelMatrix));
	mat4 NormalWorldMatrix = Transpose(Inverse(State->ObjectModelMatrix));

	UseShader(State->GLState, State->ShadowMappingShader);

	SetUniform(State->ShadowMappingShader, MVPObjectMatrix, "MVPMatrix");
	SetUniform(State->ShadowMappingShader, NormalObjectMatrix, "NormalMatrix");
	SetUniform(State->ShadowMappingShader, ViewMatrix, "ViewMatrix");
	SetUniform(State->ShadowMappingShader, State->ObjectModelMatrix, "ModelObjectMatrix");
	SetUniform(State->ShadowMappingShader, State->LightCount, "LightCount");
	SetUniform(State->ShadowMappingShader, State->Alpha, "Alpha");
	SetUniform(State->ShadowMappingShader, State->LightIntensity, "LightIntensity");
	SetUniform(State->ShadowMappingShader, NormalWorldMatrix, "NormalWorldMatrix");


	for(u32 LightIndex = 0; LightIndex < State->LightCount; ++LightIndex)
	{
		char Buffer[80];
		char StringNum[2];
		sprintf(StringNum, "%i", LightIndex);
		strcpy(Buffer, "LightPos[");
		strcat(Buffer, StringNum);
		strcat(Buffer, "]");

		SetUniform(State->ShadowMappingShader, State->Lights[LightIndex].Pos, Buffer);

		memset(Buffer, 0, ArrayCount(Buffer));
		strcpy(Buffer, "LightColor[");
		strcat(Buffer, StringNum);
		strcat(Buffer, "]");

		SetUniform(State->ShadowMappingShader, State->Lights[LightIndex].Color, Buffer);

		mat4 LightLookAt = LookAt(State->Lights[LightIndex].Pos, State->Lights[LightIndex].Target, V3(0.0f, 1.0f, 0.0f));
		LightSpaceMatrix[LightIndex] = LightProjectionMatrix * LightLookAt;
		memset(Buffer, 0, ArrayCount(Buffer));
		strcpy(Buffer, "LightSpaceMatrix[");
		strcat(Buffer, StringNum);
		strcat(Buffer, "]");
		SetUniform(State->ShadowMappingShader, LightSpaceMatrix[LightIndex], Buffer);

		memset(Buffer, 0, ArrayCount(Buffer));
		strcpy(Buffer, "ShadowMap[");
		strcat(Buffer, StringNum);
		strcat(Buffer, "]");
		ActiveTexture(State->GLState, GL_TEXTURE0 + LightIndex);
		SetUniform(State->ShadowMappingShader, LightIndex, Buffer);
		BindTexture(State->GLState, GL_TEXTURE_2D, State->Lights[LightIndex].DepthFramebuffer.Texture.ID);
	}

	SetUniform(State->ShadowMappingShader, State->CookTorranceF0, "CTF0");
	SetUniform(State->ShadowMappingShader, State->CookTorranceM, "CTM");


	for(u32 ObjectIndex = 0; ObjectIndex < State->ObjectCount; ++ObjectIndex)
	{
		object* Object = State->Objects + ObjectIndex;
		if(Object->Visible)
		{
			bool ShouldDraw = true;
			if(FrustumBoundingBox)
			{
				ShouldDraw = Intersect3(*FrustumBoundingBox, Object->BoundingBox);
			}
			if(ShouldDraw)
			{
				SetUniform(State->ShadowMappingShader, Object->Albedo, "ObjectColor");
				DrawTriangleObject(State->GLState, Object);
			}
		}
	}
	ActiveTexture(State->GLState, GL_TEXTURE0);

	// NOTE(hugo) : Drawing Light Mesh
	for(u32 LightIndex = 0; LightIndex < State->LightCount; ++LightIndex)
	{
		if(State->Lights[LightIndex].Object)
		{
			mat4 MVPLightMatrix = ProjectionMatrix * ViewMatrix * GetLightModelMatrix(State->Lights[LightIndex]);
			UseShader(State->GLState, State->BasicShader);
			SetUniform(State->BasicShader, MVPLightMatrix, "MVPMatrix");
			SetUniform(State->BasicShader, State->Lights[LightIndex].Color, "ObjectColor");
			DrawTriangleObject(State->GLState, State->Lights[LightIndex].Object);
		}
	}
}

void RenderSimpleScene(game_state* State, v3 CameraPos, v3 CameraTarget, v3 CameraUp, mat4 ProjectionMatrix)
{
	mat4 ViewMatrix = LookAt(CameraPos, CameraTarget, CameraUp);
	mat4 MVPObjectMatrix = ProjectionMatrix * ViewMatrix * State->ObjectModelMatrix;

	UseShader(State->GLState, State->BasicShader);
	SetUniform(State->BasicShader, MVPObjectMatrix, "MVPMatrix");
	//DrawTriangleMeshInstances(&State->ObjectMesh, GlobalTeapotInstanceCount);
	for(u32 ObjectIndex = 0; ObjectIndex < State->ObjectCount; ++ObjectIndex)
	{
		object* Object = State->Objects + ObjectIndex;
		if(Object->Visible)
		{
			DrawTriangleObject(State->GLState, Object);
		}
	}
}

void RenderShadowSceneOnFramebuffer(game_state* State, 
		v3 CameraPos, v3 CameraTarget, v3 CameraUp, 
		mat4 ProjectionMatrix, mat4 LightProjectionMatrix, 
		geometry_framebuffer Framebuffer, 
		v4 ClearColor = V4(0.0f, 0.0f, 0.0f, 1.0f),
		bool SkyboxRender = true,
		rect3* FrustumBoundingBox = 0)
{
	BindFramebuffer(State->GLState, GL_FRAMEBUFFER, Framebuffer.FBO);
	ClearColorAndDepth(State->GLState, ClearColor);
	Enable(State->GLState, GL_DEPTH_TEST);

	RenderShadowedScene(State, 
			CameraPos, CameraTarget, CameraUp, 
			ProjectionMatrix, 
			LightProjectionMatrix, 
			FrustumBoundingBox);
	if(SkyboxRender)
	{
		RenderSkybox(State, CameraPos, CameraTarget, CameraUp, ProjectionMatrix);
	}

	BindFramebuffer(State->GLState, GL_FRAMEBUFFER, 0);

}

void RenderTextureOnQuadScreen(game_state* State, texture Texture)
{
	Assert(Texture.IsValid);
	ClearColor(State->GLState, V4(1.0f, 1.0f, 1.0f, 1.0f));
	glClear(GL_COLOR_BUFFER_BIT);

	// NOTE(hugo) : Quad rendering
	UseShader(State->GLState, State->DepthDebugQuadShader);
	SetUniform(State->DepthDebugQuadShader, State->Sigma, "Sigma");
	BindVertexArray(State->GLState, State->QuadVAO);
	Disable(State->GLState, GL_DEPTH_TEST);
	ActiveTexture(State->GLState, GL_TEXTURE0);
	BindTexture(State->GLState, GL_TEXTURE_2D, Texture.ID);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	Enable(State->GLState, GL_DEPTH_TEST);
	BindTexture(State->GLState, GL_TEXTURE_2D, 0);
	BindVertexArray(State->GLState, 0);
}

void RenderShadowSceneOnQuad(game_state* State, 
		v3 CameraPos, v3 CameraTarget, v3 CameraUp, 
		mat4 ProjectionMatrix, mat4 LightProjectionMatrix, 
		geometry_framebuffer Framebuffer,
		rect3* FrustumBoundingBox = 0)
{
	RenderShadowSceneOnFramebuffer(State, 
			CameraPos, CameraTarget, CameraUp, 
			ProjectionMatrix, LightProjectionMatrix, 
			Framebuffer, V4(1.0f, 0.0f, 0.5f, 1.0f), 
			true,
			FrustumBoundingBox);
	RenderTextureOnQuadScreen(State, Framebuffer.ScreenTexture);
}

v3 ComputePositionOfPixel(camera Camera, u32 PixelX, u32 PixelY, 
		float PixelsToMeters, mat4 InvMicroCameraLookAt, 
		u32 BufferWidth, u32 BufferHeight)
{
	s32 PixelPosX = s32(PixelX) - 0.5f * float(BufferWidth);
	s32 PixelPosY = s32(PixelY) - 0.5f * float(BufferHeight);
	v3 PixelCameraPos = {};
	PixelCameraPos.x = PixelPosX * PixelsToMeters;
	PixelCameraPos.y = PixelPosY * PixelsToMeters;
	PixelCameraPos.z = - Camera.NearPlane;

	v3 PixelWorldPos = (InvMicroCameraLookAt * ToV4(PixelCameraPos)).xyz;

	return(PixelWorldPos);
}

float FresnelSchlickFactor(float F0, float LightDirDotHalfDir)
{
	float Result = F0 + (1.0f - F0) * Power((1.0f - LightDirDotHalfDir), 5);
	return(Result);
}

float GGXDistributionTerm(float AlphaSqr, float NormalDotHalfDir)
{
	float Denominator = ((NormalDotHalfDir * NormalDotHalfDir) * (AlphaSqr - 1.0f)) + 1.0f;
	Denominator = PI * Denominator * Denominator;
	float Result = AlphaSqr / Denominator;

	return(Result);
}

float GGXBRDF(v3 Normal, v3 LightDir, v3 HalfDir, float NormalDotViewDir, float Alpha, float CookTorranceF0)
{
	float NormalDotHalfDir = DotClamp(Normal, HalfDir);
	float NormalDotLightDir = DotClamp(Normal, LightDir);
	float LightDirDotHalfDir = DotClamp(LightDir, HalfDir);

	float AlphaSqr = Alpha * Alpha;
	float F = FresnelSchlickFactor(CookTorranceF0, LightDirDotHalfDir);
	float D = GGXDistributionTerm(AlphaSqr, NormalDotHalfDir);
	float OneOverGL = NormalDotLightDir + sqrt(AlphaSqr + ((1.0f - AlphaSqr) * (NormalDotLightDir * NormalDotLightDir)));
	float OneOverGV = NormalDotViewDir + sqrt(AlphaSqr + ((1.0f - AlphaSqr) * (NormalDotViewDir * NormalDotViewDir)));

	float DiffuseFactor = 0.0f;
	float Result = DiffuseFactor + ((F * D) / (OneOverGL * OneOverGV));

	return(Result);
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

v3 NormalizedLInf(v3 A)
{
	v3 Result = {};
	float MaxComponent = 0.0f;
	for(u32 Index = 0; Index < ArrayCount(A.E); ++Index)
	{
		if(Abs(A.E[Index]) > MaxComponent)
		{
			MaxComponent = Abs(A.E[Index]);
		}
	}

	Assert(MaxComponent != 0.0f);

	Result = A / MaxComponent;

	return(Result);
}

v4 NormalizedLInf(v4 A)
{
	v4 Result = {};
	float MaxComponent = 0.0f;
	for(u32 Index = 0; Index < ArrayCount(A.E); ++Index)
	{
		if(Abs(A.E[Index]) > MaxComponent)
		{
			MaxComponent = Abs(A.E[Index]);
		}
	}

	Assert(MaxComponent != 0.0f);

	Result = A / MaxComponent;

	return(Result);
}

void ComputeGlobalIllumination(game_state* State, camera Camera, v3 CameraUp, mat4 LightProjectionMatrix)
{
	if((State->HemicubeFramebuffer.Width != GlobalMicrobufferWidth) ||
			(State->HemicubeFramebuffer.Height != GlobalMicrobufferHeight))
	{
		UpdateHemicubeScreenFramebuffer(State->GLState, &State->HemicubeFramebuffer, GlobalMicrobufferWidth, GlobalMicrobufferHeight);
	}
	u32* AlbedoPixels = AllocateArray(u32, GlobalWindowWidth * GlobalWindowHeight);
	float* DepthPixels = AllocateArray(float, GlobalWindowWidth * GlobalWindowHeight);
	BindFramebuffer(State->GLState, GL_FRAMEBUFFER, State->ScreenFramebuffer.FBO);

	State->IndirectIlluminationBuffer = AllocateArray(u32, GlobalWindowWidth * GlobalWindowHeight);

	u32* Pixels = AllocateArray(u32, GlobalMicrobufferWidth * GlobalMicrobufferHeight);

	// NOTE(hugo) : Reading depth of touched pixel
	// Another possibility would be to store the position map in
	// the GBuffer. This query would take all three composants and would
	// not require any unprojection. However more space is needed.
	// I think I prefer unproject pixel for now
	glReadPixels(0, 0, GlobalWindowWidth, GlobalWindowHeight, GL_DEPTH_COMPONENT, GL_FLOAT, DepthPixels);

	// NOTE(hugo) : Reading albedo of touched pixel
	ReadBuffer(State->GLState, GL_COLOR_ATTACHMENT2);
	glReadPixels(0, 0, GlobalWindowWidth, GlobalWindowHeight, GL_RGBA, GL_UNSIGNED_BYTE, AlbedoPixels);

	// NOTE(hugo) : Reading normal of touched pixel
	ReadBuffer(State->GLState, GL_COLOR_ATTACHMENT1);
	v3* Normals = AllocateArray(v3, GlobalWindowWidth * GlobalWindowHeight);
	glReadPixels(0, 0, GlobalWindowWidth, GlobalWindowHeight, GL_RGB, GL_FLOAT, Normals);

	ReadBuffer(State->GLState, GL_COLOR_ATTACHMENT0);
	u32* ScreenBuffer = AllocateArray(u32, GlobalWindowWidth * GlobalWindowHeight);
	glReadPixels(0, 0, GlobalWindowWidth, GlobalWindowHeight, GL_RGBA, GL_UNSIGNED_BYTE, ScreenBuffer);
	BindFramebuffer(State->GLState, GL_FRAMEBUFFER, 0);

	for(u32 Y = 0; Y < (u32)(GlobalWindowHeight); ++Y)
	{
		for(u32 X = 0; X < (u32)(GlobalWindowWidth); ++X)
		{
			float PixelDepth = DepthPixels[GlobalWindowWidth * Y + X];
			u32 PixelAlbedo = AlbedoPixels[GlobalWindowWidth * Y + X];
			v3 Normal = Normals[GlobalWindowWidth * Y + X];

			v4 Albedo = ColorU32ToV4(PixelAlbedo);

			BindFramebuffer(State->GLState, GL_FRAMEBUFFER, 0);

			float NearPlane = State->Camera.NearPlane;
			float FarPlane = State->Camera.FarPlane;
			PixelDepth = 2.0f * PixelDepth - 1.0f;
			// TODO(hugo) : I don't think this next computation works if I am using an orthographic projection
			PixelDepth = 2.0f * NearPlane * FarPlane / (NearPlane + FarPlane - PixelDepth * (FarPlane - NearPlane));

			mat4 InvLookAtCamera = Inverse(LookAt(Camera.Pos, Camera.Target, CameraUp));

			Normal = Normalized(2.0f * Normal - V3(1.0f, 1.0f, 1.0f));
			v3 WorldUp = V3(0.0f, 1.0f, 0.0f);

			// NOTE(hugo) : Render directions are, in order : FRONT / LEFT / RIGHT / TOP / BOTTOM
			// with FRONT being the direction of the normal previously found;
			camera MicroCameras[5];
			mat4 MicroCameraProjections[5];
			{
				v4 PixelPos = {};
				PixelPos.z = -PixelDepth;
				PixelPos.w = 1.0f;

				PixelPos.x = (float)(X) / float(GlobalWindowWidth);
				PixelPos.y = (float)(Y) / float(GlobalWindowHeight);

				PixelPos.xy = 2.0f * PixelPos.xy - V2(1.0f, 1.0f);

				PixelPos.x = - State->Camera.Aspect * Tan(0.5f * State->Camera.FoV) * PixelPos.z * PixelPos.x;
				PixelPos.y = - Tan(0.5f * State->Camera.FoV) * PixelPos.z * PixelPos.y;

				PixelPos = InvLookAtCamera * PixelPos;
				v3 MicroCameraPos = PixelPos.xyz;

				v3 MicroRenderDir[5];
				MicroRenderDir[0] = Normal;
				MicroRenderDir[1] = Cross(Normal, WorldUp);
				MicroRenderDir[2] = -1.0f * MicroRenderDir[1];
				MicroRenderDir[3] = Cross(Normal, MicroRenderDir[1]);
				MicroRenderDir[4] = -1.0f * MicroRenderDir[3];
				for(u32 MicroCameraIndex = 0; MicroCameraIndex < ArrayCount(MicroCameras); ++MicroCameraIndex)
				{
					MicroCameras[MicroCameraIndex].Pos = MicroCameraPos;
					MicroCameras[MicroCameraIndex].Target = MicroCameraPos + MicroRenderDir[MicroCameraIndex];

					v3 MicroCameraUp = WorldUp;
					if(MicroCameraIndex != 0)
					{
						MicroCameraUp = Normal;
					}

					MicroCameras[MicroCameraIndex].Right = Normalized(Cross(MicroRenderDir[MicroCameraIndex], MicroCameraUp));
					MicroCameras[MicroCameraIndex].FoV = Radians(State->MicroFoVInDegrees);
					MicroCameras[MicroCameraIndex].Aspect = float(State->HemicubeFramebuffer.MicroBuffers[MicroCameraIndex].Width) / float(State->HemicubeFramebuffer.MicroBuffers[MicroCameraIndex].Height);
					MicroCameras[MicroCameraIndex].NearPlane = 0.1f * State->Camera.NearPlane;
					MicroCameras[MicroCameraIndex].FarPlane = 0.3f * State->Camera.FarPlane;

					MicroCameraProjections[MicroCameraIndex] = GetCameraPerspective(MicroCameras[MicroCameraIndex]);
				}
			}


			for(u32 FaceIndex = 0; FaceIndex < ArrayCount(MicroCameras); ++FaceIndex)
			{
				camera MicroCamera = MicroCameras[FaceIndex];
				SetViewport(State->GLState, State->HemicubeFramebuffer.MicroBuffers[FaceIndex].Width, 
						State->HemicubeFramebuffer.MicroBuffers[FaceIndex].Height);
				RenderShadowSceneOnFramebuffer(State, MicroCamera.Pos, MicroCamera.Target, 
						Cross(MicroCamera.Right, MicroCamera.Target - MicroCamera.Pos), MicroCameraProjections[FaceIndex], 
						LightProjectionMatrix, 
						State->HemicubeFramebuffer.MicroBuffers[FaceIndex],
						V4(0.0f, 0.0f, 0.0f, 1.0f), false);

			}
			SetViewport(State->GLState, GlobalWindowWidth, GlobalWindowHeight);

#if 0
			for(u32 FaceIndex = 0; FaceIndex < ArrayCount(State->HemicubeFramebuffer.MicroBuffers); ++FaceIndex)
			{
				RenderTextureOnQuadScreen(State, State->HemicubeFramebuffer.MicroBuffers[FaceIndex].ScreenTexture);
				SDL_GL_SwapWindow(GlobalWindow);
				SDL_Delay(1000);
			}
#endif

			// NOTE(hugo) : Now we have the whole hemicube rendered. We can sample it to 
			v4 ColorBleeding = {};

			// NOTE(hugo) : This works because all microbuffers have the same width (but different heights)
			float MicrobufferWidthInMeters = 2.0f * MicroCameras[0].NearPlane * Tan(0.5f * MicroCameras[0].FoV);
			float PixelsToMeters = MicrobufferWidthInMeters / float(State->HemicubeFramebuffer.MicroBuffers[0].Width);
			float PixelSurfaceInMeters = PixelsToMeters * PixelsToMeters;

			for(u32 FaceIndex = 0; FaceIndex < ArrayCount(MicroCameras); ++FaceIndex)
			{
				geometry_framebuffer Microbuffer = State->HemicubeFramebuffer.MicroBuffers[FaceIndex];

				BindFramebuffer(State->GLState, GL_FRAMEBUFFER, Microbuffer.FBO);
				ReadBuffer(State->GLState, GL_COLOR_ATTACHMENT0);
				glReadPixels(0, 0, Microbuffer.Width, Microbuffer.Height, GL_RGBA, GL_UNSIGNED_BYTE, Pixels);
				BindFramebuffer(State->GLState, GL_FRAMEBUFFER, 0);

				v3 Wo = Normalized(Camera.Pos - MicroCameras[FaceIndex].Pos);
				float NormalDotWo = DotClamp(Normal, Wo);
				camera MicroCamera = MicroCameras[FaceIndex];
				v3 MicroCameraLookingDir = Normalized(MicroCamera.Target - MicroCamera.Pos);
				v3 MicroCameraUp = Cross(MicroCamera.Right, MicroCameraLookingDir);
				mat4 InvMicroCameraLookAt = Inverse(LookAt(MicroCamera.Pos, MicroCamera.Target, MicroCameraUp));

				for(u32 PixelX = 0; PixelX < Microbuffer.Width; ++PixelX)
				{
					u32 StartingRow = 0;
					if(FaceIndex > 0)
					{
						StartingRow = 0.5f * Microbuffer.Height;
					}

					for(u32 PixelY = StartingRow; PixelY < Microbuffer.Height; ++PixelY)
					{
						v3 PixelWorldPos = ComputePositionOfPixel(MicroCamera, PixelX, PixelY, 
									PixelsToMeters, InvMicroCameraLookAt, 
									Microbuffer.Width, Microbuffer.Height);
						//Assert(DotClamp(Normal, Wi) > 0.0f);
						float DistanceMiroCameraPixelSqr = LengthSqr(MicroCamera.Pos - PixelWorldPos);
						v3 Wi = (PixelWorldPos - MicroCamera.Pos) / (sqrt(DistanceMiroCameraPixelSqr));

						// NOTE(hugo) : In theory, every pixel I sample from
						// should have a dot product strictly positive
						// but I guess there are a numerical error at stake.
						// I chose to ignore it because, even it there are pixels that should be sampled
						// from, their dot product is so small that their impact on the color
						// bleeding is negligeable
						if(DotClamp(Normal, Wi) > 0.0f)
						{
							// NOTE(hugo) : From OpenGL documentation on glReadPixels :
							// the pixels are given beginning in the lower left corner by rows
							v4 PixelColor = ColorU32ToV4(Pixels[Microbuffer.Width * PixelY + PixelX]);

							if(LengthSqr(PixelColor.rgb) > 0.0f)
							{
								v3 H = Normalized(0.5f * (Wi + Wo));
								float SolidAngle = (PixelSurfaceInMeters / DistanceMiroCameraPixelSqr) 
									* Dot(Wi, MicroCameraLookingDir);
								float BRDF = GGXBRDF(Normal, Wi, H, NormalDotWo, State->Alpha, State->CookTorranceF0);
								// TODO(hugo) : Check earlier if Albedo.rgb == 0
								ColorBleeding += BRDF * DotClamp(Normal, Wi) * SolidAngle * Hadamard(Albedo, PixelColor);
							}
						}
					}
				}

			}

			v4 DirectIlluminationColor = ColorU32ToV4(ScreenBuffer[GlobalWindowWidth * Y + X]);
			v3 RealColorBleeding = ColorBleeding.rgb + DirectIlluminationColor.rgb;
			RealColorBleeding = Clamp01(RealColorBleeding);

			u32 ColorBleedingPixel = ColorV4ToU32(ToV4(RealColorBleeding));
			//State->IndirectIlluminationBuffer[GlobalWindowWidth * Y + X] = ColorBleedingPixel;
			ScreenBuffer[GlobalWindowWidth * Y + X] = ColorBleedingPixel;
			if(X == 0 && (Y % 1 == 0))
			{
				image_texture_loading_params Params = DefaultImageTextureLoadingParams(GlobalWindowWidth, GlobalWindowHeight, ScreenBuffer);
				LoadImageToTexture(State->GLState, &State->IndirectIlluminationTexture, Params);

				BindFramebuffer(State->GLState, GL_FRAMEBUFFER, 0);
				RenderTextureOnQuadScreen(State, 
						State->IndirectIlluminationTexture);

				SDL_GL_SwapWindow(GlobalWindow);
			}
		}
	}

	// NOTE(hugo) : Saving the computed image to disk.
	{
		// NOTE(hugo) : Post-process of screenshot image
		u32* ScreenshotBuffer = AllocateArray(u32, GlobalWindowWidth * GlobalWindowHeight);
		for(u32 Y = 0; Y < GlobalWindowHeight; ++Y)
		{
			for(u32 X = 0; X < GlobalWindowWidth; ++X)
			{
				v4 UncorrectedColor = ColorU32ToV4(ScreenBuffer[GlobalWindowWidth * (GlobalWindowHeight - Y - 1) + X]);
				UncorrectedColor = SquareRoot4(UncorrectedColor);
				ScreenshotBuffer[GlobalWindowWidth * Y + X] = ColorV4ToU32(UncorrectedColor);
			}
		}

		u32 Stride = sizeof(u32) * GlobalWindowWidth;
		stbi_write_png("indirect_illumination.png",
				GlobalWindowWidth, GlobalWindowHeight,
				4, ScreenshotBuffer, Stride);
		Free(ScreenshotBuffer);
	}

	Free(Pixels);
	Free(Normals);
	Free(DepthPixels);
	Free(AlbedoPixels);
	Free(ScreenBuffer);
	Free(State->IndirectIlluminationBuffer);
#if 0
	//ImGui::ColorEdit3("Color Picked", ColorFloat.E);
	ImGui::Value("Depth @ Pixel", PixelDepth);
	//ImGui::Text("Position pointed at screen : (%f, %f, %f, %f)", PixelPos.x, PixelPos.y, PixelPos.z, PixelPos.w);
	ImGui::Text("Normal pointed at screen : (%f, %f, %f)", Normal.x, Normal.y, Normal.z);
	//SDL_Delay(2000);
#endif
}

v4 UnprojectPixel(float PixelDepth, u32 X, u32 Y, u32 Width, u32 Height,
		camera Camera, mat4 InvLookAtCamera)
{
	v4 PixelPos = {};
	PixelPos.z = -PixelDepth;
	PixelPos.w = 1.0f;

	PixelPos.x = (float)(X) / float(Width);
	PixelPos.y = (float)(Y) / float(Height);

	PixelPos.xy = 2.0f * PixelPos.xy - V2(1.0f, 1.0f);

	PixelPos.x = - Camera.Aspect * Tan(0.5f * Camera.FoV) * PixelPos.z * PixelPos.x;
	PixelPos.y = - Tan(0.5f * Camera.FoV) * PixelPos.z * PixelPos.y;

	PixelPos = InvLookAtCamera * PixelPos;
	return(PixelPos);
}

void ComputeGlobalIlluminationWithPatch(game_state* State, 
		camera Camera, 
		v3 CameraUp, 
		mat4 LightProjectionMatrix,
		u32 PatchSizeInPixels = 32)
{
	if((State->HemicubeFramebuffer.Width != GlobalMicrobufferWidth) ||
			(State->HemicubeFramebuffer.Height != GlobalMicrobufferHeight))
	{
		UpdateHemicubeScreenFramebuffer(State->GLState, &State->HemicubeFramebuffer, GlobalMicrobufferWidth, GlobalMicrobufferHeight);
	}

	u32 PatchXCount = Ceil(GlobalWindowWidth / (float)PatchSizeInPixels);
	u32 PatchYCount = Ceil(GlobalWindowHeight / (float)PatchSizeInPixels);

	u32 PatchSizeInPixelsSqr = PatchSizeInPixels * PatchSizeInPixels;

	float MicroCameraNearPlane = 0.2f;
	// NOTE(hugo) : This works because all microbuffers have the same width (but different heights)
	float MicrobufferWidthInMeters = 2.0f * MicroCameraNearPlane * Tan(0.5f * Radians(90));
	float PixelsToMeters = MicrobufferWidthInMeters / float(State->HemicubeFramebuffer.Width);
	float PixelSurfaceInMeters = PixelsToMeters * PixelsToMeters;

	for(u32 PatchY = 0; PatchY < PatchYCount; ++PatchY)
	{
		for(u32 PatchX = 0; PatchX < PatchXCount; ++PatchX)
		{
			u32* ScreenPatch = AllocateArray(u32, PatchSizeInPixelsSqr);
			v3* Normals = AllocateArray(v3, PatchSizeInPixelsSqr);
			u32* AlbedoPixels = AllocateArray(u32, PatchSizeInPixelsSqr);
			float* Depths = AllocateArray(float, PatchSizeInPixelsSqr);

			BindFramebuffer(State->GLState, GL_FRAMEBUFFER, State->ScreenFramebuffer.FBO);
			glReadPixels(PatchX * PatchSizeInPixels, PatchY * PatchSizeInPixels, PatchSizeInPixels, PatchSizeInPixels,
					GL_DEPTH_COMPONENT, GL_FLOAT, Depths);

			ReadBuffer(State->GLState, GL_COLOR_ATTACHMENT0);
			glReadPixels(PatchX * PatchSizeInPixels, PatchY * PatchSizeInPixels, PatchSizeInPixels, PatchSizeInPixels,
					GL_RGBA, GL_UNSIGNED_BYTE, ScreenPatch);

			ReadBuffer(State->GLState, GL_COLOR_ATTACHMENT1);
			glReadPixels(PatchX * PatchSizeInPixels, PatchY * PatchSizeInPixels, PatchSizeInPixels, PatchSizeInPixels,
					GL_RGB, GL_FLOAT, Normals);

			ReadBuffer(State->GLState, GL_COLOR_ATTACHMENT2);
			glReadPixels(PatchX * PatchSizeInPixels, PatchY * PatchSizeInPixels, PatchSizeInPixels, PatchSizeInPixels,
					GL_RGBA, GL_UNSIGNED_BYTE, AlbedoPixels);

			mat4 InvLookAtCamera = Inverse(LookAt(Camera.Pos, Camera.Target, CameraUp));
#if 0
			v4* WorldPositionPatch = AllocateArray(v4, PatchSizeInPixelsSqr);
			for(u32 Y = 0; Y < PatchSizeInPixels; ++Y)
			{
				for(u32 X = 0; X < PatchSizeInPixels; ++X)
				{
					float PixelDepth = Depths[Y * PatchSizeInPixels + X];
					PixelDepth = 2.0f * PixelDepth - 1.0f;
					// TODO(hugo) : I don't think this next computation works if I am using an orthographic projection
					float NearPlane = State->Camera.NearPlane;
					float FarPlane = State->Camera.FarPlane;
					PixelDepth = 2.0f * NearPlane * FarPlane / (NearPlane + FarPlane - PixelDepth * (FarPlane - NearPlane));
					v4 UnprojectedPixel = UnprojectPixel(PixelDepth,
							X + PatchX * PatchSizeInPixels, Y + PatchY * PatchSizeInPixels,
							GlobalWindowWidth, GlobalWindowHeight, 
							State->Camera, InvLookAtCamera);
					WorldPositionPatch[Y * PatchSizeInPixels + X] = UnprojectedPixel;
				}
			}
#endif

			// NOTE(hugo) : Create textures
			texture NormalPatchTexture = CreateTexture();
			image_texture_loading_params Params = DefaultImageTextureLoadingParams(
					PatchSizeInPixels, PatchSizeInPixels, Normals);
			Params.ExternalFormat = GL_RGB;
			Params.ExternalType = GL_FLOAT;
			Params.MinFilter = GL_LINEAR;
			Params.MagFilter = GL_LINEAR;
			LoadImageToTexture(State->GLState, &NormalPatchTexture, Params);

			texture AlbedoPatchTexture = CreateTexture();
			Params = DefaultImageTextureLoadingParams(
					PatchSizeInPixels, PatchSizeInPixels, AlbedoPixels);
			Params.MinFilter = GL_LINEAR;
			Params.MagFilter = GL_LINEAR;
			LoadImageToTexture(State->GLState, &AlbedoPatchTexture, Params);

			SetViewport(State->GLState, PatchSizeInPixels, PatchSizeInPixels);
			BindFramebuffer(State->GLState, GL_FRAMEBUFFER, 0);
			RenderTextureOnQuadScreen(State, AlbedoPatchTexture);
			SDL_GL_SwapWindow(GlobalWindow);

#if 0
			texture WorldPositionPatchTexture = CreateTexture();
			Params = DefaultImageTextureLoadingParams(
					PatchSizeInPixels, PatchSizeInPixels, WorldPositionPatch);
			Params.ExternalType = GL_FLOAT;
			LoadImageToTexture(State->GLState, &WorldPositionPatchTexture, Params);
#else
			texture DepthPatchTexture = CreateTexture();
			Params = DefaultImageTextureLoadingParams(
					PatchSizeInPixels, PatchSizeInPixels, Depths);
			Params.InternalFormat = GL_DEPTH_COMPONENT;
			Params.ExternalFormat = GL_DEPTH_COMPONENT;
			Params.ExternalType = GL_FLOAT;
			Params.MinFilter = GL_LINEAR;
			Params.MagFilter = GL_LINEAR;
			LoadImageToTexture(State->GLState, &DepthPatchTexture, Params);
#endif

			u32* MegaTexture = AllocateArray(u32, PatchSizeInPixels * PatchSizeInPixels * GlobalMicrobufferWidth * GlobalMicrobufferHeight * 5);
			u32* MegaTexturePixel = MegaTexture;
			v3 WorldUp = V3(0.0f, 1.0f, 0.0f);

			float NearPlane = Camera.NearPlane;
			float FarPlane = Camera.FarPlane;
			// TODO(hugo) : Make sure you don't go too far off the screen
			for(u32 Y = 0; Y < PatchSizeInPixels; ++Y)
			{
				for(u32 X = 0; X < PatchSizeInPixels; ++X)
				{
					//u32 PixelIndex = X + PatchX * PatchSizeInPixels + (Y + PatchY * PatchSizeInPixels) * GlobalWindowWidth;
					u32 PixelIndex = X + PatchSizeInPixels * Y;
					float PixelDepth = Depths[PixelIndex];
					u32 PixelAlbedo = AlbedoPixels[PixelIndex];
					v3 Normal = Normals[PixelIndex];

					v4 Albedo = ColorU32ToV4(PixelAlbedo);

					PixelDepth = 2.0f * PixelDepth - 1.0f;
					// TODO(hugo) : I don't think this next computation works if I am using an orthographic projection
					PixelDepth = 2.0f * NearPlane * FarPlane / (NearPlane + FarPlane - PixelDepth * (FarPlane - NearPlane));

					mat4 InvLookAtCamera = Inverse(LookAt(Camera.Pos, Camera.Target, CameraUp));

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
							MicroCameras[MicroCameraIndex].Pos = MicroCameraPos;
							MicroCameras[MicroCameraIndex].Target = MicroCameraPos + MicroRenderDir[MicroCameraIndex];

							v3 MicroCameraUp = WorldUp;
							if(MicroCameraIndex != 0)
							{
								MicroCameraUp = Normal;
							}

							MicroCameras[MicroCameraIndex].Right = Normalized(Cross(MicroRenderDir[MicroCameraIndex], MicroCameraUp));
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
						SetViewport(State->GLState, State->HemicubeFramebuffer.MicroBuffers[FaceIndex].Width, 
								State->HemicubeFramebuffer.MicroBuffers[FaceIndex].Height);
						RenderShadowSceneOnFramebuffer(State, MicroCamera.Pos, MicroCamera.Target, 
								Cross(MicroCamera.Right, MicroCamera.Target - MicroCamera.Pos), MicroCameraProjections[FaceIndex], 
								LightProjectionMatrix, 
								State->HemicubeFramebuffer.MicroBuffers[FaceIndex],
								V4(0.0f, 0.0f, 0.0f, 1.0f), false);

						BindFramebuffer(State->GLState, 
								GL_FRAMEBUFFER, 
								State->HemicubeFramebuffer.MicroBuffers[FaceIndex].FBO);

						u32* FaceRenderedPixels = AllocateArray(u32, 
								State->HemicubeFramebuffer.Width * State->HemicubeFramebuffer.Height);
						glReadPixels(0, 0, State->HemicubeFramebuffer.Width,
								State->HemicubeFramebuffer.Height,
								GL_RGBA, GL_UNSIGNED_BYTE,
								FaceRenderedPixels);
						BindFramebuffer(State->GLState, GL_FRAMEBUFFER, 0);

						for(u32 PixelIndex = 0; 
								PixelIndex < State->HemicubeFramebuffer.Width * State->HemicubeFramebuffer.Height; 
								++PixelIndex)
						{
							*MegaTexturePixel = *(FaceRenderedPixels + PixelIndex);
							++MegaTexturePixel;
						}

						Free(FaceRenderedPixels);
					}
				}
			}

			texture Mega = CreateTexture();
			Params = DefaultImageTextureLoadingParams(
					PatchSizeInPixels * PatchSizeInPixels * GlobalMicrobufferWidth * GlobalMicrobufferHeight * 5, 
					1,
					MegaTexture);
			Params.MinFilter = GL_NEAREST;
			LoadImageToTexture(State->GLState, &Mega, Params);

			// NOTE(hugo) : The megatexture is full ! Now we apply the shader
			UseShader(State->GLState, State->BRDFConvShader);

			ActiveTexture(State->GLState, GL_TEXTURE0);
			SetUniform(State->BRDFConvShader, (u32)0, "MegaTexture");
			BindTexture(State->GLState, GL_TEXTURE_2D, Mega.ID);
			
#if 0
			ActiveTexture(State->GLState, GL_TEXTURE1);
			SetUniform(State->BRDFConvShader, (u32)1, "WorldPositionPatch");
			BindTexture(State->GLState, GL_TEXTURE_2D, WorldPositionPatchTexture.ID);
#else
			ActiveTexture(State->GLState, GL_TEXTURE1);
			SetUniform(State->BRDFConvShader, (u32)1, "DepthPatch");
			BindTexture(State->GLState, GL_TEXTURE_2D, DepthPatchTexture.ID);
#endif

			ActiveTexture(State->GLState, GL_TEXTURE2);
			SetUniform(State->BRDFConvShader, (u32)2, "NormalPatch");
			BindTexture(State->GLState, GL_TEXTURE_2D, NormalPatchTexture.ID);

			ActiveTexture(State->GLState, GL_TEXTURE3);
			SetUniform(State->BRDFConvShader, (u32)3, "AlbedoPatch");
			BindTexture(State->GLState, GL_TEXTURE_2D, AlbedoPatchTexture.ID);

			// TODO(hugo) : Check that every uniform has been set
			SetUniform(State->BRDFConvShader, PatchSizeInPixels, "PatchSizeInPixels");
			SetUniform(State->BRDFConvShader, PatchX, "PatchX");
			SetUniform(State->BRDFConvShader, PatchY, "PatchY");
			SetUniform(State->BRDFConvShader, State->HemicubeFramebuffer.Width, "MicrobufferWidth");
			SetUniform(State->BRDFConvShader, State->HemicubeFramebuffer.Height, "MicrobufferHeight");
			SetUniform(State->BRDFConvShader, Camera.Pos, "CameraPos");
			SetUniform(State->BRDFConvShader, PixelSurfaceInMeters, "PixelSurfaceInMeters");
			SetUniform(State->BRDFConvShader, PixelsToMeters, "PixelsToMeters");
			SetUniform(State->BRDFConvShader, State->Alpha, "Alpha");
			SetUniform(State->BRDFConvShader, State->CookTorranceF0, "CookTorranceF0");
			SetUniform(State->BRDFConvShader, MicroCameraNearPlane, "MicroCameraNearPlane");
			SetUniform(State->BRDFConvShader, WorldUp, "WorldUp");
			SetUniform(State->BRDFConvShader, Camera.Aspect, "MainCameraAspect");
			SetUniform(State->BRDFConvShader, Camera.FoV, "MainCameraFoV");
			SetUniform(State->BRDFConvShader, Camera.NearPlane, "MainCameraNearPlane");
			SetUniform(State->BRDFConvShader, Camera.FarPlane, "MainCameraFarPlane");
			SetUniform(State->BRDFConvShader, InvLookAtCamera, "InvLookAtCamera");
			SetUniform(State->BRDFConvShader, GlobalWindowWidth, "WindowWidth");
			SetUniform(State->BRDFConvShader, GlobalWindowHeight, "WindowHeight");

			SetViewport(State->GLState, PatchSizeInPixels, PatchSizeInPixels);
			BindFramebuffer(State->GLState, GL_FRAMEBUFFER, 0);
			BindVertexArray(State->GLState, State->QuadVAO);
			Disable(State->GLState, GL_DEPTH_TEST);
			//ActiveTexture(State->GLState, GL_TEXTURE0);
			//BindTexture(State->GLState, GL_TEXTURE_2D, Mega.ID);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			Enable(State->GLState, GL_DEPTH_TEST);
			//BindTexture(State->GLState, GL_TEXTURE_2D, 0);
			BindVertexArray(State->GLState, 0);

			SDL_GL_SwapWindow(GlobalWindow);

			DeleteTexture(&NormalPatchTexture);
			DeleteTexture(&AlbedoPatchTexture);
			DeleteTexture(&DepthPatchTexture);
			DeleteTexture(&Mega);
			Free(MegaTexture);

			Free(Depths);
			Free(AlbedoPixels);
			Free(Normals);
			Free(ScreenPatch);
		}
	}
}

void PushObject(game_state* State, object* Object)
{
	Assert(State->ObjectCount < ArrayCount(State->Objects));

	State->Objects[State->ObjectCount] = *Object;
	State->ObjectCount++;
}

void PushLight(game_state* State, light Light)
{
	Assert(State->LightCount < ArrayCount(State->Lights));

	State->Lights[State->LightCount] = Light;
	State->LightCount++;
}

void LoadShaders(game_state* State)
{
	State->BasicShader = LoadShader("../src/shaders/basic_v.glsl", "../src/shaders/basic_f.glsl");
	State->DepthDebugQuadShader = LoadShader("../src/shaders/depth_debug_quad_v.glsl", "../src/shaders/depth_debug_quad_f.glsl");
	State->ShadowMappingShader = LoadShader("../src/shaders/shadow_mapping_v.glsl", "../src/shaders/shadow_mapping_f.glsl");
	State->SkyboxShader = LoadShader("../src/shaders/skybox_v.glsl", "../src/shaders/skybox_f.glsl");
	State->BRDFConvShader = LoadShader("../src/shaders/depth_debug_quad_v.glsl", "../src/shaders/brdf_conv_f.glsl");
}

rect3 GetFrustumBoundingBox(camera Camera)
{
	rect3 Result = {};
	v3 P[8];

	P[0] = {};
	P[0].x = Camera.FarPlane * Tan(0.5f * Camera.FoV);
	P[0].y = Camera.Aspect * P[0].x;
	P[0].z = -Camera.FarPlane;

	P[1] = P[0];
	P[1].x = -P[1].x;

	P[2] = P[0];
	P[2].y = -P[2].y;

	P[3] = P[0];
	P[3].x = -P[3].x;
	P[3].y = -P[3].y;

	P[4] = {};
	P[4].x = Camera.NearPlane * Tan(0.5f * Camera.FoV);
	P[4].y = Camera.Aspect * P[4].x;
	P[4].z = -Camera.NearPlane;

	P[5] = P[4];
	P[5].x = -P[5].x;

	P[6] = P[4];
	P[6].y = -P[6].y;

	P[7] = P[4];
	P[7].x = -P[7].x;
	P[7].y = -P[7].y;

	v3 CameraUp = Cross(Camera.Right, Normalized(Camera.Target - Camera.Pos));
	mat4 InvLookAt = Inverse(LookAt(Camera.Pos, 
				Camera.Target, 
				CameraUp));

	for(u32 PointIndex = 0; PointIndex < ArrayCount(P); ++PointIndex)
	{
		AddPointToBoundingBox(&Result, (InvLookAt * ToV4(P[PointIndex])).xyz);
	}

	return(Result);
}

void GameUpdateAndRender(game_memory* Memory, game_input* Input, render_state* OpenGLState)
{
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	game_state* State = (game_state*)Memory->PermanentStorage;
	if(!Memory->IsInitialized)
	{
		State->GLState = OpenGLState;
		rect3 Box = MaxBoundingBox();
		{
			std::vector<object> Objects = LoadOBJ("../models/cornell_box/", "CornellBox-Original.obj");
			//std::vector<object> Objects = LoadOBJ("../models/house/", "house.obj");
			//std::vector<object> Objects = LoadOBJ("../models/sponza/", "sponza.obj");
			for(u32 ObjectIndex = 0; ObjectIndex < Objects.size(); ++ObjectIndex)
			{
				PushObject(State, &Objects[ObjectIndex]);

				// NOTE(hugo) : Computing bounding box
				rect3 ObjectBox = State->Objects[ObjectIndex].BoundingBox;
				if(ObjectBox.Max.x > Box.Max.x)
				{
					Box.Max.x = ObjectBox.Max.x;
				}
				if(ObjectBox.Min.x < Box.Min.x)
				{
					Box.Min.x = ObjectBox.Min.x;
				}

				if(ObjectBox.Max.y > Box.Max.y)
				{
					Box.Max.y = ObjectBox.Max.y;
				}
				if(ObjectBox.Min.y < Box.Min.y)
				{
					Box.Min.y = ObjectBox.Min.y;
				}

				if(ObjectBox.Max.z > Box.Max.z)
				{
					Box.Max.z = ObjectBox.Max.z;
				}
				if(ObjectBox.Min.z < Box.Min.z)
				{
					Box.Min.z = ObjectBox.Min.z;
				}
			}
		}

		State->ObjectModelMatrix = Identity4();
		LoadShaders(State);

		light Light = {0, V3(0.0f, 1.0f, 3.0f), V4(1.0f, 1.0f, 1.0f, 1.0f), V3(0.0f, 1.0f, 0.0f)};
		Light.DepthFramebuffer = CreateDepthFramebuffer(State->GLState, GlobalShadowWidth, GlobalShadowHeight);
		PushLight(State, Light);
		
		State->LightType = LightType_Perspective;

		switch(State->LightType)
		{
			case LightType_Orthographic:
				{
					State->ProjectionParams = {5.0f, 5.0f, 1.0f, 5.5f};
				} break;
			case LightType_Perspective:
				{
					State->ProjectionParams = {Radians(45), float(GlobalWindowWidth) / float(GlobalWindowHeight), 1.0f, 5.5f};
				} break;
			case LightType_PointLight:
				{
					InvalidCodePath;
				} break;
				InvalidDefaultCase;
		}

		State->Time = 0.0f;

		State->Camera = {};
		State->Camera.Pos = V3(0.0f, 0.0f, 2.0f * (Box.Max.z - Box.Min.z));
		State->Camera.Target = 0.5f * (Box.Max + Box.Min);
		v3 LookingDir = State->Camera.Target - State->Camera.Pos;
		v3 WorldUp = V3(0.0f, 1.0f, 0.0f);
		State->Camera.Right = Normalized(Cross(LookingDir, WorldUp));
		State->Camera.FoV = Radians(45);
		State->Camera.Aspect = float(GlobalWindowWidth) / float(GlobalWindowHeight);
		float Epsilon = 0.2f;
		State->Camera.NearPlane = (1.0f - Epsilon) * Abs(State->Camera.Pos.z - Box.Max.z);
		State->Camera.FarPlane = (1.0f + Epsilon) * Abs(State->Camera.Pos.z - Box.Min.z);
		State->FrustumBoundingBox = GetFrustumBoundingBox(State->Camera);

		State->MouseXInitial = 0;
		State->MouseYInitial = 0;
		State->MouseDragging = false;

		State->BlinnPhongShininess = 32;
		State->CookTorranceF0 = 0.5f;
		State->CookTorranceM = 0.5f;
		State->Alpha = 0.5f;
		State->Sigma = 0.0f;
		State->LightIntensity = 1.5f;

		State->MicroFoVInDegrees = 90;

		State->ScreenFramebuffer = CreateGeometryFramebuffer(State->GLState, GlobalWindowWidth, GlobalWindowHeight);
		State->HemicubeFramebuffer = CreateHemicubeScreenFramebuffer(State->GLState, GlobalMicrobufferWidth, GlobalMicrobufferHeight);

		State->IndirectIlluminationBuffer = 0;
		State->IndirectIlluminationTexture = CreateTexture();

		// NOTE(hugo) : Initializing Quad data 
		// {
		glGenVertexArrays(1, &State->QuadVAO);
		glGenBuffers(1, &State->QuadVBO);
		BindVertexArray(State->GLState, State->QuadVAO);
		BindBuffer(State->GLState, GL_ARRAY_BUFFER, State->QuadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(QuadVertices), &QuadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)(2 * sizeof(GLfloat)));
		BindVertexArray(State->GLState, 0);
		// }

		// NOTE(hugo) : Initializing Skybox data 
		// {
		glGenVertexArrays(1, &State->SkyboxVAO);
		glGenBuffers(1, &State->SkyboxVBO);
		BindVertexArray(State->GLState, State->SkyboxVAO);
		BindBuffer(State->GLState, GL_ARRAY_BUFFER, State->SkyboxVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(SkyboxVertices), &SkyboxVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
		BindVertexArray(State->GLState, 0);

		const char* SkyboxFilenames[6] = {"../models/skybox/right.jpg", "../models/skybox/left.jpg", "../models/skybox/top.jpg", "../models/skybox/bottom.jpg", "../models/skybox/back.jpg", "../models/skybox/front.jpg"};
		State->SkyboxTexture = LoadCubemap(State, SkyboxFilenames);
		// }

		// NOTE(hugo) : This must be the last command of the initialization of memory
		Memory->IsInitialized = true;
	}
	// TODO(hugo) : I should not enable the depth test each frame. This is a hack that fixes some issues on Linux (Ubuntu 16.04).
	// For more information, go to http://stackoverflow.com/questions/24990637/opengl-radeon-driver-seems-to-mess-with-depth-testing
	Enable(State->GLState, GL_DEPTH_TEST);
	DepthFunc(State->GLState, GL_LEQUAL);
	DepthMask(State->GLState, GL_TRUE);

	if(Input->WindowResized)
	{
		UpdateGeometryFramebuffer(State->GLState, &State->ScreenFramebuffer, GlobalWindowWidth, GlobalWindowHeight);
	}

	State->Camera.Aspect = float(GlobalWindowWidth) / float(GlobalWindowHeight);

	State->Time += Input->dtForFrame;

	ClearColorAndDepth(State->GLState, V4(0.4f, 0.6f, 0.2f, 1.0f));

	float DeltaMovement = 1.0f * 0.5f;
	v3 LookingDir = Normalized(State->Camera.Target - State->Camera.Pos);
	State->Camera.Pos += Input->MouseZ * DeltaMovement * LookingDir;

	// NOTE(hugo) : Live shader reloading
	if(IsKeyPressed(Input, SCANCODE_SPACE))
	{
		glDeleteShader(State->BasicShader.Program);
		glDeleteShader(State->DepthDebugQuadShader.Program);
		glDeleteShader(State->ShadowMappingShader.Program);
		glDeleteShader(State->SkyboxShader.Program);
		LoadShaders(State);
	}

	if(Input->MouseButtons[0].EndedDown)
	{
		if(!State->MouseDragging)
		{
			State->MouseDragging = true;
			State->MouseXInitial = Input->MouseX;
			State->MouseYInitial = Input->MouseY;
		}
	}

	camera NextCamera = State->Camera;
	v3 NextCameraUp = Cross(State->Camera.Right, Normalized(State->Camera.Target - State->Camera.Pos));
	if(State->MouseDragging)
	{
		s32 DeltaX = Input->MouseX - State->MouseXInitial;
		s32 DeltaY = Input->MouseY - State->MouseYInitial;
		v3 WorldUp = V3(0.0f, 1.0f, 0.0f);
		NextCamera.Pos = (Rotation(-Sign(Dot(WorldUp, NextCameraUp)) * Radians(DeltaX), WorldUp) * Rotation(-Radians(DeltaY), NextCamera.Right) * ToV4(State->Camera.Pos)).xyz;
		NextCamera.Right = (Rotation(-Sign(Dot(WorldUp, NextCameraUp)) * Radians(DeltaX), V3(0.0f, 1.0f, 0.0f)) * Rotation(-Radians(DeltaY), NextCamera.Right) * ToV4(State->Camera.Right)).xyz;
		v3 LookingDir = Normalized(State->Camera.Target - NextCamera.Pos);
		NextCameraUp = Cross(NextCamera.Right, LookingDir);

		State->FrustumBoundingBox = GetFrustumBoundingBox(NextCamera);
	}

	if(State->MouseDragging && !(Input->MouseButtons[0].EndedDown))
	{
		State->Camera.Pos = NextCamera.Pos;
		State->Camera.Right = NextCamera.Right;
		State->MouseDragging = false;
	}

	// NOTE(hugo) : Direct lighting rendering
	// {
	mat4 LightProjectionMatrix = {};
	switch(State->LightType)
	{
		case LightType_Orthographic:
			{
				LightProjectionMatrix = Orthographic(State->ProjectionParams.Width, State->ProjectionParams.Height, State->ProjectionParams.NearPlane, State->ProjectionParams.FarPlane);
			} break;
		case LightType_Perspective:
			{
				LightProjectionMatrix = Perspective(State->ProjectionParams.FoV, State->ProjectionParams.Aspect, State->ProjectionParams.NearPlane, State->ProjectionParams.FarPlane);
			} break;
		case LightType_PointLight:
			{
				InvalidCodePath;
			} break;
			InvalidDefaultCase;
	}
	SetViewport(State->GLState, GlobalShadowWidth, GlobalShadowHeight);
	for(u32 LightIndex = 0; LightIndex < State->LightCount; ++LightIndex)
	{
		BindFramebuffer(State->GLState, GL_FRAMEBUFFER, State->Lights[LightIndex].DepthFramebuffer.FBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		CullFace(State->GLState, GL_FRONT);
		RenderSimpleScene(State, State->Lights[LightIndex].Pos, State->Lights[LightIndex].Target, V3(0.0f, 1.0f, 0.0f), LightProjectionMatrix);
		CullFace(State->GLState, GL_BACK);
	}
	BindFramebuffer(State->GLState, GL_FRAMEBUFFER, 0);

	SetViewport(State->GLState, GlobalWindowWidth, GlobalWindowHeight);
	ClearColorAndDepth(State->GLState, V4(1.0f, 0.0f, 0.5f, 1.0f));

	mat4 ProjectionMatrix = GetCameraPerspective(State->Camera);

	RenderShadowSceneOnQuad(State, 
			NextCamera.Pos, NextCamera.Target, NextCameraUp, 
			ProjectionMatrix, 
			LightProjectionMatrix, 
			State->ScreenFramebuffer,
			&State->FrustumBoundingBox);
	// }

	if(ImGui::Button("Compute Indirect Illumination"))
	{
		ComputeGlobalIlluminationWithPatch(State, NextCamera, NextCameraUp, LightProjectionMatrix);
	}

#if 1
	if(ImGui::CollapsingHeader("BRDF Data"))
	{
		//ImGui::SliderInt("Blinn-Phong Shininess", (int*)&State->BlinnPhongShininess, 1, 256);
		ImGui::SliderFloat("Cook-Torrance F0", (float*)&State->CookTorranceF0, 0.0f, 1.0f);
		ImGui::SliderFloat("Cook-Torrance M", (float*)&State->CookTorranceM, 0.0f, 1.0f);
		ImGui::SliderFloat("Blur Sigma", (float*)&State->Sigma, 0.0f, 50.0f);
		ImGui::SliderFloat("Alpha", (float*)&State->Alpha, 0.0f, 1.0f);
		ImGui::SliderInt("Microbuffer Pixel Size", (int*)&GlobalMicrobufferWidth, 0, 128);
		if(GlobalMicrobufferHeight != GlobalMicrobufferWidth)
		{
			GlobalMicrobufferHeight = GlobalMicrobufferWidth;
		}
	}

	if(ImGui::CollapsingHeader("Light Data"))
	{
		ImGui::SliderFloat("Light Intensity", (float*)&State->LightIntensity, 0.0f, 10.0f);
		ImGui::SliderFloat3("Light Position", State->Lights[0].Pos.E, -3.0f, 3.0f);
		switch(State->LightType)
		{
			case LightType_Orthographic:
				{
					ImGui::SliderFloat("Orthographic Width", &State->ProjectionParams.Width, 0.0f, 5.0f);
					ImGui::SliderFloat("Orthographic Height", &State->ProjectionParams.Height, 0.0f, 5.0f);
					ImGui::SliderFloat("Orthographic Near Plane", &State->ProjectionParams.NearPlane, 0.0f, 1.0f);
					ImGui::SliderFloat("Orthographic Far Plane", &State->ProjectionParams.FarPlane, 1.0f, 8.0f);
				} break;
			case LightType_Perspective:
				{
					ImGui::SliderFloat("Perspective FoV", &State->ProjectionParams.FoV, 0.01f, 3.14f);
					ImGui::SliderFloat("Perspective Aspect", &State->ProjectionParams.Aspect, 0.01f, 5.0f);
					ImGui::SliderFloat("Perspective Near Plane", &State->ProjectionParams.NearPlane, 0.0f, 1.0f);
					ImGui::SliderFloat("Perspective Far Plane", &State->ProjectionParams.FarPlane, 1.0f, 8.0f);
				} break;
			case LightType_PointLight:
				{
					InvalidCodePath;
				} break;
				InvalidDefaultCase;
		}
	}

	if(ImGui::CollapsingHeader("Camera Data"))
	{
		ImGui::Text("Pos = (%f, %f, %f)", NextCamera.Pos.x, NextCamera.Pos.y, NextCamera.Pos.z);
		ImGui::Text("Target = (%f, %f, %f)", NextCamera.Target.x, NextCamera.Target.y, NextCamera.Target.z);
		ImGui::Text("Right = (%f, %f, %f)", NextCamera.Right.x, NextCamera.Right.y, NextCamera.Right.z);
		ImGui::Text("Aspect = %f", State->Camera.Aspect);
		ImGui::SliderFloat("FoV", &State->Camera.FoV, 0.0f, 3.1415f);
		ImGui::SliderFloat("NearPlane", &State->Camera.NearPlane, 0.0f, 100.0f);
		ImGui::SliderFloat("FarPlane", &State->Camera.FarPlane, State->Camera.NearPlane, 200.0f);

		ImGui::Text("Frustum Bounding Box Min: (%f, %f, %f)", State->FrustumBoundingBox.Min.x, State->FrustumBoundingBox.Min.y, State->FrustumBoundingBox.Min.z);
		ImGui::Text("Frustum Bounding Box Max: (%f, %f, %f)", State->FrustumBoundingBox.Max.x, State->FrustumBoundingBox.Max.y, State->FrustumBoundingBox.Max.z);
	}

	if(ImGui::CollapsingHeader("Objects Data"))
	{
		for(u32 ObjectIndex = 0; ObjectIndex < State->ObjectCount; ++ObjectIndex)
		{
			object* Object = State->Objects + ObjectIndex;
			char Buffer[64];
			if(IsEmptyString(Object->Name))
			{
				sprintf(Buffer, "Object #%i", ObjectIndex);
			}
			else
			{
				sprintf(Buffer, "Object #%i (%s)", ObjectIndex, Object->Name);
			}
			if(ImGui::TreeNode(Buffer))
			{
				if(!IsEmptyString(Object->Name))
				{
					ImGui::Text("Name: %s", Object->Name);
				}
				ImGui::Checkbox("Visible", &Object->Visible);
				ImGui::ColorEdit3("Albedo", Object->Albedo.E);
				ImGui::Text("Vertex Count: %d", Object->Mesh.VertexCount);
				ImGui::Text("Triangle Count: %d", Object->Mesh.TriangleCount);
				ImGui::Text("Bounding Box Min: (%f, %f, %f)", Object->BoundingBox.Min.x, Object->BoundingBox.Min.y, Object->BoundingBox.Min.z);
				ImGui::Text("Bounding Box Max: (%f, %f, %f)", Object->BoundingBox.Max.x, Object->BoundingBox.Max.y, Object->BoundingBox.Max.z);
				ImGui::TreePop();
			}
		}
	}
#endif

	ImGui::PlotLines("FPS", &DEBUGCounters[0], ArrayCount(DEBUGCounters));
	ImGui::PlotLines("OpenGL Stage Changes", &DEBUGGLStateChangeCounters[0], ArrayCount(DEBUGGLStateChangeCounters));
}
