#pragma once

#include "SoundFile.h"

class Ogg : public SoundFile {
public:
	Ogg();
	virtual ~Ogg() { destory(); }
	virtual bool Open(const char* fileName) override;
	virtual int Read(void * buff, int size) override;
	virtual void Close() override;

protected:
	void* ovFile = nullptr;
	void destory();

public:
	static void SetOggApis(void * ov_open_callbacks,
		void * ov_info, void * ov_read, void * ov_clear, 
		void * ov_time_total);

private:
	Ogg(const Ogg&) = delete;
	Ogg& operator=(const Ogg&) = delete;

public:
	Ogg(Ogg && _Right) : SoundFile(_Right),
		ovFile(_Right.ovFile) {
		_Right.ovFile = nullptr;
	}

	Ogg& operator=(Ogg && _Right) {
		destory();
		SoundFile::operator=(_Right);
		ovFile = _Right.ovFile;
		_Right.ovFile = nullptr;
		return *this;
	}
};

class Wav : public SoundFile {
public:
	Wav();
	virtual ~Wav() { destory(); }
	virtual bool Open(const char* fileName) override;
	virtual int Read(void * buff, int size) override;
	virtual void Close() override;

protected:
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
	void *file = nullptr;
	void destory();

private:
	Wav(const Wav&) = delete;
	Wav& operator=(const Wav&) = delete;

public:
	Wav(Wav && _Right) : SoundFile(_Right),
		head(_Right.head), remain(_Right.remain), file(_Right.file) {
		_Right.file = nullptr;
	}

	Wav& operator=(Wav && _Right) {
		destory();
		SoundFile::operator=(_Right);
		head = _Right.head;
		remain = _Right.remain;
		file = _Right.file;
		_Right.file = nullptr;
	}
};

