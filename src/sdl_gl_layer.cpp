#include "platform.h"
//#include "viewer.cpp"

// NOTE(hugo): The includes below are for the SDL/GL layer only
#include <SDL.h>
#define GLEW_STATIC
#include <GL\glew.h>
#include <stdio.h>

#include "math/vector.h"
#include "math/functions.h"
#include "math/matrix.h"

global_variable bool GlobalRunning = false;
global_variable const int GlobalWindowWidth = 512;
global_variable const int GlobalWindowHeight = 512;

#include "shader.h"
#include "mesh.h"

struct game_state
{
	mesh Mesh;
	shader BasicShader;
	float Time;
};

void GameUpdateAndRender(thread_context* Thread, game_memory* Memory, game_input* Input, game_offscreen_buffer* Screenbuffer)
{
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	game_state* State = (game_state*)Memory->PermanentStorage;
	if(!Memory->IsInitialized)
	{
		State->Mesh = LoadOBJ("../models/teapot.obj");
		State->BasicShader = shader("../src/shaders/basic_v.glsl", "../src/shaders/basic_f.glsl");
		State->Time = 0.0f;

		// NOTE(hugo) : This must be the last command of the initialization of memory
		Memory->IsInitialized = true;
	}

	glEnable(GL_DEPTH_TEST);

	State->Time += Input->dtForFrame;

	glClearColor(0.4f, 0.6f, 0.2f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#if 0
	v3 Vertices[] = 
	{
	    V3(-1.0f, -1.0f,  1.0f), V3(1.0f, 0.0f, 0.0f),
	    V3( 1.0f, -1.0f,  1.0f), V3(1.0f, 1.0f, 0.0f),
	    V3(-1.0f,  1.0f,  1.0f), V3(0.0f, 1.0f, 0.0f),
	    V3( 1.0f,  1.0f,  1.0f), V3(0.0f, 1.0f, 1.0f),
	    V3(-1.0f, -1.0f, -1.0f), V3(0.0f, 0.0f, 1.0f),
	    V3( 1.0f, -1.0f, -1.0f), V3(1.0f, 0.0f, 1.0f),
	    V3(-1.0f,  1.0f, -1.0f), V3(1.0f, 1.0f, 1.0f),
	    V3( 1.0f,  1.0f, -1.0f), V3(0.0f, 0.0f, 0.0f),
	};

	mat4 ModelMatrix = Translation(V3(0.5f, 0.0f, 0.0f)) * Rotation(GlobalTime, V3(0.0f, 1.0f, 0.0f)) * Scaling(V3(0.2f, 0.2f, 0.2f));
	mat4 ViewMatrix = LookAt(V3(0.0f, 1.0f, 0.0f), V3(0.2f, 0.0f, 0.0f), V3(0.0f, 0.0f, 1.0f));
	mat4 ProjectionMatrix = Perspective(Radians(60), GlobalWindowWidth / GlobalWindowHeight, 0.1f, 10.0f);

	for(u32 VertexIndex = 0; VertexIndex < ArrayCount(Vertices); VertexIndex++)
	{
		if(VertexIndex % 2 == 0)
		{
			Vertices[VertexIndex] = (ProjectionMatrix * ViewMatrix * ModelMatrix * ToV4(Vertices[VertexIndex])).xyz;
		}
	}

	GLuint Indices[] =
	{
		0, 1, 3,
		0, 3, 2,
		1, 5, 7,
		1, 7, 3,
		5, 4, 6,
		5, 6, 7,
		4, 0, 2,
		4, 2, 6,
		3, 6, 7,
		3, 2, 6,
		1, 4, 5,
		1, 0, 4,
	};

	GLuint TriangleVAO;
	glGenVertexArrays(1, &TriangleVAO);
	GLuint TriangleVBO;
	glGenBuffers(1, &TriangleVBO);
	GLuint TriangleEBO;
	glGenBuffers(1, &TriangleEBO);

	glBindVertexArray(TriangleVAO);

	glBindBuffer(GL_ARRAY_BUFFER, TriangleVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, TriangleEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
	shader BasicShader = shader("shaders/basic_v.glsl", "shaders/basic_f.glsl");

	UseShader(BasicShader);
	glBindVertexArray(TriangleVAO);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
#endif
	//State->Mesh.ModelMatrix = Rotation(State->Time, V3(0.0f, 1.0f, 0.0f)) * Scaling(V3(0.2f, 0.2f, 0.2f));
	State->Mesh.ModelMatrix = Scaling(V3(0.2f, 0.2f, 0.2f));
	v3 CameraPos = V3(0.0f, 0.0f, 5.0f);
	CameraPos = (Rotation(State->Time, V3(0.0f, 1.0f, 0.0f)) * ToV4(CameraPos)).xyz;
	v3 CameraTarget = V3(0.0f, 0.0f, 0.0f);
	mat4 ViewMatrix = LookAt(CameraPos, CameraTarget, V3(0.0f, 1.0f, 0.0f));
	mat4 ProjectionMatrix = Perspective(Radians(45), float(GlobalWindowWidth) / float(GlobalWindowHeight), 0.5f, 30.0f);

	mat4 MVPMatrix = ProjectionMatrix * ViewMatrix * State->Mesh.ModelMatrix;

	UseShader(State->BasicShader);
	GLuint MVPMatrixLocation = glGetUniformLocation(State->BasicShader.Program, "MVPMatrix");
	glUniformMatrix4fv(MVPMatrixLocation, 1, GL_FALSE, MVPMatrix.Data_); 

	mat4 NormalMatrix = Transpose(Inverse(ViewMatrix * State->Mesh.ModelMatrix));
	GLuint NormalMatrixLocation = glGetUniformLocation(State->BasicShader.Program, "NormalMatrix");
	glUniformMatrix4fv(NormalMatrixLocation, 1, GL_FALSE, NormalMatrix.Data_); 

	GLuint ViewMatrixLocation = glGetUniformLocation(State->BasicShader.Program, "ViewMatrix");
	glUniformMatrix4fv(ViewMatrixLocation, 1, GL_FALSE, ViewMatrix.Data_); 

	DrawTrianglesMesh(&State->Mesh);

}

static void SDLProcessKeyboardMessage(game_button_state* NewButtonState, bool IsDown)
{
    if(NewButtonState->EndedDown != IsDown)
    {
        NewButtonState->EndedDown = IsDown;
        ++NewButtonState->HalfTransitionCount;
    }
}

float SDLProcessJoystickStickValue(s16 StickValue, float DeadZoneThreshold)
{
    float Result = 0.0f;

    if(StickValue < - DeadZoneThreshold)
    {
        Result = (((float)(StickValue)) + DeadZoneThreshold) / (32768.0f - DeadZoneThreshold);
    }
    else if(StickValue > DeadZoneThreshold)
    {
        Result = (((float)(StickValue)) - DeadZoneThreshold) / (32767.0f - DeadZoneThreshold);
    }

    return(Result);
}

static void SDLProcessDigitalButton(game_button_state* OldButtonState, game_button_state* NewButtonState, bool IsDown)
{
    NewButtonState->EndedDown = IsDown;
    NewButtonState->HalfTransitionCount = (OldButtonState->EndedDown != NewButtonState->EndedDown) ? 1 : 0;
}

static void SDLProcessDigitalButton(game_button_state* OldButtonState, game_button_state* NewButtonState, Uint8 CurrentButtonState)
{
    bool IsDown = (CurrentButtonState == 1);
    SDLProcessDigitalButton(OldButtonState, NewButtonState, IsDown);
}

static void SDLProcessPendingMessages(game_controller_input* KeyboardController)
{
    SDL_Event Event;
    // TODO(hugo) : Peep or Poll ? 
    while(SDL_PollEvent(&Event))
    {
        switch(Event.type)
        {
            case SDL_QUIT:
                {
                    GlobalRunning = false;
                } break;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                {
                    SDL_Keycode Keycode = Event.key.keysym.sym;
                    bool WasDown = ((Event.key.state == SDL_RELEASED) || (Event.key.repeat != 0));
                    bool IsDown = (Event.key.state == SDL_PRESSED);
                    if(IsDown != WasDown)
                    {
                        if(Keycode == SDLK_z)
                        {
                            SDLProcessKeyboardMessage(&KeyboardController->MoveUp, IsDown);
                        }
                        else if(Keycode == SDLK_q)
                        {
                            SDLProcessKeyboardMessage(&KeyboardController->MoveLeft, IsDown);
                        }
                        else if(Keycode == SDLK_s)
                        {
                            SDLProcessKeyboardMessage(&KeyboardController->MoveDown, IsDown);
                        }
                        else if(Keycode == SDLK_d)
                        {
                            SDLProcessKeyboardMessage(&KeyboardController->MoveRight, IsDown);
                        }
                        else if(Keycode == SDLK_a)
                        {
                            SDLProcessKeyboardMessage(&KeyboardController->LeftShoulder, IsDown);
                        }
                        else if(Keycode == SDLK_e)
                        {
                            SDLProcessKeyboardMessage(&KeyboardController->RightShoulder, IsDown);
                        }
                        else if(Keycode == SDLK_UP)
                        {
                            SDLProcessKeyboardMessage(&KeyboardController->ActionUp, IsDown);
                        }
                        else if(Keycode == SDLK_LEFT)
                        {
                            SDLProcessKeyboardMessage(&KeyboardController->ActionLeft, IsDown);
                        }
                        else if(Keycode == SDLK_DOWN)
                        {
                            SDLProcessKeyboardMessage(&KeyboardController->ActionDown, IsDown);
                        }
                        else if(Keycode == SDLK_RIGHT)
                        {
                            SDLProcessKeyboardMessage(&KeyboardController->ActionRight, IsDown);
                        }
                        else if(Keycode == SDLK_ESCAPE)
                        {
                            SDLProcessKeyboardMessage(&KeyboardController->Back, IsDown);
                        }
                        else if(Keycode == SDLK_SPACE)
                        {
                            SDLProcessKeyboardMessage(&KeyboardController->Start, IsDown);
                        }
                    }
                } break;

            default:
                {
                } break;
        }
    }
}

void SDLProcessKeyboardState(game_input* Input)
{
    int KeyCount;
    const u8* KeyboardState = SDL_GetKeyboardState(&KeyCount);
    // TODO(hugo) : Evaluate performance of this compared to a C copy using sprintf or an equivalent
    for(s32 KeyIndex = 0; KeyIndex < KeyCount; ++KeyIndex)
    {
        Input->KeyboardButtons[KeyIndex] = KeyboardState[KeyIndex];
    }
}

int main(int argc, char** argv)
{
    SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO);

    SDL_Window* Window = SDL_CreateWindow("3d viewer @ rivten", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            GlobalWindowWidth, GlobalWindowHeight, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);

    if(!Window)
    {
        return(1);
    }

	// NOTE(hugo) : Disabling SDL_GL_ACCELERATED_VISUAL seems to get rid of flickering. Why ?
    //SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetSwapInterval(0);

    SDL_GLContext GLContext = SDL_GL_CreateContext(Window);

    // NOTE(hugo) : Initializating glew
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();
    if(GlewInitResult == GLEW_OK)
    { 
        GlobalRunning = true;

        u32 MonitorRefreshHz = 60;
        SDL_DisplayMode DisplayMode = {};
        if(SDL_GetCurrentDisplayMode(0, &DisplayMode) == 0)
        {
            if(DisplayMode.refresh_rate != 0)
            {
                MonitorRefreshHz = DisplayMode.refresh_rate;
            }
        }
        float GameUpdateHz = (MonitorRefreshHz / 2.0f);
        float TargetSecondsPerFrame = 1.0f / (float)GameUpdateHz;

        game_memory GameMemory = {};
        GameMemory.PermanentStorageSize = Megabytes(256);
        GameMemory.TransientStorageSize = Gigabytes(1);

        memory_index TotalMemorySize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
        void* GameMemoryBlock = malloc(TotalMemorySize);
        GameMemory.PermanentStorage = GameMemoryBlock;
        GameMemory.TransientStorage = ((u8*)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);
        
        if(GameMemory.PermanentStorage && GameMemory.TransientStorage)
        {
            game_input Input[2] = {};
            game_input* OldInput = Input;
            game_input* NewInput = Input + 1;
            u32 LastCounter = SDL_GetTicks();
            while(GlobalRunning)
            {
                NewInput->dtForFrame = TargetSecondsPerFrame;
                game_controller_input* OldKeyboardController = GetController(OldInput, 0);
                game_controller_input* NewKeyboardController = GetController(NewInput, 0);
                *NewKeyboardController = {};
                NewKeyboardController->IsConnected = true;
                for(u32 ButtonIndex = 0; ButtonIndex < ArrayCount(NewKeyboardController->Buttons); ++ButtonIndex)
                {
                    NewKeyboardController->Buttons[ButtonIndex].EndedDown = 
                        OldKeyboardController->Buttons[ButtonIndex].EndedDown;
                }

                SDLProcessPendingMessages(NewKeyboardController);

                SDLProcessKeyboardState(NewInput);

                SDL_GetMouseState(&NewInput->MouseX, &NewInput->MouseY);
                SDLProcessKeyboardMessage(&NewInput->MouseButtons[0], (SDL_GetMouseState(0, 0) & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0);
                SDLProcessKeyboardMessage(&NewInput->MouseButtons[1], (SDL_GetMouseState(0, 0) & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0);
                SDLProcessKeyboardMessage(&NewInput->MouseButtons[2], (SDL_GetMouseState(0, 0) & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0);

                u32 MaxControllerCount = ArrayCount(NewInput->Controllers) - 1;
                u32 ControllerCount = SDL_NumJoysticks();
                Assert(ControllerCount <= MaxControllerCount);
                SDL_JoystickUpdate();
                for(u32 ControllerIndex = 0; ControllerIndex < MaxControllerCount; ++ControllerIndex)
                {
                    u32 OurControllerIndex = ControllerIndex + 1;
                    game_controller_input* OldController = GetController(OldInput, OurControllerIndex);
                    game_controller_input* NewController = GetController(NewInput, OurControllerIndex);

                    SDL_Joystick* Joystick = SDL_JoystickOpen(ControllerIndex);
                    if(Joystick)
                    {
                        NewController->IsConnected = true; 
                        NewController->IsAnalog = OldController->IsAnalog;

                        float JoystickAxisThreshold = 32767.0f / 10.0f;
                        NewController->StickAverageX = SDLProcessJoystickStickValue(SDL_JoystickGetAxis(Joystick, 0), JoystickAxisThreshold);
                        NewController->StickAverageY = -SDLProcessJoystickStickValue(SDL_JoystickGetAxis(Joystick, 1), JoystickAxisThreshold);
                        if((NewController->StickAverageX != 0.0f) ||
                                (NewController->StickAverageY != 0.0f))
                        {
                            NewController->IsAnalog = true;
                        }

                        // TODO(hugo) : BUTTTTTOOOOONS
                        if(SDL_JoystickGetButton(Joystick, 0))
                        {
                            NewController->StickAverageY = 1.0f;
                            NewController->IsAnalog = false;
                        }
                        if(SDL_JoystickGetButton(Joystick, 1))
                        {
                            NewController->StickAverageY = -1.0f;
                            NewController->IsAnalog = false;
                        }
                        if(SDL_JoystickGetButton(Joystick, 2))
                        {
                            NewController->StickAverageX = -1.0f;
                            NewController->IsAnalog = false;
                        }
                        if(SDL_JoystickGetButton(Joystick, 3))
                        {
                            NewController->StickAverageX = 1.0f;
                            NewController->IsAnalog = false;
                        }

                        float Threshold = 0.5f;
                        SDLProcessDigitalButton(&OldController->MoveLeft, &NewController->MoveLeft, (NewController->StickAverageX < -Threshold));
                        SDLProcessDigitalButton(&OldController->MoveRight, &NewController->MoveRight, (NewController->StickAverageX > Threshold));
                        SDLProcessDigitalButton(&OldController->MoveUp, &NewController->MoveUp, (NewController->StickAverageY > Threshold));
                        SDLProcessDigitalButton(&OldController->MoveDown, &NewController->MoveDown, (NewController->StickAverageY < -Threshold));

                        // TODO(hugo) : Find out which button is which because I took them from a website and they do not seem to fit to what I would expect
                        SDLProcessDigitalButton(&OldController->ActionDown, &NewController->ActionDown, SDL_JoystickGetButton(Joystick, 10));
                        SDLProcessDigitalButton(&OldController->ActionRight, &NewController->ActionRight, SDL_JoystickGetButton(Joystick, 11));
                        SDLProcessDigitalButton(&OldController->ActionLeft, &NewController->ActionLeft, SDL_JoystickGetButton(Joystick, 12));
                        SDLProcessDigitalButton(&OldController->ActionUp, &NewController->ActionUp, SDL_JoystickGetButton(Joystick, 13));

                        SDLProcessDigitalButton(&OldController->LeftShoulder, &NewController->LeftShoulder, SDL_JoystickGetButton(Joystick, 8));
                        SDLProcessDigitalButton(&OldController->RightShoulder, &NewController->RightShoulder, SDL_JoystickGetButton(Joystick, 9));

                        SDLProcessDigitalButton(&OldController->Start, &NewController->Start, SDL_JoystickGetButton(Joystick, 4));
                        SDLProcessDigitalButton(&OldController->Back, &NewController->Back, SDL_JoystickGetButton(Joystick, 5));
                    }
                    else
                    {
                        NewController->IsConnected = false;
                    }
                }

                thread_context Thread = {};
                
                // TODO(hugo) : Get the screen earlier to avoid re-getting each frame ? But how to handle screen redimensionning in this case ?

                SDL_Surface* Screen = SDL_GetWindowSurface(Window);
                game_offscreen_buffer Buffer = {};
                Buffer.Memory = (void*)Screen->pixels;
                Buffer.Width = Screen->w;
                Buffer.Height = Screen->h;
                Buffer.Pitch = Screen->pitch;

                GameUpdateAndRender(&Thread, &GameMemory, NewInput, &Buffer);
                SDL_GL_SwapWindow(Window);

                // TODO(hugo) : Sound in SDL !!
                float WorkCounter = SDL_GetTicks();
                float WorkSecondsElapsed = ((float)(WorkCounter - LastCounter)) / 1000.0f;

                float SecondsElapsedForFrame = WorkSecondsElapsed;
#if RIVTEN_SLOW
                char WindowTitleBuffer[128];
                sprintf_s(WindowTitleBuffer, sizeof(WindowTitleBuffer), "3D viewer @ rivten - %dms - mouse pos (%d, %d)", (int)(SecondsElapsedForFrame * 1000.0f), Input[1].MouseX, Input[1].MouseY);
                SDL_SetWindowTitle(Window, WindowTitleBuffer);
#endif
                if(SecondsElapsedForFrame < TargetSecondsPerFrame)
                {
                    u32 SleepMS = (u32)(1000.0f * (TargetSecondsPerFrame - SecondsElapsedForFrame));
                    if(SleepMS > 0)
                    {
                        SDL_Delay(SleepMS);
                    }
                }
                else
                {
                    // TODO(hugo) : Missed framerate
                }

                float EndWorkCounter = SDL_GetTicks();
                //SDL_Log("%ums", (u32)(1000.0f * WorkSecondsElapsed));
                LastCounter = EndWorkCounter;

				// TODO(hugo) : Formalize how I handle ScreenBuffer with the game architecture
                //SDL_UpdateWindowSurface(Window);

                game_input* Temp = NewInput;
                NewInput = OldInput;
                OldInput = Temp;
            }
        }
    }
    else
    {
        SDL_Log("%s", glewGetErrorString(GlewInitResult));
        return(1);
    }

    SDL_GL_DeleteContext(GLContext);
    SDL_DestroyWindow(Window);
    SDL_Quit();
    return(0);
}
