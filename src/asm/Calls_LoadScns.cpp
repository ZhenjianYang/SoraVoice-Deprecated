#include "Calls.h"

#include <stdio.h>
#include <string.h>

#define PATH_SN "voice//scena/"
#define OLD_PATH_SN "./data/scena/"
#define MAX_NAME_LEN 12

#define BUFF_SIZE 0x10000

#define SCN_NUM 8

int SVCALL ASM_LoadScn(const char* name, char* buff) {
	char path[sizeof(PATH_SN) + MAX_NAME_LEN] = PATH_SN;
	path[sizeof(path) - 1] = '\0';

	strncpy(path + sizeof(PATH_SN) - 1, name, MAX_NAME_LEN);

	FILE* f = fopen(path, "rb");
	if (!f) return 0;

	int size = fread(buff, 1, BUFF_SIZE, f);
	fclose(f);

	return size;
}

int SVCALL ASM_LoadScns(char* buffs[], int id, char **pp_t) {
	for (unsigned i = 0; i < SCN_NUM; i++) {
		memset(buffs[i], 0, BUFF_SIZE);
	}

	char* sn_name = *(pp_t + (id >> 16)) + 36 * (id & 0xFFFF);
	if (ASM_LoadScn(sn_name, buffs[0]) < 0x40) return 0;

	for (unsigned i = 1; i < SCN_NUM; i++) {
		int id_tmp = *(int*)(buffs[0] + 0x20 + 4 * i);
		if (id_tmp != -1) {
			char* sn_name_tmp = *(pp_t + (id_tmp >> 16)) + 36 * (id_tmp & 0xFFFF);
			if (!ASM_LoadScn(sn_name_tmp, buffs[i])) {
				return 0;
			};
		}
	}
	return 1;
}

#include <io.h>
void SVCALL ASM_RdScnPath(char* path) {
	constexpr int len_old = sizeof(OLD_PATH_SN) - 1;
	constexpr int len_new = sizeof(PATH_SN) - 1;
	static_assert(len_new == len_old, "len_new != len_old");

	int i;
	for (i = 0; i < len_old; i++) {
		if (path[i] != OLD_PATH_SN[i]) {
			break;
		}
		path[i] = PATH_SN[i];
	}

	if (i != len_old || -1 == _access(path, 4)) {
		while (--i >= 0) path[i] = OLD_PATH_SN[i];
	}
}


