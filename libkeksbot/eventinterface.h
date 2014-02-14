#ifndef EVENTINTERFACE_H
#define EVENTINTERFACE_H

#include "configs.h"
#include <string>
#include <vector>

class Server;


enum EventType
{
	TYPE_SIMPLE = 0,
	TYPE_MISC,
	TYPE_END
};

class EventFilter
{
public:
	virtual ~EventFilter() { }
	virtual bool DoesHandle(Server& srv,
	                        const std::string& event,
        	                const std::string& origin,
	                        const std::vector<std::string>& params) = 0;
};

class EventHandler
{
private:
	EventType type;
	std::string alias;
	std::string description;
	EventFilter* filter;
public:
	virtual ~EventHandler() { }
	virtual void OnEvent(Server& srv,
	                     const std::string& event,
        	             const std::string& origin,
	                     const std::vector<std::string>& params) = 0;
	virtual bool DoesHandle(Server& srv,
	                        const std::string& event,
	                        const std::string& origin,
	                        const std::vector<std::string>& params);

	virtual EventType GetType()
	{
		return type;
	}
	virtual void SetType(EventType newType)
	{
		type = newType;
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

EventHandler* CreateEventHandler(const SubsectionSettingsPair& config, EventManager* man);

#endif
