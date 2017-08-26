#pragma once

#include "Decoder.h"

class Ogg : private Decoder {
public:
	static constexpr char Attr[] = "ogg";
	static Decoder* const ogg;

	virtual bool Open(const char* fileName) override;
	virtual int Read(void * buff, int bytes) override;
	virtual void Close() override;

public:
	static void SetOggApis(void * ov_open_callbacks,
		void * ov_info, void * ov_read, void * ov_clear, 
		void * ov_pcm_total);

private:
	static Ogg _ogg;

	void* ovFile = nullptr;
	void destory();

	virtual ~Ogg() { destory(); }
	Ogg();

	Ogg(const Ogg&) = delete;
	Ogg& operator=(const Ogg&) = delete;
	Ogg(Ogg&&) = delete;
	Ogg& operator=(Ogg&&) = delete;
};

