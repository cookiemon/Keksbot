#ifndef STATREQUESTER_H
#define STATREQUESTER_H

#include "nick.h"
#include <map>
#include <stdint.h>
#include <string>
#include <vector>

struct sqlite3;

class StatRequester
{
private:
	sqlite3* db;

public:
	enum TimePeriod
	{
		PERIOD_ALL,
		PERIOD_YEAR,
		PERIOD_MONTH,
		PERIOD_DAY,
		PERIOD_END
	};
	enum CountType
	{
		COUNTTYPE_CHAR,
		COUNTTYPE_BEER,
		COUNTTYPE_WORD,
		COUNTTYPE_LINE,
		COUNTTYPE_END
	};

	StatRequester(const std::string& dbfile);
	~StatRequester();

	void RequestData(const std::string& server,
		const std::string& channel,
		const TimePeriod time,
		const CountType type,
		const int64_t limit,
		std::vector<std::pair<Nick, int64_t> >& out);

private:
	std::string CreateStatement(CountType type, bool usePeriod, bool useLimit);
	std::string GetPeriodString(TimePeriod period);
	std::string GetCountTypeColumn(CountType type);
};

#endif
