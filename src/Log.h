#ifndef __LOG_H__
#define __LOG_H__

#define LOG_OUT_FILE 0x01
#define LOG_OUT_STDLOG 0x02
#define LOG_OUT_STDOUT 0x04
#define LOG_OUT_ALL (LOG_OUT_FILE | LOG_OUT_STDLOG | LOG_OUT_STDOUT)
#define LOG_OUT_DFT (LOG_OUT_FILE | LOG_OUT_STDLOG)

#define LOG_TYPE_INFO 0x10
#define LOG_TYPE_DEBUG 0x20
#define LOG_TYPE_ERROR 0x40
#define LOG_TYPE_ALL (LOG_TYPE_INFO | LOG_TYPE_DEBUG | LOG_TYPE_ERROR)
#define LOG_TYPE_DFT (LOG_TYPE_INFO | LOG_TYPE_ERROR)
#define LOG_TYPE_RUNMODE (LOG_TYPE_INFO | LOG_TYPE_ERROR)
#define LOG_TYPE_DEBUGMODE LOG_TYPE_ALL

#define LOG_PARAM_ALL (LOG_OUT_ALL | LOG_TYPE_ALL)
#define LOG_PARAM_DFT (LOG_OUT_DFT | LOG_TYPE_DFT)
#define LOG_PARAM_RUNMODE (LOG_OUT_DFT | LOG_TYPE_RUNMODE)
#define LOG_PARAM_DEBUGMODE (LOG_OUT_DFT | LOG_TYPE_DEBUGMODE)

#define LOG_PARAM_NOPREINFO 0x1000

#define LOG_STRMARK_INFO "INF"
#define LOG_STRMARK_DEBUG "DBG"
#define LOG_STRMARK_ERROR "ERR"

#define LOG_LOGFILE_DFT "log.txt"

#if !LOG_NOLOG

#define LOG_SETLOGFILE(filename) _log_setlogfile(filename)
#define LOG_SETPARAM(_param) _log_setparam(_param)
#define LOG_ADDPARAM(_param) _log_addparam(_param)
#define LOG_DELPARAM(_param) _log_delparam(_param)
#define LOG_OPEN _log_open(LOG_PARAM_DFT)
#define LOG_OPEN_WITHPARAM(_param) _log_open(_param)
#define LOG_CLOSE _log_close()

#define LOG_INFO(format, ...)  _log_print(LOG_TYPE_INFO, format , __VA_ARGS__)
#define LOG_DEBUG(format, ...) _log_print(LOG_TYPE_DEBUG, format, __VA_ARGS__)
#define LOG_ERROR(format, ...) _log_print(LOG_TYPE_ERROR, format, __VA_ARGS__)
#define LOG(format, ...) LOG_INFO(format, __VA_ARGS__)

#define LOG_EMPTY_LINE _log_empty_line()

void _log_addparam(int _param);
void _log_delparam(int _param);
void _log_setparam(int _param);
void _log_setlogfile(const char* filename);
void _log_open(int _param);
void _log_close();

void _log_print(int type, const char* format, ...);

void _log_empty_line();

#else 

#define LOG_SETLOGFILE(filename)
#define LOG_SETPARAM(_param)
#define LOG_ADDPARAM(_param)
#define LOG_DELPARAM(_param)
#define LOG_OPEN
#define LOG_OPEN_WITHPARAM(_param)
#define LOG_CLOSE _log_close()

#define LOG_INFO(format, ...)
#define LOG_DEBUG(format, ...)
#define LOG_ERROR(format, ...)
#define LOG(format, ...)

#define LOG_EMPTY_LINE
	
#endif //LOG_NOLOG

#endif //__LOG_H__
