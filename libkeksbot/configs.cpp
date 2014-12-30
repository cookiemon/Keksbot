#include "configs.h"
#include "stringhelpers.h"
#include <algorithm>
#include <ctype.h>
#include <fstream>
#include <functional>
#include <stack>

void Configs::Open(std::istream& input)
{
	std::stack<Configs*> stck;
	stck.push(this);
	std::string line;
	while(std::getline(input, line))
	{
		Trim(line);
		if(line.empty() || line[0] == '#' || line[0] == ';')
			continue;
		else if(line[0] == '[')
		{
			size_t lvl = line.find_first_not_of("[");
			size_t strEnd = line.find_last_not_of("]");
			if(lvl == std::string::npos
				|| strEnd == std::string::npos
				|| strEnd < lvl)
				throw ConfigException("Invalid subsection head: " + line);
			if(lvl > stck.size())
				throw ConfigException("Nesting in config file is broken");
			
			while(lvl < stck.size())
				stck.pop();
			std::string key(&line[lvl], &line[strEnd+1]);
			Configs& newConf = stck.top()->CreateSubsection(key);
			stck.push(&newConf);
		}
		else
		{
			size_t separator = line.find('=');
			if(separator == std::string::npos)
				throw ConfigException("Invalid line in config: " + line);

			std::string key = line.substr(0, separator);
			std::string value = line.substr(separator + 1);
			Trim(key);
			Trim(value);
			if(*(value.begin()) == '"'
				&& *(value.rbegin()) == '"'
				&& value.size() > 1)
			{
				value = value.substr(1, value.size() - 2);
			}
			stck.top()->AddValue(key, value);
		}
	}
}

