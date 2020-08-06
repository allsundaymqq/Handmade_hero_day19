#if !defined(WIN32_HANDMADE_H)


#include <windows.h>

struct Win32OffscreenBuffer {
	BITMAPINFO info;
	void *memory;
	int width;
	int height;
	int pitch;
	int BytesPerPixel;
};

struct Win32WindowDimension {
	int width;
	int height;
};
struct Win32SoundOutput {
	int samplesPerSecond;
	//int toneHz;
	// int16 toneVolume;
	uint32 runningSampleIndex;
	//int wavePeriod;
	int bytesPerSample;
	DWORD secondaryBufferSize;
	real32 tSine;
	int latencySampleCount;
};

struct win32_debug_time_marker
{
	DWORD PlayCursor;
	DWORD WriteCursor;
};

#define WIN32_HANDMADE_H
#endif