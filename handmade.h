#if !defined(HANDMADE_H)

/*
	Handmade_internal
	0 - build for public release
	1 - build for developer only

	handmade_slow
	0 - not slow code allowed
	1 - slow code enabled

*/


#if HANDMADE_SLOW
#define Assert(exp) \
if(!(exp)){*(int*)0=0;}

#else
#define Assert(exp)
#endif

#define Kilobytes(value) ((value)*1024LL)
#define Megabytes(value) (Kilobytes(value)*1024LL)
#define Gigabytes(value) (Megabytes(value)*1024LL)

#define ArrayCount(array) (sizeof(array)/sizeof((array)[0]))

inline uint32
SafeTruncatUint64(uint64 value)
{
	Assert(value <= 0xFFFFFFFF);
	uint32 result = (uint32)value;
	return result;

}

#if HANDMADE_INTERNAL
struct debug_read_file_result
{
	uint32 ContentSize;
	void *Contents;
};
internal debug_read_file_result DEBUGPlatformReadEntireFile(char *filename);
internal void DEBUGPlatformFreeFileMemory(void *memory);
internal bool32 DEBUGPlatformWriteEntireFile(char *filename, uint32 memorySize, void *memory);
#endif // HANDMADE_INTERNAL

struct GameOffscreenBuffer {
  void *memory;
  int width;
  int height;
  int pitch;
};

struct GameSoundOutputBuffer {
  int samplesPerSecond;
  int sampleCount;
  int16 *samples;
};

struct game_button_state
{
	int	HalfTransitionCount;
	bool32 EndedDown;
};

struct game_controller_input
{
	bool32 IsAnalog;
	bool32 IsConnected;
	real32 StickAverageX;
	real32 StickAverageY;
	union
	{
		//buttons[0] the same memory as Up ,etc..
		game_button_state Buttons[12];
		struct
		{
			game_button_state MoveUp;
			game_button_state MoveDown;
			game_button_state MoveLeft;
			game_button_state MoveRight;

			game_button_state ActionUp;
			game_button_state ActionDown;
			game_button_state ActionLeft;
			game_button_state ActionRight;

			game_button_state LeftShoulder;
			game_button_state RightShoulder;

			game_button_state Back;
			game_button_state Start;
		};

	};
};

struct game_input
{
	game_controller_input Controllers[4];
};
inline game_controller_input *GetController(game_input *Input, int unsigned ControllerIndex)
{
	Assert(ControllerIndex<ArrayCount(Input->Controllers));
	game_controller_input *result = &Input->Controllers[ControllerIndex];
	return result;
}


struct game_memory
{
	bool32 IsInitialized;
	uint64 PermanentStorageSize;
	void *PermanentStorage;

	uint64 TransientStorageSize;
	void *TransientStorage;
};



internal void GameUpdateAndRender(game_memory *memory,
								  game_input *Input,GameOffscreenBuffer *a_Buffer,
                                  GameSoundOutputBuffer *a_SoundBuffer);
struct game_state
{
	int a_BlueOffset;
	int a_GreenOffset;
	int a_ToneHz;
};
#define HANDMADE_H
#endif