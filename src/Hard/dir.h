#pragma once

#include "dir_fc.h"
#include "dir_sc.h"
#include "dir_3rd.h"
#include "dir_3rd_quiz.h"

constexpr int MAX_NAME_LEN = 12;
constexpr int NUM_FC = sizeof(DIR_FC) / sizeof(*DIR_FC);
constexpr int NUM_SC = sizeof(DIR_SC) / sizeof(*DIR_SC);
constexpr int NUM_3RD = sizeof(DIR_3RD) / sizeof(*DIR_3RD);
constexpr int NUM_3RD_QUIZ = sizeof(DIR_3RD_QUIZ) / sizeof(*DIR_3RD_QUIZ);

static_assert(MAX_NAME_LEN == sizeof(*DIR_FC) - 1, "MAX NAME LEN of DIR_FC is not 12!");
static_assert(MAX_NAME_LEN == sizeof(*DIR_SC) - 1, "MAX NAME LEN of DIR_SC is not 12!");
static_assert(MAX_NAME_LEN == sizeof(*DIR_3RD) - 1, "MAX NAME LEN of DIR_3RD is not 12!");
static_assert(MAX_NAME_LEN == sizeof(*DIR_3RD_QUIZ) - 1, "MAX NAME LEN of DIR_3RD_QUIZ is not 12!");

static const struct {
	int Num;
	const char (*Dir)[MAX_NAME_LEN + 1];
} DIRS[] = {
	{ 0, nullptr },
	{ NUM_FC,  DIR_FC },
	{ NUM_SC,  DIR_SC },
	{ NUM_3RD,  DIR_3RD },
	{ NUM_3RD_QUIZ,  DIR_3RD_QUIZ },
};
constexpr int MAX_DIR_NO = sizeof(DIRS) / sizeof(*DIRS);
