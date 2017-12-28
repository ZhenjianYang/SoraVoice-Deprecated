#include "MemPatch.h"
#include <cstring>
#include <algorithm>

void MemPatch::SetOld(const void * data, unsigned len)
{
	if (!data) len = 0;

	if (len > this->len_old) {
		delete[] data_old;
		this->data_old = new char[len];
	}

	if (len == 0) {
		delete[] data_old;
		data_old = nullptr;
	}
	else {
		std::memcpy(this->data_old, data, len);
	}
	this->len_old = len;
}
void MemPatch::SetOld(const char * data)
{
	unsigned len = data ? strlen(data) + 1 : 0;
	SetOld(data, len);
}

void MemPatch::SetNew(const void * data, unsigned len)
{
	if (!data) len = 0;

	if (len > this->len_new) {
		delete[] data_new;
		this->data_new = new char[len];
	}

	if (len == 0) {
		delete[] data_new;
		data_new = nullptr;
	}
	else {
		std::memcpy(this->data_new, data, len);
	}
	this->len_new = len;
}
void MemPatch::SetNew(const char * data)
{
	unsigned len = data ? strlen(data) + 1 : 0;
	SetNew(data, len);
}

MemPatch::MemPatch(const MemPatch& _Other) {
	this->offset = _Other.offset;
	SetOld(_Other.data_old, _Other.len_old);
	SetNew(_Other.data_new, _Other.len_new);
}

MemPatch& MemPatch::operator=(const MemPatch & _Other) {
	this->offset = _Other.offset;
	SetOld(_Other.data_old, _Other.len_old);
	SetNew(_Other.data_new, _Other.len_new);
	return *this;
}

MemPatch::MemPatch(MemPatch && _Right)
{
	std::swap(this->offset, _Right.offset);
	std::swap(this->data_old, _Right.data_old);
	std::swap(this->data_new, _Right.data_new);
	std::swap(this->len_old, _Right.len_old);
	std::swap(this->len_new, _Right.len_new);
}

MemPatch & MemPatch::operator=(MemPatch && _Right)
{
	std::swap(this->offset, _Right.offset);
	std::swap(this->data_old, _Right.data_old);
	std::swap(this->data_new, _Right.data_new);
	std::swap(this->len_old, _Right.len_old);
	std::swap(this->len_new, _Right.len_new);

	return *this;
}

void MemPatch::destory()
{
	delete[] data_new;
	delete[] data_old;
	offset = 0;
	len_new = len_old = 0;
	data_new = data_old = nullptr;
}
