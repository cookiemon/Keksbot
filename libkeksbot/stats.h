#ifndef STATS_H
#define STATS_H

#include "eventinterface.h"
#include "configs.h"
#include "statrequester.h"
#include <stdint.h>
#include <string>

class Stats : public EventHandler
{
private:
	std::string dbfile;
public:
	Stats(const Configs& settings);
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
					StatRequester::TimePeriod period, StatRequester::CountType type);
	void LoadValues(const std::string& server, const std::string& channel,
					StatRequester::TimePeriod period, StatRequester::CountType type,
					std::vector<std::pair<std::string, int64_t> >& out);
};

#endif
