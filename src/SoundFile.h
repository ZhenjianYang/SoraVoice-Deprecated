#pragma once

#include "Type.h"

class SoundFile {
protected:
	struct WAVEFORMAT {
		u16 wFormatTag;        /* format type */
		u16 nChannels;         /* number of channels (i.e. mono, stereo, etc.) */
		u32 nSamplesPerSec;    /* sample rate */
		u32 nAvgBytesPerSec;   /* for buffer estimation */
		u16 nBlockAlign;       /* block size of data */
		u16 wBitsPerSample;    /* number of bits per sample of mono data */
	};

public:
	virtual bool Open(const char* fileName) = 0;
	virtual int Read(void * buff, int size) = 0;
	virtual void Close() = 0;
	const WAVEFORMAT& WaveFormat = waveFormat;
	
	unsigned Length() const { return len; }
public:
	virtual ~SoundFile() = default;

protected:
	WAVEFORMAT waveFormat{};
	unsigned len = 0;

	SoundFile() { }
	SoundFile(const SoundFile& _Other) : waveFormat(_Other.waveFormat), len(_Other.len) { }
	SoundFile& operator=(const SoundFile& _Other) {
		waveFormat = _Other.waveFormat;
		len = _Other.len;
	}

public:
	static constexpr int TimeUnitsPerSecond = 1000;
};
