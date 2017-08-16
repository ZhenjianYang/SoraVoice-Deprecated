#define CINTERFACE
#include <dinput.h>

inline void** GetAddrGetDeviceState(void* pDID) {
	return (void**)&((IDirectInputDevice8A*)pDID)->lpVtbl->GetDeviceState;
}


