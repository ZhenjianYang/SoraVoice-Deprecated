#pragma once

namespace ApiPack
{
	void AddApi(const char* name, void* ptrApi);
	void* GetApi(const char* name);
};

