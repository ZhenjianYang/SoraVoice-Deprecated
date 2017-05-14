#include "SoundFileImpl.h"

#include <vorbis\vorbisfile.h>

namespace OggApi {
	static decltype(::ov_open_callbacks)* ov_open_callbacks;
	static decltype(::ov_info)* ov_info;
	static decltype(::ov_read)* ov_read;
	static decltype(::ov_clear)* ov_clear;
	static decltype(::ov_time_total)* ov_time_total;
}

bool Ogg::Open(const char* fileName) {
	FILE* file = fopen(fileName, "rb");
	if (file == NULL) {
		return false;
	}

	if (OggApi::ov_open_callbacks(file, (OggVorbis_File*)ovFile, nullptr, 0, OV_CALLBACKS_DEFAULT) != 0) {
		fclose(file);
		return false;
	}

	vorbis_info* info = OggApi::ov_info((OggVorbis_File*)ovFile, -1);

	waveFormat.wFormatTag = 1;
	waveFormat.nChannels = info->channels;
	waveFormat.nSamplesPerSec = info->rate;
	waveFormat.wBitsPerSample = 16;
	waveFormat.nBlockAlign = info->channels * 16 / 8;
	waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;

	if (OggApi::ov_time_total) {
		double total_time = OggApi::ov_time_total((OggVorbis_File*)ovFile, -1);
		if (total_time > 0) {
			len = unsigned(total_time * TimeUnitsPerSecond);
		}
	}

	return true;
}

int Ogg::Read(void * buff, int size) {
	if (!buff || !size) return 0;

	int bitstream = 0;
	int total = 0;
	constexpr int block = 4096;
	while (total < size)
	{
		int request = size - total < block ? size - total : block;
		int read = OggApi::ov_read((OggVorbis_File*)ovFile, (char*)buff + total, request, 0, 2, 1, &bitstream);
		if (read <= 0) break;
		else total += read;
	}
	for (char* p = (char*)buff + total; p < (char*)buff + size; p++) *p = 0;
	return total;

}

void Ogg::Close() {
	OggApi::ov_clear((OggVorbis_File*)ovFile);
}

void Ogg::destory()
{
	Close();
	delete (OggVorbis_File*)ovFile;
	ovFile = nullptr;
}

Ogg::Ogg() : ovFile(new OggVorbis_File{}) { }

void Ogg::SetOggApis(void * ov_open_callbacks,
	void * ov_info, void * ov_read, void * ov_clear, 
	void * ov_time_total) {
	OggApi::ov_open_callbacks = (decltype(OggApi::ov_open_callbacks))ov_open_callbacks;
	OggApi::ov_info           = (decltype(OggApi::ov_info))          ov_info;
	OggApi::ov_read           = (decltype(OggApi::ov_read))          ov_read;
	OggApi::ov_clear          = (decltype(OggApi::ov_clear))         ov_clear;
	OggApi::ov_time_total     = (decltype(OggApi::ov_time_total))    ov_time_total;
}



static constexpr u32 tag_RIFF = 0x46464952;
static constexpr u32 tag_WAVE = 0x45564157;
static constexpr u32 tag_fmt  = 0x20746D66;
static constexpr u32 tag_data = 0x61746164;

bool Wav::Open(const char* fileName) {
	file = fopen(fileName, "rb");
	if (file == NULL) {
		return false;
	}
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

	remain = head.size_data;
	if (remain >= 0) {
		len = unsigned(double(head.size_data) / waveFormat.nAvgBytesPerSec * TimeUnitsPerSecond);
	}
	return remain >= 0;
}

int Wav::Read(void * buff, int size) {
	int request = size < remain ? size : remain;
	int read = fread(buff, 1, request, (FILE*)file);
	remain -= read;
	for (char* p = (char*)buff + read; p < (char*)buff + size; p++) *p = 0;
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
	remain = 0;
}

Wav::Wav() {};
