#pragma once

class ApiPack
{
public:
	static void AddApi(const char* name, void* ptrApi);
	static void* GetApi(const char* name);

private:
	virtual ~ApiPack() = 0;
};

