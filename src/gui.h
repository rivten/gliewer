#pragma once

#include <imgui_demo.cpp>

void HandleGUI(game_state* State)
{
	if(ImGui::CollapsingHeader("GI"))
	{
		ImGui::Checkbox("Save First MegaTexture", &State->SaveFirstMegaTexture);
		if(ImGui::Button("Compute Indirect Illumination"))
		{
			ComputeGlobalIlluminationWithPatch(State, 
					State->Camera, 
					State->LightProjectionMatrix, State->PatchSizeInPixels,
					State->SaveFirstMegaTexture);
		}
		ImGui::SliderInt("Microbuffer Pixel Size", (int*)&GlobalMicrobufferWidth, 0, 128);
		if(GlobalMicrobufferHeight != GlobalMicrobufferWidth)
		{
			GlobalMicrobufferHeight = GlobalMicrobufferWidth;
		}
	}

	if(ImGui::Button("Screenshot"))
	{
		ScreenshotBufferAttachment("screenshot.png",
				State->RenderState, State->PreFXAA.ID,
				0, GlobalWindowWidth, GlobalWindowHeight,
				GL_RGBA, GL_UNSIGNED_BYTE);
	}

#if 1
	if(ImGui::CollapsingHeader("BRDF Data"))
	{
		//ImGui::SliderInt("Blinn-Phong Shininess", (int*)&State->BlinnPhongShininess, 1, 256);
		ImGui::SliderFloat("Cook-Torrance F0", (float*)&State->CookTorranceF0, 0.0f, 1.0f);
		ImGui::SliderFloat("Cook-Torrance M", (float*)&State->CookTorranceM, 0.0f, 1.0f);
		ImGui::SliderFloat("Blur Sigma", (float*)&State->Sigma, 0.0f, 50.0f);
		ImGui::SliderFloat("Alpha", (float*)&State->Alpha, 0.0f, 1.0f);
		ImGui::SliderFloat("Ambient factor", (float*)&State->AmbientFactor, 0.0f, 1.0f);

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
			if(StringEmpty(Object->Name))
			{
				sprintf(Buffer, "Object #%i", ObjectIndex);
			}
			else
			{
				sprintf(Buffer, "Object #%i (%s)", ObjectIndex, Object->Name);
			}
			if(ImGui::TreeNode(Buffer))
			{
				if(!StringEmpty(Object->Name))
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
					if(!StringEmpty(Object->Material.Name))
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

	//ImGui::ShowTestWindow();
}
