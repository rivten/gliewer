#include "platform.h"

// NOTE(hugo): The includes below are for the SDL/GL layer only
#ifdef _WIN32
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#define GLEW_STATIC
#include <GL/glew.h>
#include <stdio.h>

#include "imgui_layer.h"

#include "rivten_math.h"

global_variable bool GlobalRunning = false;
global_variable u32 GlobalWindowWidth = 2 * 256;
//global_variable u32 GlobalWindowWidth = 128;
global_variable u32 GlobalWindowHeight = GlobalWindowWidth;
global_variable SDL_Window* GlobalWindow = 0;

const u32 FrameTrackingCount = 128;
global_variable float DEBUGCounters[FrameTrackingCount];
global_variable u32 DEBUGCurrentCounter = 0;

global_variable float DEBUGRenderStateChangeCounters[FrameTrackingCount];
global_variable u32 DEBUGGLCurrentFrameStateChangeCount = 0;
global_variable u32 DEBUGRenderStateChangeCurrentCounter = 0;

#include "shader.h"
#include "gl_layer.h"
#include "mesh.h"

#include "global_illumination.h"
#include "viewer.h"
#include "viewer.cpp"

struct platform_work_queue;
#define PLATFORM_WORK_QUEUE_CALLBACK(name) void name(platform_work_queue* Queue, void* Data)
typedef PLATFORM_WORK_QUEUE_CALLBACK(platform_work_queue_callback);

struct platform_work_queue_entry
{
	platform_work_queue_callback* Callback;
	void* Data;
};

struct platform_work_queue
{
	u32 volatile CompletionGoal;
	u32 volatile CompletionCount;

	u32 volatile NextEntryToRead;
	u32 volatile NextEntryToWrite;

	SDL_sem* SemaphoreHandle;
	platform_work_queue_entry Entries[256];
};

struct sdl_thread_startup
{
	platform_work_queue* Queue;
};

static void SDLAddEntry(platform_work_queue* Queue, platform_work_queue_callback* Callback, void* Data)
{
	u32 NewNextEntryToWrite = (Queue->NextEntryToWrite + 1) % ArrayCount(Queue->Entries);
	Assert(NewNextEntryToWrite != Queue->NextEntryToRead);
	platform_work_queue_entry* Entry = Queue->Entries + Queue->NextEntryToWrite;
	Entry->Callback = Callback;
	Entry->Data = Data;
	++Queue->CompletionGoal;
	SDL_CompilerBarrier();
	Queue->NextEntryToWrite = NewNextEntryToWrite;
	SDL_SemPost(Queue->SemaphoreHandle);
}

static bool SDLDoNextWorkQueueEntry(platform_work_queue* Queue)
{
	bool WeShouldSleep = false;

	u32 OriginalNextEntryToRead = Queue->NextEntryToRead;
	u32 NewNextEntryToRead = (OriginalNextEntryToRead + 1) % ArrayCount(Queue->Entries);
	if(OriginalNextEntryToRead != Queue->NextEntryToWrite)
	{
		SDL_bool WasSet = SDL_AtomicCAS((SDL_atomic_t *)&Queue->NextEntryToRead,
				OriginalNextEntryToRead, NewNextEntryToRead);

		if(WasSet)
		{
			platform_work_queue_entry Entry = Queue->Entries[OriginalNextEntryToRead];
			Entry.Callback(Queue, Entry.Data);
			SDL_AtomicIncRef((SDL_atomic_t *)&Queue->CompletionCount);
		}
	}
	else
	{
		WeShouldSleep = true;
	}

	return(WeShouldSleep);
}

static void SDLCompleteAllWork(platform_work_queue* Queue)
{
	while(Queue->CompletionGoal != Queue->CompletionCount)
	{
		SDLDoNextWorkQueueEntry(Queue);
	}

	Queue->CompletionGoal = 0;
	Queue->CompletionCount = 0;
}

int ThreadProc(void* Parameter)
{
	sdl_thread_startup* Thread = (sdl_thread_startup *)Parameter;
	platform_work_queue* Queue = Thread->Queue;

	for(;;)
	{
		if(SDLDoNextWorkQueueEntry(Queue))
		{
			SDL_SemWait(Queue->SemaphoreHandle);
		}
	}
}

static void SDLMakeQueue(platform_work_queue* Queue, u32 ThreadCount,
		sdl_thread_startup* Startups)
{
	Queue->CompletionGoal = 0;
	Queue->CompletionCount = 0;

	Queue->NextEntryToWrite = 0;
	Queue->NextEntryToRead = 0;

	u32 InitialCount = 0;
	Queue->SemaphoreHandle = SDL_CreateSemaphore(InitialCount);

	for(u32 ThreadIndex = 0; ThreadIndex < ThreadCount; ++ThreadIndex)
	{
		sdl_thread_startup* Startup = Startups + ThreadIndex;
		Startup->Queue = Queue;

		SDL_Thread* ThreadHandle = SDL_CreateThread(ThreadProc, 0, Startup);
		SDL_DetachThread(ThreadHandle);
	}
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

static void SDLProcessPendingMessages(game_input* Input)
{
    SDL_Event Event;
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
                            SDLProcessKeyboardMessage(&Input->Controllers[0].MoveUp, IsDown);
                        }
                        else if(Keycode == SDLK_q)
                        {
                            SDLProcessKeyboardMessage(&Input->Controllers[0].MoveLeft, IsDown);
                        }
                        else if(Keycode == SDLK_s)
                        {
                            SDLProcessKeyboardMessage(&Input->Controllers[0].MoveDown, IsDown);
                        }
                        else if(Keycode == SDLK_d)
                        {
                            SDLProcessKeyboardMessage(&Input->Controllers[0].MoveRight, IsDown);
                        }
                        else if(Keycode == SDLK_a)
                        {
                            SDLProcessKeyboardMessage(&Input->Controllers[0].LeftShoulder, IsDown);
                        }
                        else if(Keycode == SDLK_e)
                        {
                            SDLProcessKeyboardMessage(&Input->Controllers[0].RightShoulder, IsDown);
                        }
                        else if(Keycode == SDLK_UP)
                        {
                            SDLProcessKeyboardMessage(&Input->Controllers[0].ActionUp, IsDown);
                        }
                        else if(Keycode == SDLK_LEFT)
                        {
                            SDLProcessKeyboardMessage(&Input->Controllers[0].ActionLeft, IsDown);
                        }
                        else if(Keycode == SDLK_DOWN)
                        {
                            SDLProcessKeyboardMessage(&Input->Controllers[0].ActionDown, IsDown);
                        }
                        else if(Keycode == SDLK_RIGHT)
                        {
                            SDLProcessKeyboardMessage(&Input->Controllers[0].ActionRight, IsDown);
                        }
                        else if(Keycode == SDLK_ESCAPE)
                        {
                            SDLProcessKeyboardMessage(&Input->Controllers[0].Back, IsDown);
                        }
                        else if(Keycode == SDLK_SPACE)
                        {
                            SDLProcessKeyboardMessage(&Input->Controllers[0].Start, IsDown);
                        }
						else if(Keycode == SDLK_c)
						{
							GlobalRunning = false;
						}
                    }
                } break;

            case SDL_MOUSEWHEEL:
				{
					Input->MouseZ = Event.wheel.y;
				} break;

			case SDL_TEXTINPUT:
				{
					memcpy(Input->Text, Event.text.text, 
							ArrayCount(Input->Text));
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
	memcpy(Input->KeyboardButtons, KeyboardState, KeyCount);
}

PLATFORM_WORK_QUEUE_CALLBACK(PrintNumber)
{
	int* Number = (int *)Data;
	printf("%d\n", *Number);
}

int main(int argc, char** argv)
{
    SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO);

#if 0
	SDL_version Compiled;
	SDL_version Linked;

	SDL_VERSION(&Compiled);
	SDL_GetVersion(&Linked);
	printf("We compiled against SDL version %d.%d.%d ...\n",
		   Compiled.major, Compiled.minor, Compiled.patch);
	printf("We are linking against SDL version %d.%d.%d.\n",
		   Linked.major, Linked.minor, Linked.patch);
#endif

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);

	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	//SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
	SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
#ifdef _WIN32
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 1);
#endif

    SDL_Window* Window = SDL_CreateWindow("3d viewer @ rivten", 
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
			GlobalWindowWidth, GlobalWindowHeight, 
			SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	GlobalWindow = Window;

    if(!Window)
    {
		const char* Error = SDL_GetError();
		SDL_Log("Window could not be created : %s", Error);
        return(1);
    }

	sdl_thread_startup HighPriorityStartups[6] = {};
	platform_work_queue HighPriorityQueue = {};
	SDLMakeQueue(&HighPriorityQueue, 
			ArrayCount(HighPriorityStartups), HighPriorityStartups);

	SDL_GLContext GLContext = SDL_GL_CreateContext(Window);
	Assert(GLContext != 0);

	bool UseVSync = false;

	if(UseVSync)
	{
		s32 VSyncResult = SDL_GL_SetSwapInterval(1);
		if(VSyncResult == -1)
		{
			VSyncResult = SDL_GL_SetSwapInterval(0);
			Assert(VSyncResult == 0);
			SDL_Log("VSync was not enabled.");
		}
		else
		{
			SDL_Log("VSync is enabled.");
		}
	}
	else
	{
		SDL_GL_SetSwapInterval(0);
	}

    // NOTE(hugo) : Initializating glew
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();
	DetectErrors("Known bug of GLEW");
    if(GlewInitResult == GLEW_OK)
    { 
		ImGuiInit(Window);

		render_state RenderState = CreateDefaultRenderState();

#ifdef _WIN32
		Enable(&RenderState, GL_MULTISAMPLE);
#endif
		Enable(&RenderState, GL_DEPTH_TEST);
		DepthFunc(&RenderState, GL_LEQUAL);
		DepthMask(&RenderState, GL_TRUE);
		//Enable(&RenderState, GL_FRAMEBUFFER_SRGB);

#ifdef _WIN32
		Disable(&RenderState, GL_CULL_FACE);
#else
		Enable(&RenderState, GL_CULL_FACE);
		CullFace(&RenderState, GL_BACK);
		FrontFace(&RenderState, GL_CCW);
#endif

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
        //GameMemory.TransientStorageSize = Gigabytes(1);
		GameMemory.TransientStorageSize = 1;

        memory_index TotalMemorySize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
        void* GameMemoryBlock = malloc(TotalMemorySize);
		memset(GameMemoryBlock, 0, TotalMemorySize);
        GameMemory.PermanentStorage = GameMemoryBlock;
        GameMemory.TransientStorage = ((u8*)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);

		GameMemory.HighPriorityQueue = &HighPriorityQueue;
        
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

                SDLProcessPendingMessages(NewInput);

                SDLProcessKeyboardState(NewInput);

                u8 ButtonState = SDL_GetMouseState(&NewInput->MouseX, &NewInput->MouseY);
                SDLProcessKeyboardMessage(&NewInput->MouseButtons[0], (ButtonState & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0);
                SDLProcessKeyboardMessage(&NewInput->MouseButtons[1], (ButtonState & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0);
                SDLProcessKeyboardMessage(&NewInput->MouseButtons[2], (ButtonState & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0);

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

				// TODO(hugo) : Make sure this is not a perf issue
                SDL_Surface* Screen = SDL_GetWindowSurface(Window);

				// TODO(hugo) : Make this resolution-independant ?
				SetViewport(&RenderState, Screen->w, Screen->h);

				// TODO(hugo) : Formalize this !
				if(((u32)(Screen->w) != GlobalWindowWidth) 
							|| ((u32)(Screen->h) != GlobalWindowHeight))
				{
					GlobalWindowWidth = u32(Screen->w);
					GlobalWindowHeight = u32(Screen->h);
					NewInput->WindowResized = true;
				}
				else
				{
					// TODO(hugo) : Not sure if necessary. Check if it is
					NewInput->WindowResized = false;
				}

				ImGuiNewFrame(Window, NewInput);

                GameUpdateAndRender(&GameMemory, NewInput, &RenderState);
				ImGui::Render();
                SDL_GL_SwapWindow(Window);

                // TODO(hugo) : Sound in SDL !!
                float WorkCounter = SDL_GetTicks();
                float WorkSecondsElapsed = ((float)(WorkCounter - LastCounter)) / 1000.0f;

                float SecondsElapsedForFrame = WorkSecondsElapsed;
#if RIVTEN_SLOW
                char WindowTitleBuffer[128];
                snprintf(WindowTitleBuffer, 
						sizeof(WindowTitleBuffer), 
						"3d viewer @ rivten - %dms - mouse pos (%d, %d)", 
						(int)(SecondsElapsedForFrame * 1000.0f), 
						Input[1].MouseX, Input[1].MouseY);
                SDL_SetWindowTitle(Window, WindowTitleBuffer);

				if(DEBUGCurrentCounter < ArrayCount(DEBUGCounters))
				{
					DEBUGCounters[DEBUGCurrentCounter] = SecondsElapsedForFrame * 1000.0f;
					++DEBUGCurrentCounter;
				}
				else
				{
					for(u32 CounterIndex = 0; CounterIndex < (ArrayCount(DEBUGCounters) - 1); ++CounterIndex)
					{
						DEBUGCounters[CounterIndex] = DEBUGCounters[CounterIndex + 1];
					} DEBUGCounters[DEBUGCurrentCounter - 1] = SecondsElapsedForFrame * 1000.0f;
				}

				if(DEBUGRenderStateChangeCurrentCounter < ArrayCount(DEBUGRenderStateChangeCounters))
				{
					DEBUGRenderStateChangeCounters[DEBUGRenderStateChangeCurrentCounter] = DEBUGGLCurrentFrameStateChangeCount;
					++DEBUGRenderStateChangeCurrentCounter;
				}
				else
				{
					for(u32 CounterIndex = 0; 
							CounterIndex < (ArrayCount(DEBUGRenderStateChangeCounters) - 1); 
							++CounterIndex)
					{
						DEBUGRenderStateChangeCounters[CounterIndex] = DEBUGRenderStateChangeCounters[CounterIndex + 1];
					}
					DEBUGRenderStateChangeCounters[DEBUGRenderStateChangeCurrentCounter - 1] = DEBUGGLCurrentFrameStateChangeCount;
				}
				DEBUGGLCurrentFrameStateChangeCount = 0;
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
                LastCounter = EndWorkCounter;

				// TODO(hugo) : Properly cleanup Input::MouseZ value
				NewInput->MouseZ = 0;
				ZeroStruct(NewInput->Text);

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

	ImGuiShutdown();
    SDL_GL_DeleteContext(GLContext);
    SDL_DestroyWindow(Window);
    SDL_Quit();
    return(0);
}
