#include "handmade.h"

internal void GameOutputSound(GameSoundOutputBuffer *a_SoundBuffer,
                              int a_ToneHz) {
  local_persist real32 tSine;
  int16 toneVolume = 3000;
  int wavePeriod = a_SoundBuffer->samplesPerSecond / a_ToneHz;

  int16 *sampleOut = a_SoundBuffer->samples;
  for (int sampleIndex = 0; sampleIndex < a_SoundBuffer->sampleCount;
       ++sampleIndex) {
    real32 sineValue = sinf(tSine);
    int16 sampleValue = (int16)(sineValue * toneVolume);
    *sampleOut++ = sampleValue;
    *sampleOut++ = sampleValue;

    tSine += 2.f * Pi32 * 1.0f / (real32)wavePeriod;
  }
}

internal void RenderWeirdGradient(GameOffscreenBuffer *a_Buffer,
                                  int a_BlueOffset, int a_GreenOffset) {
  uint8 *row = (uint8 *)a_Buffer->memory;
  for (int y = 0; y < a_Buffer->height; ++y) {
    uint32 *pixel = (uint32 *)row;
    for (int x = 0; x < a_Buffer->width; ++x) {
      uint8 blue =(uint8) (x + a_BlueOffset);
      uint8 green = (uint8)(y + a_GreenOffset);

      *pixel++ = ((green << 8) | blue);
    }

    row += a_Buffer->pitch;
  }
}



internal void GameUpdateAndRender(game_memory *memory,
								  game_input *Input, GameOffscreenBuffer *a_Buffer,
                                  GameSoundOutputBuffer *a_SoundBuffer) 
{
	Assert(sizeof(game_state) <= memory->PermanentStorageSize);

	game_state *GameState = (game_state*)memory->PermanentStorageSize;
	if (!memory->IsInitialized)
	{
		char *filename = __FILE__;
		debug_read_file_result file = DEBUGPlatformReadEntireFile(filename);
		if (file.Contents)
		{
			DEBUGPlatformWriteEntireFile("test.out",file.ContentSize,file.Contents);
			DEBUGPlatformFreeFileMemory(file.Contents);
		}
		GameState->a_ToneHz = 256;

		//maybe more suitable for platform layer
		memory->IsInitialized = true;
	}
	//what will be useful for the game,(
	//input form platform player)
	for (int ControllerIndex = 0;
		ControllerIndex < ArrayCount(Input->Controllers);
		++ControllerIndex)
	{
		game_controller_input *Controller = GetController(Input,ControllerIndex);
		if (Controller->IsAnalog)
		{
			//use analog movement tuning
			GameState->a_BlueOffset += (int)(4.0f*Controller->StickAverageX);
			GameState->a_ToneHz = 256 + (int)(128.0f*Controller->StickAverageY);

		}
		else
		{
			//use digital movement tuning
			if (Controller->MoveLeft.EndedDown)
			{
				GameState->a_BlueOffset -= 1;
			}
			if (Controller->MoveRight.EndedDown)
			{
				GameState->a_BlueOffset += 1;
			}
		}

		if (Controller->ActionDown.EndedDown)
		{
			GameState->a_GreenOffset += 1;
		}
	}

	GameOutputSound(a_SoundBuffer, GameState->a_ToneHz);
	 RenderWeirdGradient(a_Buffer, GameState->a_BlueOffset, GameState->a_GreenOffset);
}