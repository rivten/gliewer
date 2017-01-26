#pragma once

#include <imgui_demo.cpp>

static int GlobalShadowWidth = 2 * 1024;
static int GlobalShadowHeight = 2 * 1024;
static int GlobalTeapotInstanceCount = 10;
static u32 GlobalMicrobufferWidth = 128;
static u32 GlobalMicrobufferHeight = 128;

GLuint LoadCubemap(game_state* State, const char** Filenames)
{
	GLuint Texture;
	glGenTextures(1, &Texture);

	BindTexture(State->RenderState, GL_TEXTURE_CUBE_MAP, Texture);

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

	BindTexture(State->RenderState, GL_TEXTURE_CUBE_MAP, 0);

	return(Texture);
}

mat4 GetLightModelMatrix(light Light)
{
		mat4 ModelMatrix = Translation(Light.Pos) * Scaling(V3(0.2f, 0.2f, 0.2f));

		return(ModelMatrix);
}

mat4 LookAt(camera Camera)
{
	mat4 Result = LookAt(Camera.P, Camera.XAxis, Camera.ZAxis);
	return(Result);
}

mat4 GetCameraPerspective(camera Camera)
{
	mat4 Result = Perspective(Camera.FoV, Camera.Aspect, Camera.NearPlane, Camera.FarPlane);

	return(Result);
}

// TODO(hugo) : A lot of render call recompute the LookAt matrix. Factorize this to compute it only once
void RenderSkybox(game_state* State, camera Camera, mat4 ProjectionMatrix)
{
	DepthMask(State->RenderState, false);
	UseShader(State->RenderState, State->Shaders[ShaderType_Skybox]);

	mat4 UntranslatedView = RemoveTranslationPart(LookAt(Camera));
	SetUniform(State->Shaders[ShaderType_Skybox], ProjectionMatrix, "Projection");
	SetUniform(State->Shaders[ShaderType_Skybox], UntranslatedView, "View");

	BindVertexArray(State->RenderState, State->SkyboxVAO);
	BindTexture(State->RenderState, GL_TEXTURE_CUBE_MAP, State->SkyboxTexture);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	BindVertexArray(State->RenderState, 0);
	DepthMask(State->RenderState, true);
}

void RenderSimpleScene(game_state* State, camera Camera, mat4 ProjectionMatrix, rect3* FrustumBoundingBox)
{
	mat4 ViewMatrix = LookAt(Camera);
	mat4 MVPObjectMatrix = ProjectionMatrix * ViewMatrix * State->ObjectModelMatrix;

	UseShader(State->RenderState, State->Shaders[ShaderType_LowCost]);
	SetUniform(State->Shaders[ShaderType_LowCost], MVPObjectMatrix, "MVPMatrix");
	//DrawTriangleMeshInstances(&State->ObjectMesh, GlobalTeapotInstanceCount);
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
				DrawTriangleObject(State->RenderState, Object);
			}
		}
	}
}

void RenderTextureOnQuadScreen(game_state* State, texture Texture)
{
	Assert(Texture.IsValid);
	ClearColor(State->RenderState, V4(1.0f, 1.0f, 1.0f, 1.0f));
	glClear(GL_COLOR_BUFFER_BIT);

	mat4 InvCameraViewMatrix = Inverse(LookAt(State->Camera));

	// NOTE(hugo) : Quad rendering
	UseShader(State->RenderState, State->Shaders[ShaderType_PostProcess]);
	SetUniform(State->Shaders[ShaderType_PostProcess], State->Sigma, "Sigma");
	SetUniform(State->Shaders[ShaderType_PostProcess], State->Camera.NearPlane, "NearPlane");
	SetUniform(State->Shaders[ShaderType_PostProcess], State->Camera.FarPlane, "FarPlane");
	SetUniform(State->Shaders[ShaderType_PostProcess], State->Camera.FoV, "FoV");
	SetUniform(State->Shaders[ShaderType_PostProcess], State->Camera.Aspect, "Aspect");
	SetUniform(State->Shaders[ShaderType_PostProcess], InvCameraViewMatrix, "InvView");
	SetUniform(State->Shaders[ShaderType_PostProcess], State->SSAOParams.SampleCount, "AOSamples");
	SetUniform(State->Shaders[ShaderType_PostProcess], State->SSAOParams.Intensity, "AOIntensity");
	SetUniform(State->Shaders[ShaderType_PostProcess], State->SSAOParams.Scale, "AOScale");
	SetUniform(State->Shaders[ShaderType_PostProcess], State->SSAOParams.SamplingRadius, "AORadius");
	SetUniform(State->Shaders[ShaderType_PostProcess], State->SSAOParams.Bias, "AOBias");

	SetUniform(State->Shaders[ShaderType_PostProcess], GlobalWindowWidth, "WindowWidth");
	SetUniform(State->Shaders[ShaderType_PostProcess], GlobalWindowHeight, "WindowHeight");

	SetUniform(State->Shaders[ShaderType_PostProcess], State->MotionBlur, "DoMotionBlur");
	SetUniform(State->Shaders[ShaderType_PostProcess], State->PreviousViewProj, "PreviousViewProj");
	SetUniform(State->Shaders[ShaderType_PostProcess], State->MotionBlurSampleCount, "MotionBlurSampleCount");

	ActiveTexture(State->RenderState, GL_TEXTURE0);
	SetUniform(State->Shaders[ShaderType_PostProcess], (u32)0, "ScreenTexture");
	BindTexture(State->RenderState, GL_TEXTURE_2D, Texture.ID);

	ActiveTexture(State->RenderState, GL_TEXTURE1);
	SetUniform(State->Shaders[ShaderType_PostProcess], (u32)1, "DepthTexture");
	BindTexture(State->RenderState, GL_TEXTURE_2D, State->GBuffer.DepthTexture.ID);

	ActiveTexture(State->RenderState, GL_TEXTURE2);
	SetUniform(State->Shaders[ShaderType_PostProcess], (u32)2, "NormalTexture");
	BindTexture(State->RenderState, GL_TEXTURE_2D, State->GBuffer.NormalTexture.ID);

	BindVertexArray(State->RenderState, State->QuadVAO);
	Disable(State->RenderState, GL_DEPTH_TEST);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	Enable(State->RenderState, GL_DEPTH_TEST);
	BindVertexArray(State->RenderState, 0);
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

void FillGBuffer(game_state* State, geometry_framebuffer GBuffer, camera Camera,
		mat4 CameraProj)
{
	BindFramebuffer(State->RenderState, GL_FRAMEBUFFER, GBuffer.ID);
	ClearColorAndDepth(State->RenderState, V4(0.0f, 0.0f, 0.0f, 1.0f));
	Enable(State->RenderState, GL_DEPTH_TEST);

	UseShader(State->RenderState, State->Shaders[ShaderType_FillGBuffer]);

	mat4 ViewMatrix = LookAt(Camera);

	// NOTE(hugo) : Drawing Object Mesh
	mat4 MVPMatrix = CameraProj * ViewMatrix * State->ObjectModelMatrix;
	mat4 NormalWorldMatrix = Transpose(Inverse(State->ObjectModelMatrix));

	SetUniform(State->Shaders[ShaderType_FillGBuffer], MVPMatrix, "MVPMatrix");
	SetUniform(State->Shaders[ShaderType_FillGBuffer], NormalWorldMatrix, "NormalWorldMatrix");

	for(u32 ObjectIndex = 0; ObjectIndex < State->ObjectCount; ++ObjectIndex)
	{
		object* Object = State->Objects + ObjectIndex;
		if(Object->Visible)
		{
			bool ShouldDraw = Intersect3(State->FrustumBoundingBox, Object->BoundingBox);
			Object->IsFrustumCulled = !ShouldDraw;
			if(ShouldDraw)
			{
				SetUniform(State->Shaders[ShaderType_FillGBuffer], 
						Object->Material.SpecularColor, "SpecularColor");
				SetUniform(State->Shaders[ShaderType_FillGBuffer], 
						Object->Material.DiffuseColor, "DiffuseColor");
				SetUniform(State->Shaders[ShaderType_FillGBuffer],
						(u32)Object->Material.UseTextureMapping, "UseTextureMapping");
				SetUniform(State->Shaders[ShaderType_FillGBuffer],
						(u32)Object->Material.UseNormalMapping, "UseNormalMapping");

				if(Object->Material.UseTextureMapping)
				{
					ActiveTexture(State->RenderState, GL_TEXTURE0);
					SetUniform(State->Shaders[ShaderType_FillGBuffer], (u32)0, "TextureMap");
					BindTexture(State->RenderState, GL_TEXTURE_2D, State->RenderState->Textures[Object->Material.TextureMapLocation].ID);
				}
				if(Object->Material.UseNormalMapping)
				{
					ActiveTexture(State->RenderState, GL_TEXTURE1);
					SetUniform(State->Shaders[ShaderType_FillGBuffer], (u32)1, "NormalMap");
					BindTexture(State->RenderState, GL_TEXTURE_2D, State->RenderState->Textures[Object->Material.NormalMapLocation].ID);
				}
				DrawTriangleObject(State->RenderState, Object);
			}
		}
	}
}

void LightGBuffer(game_state* State, 
		geometry_framebuffer GBuffer, 
		basic_framebuffer Target,
		camera Camera)
{
	BindFramebuffer(State->RenderState, GL_FRAMEBUFFER, Target.ID);
	ClearColor(State->RenderState, V4(0.0f, 0.0f, 0.0f, 1.0f));

	UseShader(State->RenderState, State->Shaders[ShaderType_DirectLighting]);

	mat4 InvView = Inverse(LookAt(State->Camera));
	SetUniform(State->Shaders[ShaderType_DirectLighting], Camera.P, "CameraPos");
	SetUniform(State->Shaders[ShaderType_DirectLighting], State->CookTorranceF0, "CTF0");
	SetUniform(State->Shaders[ShaderType_DirectLighting], State->Alpha, "Alpha");
	SetUniform(State->Shaders[ShaderType_DirectLighting], State->LightIntensity, "LightIntensity");
	SetUniform(State->Shaders[ShaderType_DirectLighting], State->Ks, "Ks");
	SetUniform(State->Shaders[ShaderType_DirectLighting], State->Kd, "Kd");
	SetUniform(State->Shaders[ShaderType_DirectLighting], InvView, "InvView");

	SetUniform(State->Shaders[ShaderType_DirectLighting], Camera.NearPlane, "CameraNearPlane");
	SetUniform(State->Shaders[ShaderType_DirectLighting], Camera.FarPlane, "CameraFarPlane");
	SetUniform(State->Shaders[ShaderType_DirectLighting], Camera.FoV, "CameraFoV");
	SetUniform(State->Shaders[ShaderType_DirectLighting], Camera.Aspect, "CameraAspect");

	SetUniform(State->Shaders[ShaderType_DirectLighting], State->AmbientFactor, "AmbientFactor");

	SetUniform(State->Shaders[ShaderType_DirectLighting], State->LightCount, "LightCount");
	for(u32 LightIndex = 0; LightIndex < State->LightCount; ++LightIndex)
	{
		light* Light = State->Lights + LightIndex;

		char Buffer[64];
		sprintf(Buffer, "LightPos[%i]", LightIndex);
		SetUniform(State->Shaders[ShaderType_DirectLighting], Light->Pos, Buffer);

		memset(Buffer, 0, ArrayCount(Buffer));
		sprintf(Buffer, "LightColor[%i]", LightIndex);
		SetUniform(State->Shaders[ShaderType_DirectLighting], Light->Color, Buffer);

		memset(Buffer, 0, ArrayCount(Buffer));
		sprintf(Buffer, "LightViews[%i]", LightIndex);
		v3 LightYAxis = V3(0.0f, 1.0f, 0.0f);
		v3 LightZAxis = Normalized(Light->Pos - Light->Target);
		v3 LightXAxis = Cross(LightYAxis, LightZAxis);
		mat4 LightViewProj = State->LightProjectionMatrix * LookAt(Light->Pos, LightXAxis, LightZAxis);
		SetUniform(State->Shaders[ShaderType_DirectLighting], LightViewProj, Buffer);

		memset(Buffer, 0, ArrayCount(Buffer));
		sprintf(Buffer, "ShadowMap[%i]", LightIndex);
		ActiveTexture(State->RenderState, GL_TEXTURE0 + LightIndex);
		SetUniform(State->Shaders[ShaderType_DirectLighting], LightIndex, Buffer);
		BindTexture(State->RenderState, GL_TEXTURE_2D, Light->DepthFramebuffer.Texture.ID);
	}

	ActiveTexture(State->RenderState, GL_TEXTURE4);
	SetUniform(State->Shaders[ShaderType_DirectLighting], (u32)4, "DepthTexture");
	BindTexture(State->RenderState, GL_TEXTURE_2D, GBuffer.DepthTexture.ID);

	ActiveTexture(State->RenderState, GL_TEXTURE5);
	SetUniform(State->Shaders[ShaderType_DirectLighting], (u32)5, "NormalTexture");
	BindTexture(State->RenderState, GL_TEXTURE_2D, GBuffer.NormalTexture.ID);

	ActiveTexture(State->RenderState, GL_TEXTURE6);
	SetUniform(State->Shaders[ShaderType_DirectLighting], (u32)6, "AlbedoTexture");
	BindTexture(State->RenderState, GL_TEXTURE_2D, GBuffer.AlbedoTexture.ID);

	ActiveTexture(State->RenderState, GL_TEXTURE7);
	SetUniform(State->Shaders[ShaderType_DirectLighting], (u32)7, "SpecularTexture");
	BindTexture(State->RenderState, GL_TEXTURE_2D, GBuffer.SpecularTexture.ID);

	BindVertexArray(State->RenderState, State->QuadVAO);
	Disable(State->RenderState, GL_DEPTH_TEST);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	Enable(State->RenderState, GL_DEPTH_TEST);
	//BindVertexArray(State->RenderState, 0);
}

#include "global_illumination.cpp"

void ApplyFXAA(game_state* State, texture Texture)
{
	Assert(Texture.IsValid);

	UseShader(State->RenderState, State->Shaders[ShaderType_FXAA]);

	SetUniform(State->Shaders[ShaderType_FXAA], State->FXAAParams.MultiplicationFactor, "FXAAMultiplicationFactor");
	SetUniform(State->Shaders[ShaderType_FXAA], State->FXAAParams.MinimalReduction, "FXAAMinimalReduction");
	SetUniform(State->Shaders[ShaderType_FXAA], State->FXAAParams.SpanMax, "FXAASpanMax");

	ActiveTexture(State->RenderState, GL_TEXTURE0);
	SetUniform(State->Shaders[ShaderType_FXAA], (u32)0, "Texture");
	BindTexture(State->RenderState, GL_TEXTURE_2D, Texture.ID);

	BindVertexArray(State->RenderState, State->QuadVAO);
	Disable(State->RenderState, GL_DEPTH_TEST);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	Enable(State->RenderState, GL_DEPTH_TEST);
	BindVertexArray(State->RenderState, 0);
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
	for(u32 ShaderIndex = 0; ShaderIndex < ShaderType_Count; ++ShaderIndex)
	{
		shader* Shader = State->Shaders + ShaderIndex;
		*Shader = LoadShader(ShaderIndex);
	}
}

rect3 GetFrustumBoundingBox(camera Camera)
{
	rect3 Result = MaxBoundingBox();
	v3 P[8];

	mat4 InvLookAt = Inverse(LookAt(Camera));
	P[0] = UnprojectPixel(Camera.FarPlane, 0, 1, 1, 1, Camera, InvLookAt).xyz;
	P[1] = UnprojectPixel(Camera.FarPlane, 1, 1, 1, 1, Camera, InvLookAt).xyz;
	P[2] = UnprojectPixel(Camera.FarPlane, 1, 0, 1, 1, Camera, InvLookAt).xyz;
	P[3] = UnprojectPixel(Camera.FarPlane, 0, 0, 1, 1, Camera, InvLookAt).xyz;
	P[4] = UnprojectPixel(Camera.NearPlane, 0, 1, 1, 1, Camera, InvLookAt).xyz;
	P[5] = UnprojectPixel(Camera.NearPlane, 1, 1, 1, 1, Camera, InvLookAt).xyz;
	P[6] = UnprojectPixel(Camera.NearPlane, 1, 0, 1, 1, Camera, InvLookAt).xyz;
	P[7] = UnprojectPixel(Camera.NearPlane, 0, 0, 1, 1, Camera, InvLookAt).xyz;

	for(u32 PointIndex = 0; PointIndex < ArrayCount(P); ++PointIndex)
	{
		AddPointToBoundingBox(&Result, P[PointIndex]);
	}

	return(Result);
}

void FinalRender(game_state* State)
{
	ClearColor(State->RenderState, V4(1.0f, 1.0f, 1.0f, 1.0f));
	glClear(GL_COLOR_BUFFER_BIT);

	mat4 InvCameraViewMatrix = Inverse(LookAt(State->Camera));

	// NOTE(hugo) : Quad rendering
	UseShader(State->RenderState, State->Shaders[ShaderType_PostProcess]);
	SetUniform(State->Shaders[ShaderType_PostProcess], State->Sigma, "Sigma");
	SetUniform(State->Shaders[ShaderType_PostProcess], State->Camera.NearPlane, "NearPlane");
	SetUniform(State->Shaders[ShaderType_PostProcess], State->Camera.FarPlane, "FarPlane");
	SetUniform(State->Shaders[ShaderType_PostProcess], State->Camera.FoV, "FoV");
	SetUniform(State->Shaders[ShaderType_PostProcess], State->Camera.Aspect, "Aspect");
	SetUniform(State->Shaders[ShaderType_PostProcess], InvCameraViewMatrix, "InvView");
	SetUniform(State->Shaders[ShaderType_PostProcess], State->SSAOParams.SampleCount, "AOSamples");
	SetUniform(State->Shaders[ShaderType_PostProcess], State->SSAOParams.Intensity, "AOIntensity");
	SetUniform(State->Shaders[ShaderType_PostProcess], State->SSAOParams.Scale, "AOScale");
	SetUniform(State->Shaders[ShaderType_PostProcess], State->SSAOParams.SamplingRadius, "AORadius");
	SetUniform(State->Shaders[ShaderType_PostProcess], State->SSAOParams.Bias, "AOBias");

	SetUniform(State->Shaders[ShaderType_PostProcess], GlobalWindowWidth, "WindowWidth");
	SetUniform(State->Shaders[ShaderType_PostProcess], GlobalWindowHeight, "WindowHeight");

	SetUniform(State->Shaders[ShaderType_PostProcess], State->MotionBlur, "DoMotionBlur");
	SetUniform(State->Shaders[ShaderType_PostProcess], State->PreviousViewProj, "PreviousViewProj");
	SetUniform(State->Shaders[ShaderType_PostProcess], State->MotionBlurSampleCount, "MotionBlurSampleCount");

	ActiveTexture(State->RenderState, GL_TEXTURE0);
	SetUniform(State->Shaders[ShaderType_PostProcess], (u32)0, "ScreenTexture");
	BindTexture(State->RenderState, GL_TEXTURE_2D, State->PreProcess.Texture.ID);

	ActiveTexture(State->RenderState, GL_TEXTURE1);
	SetUniform(State->Shaders[ShaderType_PostProcess], (u32)1, "DepthTexture");
	BindTexture(State->RenderState, GL_TEXTURE_2D, State->GBuffer.DepthTexture.ID);

	ActiveTexture(State->RenderState, GL_TEXTURE2);
	SetUniform(State->Shaders[ShaderType_PostProcess], (u32)2, "NormalTexture");
	BindTexture(State->RenderState, GL_TEXTURE_2D, State->GBuffer.NormalTexture.ID);

	mat4 UntranslatedInvView = RemoveTranslationPart(InvCameraViewMatrix);
	SetUniform(State->Shaders[ShaderType_PostProcess], UntranslatedInvView, "UntranslatedInvView");

	ActiveTexture(State->RenderState, GL_TEXTURE3);
	SetUniform(State->Shaders[ShaderType_PostProcess], (u32)3, "Skybox");
	BindTexture(State->RenderState, GL_TEXTURE_CUBE_MAP, State->SkyboxTexture);

	BindVertexArray(State->RenderState, State->QuadVAO);
	Disable(State->RenderState, GL_DEPTH_TEST);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	Enable(State->RenderState, GL_DEPTH_TEST);
	BindVertexArray(State->RenderState, 0);
}

void GameUpdateAndRender(game_memory* Memory, game_input* Input, render_state* RenderState)
{
	Assert(!DetectErrors("In Frame"));
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	game_state* State = (game_state*)Memory->PermanentStorage;
	if(!Memory->IsInitialized)
	{
		State->RenderState = RenderState;
		rect3 Box = MaxBoundingBox();
		{
			std::vector<object> Objects = LoadOBJ(State->RenderState, "../models/cornell_box/", "CornellBox-Original.obj");
			//std::vector<object> Objects = LoadOBJ(State->RenderState, "../models/house/", "house.obj");
			//std::vector<object> Objects = LoadOBJ(State->RenderState, "../models/sponza/", "sponza.obj");
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
		//light Light = {0, V3(-60.0f, 700.0f, -38.0f), V4(1.0f, 1.0f, 1.0f, 1.0f), V3(-61.0f, 700.0f, -38.0f)};
		Light.DepthFramebuffer = CreateDepthFramebuffer(State->RenderState, GlobalShadowWidth, GlobalShadowHeight);
		PushLight(State, Light);
		
		State->LightType = LightType_Perspective;
		State->LightProjectionMatrix = Identity4();

		switch(State->LightType)
		{
			case LightType_Orthographic:
				{
					State->ProjectionParams = {5.0f, 5.0f, 1.0f, 5.5f};
				} break;
			case LightType_Perspective:
				{
					State->ProjectionParams = {Radians(45), float(GlobalWindowWidth) / float(GlobalWindowHeight), 1.0f, 5.5f};
					//State->ProjectionParams = {Radians(45), float(GlobalWindowWidth) / float(GlobalWindowHeight), 100.0f, 2000.5f};
				} break;
			case LightType_PointLight:
				{
					InvalidCodePath;
				} break;
				InvalidDefaultCase;
		}

		State->Time = 0.0f;

		//State->CameraType = CameraType_Arcball;
		State->CameraType = CameraType_FirstPerson;
		State->ReferenceCamera = {};
		//State->ReferenceCamera.P = V3(0.0f, 0.0f, 2.0f * (Box.Max.z - Box.Min.z));
		State->ReferenceCamera.P = V3(0.0f, 0.0f, 0.0f);
		if(State->CameraType == CameraType_Arcball)
		{
			State->FixedTarget = 0.5f * (Box.Max + Box.Min);
			State->ReferenceCamera.ZAxis = Normalized(State->ReferenceCamera.P - State->FixedTarget);
		}
		else
		{
			State->ReferenceCamera.ZAxis = V3(0.0f, 0.0f, 1.0f);
			State->FixedTarget = 0.5f * (Box.Max + Box.Min);
			State->ReferenceCamera.ZAxis = Normalized(State->ReferenceCamera.P - State->FixedTarget);
		}

		v3 LookingDir = -1.0f * State->ReferenceCamera.ZAxis;
		v3 WorldUp = V3(0.0f, 1.0f, 0.0f);
		State->ReferenceCamera.XAxis = Normalized(Cross(LookingDir, WorldUp));
		State->ReferenceCamera.FoV = Radians(45);
		State->ReferenceCamera.Aspect = float(GlobalWindowWidth) / float(GlobalWindowHeight);
		float Epsilon = 0.2f;
		State->ReferenceCamera.NearPlane = (1.0f - Epsilon) * Abs(State->ReferenceCamera.P.z - Box.Max.z);
		State->ReferenceCamera.FarPlane = (1.0f + Epsilon) * Abs(State->ReferenceCamera.P.z - Box.Min.z);
		//State->ReferenceCamera.NearPlane = 100.0f;
		//State->ReferenceCamera.FarPlane = 2000.0f;
		State->ReferenceCamera.NearPlane = 0.5f;
		State->ReferenceCamera.FarPlane = 5.0f;
		State->FrustumBoundingBox = GetFrustumBoundingBox(State->ReferenceCamera);

		State->CameraAcceleration = 60.0f;
		State->dPCamera = {};
		State->YawSpeed = 0.0f;
		State->PitchSpeed = 0.0f;

		State->Camera = State->ReferenceCamera;

		State->MouseXInitial = 0;
		State->MouseYInitial = 0;
		State->MouseDragging = false;

		State->BlinnPhongShininess = 32;
		State->CookTorranceF0 = 0.5f;
		State->CookTorranceM = 0.5f;
		State->Alpha = 0.5f;
		State->Sigma = 0.0f;
		State->LightIntensity = 1.5f;
		State->AmbientFactor = 0.0f;

		State->MicroFoVInDegrees = 90;


		State->GBuffer = CreateGeometryFramebuffer(State->RenderState, GlobalWindowWidth, GlobalWindowHeight);
		State->PreProcess = CreateBasicFramebuffer(State->RenderState, GlobalWindowWidth, GlobalWindowHeight);
		State->PreFXAA = CreateBasicFramebuffer(State->RenderState, GlobalWindowWidth, GlobalWindowHeight);
		State->HemicubeFramebuffer = CreateHemicubeScreenFramebuffer(State->RenderState, GlobalMicrobufferWidth, GlobalMicrobufferHeight);
		State->IndirectIlluminationFramebuffer = CreateBasicFramebuffer(State->RenderState, GlobalWindowWidth, GlobalWindowHeight);

		State->SSAOParams.SampleCount = 0;
		State->SSAOParams.Intensity = 1.0f;
		State->SSAOParams.Scale = 1.0f;
		State->SSAOParams.SamplingRadius = 1.0f;
		State->SSAOParams.Bias = 0.0f;

		State->FXAAParams.MultiplicationFactor = 1.0f / 8.0f;
		State->FXAAParams.MinimalReduction = 1.0f / 128.0f;
		State->FXAAParams.SpanMax = 8.0f;

		State->MotionBlur = true;
		State->PreviousViewProj = Identity4();
		State->MotionBlurSampleCount = 16;

		State->Ks = 1.5f;
		State->Kd = 0.2f;

		// NOTE(hugo) : Initializing Quad data 
		// {
		glGenVertexArrays(1, &State->QuadVAO);
		glGenBuffers(1, &State->QuadVBO);
		BindVertexArray(State->RenderState, State->QuadVAO);
		BindBuffer(State->RenderState, GL_ARRAY_BUFFER, State->QuadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(QuadVertices), &QuadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)(2 * sizeof(GLfloat)));
		BindVertexArray(State->RenderState, 0);
		// }

		// NOTE(hugo) : Initializing Skybox data 
		// {
		glGenVertexArrays(1, &State->SkyboxVAO);
		glGenBuffers(1, &State->SkyboxVBO);
		BindVertexArray(State->RenderState, State->SkyboxVAO);
		BindBuffer(State->RenderState, GL_ARRAY_BUFFER, State->SkyboxVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(SkyboxVertices), &SkyboxVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
		BindVertexArray(State->RenderState, 0);

		const char* SkyboxFilenames[6] = {"../models/skybox/right.jpg", "../models/skybox/left.jpg", "../models/skybox/top.jpg", "../models/skybox/bottom.jpg", "../models/skybox/back.jpg", "../models/skybox/front.jpg"};
		State->SkyboxTexture = LoadCubemap(State, SkyboxFilenames);
		// }

		// NOTE(hugo) : This must be the last command of the initialization of memory
		Memory->IsInitialized = true;
	}
	// TODO(hugo) : I should not enable the depth test each frame. This is a hack that fixes some issues on Linux (Ubuntu 16.04).
	// For more information, go to http://stackoverflow.com/questions/24990637/opengl-radeon-driver-seems-to-mess-with-depth-testing
	Enable(State->RenderState, GL_DEPTH_TEST);
	DepthFunc(State->RenderState, GL_LEQUAL);
	DepthMask(State->RenderState, GL_TRUE);

	if(Input->WindowResized)
	{
		UpdateGeometryFramebuffer(State->RenderState, &State->GBuffer, GlobalWindowWidth, GlobalWindowHeight);
		UpdateBasicFramebuffer(State->RenderState, &State->PreProcess, GlobalWindowWidth, GlobalWindowHeight);
		UpdateBasicFramebuffer(State->RenderState, &State->PreFXAA, GlobalWindowWidth, GlobalWindowHeight);
		UpdateBasicFramebuffer(State->RenderState, &State->IndirectIlluminationFramebuffer, GlobalWindowWidth, GlobalWindowHeight);
	}

	State->ReferenceCamera.Aspect = float(GlobalWindowWidth) / float(GlobalWindowHeight);

	State->Time += Input->dtForFrame;

	ClearColorAndDepth(State->RenderState, V4(0.4f, 0.6f, 0.2f, 1.0f));

	// NOTE(hugo) : Live shader reloading
	if(IsKeyPressed(Input, SCANCODE_TAB))
	{
		for(u32 ShaderIndex = 0; ShaderIndex < ShaderType_Count; ++ShaderIndex)
		{
			glDeleteProgram(State->Shaders[ShaderIndex].Program);
		}
		LoadShaders(State);
	}

	// NOTE(hugo) : Camera management
	State->Camera = State->ReferenceCamera;
	v3 CameraUp = Cross(State->ReferenceCamera.XAxis, Normalized(-1.0f * State->ReferenceCamera.ZAxis));
	switch(State->CameraType)
	{
		case CameraType_Fixed:
			{
				// NOTE(hugo) : Do nothing for a fixed camera
			} break;
		case CameraType_Arcball:
			{
				if(Input->MouseButtons[MouseButton_Right].EndedDown)
				{
					if(!State->MouseDragging)
					{
						State->MouseDragging = true;
						State->MouseXInitial = Input->MouseX;
						State->MouseYInitial = Input->MouseY;
					}
				}

				float DeltaMovement = 1.0f * 0.5f;
				v3 LookingDir = Normalized(-1.0f * State->Camera.ZAxis);
				State->Camera.P += Input->MouseZ * DeltaMovement * LookingDir;

				if(State->MouseDragging)
				{
					s32 DeltaX = Input->MouseX - State->MouseXInitial;
					s32 DeltaY = Input->MouseY - State->MouseYInitial;
					v3 WorldUp = V3(0.0f, 1.0f, 0.0f);
					State->Camera.P = (Rotation(-Sign(Dot(WorldUp, CameraUp)) * Radians(DeltaX), WorldUp) * Rotation(-Radians(DeltaY), State->Camera.XAxis) * ToV4(State->ReferenceCamera.P)).xyz;
					State->Camera.XAxis = (Rotation(-Sign(Dot(WorldUp, CameraUp)) * Radians(DeltaX), V3(0.0f, 1.0f, 0.0f)) * Rotation(-Radians(DeltaY), State->Camera.XAxis) * ToV4(State->ReferenceCamera.XAxis)).xyz;
					State->Camera.ZAxis = Normalized(State->Camera.P - State->FixedTarget);
					v3 LookingDir = - 1.0f * State->Camera.ZAxis;
					CameraUp = Cross(State->Camera.XAxis, LookingDir);

					State->FrustumBoundingBox = GetFrustumBoundingBox(State->Camera);
				}

				if(State->MouseDragging && !(Input->MouseButtons[MouseButton_Right].EndedDown))
				{
					State->ReferenceCamera = State->Camera;
					State->MouseDragging = false;
				}
			} break;
		case CameraType_FirstPerson:
			{
				// NOTE(hugo) : First person camera translation
				float CameraDrag = 10.0f;
				float dt = Input->dtForFrame;
				v3 ddP = {};
				if(IsKeyPressed(Input, SCANCODE_I))
				{
					ddP += Normalized(-1.0f * State->Camera.ZAxis);
				}
				if(IsKeyPressed(Input, SCANCODE_K))
				{
					ddP += -1.0f * Normalized(-1.0f * State->Camera.ZAxis);
				}
				if(IsKeyPressed(Input, SCANCODE_L))
				{
					ddP += Normalized(State->Camera.XAxis);
				}
				if(IsKeyPressed(Input, SCANCODE_J))
				{
					ddP += -1.0f * Normalized(State->Camera.XAxis);
				}

				if(IsKeyPressed(Input, SCANCODE_SPACE))
				{
					ddP += V3(0.0f, 1.0f, 0.0f);
				}
				if(IsKeyPressed(Input, SCANCODE_RSHIFT))
				{
					ddP += -1.0f * V3(0.0f, 1.0f, 0.0f);
				}

				ddP *= State->CameraAcceleration;
				v3 DragForce = -CameraDrag * State->dPCamera;
				ddP += DragForce;
				State->dPCamera += dt * ddP;
				v3 CameraDeltaP = dt * State->dPCamera + 0.5f * dt * dt * ddP;
				State->Camera.P += CameraDeltaP;

				State->ReferenceCamera = State->Camera;

				// NOTE(hugo) : First person camera rotation
				if(Input->MouseButtons[MouseButton_Right].EndedDown)
				{
					if(!State->MouseDragging)
					{
						State->MouseDragging = true;
						State->MouseXInitial = Input->MouseX;
						State->MouseYInitial = Input->MouseY;
					}

					if(State->MouseDragging)
					{
						v3 WorldX = V3(1.0f, 0.0f, 0.0f);
						v3 WorldY = V3(0.0f, 1.0f, 0.0f);
						v3 WorldZ = V3(0.0f, 0.0f, 1.0f);

						float Yaw = GetAngle(WorldX, State->Camera.XAxis, WorldY);
						mat4 YawRotation = Rotation(Yaw, WorldY);
						float Pitch = GetAngle((YawRotation * ToV4(WorldZ)).xyz, State->Camera.ZAxis, (YawRotation * ToV4(WorldX)).xyz);

						s32 DeltaX = -(Input->MouseX - State->MouseXInitial);
						s32 DeltaY = State->MouseYInitial - Input->MouseY;

						State->MouseXInitial = Input->MouseX;
						State->MouseYInitial = Input->MouseY;

						float MouseSensitivity = 30.0f;
						float YawAccel = Radians(MouseSensitivity * DeltaX);
						float PitchAccel = Radians(MouseSensitivity * DeltaY);

						float YawPitchDrag = 10.0f;
						YawAccel += -YawPitchDrag * State->YawSpeed;
						PitchAccel += -YawPitchDrag * State->PitchSpeed;

						// NOTE(hugo) : Taylor Expansion
						float SaveYawSpeed = State->YawSpeed;
						float SavePitchSpeed = State->PitchSpeed;
						State->YawSpeed += YawAccel * dt;
						State->PitchSpeed += PitchAccel * dt;

						Yaw += SaveYawSpeed * dt + 0.5f * dt * dt * YawAccel;
						if(Yaw > PI)
						{
							Yaw = Yaw - 2.0f * PI;
						}
						else if(Yaw < -PI)
						{
							Yaw = Yaw + 2.0f * PI;
						}

						Pitch += SavePitchSpeed * dt + 0.5f * dt * dt * PitchAccel;

						//float CosPitch = Cos(Pitch);
						//float SinPitch = Sin(Pitch);
						float CosYaw = Cos(Yaw);
						float SinYaw = Sin(Yaw);

						State->Camera.XAxis.x = CosYaw;
						State->Camera.XAxis.y = 0.0f;
						State->Camera.XAxis.z = -SinYaw;

						State->Camera.ZAxis.x = SinYaw;
						State->Camera.ZAxis.y = 0.0f;
						State->Camera.ZAxis.z = CosYaw;

						State->Camera.ZAxis = (Rotation(Pitch, State->Camera.XAxis) * ToV4(State->Camera.ZAxis)).xyz;
					}
				}

				if(State->MouseDragging && !(Input->MouseButtons[MouseButton_Right].EndedDown))
				{
					State->MouseDragging = false;
				}

				State->ReferenceCamera = State->Camera;

				State->FrustumBoundingBox = GetFrustumBoundingBox(State->Camera);
			} break;
		InvalidDefaultCase;
	}

	// NOTE(hugo) : Direct lighting rendering
	// {
	switch(State->LightType)
	{
		case LightType_Orthographic:
			{
				State->LightProjectionMatrix = Orthographic(State->ProjectionParams.Width, State->ProjectionParams.Height, State->ProjectionParams.NearPlane, State->ProjectionParams.FarPlane);
			} break;
		case LightType_Perspective:
			{
				State->LightProjectionMatrix = Perspective(State->ProjectionParams.FoV, State->ProjectionParams.Aspect, State->ProjectionParams.NearPlane, State->ProjectionParams.FarPlane);
			} break;
		case LightType_PointLight:
			{
				InvalidCodePath;
			} break;
			InvalidDefaultCase;
	}
	SetViewport(State->RenderState, GlobalShadowWidth, GlobalShadowHeight);
	for(u32 LightIndex = 0; LightIndex < State->LightCount; ++LightIndex)
	{
		light* Light = State->Lights + LightIndex;
		BindFramebuffer(State->RenderState, GL_FRAMEBUFFER, State->Lights[LightIndex].DepthFramebuffer.ID);
		glClear(GL_DEPTH_BUFFER_BIT);
		CullFace(State->RenderState, GL_FRONT);
		camera LightCamera = {};
		LightCamera.P = Light->Pos;
		LightCamera.ZAxis = Normalized(LightCamera.P - Light->Target);
		LightCamera.XAxis = Cross(V3(0.0f, 1.0f, 0.0f), LightCamera.ZAxis);
		LightCamera.FoV = State->ProjectionParams.FoV;
		LightCamera.Aspect = State->ProjectionParams.Aspect;
		LightCamera.NearPlane = State->ProjectionParams.NearPlane;
		LightCamera.FarPlane = State->ProjectionParams.FarPlane;
		rect3 FrustumLightBoundingBox = GetFrustumBoundingBox(LightCamera);
		RenderSimpleScene(State, LightCamera, State->LightProjectionMatrix, &FrustumLightBoundingBox);
		CullFace(State->RenderState, GL_BACK);
	}
	BindFramebuffer(State->RenderState, GL_FRAMEBUFFER, 0);

	SetViewport(State->RenderState, GlobalWindowWidth, GlobalWindowHeight);
	ClearColorAndDepth(State->RenderState, V4(1.0f, 0.0f, 0.5f, 1.0f));

	State->CameraProj = GetCameraPerspective(State->Camera);

	FillGBuffer(State, State->GBuffer, 
			State->Camera, State->CameraProj);
	LightGBuffer(State, State->GBuffer, State->PreProcess,
			State->Camera);
	BindFramebuffer(State->RenderState, GL_FRAMEBUFFER, State->PreFXAA.ID);
	FinalRender(State);
	BindFramebuffer(State->RenderState, GL_FRAMEBUFFER, 0);
	ApplyFXAA(State, State->PreFXAA.Texture);
	// }
	
	State->PreviousViewProj = State->CameraProj * LookAt(State->Camera);

	if(ImGui::Button("Compute Indirect Illumination"))
	{
		ComputeGlobalIlluminationWithPatch(State, State->Camera, State->LightProjectionMatrix);
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
		ImGui::SliderFloat("Ambient factor", (float*)&State->AmbientFactor, 0.0f, 1.0f);
		if(GlobalMicrobufferHeight != GlobalMicrobufferWidth)
		{
			GlobalMicrobufferHeight = GlobalMicrobufferWidth;
		}

		bool UsingSSAO = (State->SSAOParams.SampleCount == 1);
		ImGui::Checkbox("Using SSAO", &UsingSSAO);
		if(UsingSSAO)
		{
			State->SSAOParams.SampleCount = 1;
		}
		else
		{
			State->SSAOParams.SampleCount = 0;
		}

		ImGui::SliderFloat("SSAO Intensity", (float*)&State->SSAOParams.Intensity, 0.0f, 5.0f);
		ImGui::SliderFloat("SSAO Scale", (float*)&State->SSAOParams.Scale, 0.0f, 1.0f);
		ImGui::SliderFloat("SSAO Sampling Radius", (float*)&State->SSAOParams.SamplingRadius, 0.0f, 2.0f);
		ImGui::SliderFloat("SSAO Bias", (float*)&State->SSAOParams.Bias, 0.0f, 1.0f);
		ImGui::Checkbox("Motion Blur", &State->MotionBlur);
		ImGui::SliderInt("Motion Blur Sample Count", (int*)&State->MotionBlurSampleCount, 0, 32);

		ImGui::SliderFloat("FXAA Multiplication Factor", (float*)&State->FXAAParams.MultiplicationFactor, 0.0f, 1.0f);
		ImGui::SliderFloat("FXAA Minimal Reduction", (float*)&State->FXAAParams.MinimalReduction, 0.0f, 1.0f);
		ImGui::SliderFloat("FXAA Span Max", (float*)&State->FXAAParams.SpanMax, 0.0f, 10.0f);

		ImGui::SliderFloat("Ks", (float*)&State->Ks, 0.0f, 10.0f);
		ImGui::SliderFloat("Kd", (float*)&State->Kd, 0.0f, 10.0f);
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
		s32 CameraTypeID = (u32)State->CameraType;
		ImGui::Combo("Camera Type", &CameraTypeID, "Fixed\0Arcball\0First Person\0\0");
		if(CameraTypeID == 0)
		{
			State->CameraType = CameraType_Fixed;
		}
		else if(CameraTypeID == 1)
		{
			State->CameraType = CameraType_Arcball;
		}
		else if(CameraTypeID == 2)
		{
			State->CameraType = CameraType_FirstPerson;
		}
		ImGui::Text("Pos = (%f, %f, %f)", State->Camera.P.x, State->Camera.P.y, State->Camera.P.z);
		ImGui::Text("XAxis = (%f, %f, %f)", State->Camera.XAxis.x, State->Camera.XAxis.y, State->Camera.XAxis.z);
		ImGui::Text("ZAxis = (%f, %f, %f)", State->Camera.ZAxis.x, State->Camera.ZAxis.y, State->Camera.ZAxis.z);
		ImGui::Text("Aspect = %f", State->Camera.Aspect);
		ImGui::SliderFloat("FoV", &State->Camera.FoV, 0.0f, 3.1415f);
		ImGui::SliderFloat("NearPlane", &State->ReferenceCamera.NearPlane, 0.0f, 100.0f);
		ImGui::SliderFloat("FarPlane", &State->ReferenceCamera.FarPlane, State->ReferenceCamera.NearPlane, 200.0f);

		ImGui::Text("Frustum Bounding Box Min: (%f, %f, %f)", State->FrustumBoundingBox.Min.x, State->FrustumBoundingBox.Min.y, State->FrustumBoundingBox.Min.z);
		ImGui::Text("Frustum Bounding Box Max: (%f, %f, %f)", State->FrustumBoundingBox.Max.x, State->FrustumBoundingBox.Max.y, State->FrustumBoundingBox.Max.z);

		v3 WorldX = V3(1.0f, 0.0f, 0.0f);
		v3 WorldY = V3(0.0f, 1.0f, 0.0f);
		v3 WorldZ = V3(0.0f, 0.0f, 1.0f);

		float Yaw = GetAngle(WorldX, State->Camera.XAxis, WorldY);
		float Pitch = GetAngle((Rotation(Yaw, WorldY) * ToV4(WorldZ)).xyz, State->Camera.ZAxis, WorldX);
		ImGui::Text("Yaw = %f", Yaw);
		ImGui::Text("Pitch = %f", Pitch);
		ImGui::Text("Yaw speed = %f", State->YawSpeed);
		ImGui::Text("Pitch speed = %f", State->PitchSpeed);

		ImGui::SliderFloat("Acceleration", &State->CameraAcceleration, 50.0f, 900.0f);
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
				ImGui::Text("Vertex Count: %d", Object->Mesh.VertexCount);
				ImGui::Text("Triangle Count: %d", Object->Mesh.TriangleCount);
				ImGui::Text("Bounding Box Min: (%f, %f, %f)", Object->BoundingBox.Min.x, Object->BoundingBox.Min.y, Object->BoundingBox.Min.z);
				ImGui::Text("Bounding Box Max: (%f, %f, %f)", Object->BoundingBox.Max.x, Object->BoundingBox.Max.y, Object->BoundingBox.Max.z);
				ImGui::Checkbox("Is Frustum Culled", &Object->IsFrustumCulled);
				if(ImGui::TreeNode("Material"))
				{
					if(!IsEmptyString(Object->Material.Name))
					{
						ImGui::Text("Name: %s", Object->Material.Name);
					}
					ImGui::ColorEdit3("Ambient", Object->Material.AmbientColor.E);
					ImGui::ColorEdit3("Diffuse", Object->Material.DiffuseColor.E);
					ImGui::ColorEdit3("Specular", Object->Material.SpecularColor.E);
					ImGui::SliderFloat("Specular Component", &Object->Material.SpecularComponent, 0.0f, 10.0f);
					ImGui::TreePop();
				}
				ImGui::TreePop();
			}
		}
	}
#endif

	ImGui::PlotLines("FPS", &DEBUGCounters[0], ArrayCount(DEBUGCounters));
	ImGui::PlotLines("OpenGL Stage Changes", &DEBUGRenderStateChangeCounters[0], ArrayCount(DEBUGRenderStateChangeCounters));
}
