#include "Hook_Apis.h"

#define CINTERFACE
#include <dinput.h>

void** GetAddrGetDeviceState(void* pDID) {
	return (void**)&((IDirectInputDevice8A*)pDID)->lpVtbl->GetDeviceState;
}


