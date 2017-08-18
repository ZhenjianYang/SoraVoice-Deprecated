#pragma once

#ifndef LOG
#ifdef LOG_NOLOG
#define LOG(...)
#else
#include <chrono>
#include <ctime>
#include <cstdio>

#ifndef LOG_FILENAME
#define LOG_FILENAME "voice/ed_voice.log"
#endif

#ifndef LOG_NOFILEOUT
#define LOG_TO_FILE(format, ...) \
		auto _f = std::fopen(LOG_FILENAME, "a");\
		std::fprintf(_f, "[%04d-%02d-%02d %02d:%02d:%02d.%03d]" format "\n", _t->tm_year + 1900, _t->tm_mon + 1, _t->tm_mday,\
						_t->tm_hour, _t->tm_min, _t->tm_sec, (int)_millis_part, ##__VA_ARGS__);\
		std::fclose(_f);
#else
#define LOG_TO_FILE(...)
#endif

#ifndef LOG_NOFSTDOUT
#define LOG_TO_STDOUT(format, ...) \
		std::printf("[%04d-%02d-%02d %02d:%02d:%02d.%03d]" format "\n", _t->tm_year + 1900, _t->tm_mon + 1, _t->tm_mday,\
						_t->tm_hour, _t->tm_min, _t->tm_sec, (int)_millis_part, ##__VA_ARGS__);
#else
#define LOG_TO_STDOUT(...)
#endif

#define LOG(format, ...) 	{\
		using _Clock = std::chrono::system_clock;\
		auto _now = _Clock::now();\
		auto _millis_part = std::chrono::duration_cast<std::chrono::milliseconds>(\
				_now.time_since_epoch()).count() % 1000;\
		auto tt = _Clock::to_time_t(_now);\
		auto _t = std::localtime(&tt);\
		LOG_TO_FILE(format, ##__VA_ARGS__);\
		LOG_TO_STDOUT(format, ##__VA_ARGS__);\
	}

#endif // !LOG_NOLOG
#endif //!LOG


