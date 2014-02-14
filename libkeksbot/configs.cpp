#include "logging.h"
#include "configs.h"
#include <algorithm>
#include <ctype.h>
#include <fstream>
#include <functional>

struct IsSpaceFunctor
{
	bool operator()(char stuff) const { return isspace(stuff); }
	typedef char argument_type;
};

void TrimLeft(std::string& str)
{
	str.erase(str.begin(), std::find_if(str.begin(), str.end(), std::not1(IsSpaceFunctor())));
}

void TrimRight(std::string& str)
{
	std::string::reverse_iterator pos = std::find_if(str.rbegin(), str.rend(),
	                                                 std::not1(IsSpaceFunctor()));
	if(pos != str.rend() && pos != str.rbegin())
		str.erase(pos.base() + 1);
}

void Trim(std::string& str)
{
	TrimRight(str);
	TrimLeft(str);
}

Configs::Configs(const std::string& filename)
{
	std::ifstream file(filename.c_str());
	Open(file);
}

Configs::Configs(std::istream& input)
{
	Open(input);
}

void Configs::Open(std::istream& input)
{
	std::string line;
	std::string section;
	std::string subsection;
	Trim(line);
	while(std::getline(input, line))
	{
		if(line.empty() || line[0] == ';' || line[0] == '#')
			continue;
		else if(line[0] == '[' && line[line.size()-1] == ']')
		{
			if(line[1] == '[' && line[line.size()-2] == ']')
			{
				subsection = line.substr(2, line.size() - 4);
				settings[section].insert(SubsectionSettingsPair(subsection, defaults[section]));
			}
			else
			{
				section = line.substr(1, line.size() - 2);
				subsection = "";
				defaults.insert(DefaultSettingsPair(section, KeyValueMap()));
				settings.insert(SectionSettingsPair(section, SubsectionSettings()));
			}
		}
		else
		{
			AddValue(section, subsection, line);
		}
	}
}

void Configs::AddValue(const std::string& section,
	const std::string& subsection,
	const std::string& value)
{
	size_t pos = value.find('=');
	if(pos == std::string::npos)
	{
		Log(LOG_ERR, "Config line contains no '=': %s", value.c_str());
		return;
	}

	std::string key = value.substr(0, pos);
	Trim(key);
	std::string val = value.substr(pos + 1);
	Trim(val);
	if(val[0] == '"')
	{
		size_t end = val.rfind('"');
		if(end != 0)
			val = val.substr(1, end - 1);
	}

	if(subsection == "")
	{
		defaults[section][key] = val;
	}
	else
	{
		settings[section][subsection][key] = val;
	}
}

const SectionSettings& Configs::GetSettings()
{
	return settings;
}

