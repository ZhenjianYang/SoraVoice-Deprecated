#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>

class ExData {
};
using ExDataList = std::unordered_map<int, ExData*>;

class SNTItem
{
	friend class SoraSNT;
public:
	using Line = struct { std::string content; ExData* exData; };
	using Lines = std::vector<Line>;
	enum class Type
	{
		Nomarl,
		Say,
		Text,
		Talk,
	};
	int Num() const { return lines.size(); }
	Type GetType() const { return type; }
	int GetId() const { return id; }
	Lines& GetLines() { return lines; }
	const Lines& GetLines() const { return lines; }
	
	void Output(std::ostream& ostr) const;
	SNTItem(int id, Type type = Type::Nomarl) : id(id), type(type) { };
private:
	int id = 0;
	Type type;
	Lines lines;
};

class SoraSNT
{
public:
	using Items = std::vector<SNTItem>;

	SoraSNT(std::istream& istr);
	SoraSNT(std::istream& istr, const ExDataList& exDataList);

	int Num() const { return items.size(); }

	Items& GetItems() { return items; }
	const Items& GetItems() const { return items; }
	
	SNTItem& Get(int id) { return items[id]; }
	const SNTItem& Get(int id) const { return items[id]; }

	SNTItem& operator[](int id) { return Get(id); }
	const SNTItem& operator[](int id) const { return Get(id); }

	void Output(std::ostream& ostr) const;
private:
	Items items;
};

