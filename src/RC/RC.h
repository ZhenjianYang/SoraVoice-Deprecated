#pragma once

class RC
{
public:
	static constexpr unsigned MaxRCSize = 128 * 1024;

	virtual const char* First() const = 0;
	virtual unsigned Size() const = 0;

	virtual ~RC() { }

	static RC* Get(const char* name);

	struct RcItem {
		const char* name;
		int id;
		const char* type;
	};
	static void SetRcTable(RcItem* rcItemList);
	static void SetModuleHandle(void* mh);
};

