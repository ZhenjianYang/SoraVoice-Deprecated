#define CINTERFACE

#include <stdio.h>
#include <d3d8/d3d8.h>

#define ERROR_EXIT(condition) if(condition) { printf(#condition "\n"); return false; }

bool GetDX8() {
	HMODULE md_d3d8 = LoadLibraryA("C:\\Windows\\System32\\d3d8.dll");
	auto pDirect3DCreate8 = (decltype(Direct3DCreate8)*)GetProcAddress(md_d3d8, "Direct3DCreate8");
	ERROR_EXIT(!pDirect3DCreate8);

	IDirect3D8* pD3D = pDirect3DCreate8(D3D_SDK_VERSION);
	ERROR_EXIT(!pD3D);

	printf("Direct3DCreate8 : d3d8.dll+%08X\n", (unsigned)pDirect3DCreate8 - (unsigned)md_d3d8);
	printf("IDirect3D8::CreateDevice : d3d8.dll+%08X\n", (unsigned)pD3D->lpVtbl->CreateDevice - (unsigned)md_d3d8);
	printf("\n");

	pD3D->lpVtbl->Release(pD3D);

	return true;
}

