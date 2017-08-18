#include "Decoder_Ogg.h"

#include <vorbis\vorbisfile.h>
#include <memory.h>

namespace OggApi {
	static decltype(::ov_open_callbacks)* ov_open_callbacks;
	static decltype(::ov_info)* ov_info;
	static decltype(::ov_read)* ov_read;
	static decltype(::ov_clear)* ov_clear;
	static decltype(::ov_pcm_total)* ov_pcm_total;
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
	waveFormat.nChannels = (u16)info->channels;
	waveFormat.nSamplesPerSec = info->rate;
	waveFormat.wBitsPerSample = 16;
	waveFormat.nBlockAlign = (u16)(info->channels * 16 / 8);
	waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;

	samples_read = 0;
	samples_total = (int)OggApi::ov_pcm_total((OggVorbis_File*)ovFile, -1);

	return true;
}

int Ogg::Read(void * buff, int bytes) {
	if (!buff || bytes < 0) return 0;

	int request = (samples_total - samples_read) * waveFormat.nBlockAlign;
	if (request > bytes) request = bytes;

	constexpr int block = 4096;
	int bitstream = 0;
	int read = 0;

	char* tBuff = (char*)buff;

	while (read < request) {
		int req = bytes - read;
		if(req > block) req = block;
		int tread = OggApi::ov_read((OggVorbis_File*)ovFile, tBuff, req, 0, 2, 1, &bitstream);
		if (tread <= 0) break;

		tBuff += tread;
		read += tread;
	}

	memset(tBuff, 0, bytes - read);

	samples_read += read / waveFormat.nBlockAlign;
	return read;
}

void Ogg::Close() {
	if (ovFile) {
		OggApi::ov_clear((OggVorbis_File*)ovFile);
	}
}

void Ogg::destory()
{
	if(OggApi::ov_clear) Close();
	delete (OggVorbis_File*)ovFile;
	ovFile = nullptr;
}

Ogg::Ogg() : ovFile(new OggVorbis_File{}) { }

void Ogg::SetOggApis(void * ov_open_callbacks,
	void * ov_info, void * ov_read, void * ov_clear, 
	void * ov_pcm_total) {
	OggApi::ov_open_callbacks = (decltype(OggApi::ov_open_callbacks))ov_open_callbacks;
	OggApi::ov_info           = (decltype(OggApi::ov_info))          ov_info;
	OggApi::ov_read           = (decltype(OggApi::ov_read))          ov_read;
	OggApi::ov_clear          = (decltype(OggApi::ov_clear))         ov_clear;
	OggApi::ov_pcm_total      = (decltype(OggApi::ov_pcm_total))     ov_pcm_total;
}
