#include "Log.h"

#if !LOG_NOLOG

#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <iostream>
#include <fstream>

#include <Windows.h>

const char* const _strmark_info = LOG_STRMARK_INFO;
const char* const _strmark_debug = LOG_STRMARK_DEBUG;
const char* const _strmark_error = LOG_STRMARK_ERROR;

static int _logopened = 0;
static int _param;
static std::ofstream _log_ofs;
static std::ostream &_log_stdout = std::cout;
static std::ostream &_log_stdlog = std::clog;

static char filename_buf[1024];

void _log_setparam(int _param) {
	if (_logopened) {
		if (!(LOG_OUT_FILE & ::_param) && (LOG_OUT_FILE & _param)) {
			_log_ofs.open(filename_buf, std::ofstream::out | std::ofstream::app);
		}
		else if ((LOG_OUT_FILE & ::_param) && !(LOG_OUT_FILE & _param)) {
			_log_ofs.close();
		}
	}

	::_param = _param;
}
void _log_addparam(int _param) {
	_log_setparam(::_param | _param);
}
void _log_delparam(int _param) {
	_log_setparam(::_param & (~_param));
}
void _log_setlogfile(const char* filename) {
	strcpy_s(filename_buf, filename);
	if (_logopened && (LOG_OUT_FILE & _param)) {
		_log_ofs.close();
		_log_ofs.open(filename_buf, std::ofstream::out | std::ofstream::app);
	}
}
void _log_open(int _param) {
	_log_setparam(_param);
	if (filename_buf[0] == 0) strcpy_s(filename_buf, LOG_LOGFILE_DFT);
	if (LOG_OUT_FILE & _param) _log_ofs.open(filename_buf, std::ofstream::out | std::ofstream::app);
	_logopened = 1;
}
void _log_close() {
	_log_ofs.close();
	_logopened = 0;
}

void _log_print(int type, const char* format, ...) {
	if (!_logopened || !(type & _param)) return;

	const char* strmark;
	switch (type)
	{
	case LOG_TYPE_INFO:
		strmark = _strmark_info;
		break;
	case LOG_TYPE_DEBUG:
		strmark = _strmark_debug;
		break;
	case LOG_TYPE_ERROR:
		strmark = _strmark_error;
		break;
	default:
		return;
	}

	char tbuff[2048];

	SYSTEMTIME tm;
	LPSYSTEMTIME ptm = &tm;
	GetLocalTime(ptm);

	int len = 0;
	if (!(_param & LOG_PARAM_NOPREINFO)) {
		len = sprintf_s(tbuff, "[%02d-%02d-%02d %02d:%02d:%02d.%03d][%s]",
			ptm->wYear, ptm->wMonth, ptm->wDay,
			ptm->wHour, ptm->wMinute, ptm->wSecond, ptm->wMilliseconds,
			strmark);
	}

	va_list argptr;
	va_start(argptr, format);
	vsnprintf(tbuff + len, sizeof(tbuff) - len, format, argptr);
	va_end(argptr);

	if (LOG_OUT_FILE & _param) _log_ofs << tbuff << std::endl;
	if (LOG_OUT_STDOUT & _param) _log_stdout << tbuff << std::endl;
	if (LOG_OUT_STDLOG & _param) _log_stdlog << tbuff << std::endl;
}

void _log_empty_line()
{
	if (!_logopened) return;
	if (LOG_OUT_FILE & _param) _log_ofs << std::endl;
	if (LOG_OUT_STDOUT & _param) _log_stdout << std::endl;
	if (LOG_OUT_STDLOG & _param) _log_stdlog << std::endl;
}

#endif // !LOG_NOLOG
