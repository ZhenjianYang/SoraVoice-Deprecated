#pragma once

#include "Type.h"
#include <vorbis\vorbisfile.h>

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

public:
	virtual ~SoundFile() = default;

protected:
	WAVEFORMAT waveFormat;
};

class Ogg : public SoundFile {
public:
	Ogg() = default;
	~Ogg() { Close(); }
	virtual bool Open(const char* fileName) override {
		FILE* f = fopen(fileName, "rb");
		if (f == NULL) {
			return false;
		}

		if (ov_open_callbacks(f, &ovFile, nullptr, 0, OV_CALLBACKS_DEFAULT) != 0) {
			fclose(f);
			return false;
		}

		vorbis_info* info = ov_info(&ovFile, -1);

		waveFormat.wFormatTag = 1;
		waveFormat.nChannels = info->channels;
		waveFormat.nSamplesPerSec = info->rate;
		waveFormat.wBitsPerSample = 16;
		waveFormat.nBlockAlign = info->channels * 16 / 8;
		waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;

		return true;
	}
	virtual int Read(void * buff, int size) override {
		if (!buff || !size) return 0;

		int bitstream = 0;
		int total = 0;
		constexpr int block = 4096;
		while (total < size)
		{
			int request = size - total < block ? size - total : block;
			int read = ov_read(&ovFile, (char*)buff + total, request, 0, 2, 1, &bitstream);
			if (read <= 0) break;
			else total += read;
		}
		for (char* p = (char*)buff + total; p < (char*)buff + size; p++) *p = 0;
		return total;
	}
	virtual void Close() override {
		ov_clear(&ovFile);
	}

private:
	OggVorbis_File ovFile{};
	Ogg(const Ogg&) = delete;
	Ogg& operator=(const Ogg&) = delete;

public:
	static void SetOggApis(void * ov_open_callbacks,
		void * ov_info, void * ov_read, void * ov_clear) {
		Ogg::ov_open_callbacks = (decltype(Ogg::ov_open_callbacks))ov_open_callbacks;
		Ogg::ov_info           = (decltype(Ogg::ov_info))          ov_info;
		Ogg::ov_read           = (decltype(Ogg::ov_read))          ov_read;
		Ogg::ov_clear          = (decltype(Ogg::ov_clear))         ov_clear;
	}
private:
	static decltype(::ov_open_callbacks)* ov_open_callbacks;
	static decltype(::ov_info)* ov_info;
	static decltype(::ov_read)* ov_read;
	static decltype(::ov_clear)* ov_clear;
};

class Wav : public SoundFile {
	static constexpr u32 tag_RIFF = 0x46464952;
	static constexpr u32 tag_WAVE = 0x45564157;
	static constexpr u32 tag_fmt  = 0x20746D66;
	static constexpr u32 tag_data = 0x61746164;

	struct WAVHead
	{
		u32 tag_RIFF;
		u32 size;
		u32 tag_WAVE;
		u32 tag_fmt;
		u32 size_WAVEFORMAT;
		WAVEFORMAT waveFormat;
		u32 tag_data;
		s32 size_data;
	} head{};
	int remain = 0;

public:
	Wav() = default;
	~Wav() { Close(); }
	virtual bool Open(const char* fileName) override {
		f = fopen(fileName, "rb");
		if (f == NULL) {
			return false;
		}
		if (fread(&head, 1, sizeof(head), f) != sizeof(head)) {
			fclose(f); f = NULL; return false;
		}

		if (head.tag_RIFF != tag_RIFF || head.tag_WAVE != tag_WAVE || head.tag_fmt != tag_fmt || head.tag_data != tag_data
			|| head.size_WAVEFORMAT != sizeof(WAVEFORMAT)) {
			fclose(f); f = NULL; return false;
		}
		waveFormat = head.waveFormat;
		if ((waveFormat.wBitsPerSample + 7) / 8 != waveFormat.nBlockAlign
			|| waveFormat.nAvgBytesPerSec != waveFormat.nBlockAlign * waveFormat.nSamplesPerSec) {
			fclose(f); f = NULL; return false;
		}

		remain = head.size_data;
		return remain >= 0;
	}
	virtual int Read(void * buff, int size) override {
		int request = size < remain ? size : remain;
		int read = fread(buff, 1, request, f);
		remain -= read;
		for (char* p = (char*)buff + read; p < (char*)buff + size; p++) *p = 0;
		return read;
	}
	virtual void Close() override {
		if (f) {
			fclose(f);
			f = NULL;
		}
	}

private:
	FILE* f = NULL;
	Wav(const Wav&) = delete;
	Wav& operator=(const Wav&) = delete;
};

decltype(::ov_open_callbacks)* Ogg::ov_open_callbacks = nullptr;
decltype(::ov_info)* Ogg::ov_info = nullptr;
decltype(::ov_read)* Ogg::ov_read = nullptr;
decltype(::ov_clear)* Ogg::ov_clear = nullptr;
