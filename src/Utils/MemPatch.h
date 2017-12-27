#pragma once

#include <type_traits>

class MemPatch
{
public:
	template<typename DataType, typename = std::enable_if_t<!std::is_pointer<DataType>::value && !std::is_array<DataType>::value>>
	void SetOld(const DataType& data) {
		SetOld((const void*)&data, sizeof(data));
	}
	void SetOld(const void* data, unsigned len);
	void SetOld(const char* data);

	template<typename DataType, typename = std::enable_if_t<!std::is_pointer<DataType>::value && !std::is_array<DataType>::value>>
	void SetNew(const DataType& data) {
		SetNew((const char*)&data, sizeof(data));
	}
	void SetNew(const void* data, unsigned len);
	void SetNew(const char* data);

public:
	MemPatch() = default;
	~MemPatch() { this->destory(); }

	MemPatch(const MemPatch& _Other);
	MemPatch& operator= (const MemPatch& _Other);
	MemPatch(MemPatch&& _Right);
	MemPatch& operator= (MemPatch&& _Right);

public:
	unsigned GetOffset() const { return offset; };
	void SetOffset(unsigned offset) { this->offset = offset; };

	unsigned GetOldDataLen() const { return len_old; };
	unsigned GetNewDataLen() const { return len_new; };

	const char* GetOldDataBuff() const { return data_old; };
	const char* GetNewDataBuff() const { return data_new; };

private:
	unsigned offset = 0;
	unsigned len_old = 0;
	unsigned len_new = 0;
	char* data_old = nullptr;
	char* data_new = nullptr;
	void destory();
};

