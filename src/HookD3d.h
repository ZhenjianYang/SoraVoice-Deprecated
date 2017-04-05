#pragma once

struct IDirect3DDevice8;

void* Hook_IDirect3DDevice8_Present(IDirect3DDevice8* D3DD, void* sv);


