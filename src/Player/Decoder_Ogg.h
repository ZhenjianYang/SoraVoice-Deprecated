#pragma once

#include "Decoder.h"

class Ogg : public Decoder {
public:
	static constexpr char Attr[] = ".ogg";

	Ogg();
	virtual ~Ogg() { destory(); }
	virtual bool Open(const char* fileName) override;
	virtual int Read(void * buff, int samples_count) override;
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
	Ogg(Ogg && _Right) : Decoder(_Right),
		ovFile(_Right.ovFile) {
		_Right.ovFile = nullptr;
	}

	Ogg& operator=(Ogg && _Right) {
		destory();
		Decoder::operator=(_Right);
		ovFile = _Right.ovFile;
		_Right.ovFile = nullptr;
		return *this;
	}
};

