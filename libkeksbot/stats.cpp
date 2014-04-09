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
	if(params.size() > 0)
		message = params[params.size() - 1];
	const std::string& sourceChan = params.size() > 1 ? params[0] : origin;
	std::string channel = sourceChan;

	if(message[0] == '#')
	{
		size_t space = message.find_first_of(" \r\t\n");
		channel = message.substr(0, space);
		if(space != std::string::npos)
			message = message.substr(message.find_first_not_of(" \r\t\n", space));
		else
			message = "";
	}
	
	StatRequester::TimePeriod period = StatRequester::PERIOD_ALL;
	if(message.find("year") != std::string::npos)
		period = StatRequester::PERIOD_YEAR;
	else if(message.find("month") != std::string::npos)
		period = StatRequester::PERIOD_MONTH;
	else if(message.find("day") != std::string::npos)
		period = StatRequester::PERIOD_DAY;

	StatRequester::CountType type = StatRequester::COUNTTYPE_CHAR;
	if(message.find("word") != std::string::npos)
		type = StatRequester::COUNTTYPE_WORD;
	else if(message.find("line") != std::string::npos)
		type = StatRequester::COUNTTYPE_LINE;
	else if(message.find("digitaler") != std::string::npos)
		type = StatRequester::COUNTTYPE_BEER;

	StatRequester req(dbfile);
	std::vector<std::pair<std::string, int64_t> > vals;
	req.RequestData(server.GetName(), channel, period, type, 10, vals);

	SendStats(server, sourceChan,
		type == StatRequester::COUNTTYPE_BEER,
		vals);
}

void Stats::SendStats(ServerInterface& server,
	const std::string& channel,
	bool isDigitaler,
	const std::vector<std::pair<std::string, int64_t> >& vals)
{
	std::stringstream reply;
	if(!isDigitaler)
		reply << "Top 10 Spammers: ";
	else
		reply << "Digitaler in wallet: ";
	if(vals.size() != 0)
		reply << vals[0].first
			<< "(" << vals[0].second / (isDigitaler?100000:1) << ")";
	for(size_t i = 1; i < vals.size(); ++i)
		reply << ", " << vals[i].first
			<< "(" << vals[i].second / (isDigitaler?100000:1) << ")";
	server.SendMsg(channel, reply.str());
}

