#ifndef EVENTMANAGER_H
#define EVENTMANAGER_H

#include "eventinterface.h"
#include <map>
#include <string>
#include <vector>

class Server;
class SelectingInterface;

typedef std::map<std::string, EventHandler*> AliasedMap;

class EventManager
{
private:
	std::vector<Server*> serverlist;
	AliasedMap aliasedEvents;
	std::vector<EventHandler*> miscEvents;
	std::vector<EventHandler*> markedForDelete;

	std::vector<SelectingInterface*> networklisteners;
public:
	EventManager(const std::string& cfgfile);
	~EventManager(void);
	void DoSelect(void);

	void DistributeEvent(Server& source,
											const std::string& event,
											const std::string& origin,
											const ParamList& params);
	
	const std::vector<Server*>& GetServers();
	std::vector<EventHandler*> GetEvents();

	void AddNetworklistener(SelectingInterface* listener);
	void DelNetworklistener(SelectingInterface* listener);

	ServerInterface* GetServer(const std::string& name);

	void AddEvent(EventHandler* evt);
	void DelEvent(EventHandler* evt);

private:
	void DistributeSimpleEvent(Server& source,
														const std::string& event,
														const std::string& origin,
														const ParamList& params);

	void ExecuteDeletions(void);
};

#endif
