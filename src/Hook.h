#pragma once

void InitHook_SetInitParam(void* ip);

void* Hook_D3D_Present(void* pD3DD);

void* Hook_DI_GetDeviceState(void* pDID);

