#include "ed_voice.h"

#ifndef ZA

#include <string>
#include <fstream>

#define PATH_SN "voice/scena/"
#define RAW_FILE_MAGIC 0x43464445
#define MAX_BUFF_SIZE 0x10000

struct RAW_FILE {
	unsigned sizeComp;
	unsigned tag;
	unsigned sizeUncomp;
	char data[MAX_BUFF_SIZE - 12];
};

unsigned SVCALL LoadDat(const char* name, void* buff) {
	RAW_FILE* rf = (RAW_FILE*)buff;

	std::string path(PATH_SN);
	path.append(name);

	std::ifstream ifs(path, std::ios::binary);
	if (!ifs) return 0;

	ifs.seekg(0, std::ios::end);
	rf->sizeUncomp = (unsigned)ifs.tellg();
	if (rf->sizeUncomp > sizeof(rf->data)) rf->sizeUncomp = sizeof(rf->data);

	rf->tag = RAW_FILE_MAGIC;

	ifs.seekg(0, std::ios::beg);
	ifs.read(rf->data, rf->sizeUncomp);
	return (unsigned)ifs.gcount();
}

#endif // !ZA
