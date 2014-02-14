#ifndef EVENTMANAGER_H
#define EVENTMANAGER_H

#include "server.h"
#include "eventinterface.h"
#include <map>
#include <string>
#include <vector>

class EventManager
{
private:
	std::vector<Server*> serverlist;
	std::map<std::string, EventHandler*> aliasedEvents;
	std::vector<EventHandler*> miscEvents;
public:
	EventManager(const std::string& cfgfile);
	~EventManager(void);
	void DoSelect(void);

	void DistributeEvent(Server& source,
	                     const std::string& event,
						 const std::string& origin,
						 const ParamList& params);
};

#endif
