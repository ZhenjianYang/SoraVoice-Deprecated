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
	class Type
	{
	private:
		friend class All;
		unsigned text_start;
		std::string name;
		Type(unsigned text_start, const std::string& name) : text_start(text_start), name(name) {}

	public:
		class All {
		private:
			static const Type _normal;
			static const Type _text;
			static const Type _say;
			static const Type _talk;

		public:
			static constexpr const Type* Nomarl = &_normal;
			static constexpr const Type* Text = &_text;
			static constexpr const Type* Say = &_say;
			static constexpr const Type* Talk = &_talk;

		private:
			All() = delete;
		};

	public:
		const unsigned &TextStartLine = text_start;
		const std::string &Name = name;
	};
	using PType = const Type*;

	using Line = struct { std::string content; ExData* exData; };
	using Lines = std::vector<Line>;

	unsigned Num() const { return lines.size(); }
	PType GetType() const { return type; }
	int GetId() const { return id; }
	Lines& GetLines() { return lines; }
	const Lines& GetLines() const { return lines; }
	
	void Output(std::ostream& ostr) const;
	SNTItem(int id, PType type = Type::All::Nomarl) : id(id), type(type) { };
private:
	int id = 0;
	PType type;
	Lines lines;
};

class SoraSNT
{
public:
	using Type = SNTItem::Type;
	using PType = SNTItem::PType;
	using Types = Type::All;
	using Items = std::vector<SNTItem>;

	static constexpr PType TalkTypes[] = { Types::Text, Types::Say, Types::Talk };
	static constexpr int TalksNum = std::extent<decltype(TalkTypes)>::value;

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

