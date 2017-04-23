#pragma once

#include <ios>

class INI {
public:
	class Group {
	public:
		friend class INI;

		virtual bool Valid() const = 0;
		virtual int Num() const = 0;
		virtual const char* Name() const = 0;
		virtual const char* GetKey(int index) const = 0;
		virtual const char* GetValue(int index) const = 0;
		virtual const char* GetValue(const char* key) const = 0;
		const char* operator[] (int index) { return GetValue(index); }
		const char* operator[] (const char* key) { return GetValue(key); }

	private:
		Group(const Group&) = delete;
		Group& operator=(const Group&) = delete;
		Group(Group&&) = delete;
		Group& operator=(Group&&) = delete;

	protected:
		Group() = default;
		~Group() = default;
	};

	bool Valid() const { return data; }
	int Num() const;
	const Group& GetTopGroup() const { return GetGroup(0); };
	const Group& GetGroup(int index = 0) const;
	const Group& GetGroup(const char* name = nullptr) const;
	const Group& operator[](int index) { return GetGroup(index); };
	const Group& operator[](const char* name) { return GetGroup(name); };

	bool Open(const char* fileName);
	bool Open(std::istream& is);

	INI() { };
	INI(const char* fileName) { Open(fileName); };
	INI(std::istream& is) { Open(is); };

	~INI();
	INI(INI&& _Right);
	INI& operator=(INI&& _Right);
private:
	INI(const INI&) = delete;
	INI& operator=(const INI&) = delete;
	void* data = nullptr;
};


