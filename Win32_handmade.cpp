#include <math.h>
#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32_t bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

#include "handmade.cpp"
#include "handmade.h"

#include <Windows.h>
#include <Xinput.h>
#include <dsound.h>
#include <malloc.h>
#include <stdio.h>

#include "Win32_handmade.h"

global_variable bool32 g_Running;
global_variable Win32OffscreenBuffer g_Backbuffer;
global_variable LPDIRECTSOUNDBUFFER g_SecondaryBuffer;
global_variable int64 GlobalperfCountFrequency;

#define X_INPUT_GET_STATE(name)                                                \
  DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub) { return (ERROR_DEVICE_NOT_CONNECTED); }
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name)                                                \
  DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub) { return (ERROR_DEVICE_NOT_CONNECTED); }
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name)                                              \
  HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS,               \
                      LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);



internal debug_read_file_result
DEBUGPlatformReadEntireFile(char *filename)
{
	debug_read_file_result result = {}	;
	/*
	（文件名，访问模式，共享模式，安全属性，是否新建文件,文件属性之类的只读隐藏，）
	*/
	HANDLE filehandle=CreateFileA(filename,GENERIC_READ,FILE_SHARE_READ,0,
								OPEN_EXISTING,0 ,0);
	if (filehandle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER filesize;
		if (GetFileSizeEx(filehandle,&filesize))
		{
			//define for maximum value
			uint32 filesize32 = SafeTruncatUint64(filesize.QuadPart);
			result.Contents = VirtualAlloc(0, filesize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			if (result.Contents)
			{
				DWORD BytesRead;
				if (ReadFile(filehandle, result.Contents, filesize32, &BytesRead, 0)&&(filesize32 == BytesRead))
				{
					result.ContentSize = filesize32;
				}
				else
				{
					DEBUGPlatformFreeFileMemory(result.Contents);
					result.Contents = 0;
				}
			}
		}
		CloseHandle(filehandle);
	}

	return result;
}
internal void 
DEBUGPlatformFreeFileMemory(void *memory)
{
	if (memory)
	{
		VirtualFree(memory,0,MEM_RELEASE);
	}
}
internal bool32 
DEBUGPlatformWriteEntireFile(char *filename, uint32 memorySize, void *memory)
{
	bool32 result = false;
	
	HANDLE filehandle = CreateFileA(filename, GENERIC_WRITE, 0, 0,
		CREATE_ALWAYS, 0, 0);
	if (filehandle != INVALID_HANDLE_VALUE)
	{
		DWORD BytesWritten;
		if (WriteFile(filehandle, memory,memorySize,&BytesWritten,0))
		{
			result = (BytesWritten == memorySize);
			
		}
		CloseHandle(filehandle);
	}

	return result;

}

internal void Win32LoadXInput(void) {
  HMODULE xInputLibrary = LoadLibraryA("xinput1_4.dll");
  if (!xInputLibrary) {
    xInputLibrary = LoadLibraryA("xinput9_1_0.dll");
  }

  if (!xInputLibrary) {
    xInputLibrary = LoadLibraryA("xinput1_3.dll");
  }

  if (xInputLibrary) {
    XInputGetState =
        (x_input_get_state *)GetProcAddress(xInputLibrary, "XInputGetState");
    if (!XInputGetState) {
      XInputGetState = XInputGetStateStub;
    }

    XInputSetState =
        (x_input_set_state *)GetProcAddress(xInputLibrary, "XInputSetState");
    if (!XInputSetState) {
      XInputSetState = XInputSetStateStub;
    }
  } else {
  }
}

internal void Win32InitDSound(HWND a_Window, int32 a_SamplesPerSecond,
                              int32 a_BufferSize) {
  HMODULE dSoundLibrary = LoadLibraryA("dsound.dll");
  if (dSoundLibrary) {
    direct_sound_create *DirectSoundCreate =
        (direct_sound_create *)GetProcAddress(dSoundLibrary,
                                              "DirectSoundCreate");

    LPDIRECTSOUND directSound;
    if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &directSound, 0))) {
      WAVEFORMATEX waveFormat = {};
      waveFormat.wFormatTag = WAVE_FORMAT_PCM;
      waveFormat.nChannels = 2;
      waveFormat.nSamplesPerSec = a_SamplesPerSecond;
      waveFormat.wBitsPerSample = 16;
      waveFormat.nBlockAlign =
          (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
      waveFormat.nAvgBytesPerSec =
          waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
      waveFormat.cbSize = 0;

      if (SUCCEEDED(
              directSound->SetCooperativeLevel(a_Window, DSSCL_PRIORITY))) {
        DSBUFFERDESC bufferDescription = {};
        bufferDescription.dwSize = sizeof(bufferDescription);
        bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

        LPDIRECTSOUNDBUFFER primaryBuffer;
        if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription,
                                                     &primaryBuffer, 0))) {
          HRESULT error = primaryBuffer->SetFormat(&waveFormat);
          if (SUCCEEDED(error)) {
            OutputDebugStringA("Primary buffer format was set.\n");
          } else {
          }
        } else {
        }
      } else {
      }

      DSBUFFERDESC bufferDescription = {};
      bufferDescription.dwSize = sizeof(bufferDescription);
      bufferDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
      bufferDescription.dwBufferBytes = a_BufferSize;
      bufferDescription.lpwfxFormat = &waveFormat;
      HRESULT error = directSound->CreateSoundBuffer(&bufferDescription,
                                                     &g_SecondaryBuffer, 0);
      if (SUCCEEDED(error)) {
        OutputDebugStringA("Secondary buffer created successfully.\n");
      }
    } else {
    }
  } else {
  }
}

internal Win32WindowDimension Win32GetWindowDimension(HWND a_Window) {
  Win32WindowDimension result;

  RECT clientRect;
  GetClientRect(a_Window, &clientRect);
  result.width = clientRect.right - clientRect.left;
  result.height = clientRect.bottom - clientRect.top;

  return result;
}

internal void Win32ResizeDIBSection(Win32OffscreenBuffer *a_Buffer, int a_Width,
                                    int a_Height) {
  if (a_Buffer->memory) {
    VirtualFree(a_Buffer->memory, 0, MEM_RELEASE);
  }

  a_Buffer->width = a_Width;
  a_Buffer->height = a_Height;

  int bytesPerPixel = 4;
  a_Buffer->BytesPerPixel = bytesPerPixel;

  a_Buffer->info.bmiHeader.biSize = sizeof(a_Buffer->info.bmiHeader);
  a_Buffer->info.bmiHeader.biWidth = a_Buffer->width;
  a_Buffer->info.bmiHeader.biHeight = -a_Buffer->height;
  a_Buffer->info.bmiHeader.biPlanes = 1;
  a_Buffer->info.bmiHeader.biBitCount = 32;
  a_Buffer->info.bmiHeader.biCompression = BI_RGB;

  int bitmapMemorySize = (a_Buffer->width * a_Buffer->height) * bytesPerPixel;
  a_Buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_RESERVE | MEM_COMMIT,
                                  PAGE_READWRITE);
  a_Buffer->pitch = a_Width * bytesPerPixel;
  
}

internal void Win32DisplayBufferInWindow(Win32OffscreenBuffer *a_Buffer,
                                         HDC a_DeviceContext, int a_WindowWidth,
                                         int a_WindowHeight) {
  StretchDIBits(a_DeviceContext, 0, 0, a_WindowWidth, a_WindowHeight, 0, 0,
                a_Buffer->width, a_Buffer->height, a_Buffer->memory,
                &a_Buffer->info, DIB_RGB_COLORS, SRCCOPY);
}

internal LRESULT CALLBACK Win32MainWindowCallback(HWND a_Window, UINT a_Message,
                                                  WPARAM a_WParam,
                                                  LPARAM a_LParam) {
  LRESULT result = 0;

  switch (a_Message) {
  case WM_CLOSE: {
    g_Running = false;
  } break;

  case WM_ACTIVATEAPP: {
    OutputDebugStringA("WM_ACTIVATEAPP\n");
  } break;

  case WM_DESTROY: {
    g_Running = false;
  } break;

  case WM_PAINT: {
    PAINTSTRUCT Paint;
    HDC DeviceContext = BeginPaint(a_Window, &Paint);
    Win32WindowDimension Dimension = Win32GetWindowDimension(a_Window);
    Win32DisplayBufferInWindow(&g_Backbuffer, DeviceContext, Dimension.width,
                               Dimension.height);
    EndPaint(a_Window, &Paint);
  } break;

  default: {
    result = DefWindowProcA(a_Window, a_Message, a_WParam, a_LParam);
  } break;
  }

  return result;
}



internal void Win32ClearBuffer(Win32SoundOutput *a_SoundOutput) {
  VOID *region1;
  DWORD region1Size;
  VOID *region2;
  DWORD region2Size;
  if (SUCCEEDED(g_SecondaryBuffer->Lock(0, a_SoundOutput->secondaryBufferSize,
                                        &region1, &region1Size, &region2,
                                        &region2Size, 0))) {
    uint8 *destSample = (uint8 *)region1;
    for (DWORD byteIndex = 0; byteIndex < region1Size; ++byteIndex) {
      *destSample++ = 0;
    }

    destSample = (uint8 *)region2;
    for (DWORD byteIndex = 0; byteIndex < region2Size; ++byteIndex) {
      *destSample++ = 0;
    }

    g_SecondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
  }
}

internal void Win32FillSoundBuffer(Win32SoundOutput *a_SoundOutput,
                                   DWORD a_ByteToLock, DWORD a_BytesToWrite,
                                   GameSoundOutputBuffer *a_SourceBuffer) {
  VOID *region1;
  DWORD region1Size;
  VOID *region2;
  DWORD region2Size;
  if (SUCCEEDED(g_SecondaryBuffer->Lock(a_ByteToLock, a_BytesToWrite, &region1,
                                        &region1Size, &region2, &region2Size,
                                        0))) {
    DWORD region1SampleCount = region1Size / a_SoundOutput->bytesPerSample;
    int16 *destSample = (int16 *)region1;
    int16 *sourceSample = a_SourceBuffer->samples;
    for (DWORD sampleIndex = 0; sampleIndex < region1SampleCount;
         ++sampleIndex) {
      *destSample++ = *sourceSample++;
      *destSample++ = *sourceSample++;
      ++a_SoundOutput->runningSampleIndex;
    }

    DWORD region2SampleCount = region2Size / a_SoundOutput->bytesPerSample;
    destSample = (int16 *)region2;
    for (DWORD sampleIndex = 0; sampleIndex < region2SampleCount;
         ++sampleIndex) {
      *destSample++ = *sourceSample++;
      *destSample++ = *sourceSample++;
      ++a_SoundOutput->runningSampleIndex;
    }

    g_SecondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
  }
}

internal void 
Win32ProcessKeyboardMessage(game_button_state *NewState, bool32 IsDown)
{
	Assert(NewState->EndedDown != IsDown);
	NewState->EndedDown = IsDown;
	++NewState->HalfTransitionCount;

}

internal void 
Win32ProcessXInputDigitalButton(DWORD XInputButtonState,
	game_button_state *OldState,DWORD ButtonBit,
	game_button_state *NewState)
{
	NewState->EndedDown = ((XInputButtonState & ButtonBit)== ButtonBit);
	NewState->HalfTransitionCount =(OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
	
}

internal real32 
Win32ProcessXinputStickValue(SHORT value, SHORT Deadzone)
{
	real32 result=0;
	if (value < -Deadzone)
	{
		result = (real32)value / 32768.0f;
	}
	else if (value > Deadzone)
	{
		result = (real32)value / 32767.0f;
	}
	return result;
}

internal void 
Win32MessageLoop(game_controller_input *KeyboardController)
{
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
		
		switch (message.message)
		{
		case WM_QUIT:
		{
				g_Running = false;
		}break;
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			uint32 VKCode = (uint32)message.wParam;
			bool32 WasDown = ((message.lParam & (1 << 30)) != 0);
			bool32 IsDown = ((message.lParam & (1 << 31)) == 0);
			if (WasDown != IsDown)
			{
				if (VKCode == 'W') {
					Win32ProcessKeyboardMessage(&KeyboardController->MoveUp, IsDown);
				}
				else if (VKCode == 'A') {
					Win32ProcessKeyboardMessage(&KeyboardController->MoveLeft, IsDown);
				}
				else if (VKCode == 'S') {
					Win32ProcessKeyboardMessage(&KeyboardController->MoveDown, IsDown);
				}
				else if (VKCode == 'D') {
					Win32ProcessKeyboardMessage(&KeyboardController->MoveRight, IsDown);
				}
				else if (VKCode == 'Q') {
					Win32ProcessKeyboardMessage(&KeyboardController->LeftShoulder, IsDown);
				}
				else if (VKCode == 'E') {
					Win32ProcessKeyboardMessage(&KeyboardController->RightShoulder, IsDown);
				}
				else if (VKCode == VK_UP) {
					Win32ProcessKeyboardMessage(&KeyboardController->ActionUp, IsDown);
				}
				else if (VKCode == VK_LEFT) {
					Win32ProcessKeyboardMessage(&KeyboardController->ActionLeft, IsDown);
				}
				else if (VKCode == VK_DOWN) {
					Win32ProcessKeyboardMessage(&KeyboardController->ActionDown, IsDown);
				}
				else if (VKCode == VK_RIGHT) {
					Win32ProcessKeyboardMessage(&KeyboardController->ActionRight, IsDown);
				}
				else if (VKCode == VK_ESCAPE) {
					//Win32ProcessKeyboardMessage(&KeyboardController->ActionRight, IsDown);

				}
				else if (VKCode == VK_SPACE)
				{
					//Win32ProcessKeyboardMessage(&KeyboardController->ActionRight, IsDown);

				}

			}
			bool32 AltKeyWasDown = (message.lParam & (1 << 29));
			if (VKCode == VK_F4 && AltKeyWasDown)
			{
				g_Running = false;
			}

		}break;
		default:
		{
			TranslateMessage(&message);
			DispatchMessageA(&message);
		}break;
		}


	}
}


inline LARGE_INTEGER
Win32GetWallClock(void)
{
	LARGE_INTEGER result;
	QueryPerformanceCounter(&result);
	return result;

}

inline real32 
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
	real32 result = ((real32)(End.QuadPart-Start.QuadPart)/
		(real32)GlobalperfCountFrequency);
	return result;

}

internal void 
Win32DebugDrawVertical(Win32OffscreenBuffer *GlobalBackBuffer,
	int X,int  Top,int  Bottom,uint32 Color)
{
	uint8 *Pixel = ((uint8 *)GlobalBackBuffer->memory+
		X*GlobalBackBuffer->BytesPerPixel +
		Top*GlobalBackBuffer->pitch);
	for (int Y=Top;
		Y<Bottom;
		++Y)
	{
		*(uint32*)Pixel = Color;
		Pixel += GlobalBackBuffer->pitch;
	}

}

inline void 
Win32DrawSoundBufferMarker(Win32OffscreenBuffer *GlobalBackBuffer,
	Win32SoundOutput *soundOutput, real32 C,
	int PadX,int Top,int Bottom,DWORD value,uint32 Color)
{
	Assert(value < soundOutput->secondaryBufferSize);
	real32 XReal32 = (C*(real32)value);
	int X = PadX + (int)XReal32;
	Win32DebugDrawVertical(GlobalBackBuffer, X, Top, Bottom, Color);

}

internal void 
Win32DebugSyncDisplay(Win32OffscreenBuffer *GlobalBackBuffer, 
	int MarkerCount, win32_debug_time_marker *Markers,
	Win32SoundOutput *soundOutput, real32 TargetSecondsPerFrame)
{
	int PadX = 16;
	int PadY = 16;

	int Top = PadY;
	int Bottom = GlobalBackBuffer->height - PadY;

	real32 C = (real32)(GlobalBackBuffer->width-2*PadX) / (real32)soundOutput->secondaryBufferSize;
	
	for (int MarkerIndex=0;
		MarkerIndex<MarkerCount;
		++MarkerIndex)
	{
		win32_debug_time_marker *ThisMarker = &Markers[MarkerIndex];
		Win32DrawSoundBufferMarker(GlobalBackBuffer,soundOutput, C,
			PadX, Top,Bottom,ThisMarker->PlayCursor,0xFFFFFF);
		Win32DrawSoundBufferMarker(GlobalBackBuffer, soundOutput, C,
			PadX, Top, Bottom, ThisMarker->WriteCursor, 0xFF00FF);
	}
}

int CALLBACK WinMain(HINSTANCE a_Instance, HINSTANCE a_PrevInstance,
                     LPSTR a_CommandLine, int a_ShowCode) 
{


  LARGE_INTEGER perfCountFrequencyResult;
  QueryPerformanceFrequency(&perfCountFrequencyResult);
  GlobalperfCountFrequency = perfCountFrequencyResult.QuadPart;

  //because Windows isn't a living OS and Threads 
  //we fixed the Accuracy of Sleep() to 1ms 
  UINT DesiredScheduleMS = 1;
  bool32 SleepGramular = (timeBeginPeriod(DesiredScheduleMS) == TIMERR_NOERROR);
  Win32LoadXInput();

  WNDCLASSA windowClass = {};

  Win32ResizeDIBSection(&g_Backbuffer, 1280, 720);

  windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  windowClass.lpfnWndProc = Win32MainWindowCallback;
  windowClass.hInstance = a_Instance;
  windowClass.lpszClassName = "HandmadeHeroWindowClass";


  //refresh rate
  //TODO : how to query this on Windows reliably
#define FrameOfAudioLatency 4
#define MonitorRefreshHz 60
#define GameUpdateHz (MonitorRefreshHz / 2)
  real32 TargetSecondsPerFrame = 1.0f / (real32)GameUpdateHz;


  if (RegisterClassA(&windowClass)) {
    HWND window = CreateWindowExA(0, windowClass.lpszClassName, "Handmade Hero",
                                  WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                  CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                  CW_USEDEFAULT, 0, 0, a_Instance, 0);
    if (window) {
      HDC deviceContext = GetDC(window);

      Win32SoundOutput soundOutput = {};

      soundOutput.samplesPerSecond = 48000;
      soundOutput.bytesPerSample = sizeof(int16) * 2;
      soundOutput.secondaryBufferSize =
          soundOutput.samplesPerSecond * soundOutput.bytesPerSample;
      soundOutput.latencySampleCount = FrameOfAudioLatency*(soundOutput.samplesPerSecond / GameUpdateHz);
      Win32InitDSound(window, soundOutput.samplesPerSecond,
                      soundOutput.secondaryBufferSize);
      Win32ClearBuffer(&soundOutput);
      g_SecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

      g_Running = true;

      int16 *samples =
          (int16 *)VirtualAlloc(0, soundOutput.secondaryBufferSize,
                                MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#if HANDMADE_INTERNAL
	  LPVOID BaseAddress = (LPVOID)Kilobytes(1);
	  
#else
	  LPVOID BaseAddress = 0;
#endif // HANDMADE_INTERNAL

	  
	  game_memory GameMemory = {};
	  GameMemory.PermanentStorageSize = Megabytes(64);
	  GameMemory.TransientStorageSize = Megabytes(64);

	  uint64 TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;

	  GameMemory.PermanentStorage = VirtualAlloc(BaseAddress, (size_t)TotalSize,
		  MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	  GameMemory.TransientStorage = ((uint8 *)GameMemory.PermanentStorage + 
		  GameMemory.PermanentStorageSize);
	  
	  game_input Input[2] = {};
	  game_input *NewInput = &Input[0];
	  game_input *OldInput = &Input[1];

      LARGE_INTEGER LastCounter=Win32GetWallClock();

	  int DebugTimeMarkerIndex = 0;
	  win32_debug_time_marker DebugTimeMarkers[GameUpdateHz/2] = {0};

	  DWORD LastPlayCursor = 0;

	  bool32 soundIsValid = false;
      uint64 LastCycleCount = __rdtsc();
	  while (g_Running)
	  {
		  game_controller_input *OldKeyboardController = GetController(OldInput, 0);
		  game_controller_input *NewKeyboardController = GetController(NewInput, 0);
		  game_controller_input zero = {};
		  *NewKeyboardController = zero;
		  NewKeyboardController->IsConnected = true;

		  for (int ButtonIndex = 0;
			  ButtonIndex < ArrayCount(NewKeyboardController->Buttons);
			  ++ButtonIndex)
		  {
			  NewKeyboardController->Buttons[ButtonIndex].EndedDown =
				  OldKeyboardController->Buttons[ButtonIndex].EndedDown;
		  }
		  Win32MessageLoop(NewKeyboardController);


		  DWORD MaxControllerCount = XUSER_MAX_COUNT;
		  if (MaxControllerCount > (ArrayCount(NewInput->Controllers) - 1))
		  {
			  MaxControllerCount = (ArrayCount(NewInput->Controllers) - 1);
		  }
		  for (DWORD controllerIndex = 0; controllerIndex < MaxControllerCount;
			  ++controllerIndex)
		  {
			  int OurcontrollerIndex = controllerIndex + 1;
			  game_controller_input *OldController = GetController(OldInput, OurcontrollerIndex);
			  game_controller_input *NewController = GetController(NewInput, OurcontrollerIndex);

			  XINPUT_STATE controllerState;
			  if (XInputGetState(controllerIndex, &controllerState) ==
				  ERROR_SUCCESS)
			  {
				  NewController->IsConnected = true;



				  XINPUT_GAMEPAD *pad = &controllerState.Gamepad;

				  NewController->StickAverageX = Win32ProcessXinputStickValue(pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
				  NewController->StickAverageY = Win32ProcessXinputStickValue(pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
				  if ((NewController->StickAverageX != 0.0f) ||
					  (NewController->StickAverageY != 0.0f))
				  {
					  NewController->IsAnalog = true;
				  }
				  if (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
				  {
					  NewController->StickAverageY = 1.0f;
					  NewController->IsAnalog = false;
				  }
				  if (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
				  {
					  NewController->StickAverageY = -1.0f;
					  NewController->IsAnalog = false;
				  }
				  if (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
				  {
					  NewController->StickAverageX = -1.0f;
					  NewController->IsAnalog = false;
				  }
				  if (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
				  {
					  NewController->StickAverageX = 1.0f;
					  NewController->IsAnalog = false;
				  }

				  real32 Threshold = 0.5f;
				  Win32ProcessXInputDigitalButton((NewController->StickAverageX < -Threshold) ? 1 : 0
					  , &OldController->MoveLeft, 1
					  , &NewController->MoveLeft);
				  Win32ProcessXInputDigitalButton((NewController->StickAverageX > Threshold) ? 1 : 0
					  , &OldController->MoveRight, 1
					  , &NewController->MoveRight);
				  Win32ProcessXInputDigitalButton((NewController->StickAverageY < -Threshold) ? 1 : 0
					  , &OldController->MoveDown, 1
					  , &NewController->MoveDown);
				  Win32ProcessXInputDigitalButton((NewController->StickAverageY > Threshold) ? 1 : 0
					  , &OldController->MoveUp, 1
					  , &NewController->MoveUp);


				  Win32ProcessXInputDigitalButton(pad->wButtons,
					  &OldController->ActionDown, XINPUT_GAMEPAD_A,
					  &NewController->ActionDown);
				  Win32ProcessXInputDigitalButton(pad->wButtons,
					  &OldController->ActionRight, XINPUT_GAMEPAD_B,
					  &NewController->ActionRight);
				  Win32ProcessXInputDigitalButton(pad->wButtons,
					  &OldController->ActionLeft, XINPUT_GAMEPAD_X,
					  &NewController->ActionLeft);
				  Win32ProcessXInputDigitalButton(pad->wButtons,
					  &OldController->ActionUp, XINPUT_GAMEPAD_Y,
					  &NewController->ActionUp);
				  //bool32 start = (pad->wButtons & XINPUT_GAMEPAD_START);
				  // bool32 back = (pad->wButtons & XINPUT_GAMEPAD_BACK);

			  }
			  else {
				  NewController->IsConnected = false;
			  }
		  }

		  DWORD byteToLock = 0;
		  DWORD targetCursor;
		  DWORD bytesToWrite = 0;
		  if (soundIsValid)
		  {
			  byteToLock =
				  ((soundOutput.runningSampleIndex * soundOutput.bytesPerSample) %
					  soundOutput.secondaryBufferSize);

			  targetCursor = ((LastPlayCursor + (soundOutput.latencySampleCount *
				  soundOutput.bytesPerSample)) %
				  soundOutput.secondaryBufferSize);
			  if (byteToLock > targetCursor) {
				  bytesToWrite = (soundOutput.secondaryBufferSize - byteToLock);
				  bytesToWrite += targetCursor;
			  }
			  else {
				  bytesToWrite = targetCursor - byteToLock;
			  }

		  }

		  GameSoundOutputBuffer soundBuffer = {};
		  soundBuffer.samplesPerSecond = soundOutput.samplesPerSecond;
		  soundBuffer.sampleCount = bytesToWrite / soundOutput.bytesPerSample;
		  soundBuffer.samples = samples;

		  GameOffscreenBuffer buffer = {};
		  buffer.memory = g_Backbuffer.memory;
		  buffer.width = g_Backbuffer.width;
		  buffer.height = g_Backbuffer.height;
		  buffer.pitch = g_Backbuffer.pitch;
		  GameUpdateAndRender(&GameMemory, NewInput, &buffer, &soundBuffer);

		  if (soundIsValid) {
			  Win32FillSoundBuffer(&soundOutput, byteToLock, bytesToWrite,
				  &soundBuffer);
		  }

		  LARGE_INTEGER WorkCounter = Win32GetWallClock();
		  real32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);

		  real32 SecondsElapsedForFrame = WorkSecondsElapsed;

		  //if() use to ensure Don't miss a Frame
		  if (SecondsElapsedForFrame < TargetSecondsPerFrame)
		  {
			  if (SleepGramular)
			  {
				  DWORD SleepMS = (DWORD)(1000.0f*(TargetSecondsPerFrame - SecondsElapsedForFrame));
				  if (SleepMS > 0)
				  {
					  Sleep(SleepMS);
				  }
			  }
			  //real32 TestSecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter,
			  //	Win32GetWallClock());
			  //Assert(TestSecondsElapsedForFrame<TargetSecondsPerFrame);
			  while (SecondsElapsedForFrame < TargetSecondsPerFrame)
			  {

				  SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter,
					  Win32GetWallClock());

			  }
		  }
		  else
		  {
			  //missed Frame rate
		  }
		  LARGE_INTEGER EndCounter = Win32GetWallClock();
		  real32 MSPerFrame = 1000.f*Win32GetSecondsElapsed(LastCounter, EndCounter);
		  LastCounter = EndCounter;

		  Win32WindowDimension dimension = Win32GetWindowDimension(window);
#if HANDMADE_INTERNAL
		  Win32DebugSyncDisplay(&g_Backbuffer, ArrayCount(DebugTimeMarkers), DebugTimeMarkers,
			  &soundOutput, TargetSecondsPerFrame);

#endif
		  Win32DisplayBufferInWindow(&g_Backbuffer, deviceContext,
			  dimension.width, dimension.height);
		  //this is Debug code

		  DWORD PlayCursor;
		  DWORD WriteCursor;
		  if (g_SecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)==DS_OK)
		  {
			  LastPlayCursor = PlayCursor;
			  if (!soundIsValid)
			  {
				  soundOutput.runningSampleIndex = WriteCursor/ soundOutput.bytesPerSample;
				  soundIsValid = true;
			  }
			  
		  }
		  else
		  {
			  soundIsValid = false;
		  }

#if HANDMADE_INTERNAL
		  {
			  win32_debug_time_marker *Marker=&DebugTimeMarkers[DebugTimeMarkerIndex++];
			  if (DebugTimeMarkerIndex > ArrayCount(DebugTimeMarkers))
			  {
				  DebugTimeMarkerIndex = 0;
			  }
			  Marker->PlayCursor = PlayCursor;
			  Marker->WriteCursor = WriteCursor;
		  }
		  
#endif // DEBUG

		game_input *Temp = NewInput;
		NewInput = OldInput;
		OldInput = Temp;

		
		uint64 EndCycleCount = __rdtsc();
		uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
		LastCycleCount = EndCycleCount;

		real64 FPS = 0.0f;
		real64 mcpf = ((real64)CyclesElapsed / (1000.0f * 1000.0f));


		char FPSbuffer[256];
		_snprintf_s(FPSbuffer,sizeof(FPSbuffer), "%.02fms/f,  %.02ff/s,  %.02fmc/f\n", MSPerFrame, FPS, mcpf);
		OutputDebugStringA(FPSbuffer);

      }
    } else {
    }
  } else {
  }

  return (0);
}