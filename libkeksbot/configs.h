#ifndef CONFIGREADER_H
#define CONFIGREADER_H

#include <iostream>
#include <map>
#include <sstream>
#include <string>

typedef std::map<std::string, std::string> KeyValueMap;
typedef KeyValueMap::value_type KeyValuePair;
typedef std::map<std::string, KeyValueMap> SubsectionSettings;
typedef SubsectionSettings::value_type SubsectionSettingsPair;
typedef std::map<std::string, KeyValueMap> DefaultSettings;
typedef DefaultSettings::value_type DefaultSettingsPair;
typedef std::map<std::string, SubsectionSettings> SectionSettings;
typedef SectionSettings::value_type SectionSettingsPair;

template<typename T>
class Converter
{
private:
	bool success;
public:
	T Convert(const std::string& val)
	{
		std::istringstream sstr(val);
		T converted;
		sstr >> converted;
		success = !sstr.fail();
		return converted;
	}
	bool WasSuccessful()
	{
		return success;
	}
};
template<>
class Converter<bool>
{
private:
	bool success;
public:
	bool Convert(const std::string& val)
	{
		if(val == "yes" || val == "1" || val == "y" || val == "true")
		{
			success = true;
			return true;
		}
		else if(val == "no" || val == "0" || val == "n" || val == "false")
		{
			success = true;
			return false;
		}
		else
		{
			success = false;
			return false;
		}
	}

	bool WasSuccessful()
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

class Configs
{
private:
	DefaultSettings defaults;
	SectionSettings settings;
public:
	Configs(const std::string& filename);
	Configs(std::istream& input);

	const SectionSettings& GetSettings();

private:
	void Open(std::istream& input);
	void AddValue(const std::string& section,
		const std::string& subsection,
		const std::string& value);
};

#endif
