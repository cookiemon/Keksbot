#ifndef CONFIGREADER_H
#define CONFIGREADER_H

#include <iostream>
#include <map>
#include <sstream>
#include <fstream>
#include <iostream>
#include <string>
#include "exceptions.h"

template<typename T>
class Converter
{
private:
	bool success;
public:
	void Convert(const std::string& val, T& out)
	{
		std::istringstream sstr(val);
		sstr >> out;
		success = !sstr.fail();
	}
	bool WasSuccessful()
	{
		return success;
	}
};
template<>
class Converter<std::string>
{
public:
	void Convert(const std::string& val, std::string& out)
	{
		out = val;
	}
	bool WasSuccessful()
	{
		return true;
	}
};
template<>
class Converter<bool>
{
private:
	bool success;
public:
	void Convert(const std::string& val, bool& out)
	{
		if(val == "yes" || val == "1" || val == "y" || val == "true")
		{
			success = true;
			out = true;
		}
		else if(val == "no" || val == "0" || val == "n" || val == "false")
		{
			success = true;
			out = false;
		}
		else
		{
			success = false;
		}
	}

	bool WasSuccessful() const
	{
		return success;
	}
};

template<typename T>
T ConvertOrDefault(const std::string& val, const T& defaultVal)
{
	Converter<T> conv;
	T converted = conv.Convert(val);
	if(conv.WasSuccessful())
		return converted;
	else
		return defaultVal;
}

class Configs;

typedef std::map<std::string, std::string> ValueMap;
typedef std::map<std::string, Configs> SubsectionMap;

class Configs
{
private:
	const Configs* const parent;
	std::string name;
	ValueMap values;
	SubsectionMap subsections;

public:
	Configs(const std::string& filename)
		: parent(NULL)
	{
		std::ifstream file(filename.c_str());
		Open(file);
	}

	const std::string& GetName() const
	{
		return name;
	}

	template<typename T>
	void GetValue(const std::string& key, T& value) const
	{
		ValueMap::const_iterator it = values.find(key);
		if(it == values.end())
			throw ConfigException(("Could not get value " + key).c_str());

		Converter<T> conv;
		conv.Convert(it->second, value);
		if(!conv.WasSuccessful())
			throw ConfigException(("Could not convert string " +
				key + "=" + value).c_str());
	}

	template<typename T>
	void GetValueOrDefault(const std::string& key,
		T& value,
		const T& def = T()) const
	{
		ValueMap::const_iterator it = values.find(key);
		if(it != values.end())
		{
			Converter<T> conv;
			conv.Convert(it->second, value);
			if(!conv.WasSuccessful())
				value = def;
		}
		else
		{
			if(parent == NULL)
				value = def;
			else
				return parent->GetValueOrDefault(key, value, def);
		}
	}

	const Configs& GetSubsection(const std::string& key) const
	{
		SubsectionMap::const_iterator it = subsections.find(key);
		if(it == subsections.end())
			throw ConfigException(("No config sections " + key + " found").c_str());
		return it->second;
	}

	SubsectionMap::const_iterator FirstSubsection() const
	{
		return subsections.begin();
	}

	SubsectionMap::const_iterator EndSubsection() const
	{
		return subsections.end();
	}

private:
	Configs(const Configs* parent, const std::string& name)
		: parent(parent), name(name)
	{
	}

	void Open(std::istream& input);

	void AddValue(const std::string& key, const std::string& Value)
	{
		values.insert(ValueMap::value_type(key, Value));
	}
	Configs& CreateSubsection(const std::string& key)
	{
		std::pair<SubsectionMap::iterator, bool> res;
		res=subsections.insert(SubsectionMap::value_type(key, Configs(this, key)));
		return res.first->second;
	}
};

#endif
