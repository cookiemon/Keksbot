#ifndef EVENTMANAGER_H
#define EVENTMANAGER_H

#include "server.h"
#include <string>
#include <vector>

class EventManager
{
private:
	std::vector<Server*> serverlist;
public:
	EventManager(const std::string& cfgfile);
	~EventManager(void);
	void DoSelect(void);
};

#endif
