#ifndef EVENTINTERFACE_H
#define EVENTINTERFACE_H

#include "configs.h"
#include <string>
#include <vector>

enum EventType
{
	TYPE_SIMPLE = 0,
	TYPE_MISC,
	TYPE_END
};

class Server;
typedef std::vector<std::string> ParamList;

class EventHandler
{
private:
	bool shown;
	std::string alias;
	std::string description;
public:
	EventHandler()
		: shown(true)
	{
	}
	virtual ~EventHandler() { }
	virtual void OnEvent(Server& srv,
	                     const std::string& event,
        	             const std::string& origin,
	                     const std::vector<std::string>& params) = 0;
	virtual bool DoesHandle(Server& srv,
	                        const std::string& event,
	                        const std::string& origin,
	                        const std::vector<std::string>& params);

	virtual EventType GetType() = 0;

	virtual bool GetShown()
	{
		return shown;
	}
	virtual void SetShown(bool newShown)
	{
		shown = newShown;
	}
	virtual const std::string& GetAlias()
	{
		return alias;
	}
	virtual void SetAlias(const std::string& newAlias)
	{
		alias = newAlias;
	}
	virtual const std::string& GetDescription()
	{
		return description;
	}
	virtual void SetDescription(const std::string& newDesc)
	{
		description = newDesc;
	}
};

class EventManager;

EventHandler* CreateEventHandler(const Configs& config, EventManager* man);

#endif
