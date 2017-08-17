#ifndef SVCALL
#define SVCALL __stdcall
#endif

namespace ASM
{
	void SVCALL Play(const char* v);
	void SVCALL Stop();
	bool SVCALL Init();

	int SVCALL LoadScn(const char* name, char* buff);
	int SVCALL LoadScns(void* p_PScns, int id, char **pp_t);
};
