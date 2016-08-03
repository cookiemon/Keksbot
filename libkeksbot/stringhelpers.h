#ifndef STRINGHELPERS_H
#define STRINGHELPERS_H

#include <algorithm>
#include <string>

struct IsSpaceFunctor
{
	bool operator()(char stuff) const { return isspace(stuff); }
	typedef char argument_type;
};

inline void TrimLeft(std::string& str)
{
	str.erase(str.begin(), std::find_if(str.begin(), str.end(),
													std::not1(IsSpaceFunctor())));
}

inline void TrimRight(std::string& str)
{
	std::string::reverse_iterator pos = std::find_if(str.rbegin(), str.rend(),
	                                                 std::not1(IsSpaceFunctor()));
	if(pos != str.rend() && pos != str.rbegin())
		str.erase(pos.base());
}

inline void Trim(std::string& str)
{
	TrimRight(str);
	TrimLeft(str);
}

inline std::string CutFirstWord(std::string& toCut)
{
	size_t it = toCut.find_first_of(" \r\t\n");
	std::string wrd = toCut.substr(0, it);
	if(it != std::string::npos)
	{
	    toCut = toCut.substr(it + 1);
	    TrimLeft(toCut);
	}
	else
	{
	    toCut = "";
	}
	return wrd;
}

inline void ReplaceSubstring(std::string& str, const std::string& find, const std::string& rep)
{
	size_t idx = 0;
	while((idx = str.find(find, idx)) != std::string::npos)
	{
		str.replace(idx, find.size(), rep);
	}
}

inline std::string EscapeString(std::string str,
	const std::string& chars,
	const std::string& escapeChar)
{
	size_t it = 0;
	while((it = str.find_first_of(chars, it)) != std::string::npos)
	{
		str.insert(it, escapeChar);
		it += escapeChar.size() + 1;
	}
	return str;
}


#endif
