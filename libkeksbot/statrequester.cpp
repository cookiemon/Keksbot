#include "statrequester.h"
#include "exceptions.h"
#include <sqlite3.h>
#include <sstream>

const char* GET_STATS_SELECT = "SELECT COALESCE(aliases.nick, stats.nick) ";
const char* GET_STATS_COLUMN_1 = ", SUM(";
const char* GET_STATS_COLUMN_2 = ") ";
const char* GET_STATS_FROM = "FROM stats LEFT JOIN aliases "
			"ON aliases.alias = stats.nick AND aliases.server = stats.server "
			"WHERE stats.server = ?1 AND channel = ?2 ";
const char* GET_STATS_WHERE_TIME = "AND strftime('%s', 'now', 'localtime', ?3, 'utc') < timestamp ";
const char* GET_STATS_GROUP = "GROUP BY COALESCE( aliases.nick, stats.nick) ";
const char* GET_STATS_ORDER_1 = "ORDER BY SUM(";
const char* GET_STATS_ORDER_2 = ") DESC ";
const char* GET_STATS_LIMIT = "LIMIT ?4";
const char* GET_STATS_END = ";";

const char* GET_STATS_PERIOD[] = {  "hurrrrrrrrrrr",
									"start of year",
									"start of month",
									"start of day" };
const char* GET_STATS_COLUMNS[] = { "charcount",
									"charcount",
									"wordcount",
									"linecount" };

StatRequester::StatRequester(const std::string& dbfile)
{
	int res = sqlite3_open(dbfile.c_str(), &db);
	if(res != 0)
	{
		sqlite3_close(db);
		throw SqliteException(res);
	}
}

StatRequester::~StatRequester()
{
	sqlite3_close(db);
}

std::string StatRequester::CreateStatement(CountType type,
	bool usePeriod,
	bool useLimit)
{
	std::string columnname = GetCountTypeColumn(type);
	std::string sql(GET_STATS_SELECT);
	sql += GET_STATS_COLUMN_1 + columnname + GET_STATS_COLUMN_2;

	sql += GET_STATS_FROM;

	if(usePeriod)
		sql += GET_STATS_WHERE_TIME;
	sql += GET_STATS_GROUP;
	sql += GET_STATS_ORDER_1 + columnname + GET_STATS_ORDER_2;

	if(useLimit)
		sql += GET_STATS_LIMIT;
	
	sql += GET_STATS_END;
	return sql;
}

void StatRequester::RequestData(const std::string& server,
	const std::string& channel,
	TimePeriod period,
	CountType type,
	int64_t limit,
	std::vector<std::pair<Nick, int64_t> >& out)
{
	sqlite3_stmt* getStatsStmt = NULL;
	std::string sqlStatement = CreateStatement(type,
		period != PERIOD_ALL,
		limit >= 0);
	int res = sqlite3_prepare(db, sqlStatement.c_str(), -1, &getStatsStmt, NULL);
	if(res != 0)
		throw SqliteException(sqlite3_errcode(db));

	res = sqlite3_bind_text(getStatsStmt, 1, server.c_str(), -1, SQLITE_TRANSIENT);
	res |= sqlite3_bind_text(getStatsStmt, 2, channel.c_str(), -1, SQLITE_TRANSIENT);
	res |= sqlite3_bind_text(getStatsStmt, 3, GetPeriodString(period).c_str(), -1, SQLITE_TRANSIENT);
	res |= sqlite3_bind_int64(getStatsStmt, 4, limit);
	if(res != 0)
		throw SqliteException(sqlite3_errcode(db));

	while((res = sqlite3_step(getStatsStmt)) == SQLITE_ROW)
	{
		Nick nick = Nick(reinterpret_cast<const char*>(sqlite3_column_text(getStatsStmt, 0)));
		int64_t statValue = sqlite3_column_int64(getStatsStmt, 1);

		out.push_back(std::make_pair(nick, statValue));
	}
	if(res != SQLITE_DONE)
		throw SqliteException(sqlite3_errcode(db));

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
