#include "stats.h"
#include "exceptions.h"
#include "logging.h"
#include "serverinterface.h"
#include <sstream>

const char* GET_STATS_1 = "SELECT COALESCE(aliases.nick, stats.nick), SUM(";
const char* GET_STATS_2 = ") FROM stats LEFT JOIN aliases "
						" ON aliases.alias = stats.nick AND aliases.server = stats.server "
						" WHERE stats.server = ?1 AND channel = ?2 ";
const char* GET_STATS_3 = "AND strftime('%s', 'now', ";
const char* GET_STATS_4 = ") < timestamp ";
const char* GET_STATS_5 = " GROUP BY COALESCE( aliases.nick, stats.nick) ORDER BY SUM(";
const char* GET_STATS_6 = ") DESC LIMIT 10;";
const char* GET_STATS_DAY = "'start of day'";
const char* GET_STATS_WEEK = "'weekday 0', '-7 days'";
const char* GET_STATS_MONTH = "'start of month'";
const char* GET_STATS_YEAR = "'start of year'";

Stats::Stats(const KeyValueMap& settings)
{
	KeyValueMap::const_iterator it = settings.find("dbfile");
	if(it == settings.end() || it->second.empty())
	{
		Log(LOG_ERR, "No db file given");
		throw ConfigException("No db file given");
	}
	dbfile = it->second;
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
	
	TimePeriod period = PERIOD_ALL;
	if(message.find("year") != std::string::npos)
		period = PERIOD_YEAR;
	else if(message.find("month") != std::string::npos)
		period = PERIOD_MONTH;
	else if(message.find("week") != std::string::npos)
		period = PERIOD_WEEK;
	else if(message.find("day") != std::string::npos)
		period = PERIOD_DAY;
	
	CountType type = COUNTTYPE_CHAR;
	if(message.find("word") != std::string::npos)
		type = COUNTTYPE_WORD;
	else if(message.find("line") != std::string::npos)
		type = COUNTTYPE_LINE;
	else if(message.find("digitaler") != std::string::npos)
		type = COUNTTYPE_BEER;

	SendStats(server, channel, period, type);
}

std::string Stats::CreateStatement(TimePeriod period, CountType type)
{
	std::string columnname = "charcount";
	switch(type)
	{
	case COUNTTYPE_WORD:
		columnname = "wordcount";
		break;
	case COUNTTYPE_LINE:
		columnname = "linecount";
		break;
	case COUNTTYPE_CHAR:
	case COUNTTYPE_BEER:
		break;
	default:
	case COUNTTYPE_END:
		Log(LOG_ERR, "Argument error in Stats::CreateStatement");
	}
	std::string sql(GET_STATS_1);
	sql += columnname + GET_STATS_2;
	
	if(period != PERIOD_ALL)
	{
		sql += GET_STATS_3;
		switch(period)
		{
		case PERIOD_DAY:
			sql += GET_STATS_DAY;
			break;
		case PERIOD_WEEK:
			sql += GET_STATS_WEEK;
			break;
		case PERIOD_MONTH:
			sql += GET_STATS_MONTH;
			break;
		case PERIOD_YEAR:
			sql += GET_STATS_YEAR;
			break;
		case PERIOD_ALL:
			break;
		case PERIOD_END:
			Log(LOG_ERR, "Critical argument error in Stats::CreateStatement");
		}
		sql += GET_STATS_4;
	}
	sql += GET_STATS_5;
	sql += columnname + GET_STATS_6;
	return sql;
}

void Stats::LoadValues(const std::string& server,
	const std::string& channel,
	TimePeriod period,
	CountType type,
	std::vector<std::pair<std::string, int64_t> >& out)
{
	sqlite3* db = NULL;
	int res = sqlite3_open(dbfile.c_str(), &db);
	if(res != 0)
	{
		Log(LOG_ERR, "Could not open sqlite db %s", dbfile.c_str());
		sqlite3_close(db);
		return;
	}

	sqlite3_stmt* getStatsStmt = NULL;
	res = sqlite3_prepare(db, CreateStatement(period, type).c_str(), -1, &getStatsStmt, NULL);
	if(res != 0)
		Log(LOG_ERR, "Could not prepare statement");

	res = sqlite3_bind_text(getStatsStmt, 1, server.c_str(), -1, SQLITE_TRANSIENT);
	res |= sqlite3_bind_text(getStatsStmt, 2, channel.c_str(), -1, SQLITE_TRANSIENT);
	if(res != 0)
	{
		Log(LOG_ERR, "Error while binding query parameter");
	}

	while((res = sqlite3_step(getStatsStmt)) == SQLITE_ROW)
	{
		std::pair<std::string, int64_t> val;
		
		val.first = reinterpret_cast<const char*>(sqlite3_column_text(getStatsStmt, 0));
		val.second = sqlite3_column_int64(getStatsStmt, 1);

		out.push_back(val);
	}
	if(res != SQLITE_DONE)
		Log(LOG_ERR, "Error while executing sql query");

	sqlite3_finalize(getStatsStmt);
	sqlite3_close(db);
}

void Stats::SendStats(ServerInterface& server,
	const std::string& channel,
	TimePeriod period,
	CountType type)
{
	std::vector<std::pair<std::string, int64_t> > curVals;
	LoadValues(server.GetName(), channel, period, type, curVals);
	std::stringstream reply;
	if(type != COUNTTYPE_BEER)
		reply << "Top 10 Spammers: ";
	else
		reply << "Digitaler in wallet: ";
	if(curVals.size() != 0)
		reply << curVals[0].first
			<< "(" << curVals[0].second / (type == COUNTTYPE_BEER?100000:1) << ")";
	for(size_t i = 1; i < curVals.size(); ++i)
		reply << ", " << curVals[i].first
			<< "(" << curVals[i].second / (type == COUNTTYPE_BEER?100000:1) << ")";
	server.SendMsg(channel, reply.str());
}

