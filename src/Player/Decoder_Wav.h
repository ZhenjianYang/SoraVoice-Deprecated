#pragma once

#include "Decoder.h"

class Wav : public Decoder {
public:
	static constexpr char Attr[] = ".wav";
	Wav() = default;
	virtual ~Wav() { destory(); }
	virtual bool Open(const char* fileName) override;
	virtual int Read(void * buff, int samples_count) override;
	virtual void Close() override;

protected:
	void *file = nullptr;
	void destory();

private:
	Wav(const Wav&) = delete;
	Wav& operator=(const Wav&) = delete;

public:
	Wav(Wav && _Right) : Decoder(_Right), file(_Right.file) {
		_Right.file = nullptr;
	}

	Wav& operator=(Wav && _Right) {
		destory();
		Decoder::operator=(_Right);
		file = _Right.file;
		_Right.file = nullptr;
		return *this;
	}
};


