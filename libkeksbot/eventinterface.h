#ifndef EVENTINTERFACE_H
#define EVENTINTERFACE_H

#include "configs.h"
#include <string>
#include <vector>

class ServerInterface;


enum EventType
{
	TYPE_SIMPLE = 0,
	TYPE_MISC,
	TYPE_END
};

typedef std::vector<std::string> ParamList;

class EventHandler
{
private:
	EventType type;
	bool shown;
	std::string alias;
	std::string description;
public:
	EventHandler()
		: type(TYPE_END), shown(true)
	{
	}
	virtual ~EventHandler() { }
	virtual void OnEvent(ServerInterface& srv,
	                     const std::string& event,
        	             const std::string& origin,
	                     const std::vector<std::string>& params) = 0;
	virtual bool DoesHandle(ServerInterface& srv,
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

EventHandler* CreateEventHandler(const SubsectionSettingsPair& config, EventManager* man);

#endif
