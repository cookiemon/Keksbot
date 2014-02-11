#ifndef CONFIGREADER_H
#define CONFIGREADER_H

#include <iostream>
#include <map>
#include <string>

typedef std::map<std::string, std::string> KeyValueMap;
typedef KeyValueMap::value_type KeyValuePair;
typedef std::map<std::string, KeyValueMap> SubsectionSettings;
typedef SubsectionSettings::value_type SubsectionSettingsPair;
typedef std::map<std::string, KeyValueMap> DefaultSettings;
typedef DefaultSettings::value_type DefaultSettingsPair;
typedef std::map<std::string, SubsectionSettings> SectionSettings;
typedef SectionSettings::value_type SectionSettingsPair;

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
