#include "statrequester.h"
#include "exceptions.h"
#include "logging.h"
#include "serverinterface.h"
#include <sqlite3.h>
#include <sstream>

const char* GET_STATS_SELECT = "SELECT COALESCE(aliases.nick, stats.nick) ";
const char* GET_STATS_COLUMN_1 = ", SUM(";
const char* GET_STATS_COLUMN_2 = ") ";
const char* GET_STATS_FROM = "FROM stats LEFT JOIN aliases "
			" ON aliases.alias = stats.nick AND aliases.server = stats.server "
			" WHERE stats.server = ?1 AND channel = ?2 ";
const char* GET_STATS_WHERE_TIME = "AND strftime('%s', 'now', ?3) < timestamp ";
const char* GET_STATS_GROUP = "GROUP BY COALESCE( aliases.nick, stats.nick) ";
const char* GET_STATS_ORDER_1 = "ORDER BY SUM(";
const char* GET_STATS_ORDER_2 = ") DESC LIMIT ?4;";

const char* GET_STATS_PERIOD[] = {  "hurrrrrrrrrrr",
									"start of year",
									"start of month",
									"start of day" };
const char* GET_STATS_COLUMNS[] = { "charcount",
									"charcount",
									"wordcount",
									"linecount" };

StatRequester::StatRequester(const std::string& dbfile)
	: dbfile(dbfile)
{
}

StatRequester::~StatRequester()
{
}

std::string StatRequester::CreateStatement(TimePeriod period,
	CountType type,
	int64_t limit)
{
	std::string columnname = GetCountTypeColumn(type);
	std::string sql(GET_STATS_SELECT);
	sql += GET_STATS_COLUMN_1 + columnname + GET_STATS_COLUMN_2;

	sql += GET_STATS_FROM;

	if(period != PERIOD_ALL)
		sql += GET_STATS_WHERE_TIME;
	sql += GET_STATS_GROUP;
	sql += GET_STATS_ORDER_1 + columnname + GET_STATS_ORDER_2;
	return sql;
}

void StatRequester::RequestData(const std::string& server,
	const std::string& channel,
	TimePeriod period,
	CountType type,
	int64_t limit,
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
	std::string sqlStatement = CreateStatement(period, type, limit);
	res = sqlite3_prepare(db, sqlStatement.c_str(), -1, &getStatsStmt, NULL);
	if(res != 0)
		Log(LOG_ERR, "Could not prepare statement");

	res = sqlite3_bind_text(getStatsStmt, 1, server.c_str(), -1, SQLITE_TRANSIENT);
	res |= sqlite3_bind_text(getStatsStmt, 2, channel.c_str(), -1, SQLITE_TRANSIENT);
	res |= sqlite3_bind_text(getStatsStmt, 3, GetPeriodString(period).c_str(), -1, SQLITE_TRANSIENT);
	res |= sqlite3_bind_int64(getStatsStmt, 4, limit);
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

std::string StatRequester::GetPeriodString(TimePeriod period)
{
	if(period >= PERIOD_END)
		throw IllegalArgumentException("Cannot convert period to string");
	return GET_STATS_PERIOD[period];
}

std::string StatRequester::GetCountTypeColumn(CountType type)
{
	if(type >= COUNTTYPE_END)
		throw IllegalArgumentException("Cannot convert counttype to string");
	return GET_STATS_COLUMNS[type];
}
