#ifndef STATS_H
#define STATS_H

#include "eventinterface.h"
#include "configs.h"
#include <stdint.h>
#include <sqlite3.h>
#include <string>

enum CountType
{
	COUNTTYPE_CHAR,
	COUNTTYPE_WORD,
	COUNTTYPE_LINE,
	COUNTTYPE_BEER,
	COUNTTYPE_END
};

enum TimePeriod
{
	PERIOD_ALL,
	PERIOD_DAY,
	PERIOD_WEEK,
	PERIOD_MONTH,
	PERIOD_YEAR,
	PERIOD_END
};

class Stats : public EventHandler
{
private:
	std::string dbfile;
public:
	Stats(const KeyValueMap& settings);
	~Stats();

	void OnEvent(ServerInterface& server,
		const std::string& event,
		const std::string& origin,
		const ParamList& params);

	EventType GetType()
	{
		return TYPE_SIMPLE;
	}

private:
	void SendStats(ServerInterface& server, const std::string& channel,
					TimePeriod period, CountType type);
	void LoadValues(const std::string& server, const std::string& channel,
					TimePeriod period, CountType type,
					std::vector<std::pair<std::string, int64_t> >& out);
	std::string CreateStatement(TimePeriod period, CountType type);
};

#endif
