#pragma once

/*
   TODO(hugo)
     * Basic GLiewer todos
		- proper SDL-ImGui layer
		- proper normal / UV etc. computation in the normal / tangent / bitangent space
		- full material handling when OBJ parsing
		- asset streaming
	    - split the viewer.cpp code into several files
		- display frustum
		- display bounding boxes
		- display triangle mesh properly
		- non-axis aligned bounding boxes
		- light attenuation
		- normal mapping
	    - mesh code cleanup
		- possibility to see light depth buffer from GUI
		- light handling has objects having an emissivity
		- check memory footprint
		- sorting objects by bounding box for rendering (to avoid too many
			fragment computations) ?
		- logging system
		- ray tracing ability
	    - SDL code cleanup (cleaner layer, e.g. use of DLL for the main "game", etc...)
		- get rid of glew
	    - proper OBJ loading
	    - profiling
		- Mouse Wheel clean up
	    - bloom
	    - automatic reloading of shaders (as in http://swarminglogic.com/jotting/2013_10_gamedev01)
		- automatic shader gestion I/O to chain easily shaders
		- BHV if ever necessary
		- BRDF visualisation
		- cascade shadow mapping
		- voxelisation
		- lighting shader code cleanup (use structure)
		- subsurface scattering
		- mesh parametrization
		- mesh simplification

	* Some ideas
		- framebuffer have an OpenGL structure that I could exploit (with color attachment). For example, 
			I know that if I want to read into the normal map, I could read a mapping that says that the
			normal map is the nth color attachment of this framebuffer.

	* PBGI todos
		- fix mega texture sampling in the shader
		- do not pass through the CPU when computing the mega texture
		- glfw layer
 */

struct camera
{
	v3 P;
	v3 XAxis;
	v3 ZAxis;
	float FoV;
	float Aspect;
	float NearPlane;
	float FarPlane;
};

enum light_type
{
	LightType_Orthographic,
	LightType_Perspective,
	LightType_PointLight,
};

enum camera_type
{
	CameraType_Fixed,
	CameraType_Arcball,
	CameraType_FirstPerson,
};

struct projection_parameters
{
	union
	{
		struct
		{
			float FoV;
			float Aspect;
		};
		struct
		{
			float Width;
			float Height;
		};
	};

	float NearPlane;
	float FarPlane;
};

struct light
{
	// NOTE(hugo) : If Object = NULL, then the light must not be rendered
	object* Object;
	v3 Pos;
	v4 Color;
	v3 Target;

	depth_framebuffer DepthFramebuffer;
};

struct ambient_occlusion_parameters
{
	u32 SampleCount;
	float Intensity;
	float Scale;
	float Bias;
	float SamplingRadius;
};

struct fxaa_params
{
	float MultiplicationFactor;
	float MinimalReduction;
	float SpanMax;
};

struct game_state
{
	object Objects[512];
	u32 ObjectCount;
	mat4 ObjectModelMatrix;

	light Lights[4];
	u32 LightCount;

	light_type LightType;
	projection_parameters ProjectionParams;

	shader Shaders[ShaderType_Count];

	camera_type CameraType;
	camera ReferenceCamera;
	camera Camera;
	v3 FixedTarget;
	float Time;
	mat4 CameraProj;

	float CameraAcceleration;
	v3 dPCamera;
	float YawSpeed;
	float PitchSpeed;

	s32 MouseXInitial;
	s32 MouseYInitial;
	bool MouseDragging;

	// NOTE(hugo) : Render parameters
	// {
	u32 BlinnPhongShininess;
	float CookTorranceF0;
	float CookTorranceM;
	float Alpha;
	float Sigma;
	float LightIntensity;
	float AmbientFactor;
	ambient_occlusion_parameters SSAOParams;
	fxaa_params FXAAParams;
	float Ks;
	float Kd;
	// }

	bool MotionBlur;
	mat4 PreviousViewProj;
	u32 MotionBlurSampleCount;

	geometry_framebuffer GBuffer;
	geometry_framebuffer ScreenFramebuffer;
	geometry_framebuffer PreFXAAFramebuffer;
	hemicube_framebuffer HemicubeFramebuffer;
	geometry_framebuffer IndirectIlluminationFramebuffer;

	GLuint RBO;
	GLuint QuadVAO;
	GLuint QuadVBO;

	GLuint SkyboxVAO;
	GLuint SkyboxVBO;
	GLuint SkyboxTexture;

	u32 MicroFoVInDegrees;

	render_state* RenderState;
	rect3 FrustumBoundingBox;
};

#include "viewer.cpp"

