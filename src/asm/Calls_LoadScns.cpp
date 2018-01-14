#include "Calls.h"

#include <Hard/dir.h>

#include <stdio.h>
#include <string.h>

#define PATH_SN "voice/scena"
#define STR_ATTR ".bin"
#define OLD_PATH_SN "./data/scena"

#define BUFF_SIZE 0x10000
#define SCN_NUM 8

int SVCALL ASM_LoadScn(char* buff, int idx, int dir_group) {
	if (idx >= DIRS[dir_group].Num) return 0;

#define PATH_SN2 PATH_SN "/"

	char path[sizeof(PATH_SN2) + MAX_NAME_LEN] = PATH_SN2;
	path[sizeof(path) - 1] = '\0';

	strncpy(path + sizeof(PATH_SN2) - 1, DIRS[dir_group].Dir[idx], MAX_NAME_LEN);

	FILE* f = fopen(path, "rb");
	if (!f) return 0;

	int size = fread(buff, 1, BUFF_SIZE, f);
	fclose(f);

	return size;
}

int SVCALL ASM_LoadScns(char* buffs[], int idx_main, int game) {
	memset(buffs[0], 0, BUFF_SIZE);
	if (ASM_LoadScn(buffs[0], idx_main, game) < 0x40) return 0;

	for (unsigned i = 1; i < SCN_NUM; i++) {
		memset(buffs[i], 0, BUFF_SIZE);
		int id_tmp = *(int*)(buffs[0] + 0x20 + 4 * i);
		if (id_tmp != -1) {
			if (!ASM_LoadScn(buffs[i], id_tmp & 0xFFFF, game)) {
				return 0;
			};
		}
	}
	return 1;
}

#include <io.h>
int CCALL ASM_RdScnPath(void* /*ret*/, char* buff, const char* /*format*/, char* dir, const char* scn) {
	constexpr int len_old = sizeof(OLD_PATH_SN) - 1;
	constexpr int len_new = sizeof(PATH_SN) - 1;
	static_assert(len_new <= len_old, "len_new > len_old");

	if (strcmp(dir, OLD_PATH_SN)) return 0;

	int i = 0;
	for (; i < len_new; i++) {
		buff[i] = PATH_SN[i];
	}
	buff[i++] = '/';
	for (unsigned j = 0; scn[j]; j++, i++) {
		buff[i] = scn[j];
	}
	for (unsigned j = 0; j < sizeof(STR_ATTR); j++, i++) {
		buff[i] = STR_ATTR[j];
	}

	if (-1 == _access(buff, 4)) return 0;

	for (i = 0; i <= len_new; i++) {
		dir[i] = PATH_SN[i];
	}
	return 1;
}


