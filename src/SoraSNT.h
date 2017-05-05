#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>

class ExData {
};
using ExDataList = std::unordered_map<int, ExData*>;

class ItemType
{
private:
	friend class All;
	unsigned text_start;
	std::string name;
	char mark;
	ItemType(unsigned text_start, const std::string& name, char mark)
		: text_start(text_start), name(name), mark(mark) {}
	ItemType(const ItemType&) = delete;
	ItemType& operator=(const ItemType&) = delete;
	ItemType(ItemType&&) = delete;
	ItemType& operator=(ItemType&&) = delete;

public:
	class All {
	private:
		static const ItemType _normal;
		static const ItemType _text;
		static const ItemType _say;
		static const ItemType _talk;

	public:
		static constexpr const ItemType* Nomarl = &_normal;
		static constexpr const ItemType* Text = &_text;
		static constexpr const ItemType* Say = &_say;
		static constexpr const ItemType* Talk = &_talk;

	private:
		All() = delete;
	};

public:
	const unsigned &TextStartLine = text_start;
	const std::string Name = name;
	const char &Mark = mark;
};
using PItemType = const ItemType*;
using AllItemTypes = ItemType::All;

class SNTItem
{
	friend class SoraSNT;
public:
	using ItemLine = struct { std::string content; ExData* exData; };
	using ItemLinesList = std::vector<ItemLine>;

	unsigned Num() const { return lines.size(); }
	const PItemType& Type = type;
	const int& ID =  id;
	ItemLinesList &Lines = lines;

	ItemLine& Get(int id) { return lines[id]; }
	const ItemLine& Get(int id) const { return lines[id]; }

	ItemLine& operator[](int id) { return Get(id); }
	const ItemLine& operator[](int id) const { return Get(id); }

	void Output(std::ostream& ostr) const;
	SNTItem(int id, PItemType type = ItemType::All::Nomarl) : id(id), type(type) { };

	SNTItem(const SNTItem& _Other)
		: id(_Other.id), type(_Other.type), lines(_Other.lines) {}
	SNTItem& operator=(const SNTItem& _Other) {
		id = _Other.id;
		type = _Other.type;
		lines = _Other.lines;
		return *this;
	}

	SNTItem(SNTItem&& _Right)
			: id(_Right.id), type(_Right.type), lines(std::move(_Right.lines)) {}
	SNTItem& operator=(SNTItem&& _Right) {
			id = _Right.id;
			type = _Right.type;
			lines = std::move(_Right.lines);
			return *this;
	}

private:
	int id;
	PItemType type;
	ItemLinesList lines;
};

class SoraSNT
{
public:
	using ItemsList = std::vector<SNTItem>;

	static constexpr PItemType TalkTypes[] = { AllItemTypes::Text, AllItemTypes::Say, AllItemTypes::Talk };
	static constexpr int TalksNum = std::extent<decltype(TalkTypes)>::value;

	SoraSNT(std::istream& istr);
	SoraSNT(std::istream& istr, const ExDataList& exDataList);

	int Num() const { return items.size(); }

	ItemsList& Items = items;
	
	SNTItem& Get(int id) { return items[id]; }
	const SNTItem& Get(int id) const { return items[id]; }

	SNTItem& operator[](int id) { return Get(id); }
	const SNTItem& operator[](int id) const { return Get(id); }

	void Output(std::ostream& ostr) const;

	SoraSNT(const SoraSNT& _Other)
		: items(_Other.items) {}
	SoraSNT& operator=(const SoraSNT& _Other) {
		items = _Other.items;
		return *this;
	}

	SoraSNT(SoraSNT&& _Right)
		: items(std::move(_Right.items)) { }
	SoraSNT& operator=(SoraSNT&& _Right) {
		items = std::move(_Right.items);
		return *this;
	}
private:
	ItemsList items;
};

