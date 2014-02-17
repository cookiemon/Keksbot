#include "stattracker.h"
#include "configs.h"
#include "exceptions.h"
#include "logging.h"
#include <stdlib.h>

const char* CREATE_STAT_TABLE_SQL = "CREATE TABLE IF NOT EXISTS stats"
									"(nick TEXT NOT NULL, "
									"server TEXT NOT NULL, "
									"channel TEXT NOT NULL, "
									"timestamp INTEGER, "
									"charcount INTEGER, "
									"wordcount INTEGER, "
									"linecount INTEGER, "
									"PRIMARY KEY(nick, server, channel, timestamp));";
const char* CREATE_ALIAS_TABLE_SQL = "CREATE TABLE IF NOT EXISTS alias"
									"( alias TEXT NOT NULL, "
									"server TEXT NOT NULL, "
									"nick TEXT NOT NULL, "
									"PRIMARY KEY(alias, server));";

const char* GET_NICK_SQL = "SELECT nick FROM alias "
							"WHERE alias = ?1 AND "
							"server = ?2;";
const char* INSERT_STATS_SQL = "INSERT INTO stats "
							"(nick, server, channel, charcount, wordcount, linecount, timestamp) "
							"VALUES "
							"(?1, ?2, ?3, ?4, ?5, ?6, ?7);";
const char* UPDATE_STATS_SQL = "UPDATE stats "
							"SET "
							"charcount = charcount + ?4, "
							"wordcount = charcount + ?5, "
							"linecount = charcount + ?6 "
							"WHERE "
							"nick = ?1 AND server = ?2 AND channel = ?3 AND timestamp = ?7;";

StatTracker::StatTracker(const KeyValueMap& params)
	: db(NULL),
	getNickStmt(NULL),
	insertStatsStmt(NULL),
	updateStatsStmt(NULL)
{
	KeyValueMap::const_iterator it = params.find("dbfile");
	if(it == params.end() || it->second.empty())
		throw ConfigException("No db file specified");
	int res = sqlite3_open(it->second.c_str(), &db);
	if(res != 0)
	{
		if(db != NULL)
		{
			sqlite3_close(db);
			db = NULL;
		}
		throw ConfigException("Could not open database");
	}
	char* errMsg = NULL;
	res = sqlite3_exec(db, CREATE_STAT_TABLE_SQL, NULL, NULL, NULL);
	if(res != 0)
	{
		LogDBError(LOG_ERR, "Could not create stat table");
		sqlite3_free(errMsg);
	}
	res = sqlite3_exec(db, CREATE_ALIAS_TABLE_SQL, NULL, NULL, NULL);
	if(res != 0)
	{
		LogDBError(LOG_ERR, "Could not create alias table");
		sqlite3_free(errMsg);
	}

	res = sqlite3_prepare_v2(db, GET_NICK_SQL, -1, &getNickStmt, NULL);
	if(res != 0)
		LogDBError(LOG_ERR, "Could not create get nick statement");
	res = sqlite3_prepare_v2(db, INSERT_STATS_SQL, -1, &insertStatsStmt, NULL);
	if(res != 0)
		LogDBError(LOG_ERR, "Could not create insert stats statement");
	res = sqlite3_prepare_v2(db, UPDATE_STATS_SQL, -1, &updateStatsStmt, NULL);
	if(res != 0)
		LogDBError(LOG_ERR, "Could not create update stats statement");
}

StatTracker::~StatTracker()
{
	sqlite3_finalize(getNickStmt);
	sqlite3_finalize(insertStatsStmt);
	sqlite3_finalize(updateStatsStmt);
	sqlite3_close(db);
}

void StatTracker::OnEvent(ServerInterface& srv,
	const std::string& event,
	const std::string& origin,
	const ParamList& params)
{
	if(params.size() == 0)
	{
		Log(LOG_ERR, "Stattracker received empty message, ignoring");
		return;
	}
	const std::string& message = params[params.size() - 1];
	int linecount = 1;
	int charcount = message.length();
	int wordcount = 0;
	size_t it = message.find_first_not_of(" \t\r\n");
	while(it != std::string::npos)
	{
		wordcount += 1;
		it = message.find_first_of(" \t\r\n", it);
		it = message.find_first_not_of(" \t\r\n", it);
	}
	time_t lastseen = time(NULL);

	int res = BindVariables(insertStatsStmt,
							origin, srv.GetName(), params.size() > 1 ? params[0] : "",
							charcount, wordcount, linecount, lastseen);
	if(res != 0)
		LogDBError(LOG_ERR, "Could not bind variables to input statement");
	else
		res = ExecuteStatement(insertStatsStmt);

	if(res != SQLITE_DONE)
	{
		res = BindVariables(updateStatsStmt,
					origin, srv.GetName(), params.size() > 1 ? params[0] : "",
					charcount, wordcount, linecount, lastseen);
		if(res != 0)
			LogDBError(LOG_ERR, "Could not bind variables to update statement");
		else
			res = ExecuteStatement(updateStatsStmt);
	}

	if(res != SQLITE_DONE)
		LogDBError(LOG_ERR, "Could neither insert nor update stats table");
}

void StatTracker::LogDBError(int type, const std::string& msg)
{
	Log(type, "%s: %s", msg.c_str(), sqlite3_errmsg(db));
}

int StatTracker::ExecuteStatement(sqlite3_stmt* stmt)
{
	int res = SQLITE_BUSY;
	while(res == SQLITE_BUSY)
		res = sqlite3_step(stmt);
	sqlite3_reset(stmt);
	return res;
}

int StatTracker::BindVariables(sqlite3_stmt* stmt,
	const std::string& nick,
	const std::string& server,
	const std::string& channel,
	int charcount,
	int wordcount,
	int linecount,
	time_t timestamp)
{
	int res = 0;
	res |= sqlite3_bind_text(stmt, 1, nick.c_str(), -1, SQLITE_TRANSIENT);
	res |= sqlite3_bind_text(stmt, 2, server.c_str(), -1, SQLITE_TRANSIENT);
	res |= sqlite3_bind_text(stmt, 3, channel.c_str(), -1, SQLITE_TRANSIENT);
	res |= sqlite3_bind_int(stmt, 4, charcount);
	res |= sqlite3_bind_int(stmt, 5, wordcount);
	res |= sqlite3_bind_int(stmt, 6, linecount);
	res |= sqlite3_bind_int64(stmt, 7, timestamp);
	return res;
}

bool StatTracker::DoesHandle(ServerInterface& server,
	const std::string& event,
	const std::string& origin,
	const ParamList& params)
{
	return event == "CHANNEL" || event == "ACTION";
}
