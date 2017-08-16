#define CINTERFACE
#include <d3d8/d3d8.h>

inline void** GetAddrPresentDX8(void* pD3DD) {
	return (void**)&((IDirect3DDevice8*)pD3DD)->lpVtbl->Present;
}


