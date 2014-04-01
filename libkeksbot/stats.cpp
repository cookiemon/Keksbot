#include "stats.h"
#include "exceptions.h"
#include "logging.h"
#include "serverinterface.h"
#include "statrequester.h"
#include <sstream>

Stats::Stats(const Configs& settings)
{
	settings.GetValue("dbfile", dbfile);
}

Stats::~Stats()
{
}

void Stats::OnEvent(ServerInterface& server,
	const std::string& event, const std::string& origin, const ParamList& params)
{
	std::string message = "";
	std::string channel = params.size() > 1 ? params[0] : origin;
	if(params.size() > 0)
		message = params[params.size() - 1];
	
	StatRequester::TimePeriod period = StatRequester::PERIOD_ALL;
	if(message.find("year") != std::string::npos)
		period = StatRequester::PERIOD_YEAR;
	else if(message.find("month") != std::string::npos)
		period = StatRequester::PERIOD_MONTH;
	else if(message.find("week") != std::string::npos)
		period = StatRequester::PERIOD_WEEK;
	else if(message.find("day") != std::string::npos)
		period = StatRequester::PERIOD_DAY;
	
	StatRequester::CountType type = StatRequester::COUNTTYPE_CHAR;
	if(message.find("word") != std::string::npos)
		type = StatRequester::COUNTTYPE_WORD;
	else if(message.find("line") != std::string::npos)
		type = StatRequester::COUNTTYPE_LINE;
	else if(message.find("digitaler") != std::string::npos)
		type = StatRequester::COUNTTYPE_BEER;

	SendStats(server, channel, period, type);
}

void Stats::SendStats(ServerInterface& server,
	const std::string& channel,
	StatRequester::TimePeriod period,
	StatRequester::CountType type)
{
	std::vector<std::pair<std::string, int64_t> > curVals;
	StatRequester req(dbfile);
	req.RequestData(server.GetName(), channel, period, type, 10, curVals);
	std::stringstream reply;
	if(type != StatRequester::COUNTTYPE_BEER)
		reply << "Top 10 Spammers: ";
	else
		reply << "Digitaler in wallet: ";
	if(curVals.size() != 0)
		reply << curVals[0].first
			<< "(" << curVals[0].second / (type == StatRequester::COUNTTYPE_BEER?100000:1) << ")";
	for(size_t i = 1; i < curVals.size(); ++i)
		reply << ", " << curVals[i].first
			<< "(" << curVals[i].second / (type == StatRequester::COUNTTYPE_BEER?100000:1) << ")";
	server.SendMsg(channel, reply.str());
}

