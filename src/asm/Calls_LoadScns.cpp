#include "Calls.h"

#include <Hard/dir.h>

#include <stdio.h>
#include <string.h>

#define PATH_SN "voice//scena/"
#define OLD_PATH_SN "./data/scena/"

#define BUFF_SIZE 0x10000
#define SCN_NUM 8

int SVCALL ASM_LoadScn(char* buff, int idx, int dir_group) {
	if (idx >= DIRS[dir_group].Num) return 0;

	char path[sizeof(PATH_SN) + MAX_NAME_LEN] = PATH_SN;
	path[sizeof(path) - 1] = '\0';

	strncpy(path + sizeof(PATH_SN) - 1, DIRS[dir_group].Dir[idx], MAX_NAME_LEN);

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
void SVCALL ASM_RdScnPath(char* path) {
	constexpr int len_old = sizeof(OLD_PATH_SN) - 1;
	constexpr int len_new = sizeof(PATH_SN) - 1;
	static_assert(len_new == len_old, "len_new != len_old");

	int i = 0;
	while (OLD_PATH_SN[i] && path[i] == OLD_PATH_SN[i]) {
		path[i] = PATH_SN[i];
		i++;
	}

	if (i != len_old || -1 == _access(path, 4)) {
		while (--i >= 0) path[i] = OLD_PATH_SN[i];
	}
}


