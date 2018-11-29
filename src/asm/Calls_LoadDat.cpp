#include "Calls.h"

#include <SVData.h>

#include <cstring>
#include <cstdio>

#define FAKE_COMP_TAG_MAGIC 0x9CCBB9F7
#define MAX_FILES 4096
#define FILENAME_BUFF_SIZE 64

typedef struct RedirectDir {
	int idx;
	const char* dir;
} RedirectDir;
static const RedirectDir REDIRECT_DIRS[] = {
	{ 0x00, "voice/fonts" },
	{ 0x20, "voice/fonts" },
	{ 0x01, "voice/scena" },
	{ 0x21, "voice/scena" },
	{ 0x22, "voice/scena" }
};
static const char REDIRECT_DIR_DFT[] = "voice/files";

typedef struct FakeComp {
	unsigned size_comp;
	unsigned size_uncomp;
	unsigned tag;
	void* data;
} FakeComp;

typedef struct DirEntry {
	const char name[0x10];
	unsigned size;
	unsigned unknown[3];
	unsigned offset;
} DirEntry;

int CCALL ASM_LoadDat(void*, void* buff, int idx, unsigned offset, unsigned size) {
	const DirEntry* entry = *((const DirEntry**)SV.addrs.addr_pdirs + idx);
	for (int i = 0; i < MAX_FILES && (entry->offset != offset || entry->size != size); entry++, i++);
	if (entry->offset != offset || entry->size != size) return 0;

	const char* dir = REDIRECT_DIR_DFT;
	for (int i = 0; i < sizeof(REDIRECT_DIRS) / sizeof(*REDIRECT_DIRS); i++) {
		if (REDIRECT_DIRS[i].idx == idx) {
			dir = REDIRECT_DIRS[i].dir;
			break;
		}
	}

	char buff_filename[FILENAME_BUFF_SIZE];
	std::sprintf(buff_filename, "%s/%s", dir, entry->name);
	FILE* file = std::fopen(buff_filename, "rb");
	if (!file) return 0;

	std::fseek(file, 0, SEEK_END);
	int size_uncomp = (int)ftell(file);
	std::fseek(file, 0, SEEK_SET);

	FakeComp* fc = (FakeComp*)buff;
	fc->size_comp = size;
	fc->size_uncomp = size_uncomp;
	fc->tag = FAKE_COMP_TAG_MAGIC;
	fc->data = new char[size_uncomp];
	std::fread(fc->data, 1, size_uncomp, file);
	fclose(file);

	return 1;
}

int SVCALL ASM_DecompressDat(void** compressed, void** uncompressed) {
	FakeComp* fc = *(FakeComp**)compressed;
	if (fc->tag != FAKE_COMP_TAG_MAGIC || !fc->data) return 0;

	std::memcpy(*uncompressed, fc->data, fc->size_uncomp);

	*(char**)uncompressed += fc->size_uncomp;
	*(char**)compressed += fc->size_comp;

	delete[] fc->data;
	fc->data = nullptr;
	return fc->size_uncomp;
}
