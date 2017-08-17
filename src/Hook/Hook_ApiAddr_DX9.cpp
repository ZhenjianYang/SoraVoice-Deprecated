#include "Hook_Apis.h"

#define CINTERFACE
#include <d3d9/d3d9.h>

void** GetAddrPresent(void* pD3DD) {
	return (void**)&((IDirect3DDevice9*)pD3DD)->lpVtbl->Present;
}


