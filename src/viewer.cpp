#pragma once

#include <imgui_demo.cpp>

static int GlobalShadowWidth = 2 * 1024;
static int GlobalShadowHeight = 2 * 1024;
static int GlobalTeapotInstanceCount = 10;
static u32 GlobalMicrobufferWidth = 128;
static u32 GlobalMicrobufferHeight = 128;

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
u8* LoadImageRGB(const char* Filename, s32* Width, s32* Height)
{
	s32 BitPerPixel;
	u8* Result = stbi_load(Filename, Width, Height, &BitPerPixel, 3);
	Assert(Result != 0);
	return(Result);
}

void FreeImage(u8* Image)
{
	stbi_image_free(Image);
}

GLuint LoadCubemap(const char** Filenames)
{
	GLuint Texture;
	glGenTextures(1, &Texture);
	glActiveTexture(GL_TEXTURE0);

	s32 Width = 0;
	s32 Height = 0;
	u8* Image = 0;

	glBindTexture(GL_TEXTURE_CUBE_MAP, Texture);

	for(u32 FaceIndex = 0; FaceIndex < 6; ++FaceIndex)
	{
		Image = LoadImageRGB(Filenames[FaceIndex], &Width, &Height);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + FaceIndex, 0, GL_RGB, Width, Height, 0, GL_RGB, GL_UNSIGNED_BYTE, Image);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

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
	// TODO(hugo) : From learnopengl.com, there is an optim 
	// using the stencil to compute less.
	glDepthMask(GL_FALSE);
	UseShader(State->SkyboxShader);

	mat4 UntranslatedView = RemoveTranslationPart(LookAt(CameraPos, CameraTarget, CameraUp));
	SetUniform(State->SkyboxShader, ProjectionMatrix, "Projection");
	SetUniform(State->SkyboxShader, UntranslatedView, "View");

	glBindVertexArray(State->SkyboxVAO);
	glBindTexture(GL_TEXTURE_CUBE_MAP, State->SkyboxTexture);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glDepthMask(GL_TRUE);
}

// NOTE(hugo) : In order for this function to work, the light depth buffers must have been previously computed
void RenderShadowedScene(game_state* State, v3 CameraPos, v3 CameraTarget, v3 CameraUp, mat4 ProjectionMatrix, mat4 LightProjectionMatrix)
{
	mat4 LightSpaceMatrix[4];

	mat4 ViewMatrix = LookAt(CameraPos, CameraTarget, CameraUp);

	// NOTE(hugo) : Drawing Object Mesh
	mat4 MVPObjectMatrix = ProjectionMatrix * ViewMatrix * State->ObjectModelMatrix;
	mat4 NormalObjectMatrix = Transpose(Inverse(ViewMatrix * State->ObjectModelMatrix));
	mat4 NormalWorldMatrix = Transpose(Inverse(State->ObjectModelMatrix));

	UseShader(State->ShadowMappingShader);

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
		glActiveTexture(GL_TEXTURE0 + LightIndex);
		SetUniform(State->ShadowMappingShader, LightIndex, Buffer);
		glBindTexture(GL_TEXTURE_2D, State->Lights[LightIndex].DepthFramebuffer.Texture);
	}

	SetUniform(State->ShadowMappingShader, State->CookTorranceF0, "CTF0");
	SetUniform(State->ShadowMappingShader, State->CookTorranceM, "CTM");


	for(u32 MeshIndex = 0; MeshIndex < State->MeshCount; ++MeshIndex)
	{
		SetUniform(State->ShadowMappingShader, State->Meshes[MeshIndex].Color, "ObjectColor");
		DrawTriangleMesh(&State->Meshes[MeshIndex]);
	}
	glActiveTexture(GL_TEXTURE0);

	// NOTE(hugo) : Drawing Light Mesh
	for(u32 LightIndex = 0; LightIndex < State->LightCount; ++LightIndex)
	{
		if(State->Lights[LightIndex].Mesh)
		{
			mat4 MVPLightMatrix = ProjectionMatrix * ViewMatrix * GetLightModelMatrix(State->Lights[LightIndex]);
			UseShader(State->BasicShader);
			SetUniform(State->BasicShader, MVPLightMatrix, "MVPMatrix");
			SetUniform(State->BasicShader, State->Lights[LightIndex].Color, "ObjectColor");
			DrawTriangleMesh(State->Lights[LightIndex].Mesh);
		}
	}
}

void RenderSimpleScene(game_state* State, v3 CameraPos, v3 CameraTarget, v3 CameraUp, mat4 ProjectionMatrix)
{
	mat4 ViewMatrix = LookAt(CameraPos, CameraTarget, CameraUp);
	mat4 MVPObjectMatrix = ProjectionMatrix * ViewMatrix * State->ObjectModelMatrix;

	UseShader(State->BasicShader);
	SetUniform(State->BasicShader, MVPObjectMatrix, "MVPMatrix");
	//DrawTriangleMeshInstances(&State->ObjectMesh, GlobalTeapotInstanceCount);
	for(u32 MeshIndex = 0; MeshIndex < State->MeshCount; ++MeshIndex)
	{
		DrawTriangleMesh(&State->Meshes[MeshIndex]);
	}

}

void RenderShadowSceneOnFramebuffer(game_state* State, v3 CameraPos, v3 CameraTarget, v3 CameraUp, mat4 ProjectionMatrix, mat4 LightProjectionMatrix, gl_geometry_framebuffer Framebuffer)
{
	glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer.FBO);
	ClearColorAndDepth(V4(1.0f, 0.0f, 0.5f, 1.0f));
	glEnable(GL_DEPTH_TEST);

	RenderSkybox(State, CameraPos, CameraTarget, CameraUp, ProjectionMatrix);
	RenderShadowedScene(State, CameraPos, CameraTarget, CameraUp, ProjectionMatrix, LightProjectionMatrix);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void RenderTextureOnQuadScreen(game_state* State, u32 Texture)
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// NOTE(hugo) : Quad rendering
	UseShader(State->DepthDebugQuadShader);
	SetUniform(State->DepthDebugQuadShader, State->Sigma, "Sigma");
	glBindVertexArray(State->QuadVAO);
	glDisable(GL_DEPTH_TEST);
	glBindTexture(GL_TEXTURE_2D, Texture);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

void RenderShadowSceneOnQuad(game_state* State, v3 CameraPos, v3 CameraTarget, v3 CameraUp, mat4 ProjectionMatrix, mat4 LightProjectionMatrix, gl_geometry_framebuffer Framebuffer)
{
	RenderShadowSceneOnFramebuffer(State, CameraPos, CameraTarget, CameraUp, ProjectionMatrix, LightProjectionMatrix, Framebuffer);
	RenderTextureOnQuadScreen(State, Framebuffer.ScreenTexture);
}

v3 ComputeDirectionOfPixel(camera Camera, v3 WorldUp, u32 PixelX, u32 PixelY, float PixelsToMeters, mat4 InvMicroCameraLookAt, u32 BufferWidth, u32 BufferHeight)
{
	s32 PixelPosX = s32(PixelX) - 0.5f * float(BufferWidth);
	s32 PixelPosY = s32(PixelY) - 0.5f * float(BufferHeight);
	v3 PixelCameraPos = {};
	PixelCameraPos.x = PixelPosX * PixelsToMeters;
	PixelCameraPos.y = PixelPosY * PixelsToMeters;
	PixelCameraPos.z = - Camera.NearPlane;

	v3 PixelWorldPos = (InvMicroCameraLookAt * ToV4(PixelCameraPos)).xyz;

	return(Normalized(PixelWorldPos - Camera.Pos));
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

v4 ColorU32ToV4(u32 Color)
{
	v4 Result = {};

	Result.r = float(((Color >>  0) & 0x000000FF)) / 255.0f;
	Result.g = float(((Color >>  8) & 0x000000FF)) / 255.0f;
	Result.b = float(((Color >> 16) & 0x000000FF)) / 255.0f;
	Result.a = float(((Color >> 24) & 0x000000FF)) / 255.0f;

	return(Result);
}

void ComputeGlobalIllumination(game_state* State, s32 X, s32 Y, camera Camera, v3 CameraUp, mat4 LightProjectionMatrix)
{
	u32 PixelAlbedo;
	float PixelDepth;
	glBindFramebuffer(GL_FRAMEBUFFER, State->ScreenFramebuffer.FBO);

	// NOTE(hugo) : Reading depth of touched pixel
	// Another possibility would be to store the position map in
	// the GBuffer. This query would take all three composants and would
	// not require any unprojection. However more space is needed.
	// I think I prefer unproject pixel for now
	glReadPixels(X, Y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &PixelDepth);

	// NOTE(hugo) : Reading albedo of touched pixel
	glReadBuffer(GL_COLOR_ATTACHMENT2);
	glReadPixels(X, Y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &PixelAlbedo);
	v4 Albedo = ColorU32ToV4(PixelAlbedo);

	// NOTE(hugo) : Reading normal of touched pixel
	glReadBuffer(GL_COLOR_ATTACHMENT1);
	v3 Normal = {};
	glReadPixels(X, Y, 1, 1, GL_RGB, GL_FLOAT, &Normal);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

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
			MicroCameras[MicroCameraIndex].Right = Normalized(Cross(MicroRenderDir[MicroCameraIndex], WorldUp));
			MicroCameras[MicroCameraIndex].FoV = Radians(State->DEBUGMicroFoVInDegrees);
			MicroCameras[MicroCameraIndex].Aspect = float(State->HemicubeFramebuffer.MicroBuffers[MicroCameraIndex].Width) / float(State->HemicubeFramebuffer.MicroBuffers[MicroCameraIndex].Height);
			MicroCameras[MicroCameraIndex].NearPlane = 0.1f * State->Camera.NearPlane;
			MicroCameras[MicroCameraIndex].FarPlane = 0.3f * State->Camera.FarPlane;

			MicroCameraProjections[MicroCameraIndex] = GetCameraPerspective(MicroCameras[MicroCameraIndex]);
		}
	}


#if 1
	for(u32 FaceIndex = 0; FaceIndex < ArrayCount(MicroCameras); ++FaceIndex)
	{
		// TODO(hugo) : Not sure that WorldUp should be the Camera Up since this one should 
		// be computed according to the normal
		camera MicroCamera = MicroCameras[FaceIndex];
		SetViewport(State->HemicubeFramebuffer.MicroBuffers[FaceIndex].Width, 
				State->HemicubeFramebuffer.MicroBuffers[FaceIndex].Height);
		RenderShadowSceneOnFramebuffer(State, MicroCamera.Pos, MicroCamera.Target, 
				WorldUp, MicroCameraProjections[FaceIndex], 
				LightProjectionMatrix, 
				State->HemicubeFramebuffer.MicroBuffers[FaceIndex]);

	}
	SetViewport(GlobalWindowWidth, GlobalWindowHeight);
	RenderTextureOnQuadScreen(State, State->HemicubeFramebuffer.MicroBuffers[0].ScreenTexture);

	// NOTE(hugo) : Now we have the whole hemicube rendered. We can sample it to 
#if 0
	v4 ColorBleeding = {};

	float MicrobufferWidthInMeters = 2.0f * MicroCameras[0].NearPlane * Tan(0.5f * MicroCameras[0].FoV);
	float PixelsToMeters = MicrobufferWidthInMeters / float(State->HemicubeFramebuffer.MicroBuffers[0].Width);
	float PixelSurfaceInMeters = PixelsToMeters * PixelsToMeters;
	for(u32 FaceIndex = 0; FaceIndex < ArrayCount(MicroCameras); ++FaceIndex)
	{
		gl_geometry_framebuffer Microbuffer = State->HemicubeFramebuffer.MicroBuffers[FaceIndex];
		// TODO(hugo) : Use Casey Muratori's memory framework ? (Temporary Memory here ?)
		u32* Pixels = (u32*) malloc(sizeof(u32) * Microbuffer.Width * Microbuffer.Height);
		Assert(Pixels);

		glBindFramebuffer(GL_FRAMEBUFFER, Microbuffer.FBO);
		glReadBuffer(GL_COLOR_ATTACHMENT0);
		glReadPixels(0, 0, Microbuffer.Width, Microbuffer.Height, GL_RGBA, GL_UNSIGNED_BYTE, Pixels);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		v3 Wo = Normalized(Camera.Pos - MicroCameras[FaceIndex].Pos);
		float NormalDotWo = DotClamp(Normal, Wo);
		camera MicroCamera = MicroCameras[FaceIndex];
		mat4 InvMicroCameraLookAt = Inverse(LookAt(MicroCamera.Pos, MicroCamera.Target, WorldUp));
		float MicroCameraNearPlaneSqr = MicroCamera.NearPlane * MicroCamera.NearPlane;
		v3 MicroCameraLookingDir = MicroCamera.Target - MicroCamera.Pos;

		for(u32 PixelX = 0; PixelX < Microbuffer.Width; ++PixelX)
		{
			for(u32 PixelY = 0; PixelY < Microbuffer.Height; ++PixelY)
			{
				v3 Wi = ComputeDirectionOfPixel(MicroCamera, WorldUp, PixelX, PixelY, PixelsToMeters, InvMicroCameraLookAt, Microbuffer.Width, Microbuffer.Height);
				if(DotClamp(Normal, Wi) > 0.0f)
				{
					// TODO(hugo) : Check that this is the correct getter and not the other way around
					// accord to what OpenGL sends us
					v4 PixelColor = ColorU32ToV4(Pixels[Microbuffer.Width * PixelY + PixelX]);

					if(LengthSqr(PixelColor.rgb) > 0.0f)
					{
						v3 H = Normalized(0.5f * (Wi + Wo));
						float SolidAngle = PixelSurfaceInMeters / (MicroCameraNearPlaneSqr) * Dot(Wi, MicroCameraLookingDir);
						float BRDF = GGXBRDF(Normal, Wi, H, NormalDotWo, State->Alpha, State->CookTorranceF0);
						ColorBleeding += BRDF * DotClamp(Normal, Wi) * SolidAngle * Hadamard(Albedo, PixelColor);
					}
				}
			}
		}

		free((void*)Pixels);
	}
#endif
	
	SDL_GL_SwapWindow(GlobalWindow);
	SDL_Delay(2000);
#endif
#if 1
	//ImGui::ColorEdit3("Color Picked", ColorFloat.E);
	ImGui::Value("Depth @ Pixel", PixelDepth);
	//ImGui::Text("Position pointed at screen : (%f, %f, %f, %f)", PixelPos.x, PixelPos.y, PixelPos.z, PixelPos.w);
	ImGui::Text("Normal pointed at screen : (%f, %f, %f)", Normal.x, Normal.y, Normal.z);
	//SDL_Delay(2000);
#endif
}

void PushMesh(game_state* State, mesh* Mesh)
{
	Assert(State->MeshCount < ArrayCount(State->Meshes));

	State->Meshes[State->MeshCount] = *Mesh;
	State->MeshCount++;
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
}

void GameUpdateAndRender(thread_context* Thread, game_memory* Memory, game_input* Input)
{
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	game_state* State = (game_state*)Memory->PermanentStorage;
	if(!Memory->IsInitialized)
	{
		{
			std::vector<mesh> Meshes = LoadOBJ("../models/cornell_box/", "CornellBox-Original.obj");
			for(u32 MeshIndex = 0; MeshIndex < Meshes.size(); ++MeshIndex)
			{
				PushMesh(State, &Meshes[MeshIndex]);
			}
		}
		State->ObjectModelMatrix = Identity4();
		LoadShaders(State);

		light Light = {&State->CubeMesh, V3(0.0f, 1.0f, 3.0f), V4(1.0f, 1.0f, 1.0f, 1.0f), V3(0.0f, 1.0f, 0.0f)};
		Light.DepthFramebuffer = CreateDepthFramebuffer(GlobalShadowWidth, GlobalShadowHeight);
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
		State->Camera.Pos = V3(0.0f, 0.0f, 5.0f);
		State->Camera.Target = V3(0.0f, 1.0f, 0.0f);
		v3 LookingDir = State->Camera.Target - State->Camera.Pos;
		v3 WorldUp = V3(0.0f, 1.0f, 0.0f);
		State->Camera.Right = Normalized(Cross(LookingDir, WorldUp));
		State->Camera.FoV = Radians(45);
		State->Camera.Aspect = float(GlobalWindowWidth) / float(GlobalWindowHeight);
		State->Camera.NearPlane = 0.5f;
		State->Camera.FarPlane = 10.0f;

		State->MouseXInitial = 0;
		State->MouseYInitial = 0;
		State->MouseDragging = false;

		State->BlinnPhongShininess = 32;
		State->CookTorranceF0 = 0.5f;
		State->CookTorranceM = 0.5f;
		State->Alpha = 0.5f;
		State->Sigma = 0.0f;
		State->LightIntensity = 4.5f;

		State->DEBUGMicroFoVInDegrees = 45;

		// TODO(hugo) : If the window size changes, then this screenbuffer will have wrong dimensions.
		// Maybe I need to see each frame if the window dim changes. If so, update the screenbuffer.
		State->ScreenFramebuffer = CreateGeometryFramebuffer(GlobalWindowWidth, GlobalWindowHeight);
		State->HemicubeFramebuffer = CreateHemicubeScreenFramebuffer(GlobalMicrobufferWidth, GlobalMicrobufferHeight);

		// NOTE(hugo) : Initializing Quad data 
		// {
		glGenVertexArrays(1, &State->QuadVAO);
		glGenBuffers(1, &State->QuadVBO);
		glBindVertexArray(State->QuadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, State->QuadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(QuadVertices), &QuadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)(2 * sizeof(GLfloat)));
		glBindVertexArray(0);
		// }

		// NOTE(hugo) : Initializing Skybox data 
		// {
		glGenVertexArrays(1, &State->SkyboxVAO);
		glGenBuffers(1, &State->SkyboxVBO);
		glBindVertexArray(State->SkyboxVAO);
		glBindBuffer(GL_ARRAY_BUFFER, State->SkyboxVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(SkyboxVertices), &SkyboxVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
		glBindVertexArray(0);

		const char* SkyboxFilenames[6] = {"../models/skybox/right.jpg", "../models/skybox/left.jpg", "../models/skybox/top.jpg", "../models/skybox/bottom.jpg", "../models/skybox/back.jpg", "../models/skybox/front.jpg"};
		State->SkyboxTexture = LoadCubemap(SkyboxFilenames);
		// }

		// NOTE(hugo) : This must be the last command of the initialization of memory
		Memory->IsInitialized = true;
	}
	// TODO(hugo) : I should not enable the depth test each frame. This is a hack that fixes some issues on Linux (Ubuntu 16.04).
	// For more information, go to http://stackoverflow.com/questions/24990637/opengl-radeon-driver-seems-to-mess-with-depth-testing
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE);

	State->Camera.Aspect = float(GlobalWindowWidth) / float(GlobalWindowHeight);

	State->Time += Input->dtForFrame;

	ClearColorAndDepth(V4(0.4f, 0.6f, 0.2f, 1.0f));

	// TODO(hugo) : Smooth MouseWheel camera movement
	float DeltaMovement = 0.5f;
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
	}

	if(State->MouseDragging && !(Input->MouseButtons[0].EndedDown))
	{
		State->Camera.Pos = NextCamera.Pos;
		State->Camera.Right = NextCamera.Right;
		State->MouseDragging = false;
	}

	// NOTE(hugo) : Direct lighting rendering
	// {
	// TODO(hugo) : Get rid of OpenGL in here
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
	};
	SetViewport(GlobalShadowWidth, GlobalShadowHeight);
	for(u32 LightIndex = 0; LightIndex < State->LightCount; ++LightIndex)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, State->Lights[LightIndex].DepthFramebuffer.FBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		glCullFace(GL_FRONT);
		RenderSimpleScene(State, State->Lights[LightIndex].Pos, State->Lights[LightIndex].Target, V3(0.0f, 1.0f, 0.0f), LightProjectionMatrix);
		glCullFace(GL_BACK);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	SetViewport(GlobalWindowWidth, GlobalWindowHeight);
	ClearColorAndDepth(V4(1.0f, 0.0f, 0.5f, 1.0f));

	mat4 ProjectionMatrix = GetCameraPerspective(State->Camera);

	RenderShadowSceneOnQuad(State, NextCamera.Pos, NextCamera.Target, NextCameraUp, ProjectionMatrix, LightProjectionMatrix, State->ScreenFramebuffer);
	// }

	if(Input->MouseButtons[2].EndedDown)
	{
		s32 MouseX = Input->MouseX;
		s32 MouseY = GlobalWindowHeight - Input->MouseY;
		ComputeGlobalIllumination(State, MouseX, MouseY, NextCamera, NextCameraUp, LightProjectionMatrix);
	}

	ImGui::SliderFloat("Light Intensity", (float*)&State->LightIntensity, 0.0f, 10.0f);
	ImGui::SliderInt("Micro FoV", (int*)&State->DEBUGMicroFoVInDegrees, 0, 90);
#if 0
	//ImGui::SliderInt("Blinn-Phong Shininess", (int*)&State->BlinnPhongShininess, 1, 256);
	ImGui::SliderFloat("Cook-Torrance F0", (float*)&State->CookTorranceF0, 0.0f, 1.0f);
	ImGui::SliderFloat("Cook-Torrance M", (float*)&State->CookTorranceM, 0.0f, 1.0f);
	ImGui::SliderFloat("Blur Sigma", (float*)&State->Sigma, 0.0f, 50.0f);
	ImGui::SliderFloat("Alpha", (float*)&State->Alpha, 0.0f, 1.0f);

	if(ImGui::CollapsingHeader("Light Data"))
	{
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

	if(ImGui::BeginMainMenuBar())
	{
		if(ImGui::BeginMenu("Menu"))
		{
			ImGui::MenuItem("(dummy menu)", NULL, false, true);
			ImGui::EndMenu();
		}
		if(ImGui::BeginMenu("About"))
		{
            ImGui::MenuItem("Main menu bar", NULL, false, true);
			ImGui::EndMenu();
		}
        ImGui::EndMainMenuBar();
	}
#endif
}
