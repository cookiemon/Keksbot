#ifndef STATS_H
#define STATS_H

#include "configs.h"
#include "eventinterface.h"
#include "nick.h"
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

class Stats : public EventHandler
{
private:
	std::string dbfile;
public:
	Stats(const Configs& settings);
	~Stats();

	void OnEvent(Server& server,
		const std::string& event,
		const std::string& origin,
		const ParamList& params);

	EventType GetType()
	{
		return TYPE_SIMPLE;
	}

private:
	void SendStats(Server& server, const std::string& channel,
					bool isDigitaler,
					const std::vector<std::pair<Nick, int64_t> >& vals);
};

#endif
