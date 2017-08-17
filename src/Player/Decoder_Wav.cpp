#include "Decoder_Wav.h"

#include <stdio.h>
#include <memory.h>

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
	Decoder::WAVEFORMAT waveFormat;
	u32 tag_data;
	s32 size_data;
};

bool Wav::Open(const char* fileName) {
	file = fopen(fileName, "rb");
	if (file == NULL) {
		return false;
	}
	WAVHead head{};
	if (fread(&head, 1, sizeof(head), (FILE*)file) != sizeof(head)) {
		fclose((FILE*)file); file = NULL; return false;
	}

	if (head.tag_RIFF != tag_RIFF || head.tag_WAVE != tag_WAVE || head.tag_fmt != tag_fmt || head.tag_data != tag_data
		|| head.size_WAVEFORMAT != sizeof(WAVEFORMAT)) {
		fclose((FILE*)file); file = NULL; return false;
	}
	waveFormat = head.waveFormat;
	if ((waveFormat.wBitsPerSample + 7) / 8 != waveFormat.nBlockAlign
		|| waveFormat.nAvgBytesPerSec != waveFormat.nBlockAlign * waveFormat.nSamplesPerSec) {
		fclose((FILE*)file); file = NULL; return false;
	}

	samples_total = head.size_data / waveFormat.nBlockAlign;
	samples_read = 0;
	return true;
}

int Wav::Read(void * buff, int samples_count) {
	if (!buff || samples_count < 0) return 0;

	memset(buff, 0, samples_count * waveFormat.nBlockAlign);

	int request = samples_total - samples_read;
	if (request > samples_count) request = samples_count;

	int read = fread(buff, waveFormat.nBlockAlign, request, (FILE*)file);

	samples_read += read;
	return read;
}

void Wav::Close() {
	if (file) {
		fclose((FILE*)file);
		file = nullptr;
	}
}

void Wav::destory()
{
	Close();
}
