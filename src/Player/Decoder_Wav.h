#pragma once

#include "Decoder.h"

class Wav : private Decoder {
public:
	static constexpr char Attr[] = "wav";
	static Decoder* const wav;

	virtual bool Open(const char* fileName) override;
	virtual int Read(void * buff, int bytes) override;
	virtual void Close() override;

private:
	static Wav _wav;

	void *file = nullptr;
	void destory();

	Wav() = default;
	virtual ~Wav() { destory(); }

	Wav(const Wav&) = delete;
	Wav& operator=(const Wav&) = delete;
	Wav(Wav&&) = delete;
	Wav& operator=(Wav&&) = delete;
};


