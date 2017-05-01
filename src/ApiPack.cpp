#include "ApiPack.h"
#include <string>
#include <unordered_map>

using namespace std;
using MapNameApi = unordered_map<string, void*>;

MapNameApi apis;

void ApiPack::AddApi(const char * name, void * ptrApi) {
	apis[name] = ptrApi;
}

void * ApiPack::GetApi(const char * name)
{
	auto it = apis.find(name);
	return it == apis.end() ? nullptr : it->second;
}
