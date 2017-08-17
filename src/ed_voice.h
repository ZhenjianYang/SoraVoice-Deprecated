#pragma once

#ifndef SVCALL
#define SVCALL __stdcall
#endif

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	int SVCALL Start();
	int SVCALL End();

#ifdef __cplusplus
}
#endif // __cplusplus

