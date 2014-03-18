#ifndef STATTRACKER_H
#define STATTRACKER_H

#include "configs.h"
#include "eventinterface.h"
#include "serverinterface.h"
#include <string>
#include <sqlite3.h>

class StatTracker : public EventHandler
{
private:
	sqlite3* db;
	sqlite3_stmt* getNickStmt;
	sqlite3_stmt* insertStatsStmt;
	sqlite3_stmt* updateStatsStmt;
public:
	StatTracker(const Configs& params);
	~StatTracker();
	void OnEvent(ServerInterface& srv,
		const std::string& event,
		const std::string& origin,
		const ParamList& params);
	bool DoesHandle(ServerInterface& srv,
		const std::string& event,
		const std::string& origin,
		const ParamList& params);

	EventType GetType()
	{
		return TYPE_MISC;
	}

private:
	void LogDBError(int type, const std::string& msg);
	int BindVariables(sqlite3_stmt* stmt,
					const std::string& nick, const std::string& server, const std::string& channel,
					int charcount, int wordcount, int linecount,
					time_t lastseen);
	int ExecuteStatement(sqlite3_stmt* stmt);
};

#endif
