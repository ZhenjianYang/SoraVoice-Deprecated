#pragma once

#include <Utils/Type.h>

class Decoder {
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
	using SampleType = u16;

	virtual bool Open(const char* fileName) = 0;
	virtual int Read(void* buff, int samples_count) = 0;
	virtual void Close() = 0;
	const WAVEFORMAT& WaveFormat = waveFormat;
	
	int SamplesTotal() const { return samples_total; }
	int SamplesRead() const { return samples_read; }

protected:
	WAVEFORMAT waveFormat{};
	int samples_total = 0;
	int samples_read = 0;

	Decoder() { }
	virtual ~Decoder() {};

private:
	Decoder(const Decoder&) = delete;
	Decoder& operator=(const Decoder&) = delete;
	Decoder(Decoder&&) = delete;
	Decoder& operator=(Decoder&&) = delete;
};
