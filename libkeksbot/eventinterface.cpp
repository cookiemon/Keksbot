#include "eventinterface.h"
#include "exceptions.h"
#include "simpleevent.h"
#include "statichandlers.h"
#include "stattracker.h"
#include <assert.h>

EventHandler* CreateEventHandler(const SubsectionSettingsPair& configs, EventManager* man)
{
	assert(man != NULL);

	KeyValueMap::const_iterator valIt = configs.second.find("alias");
	if(valIt == configs.second.end() || valIt->second.empty())
		throw ConfigException("alias missing");
	std::string alias = valIt->second;

	EventHandler* newHandler;

	valIt = configs.second.find("type");
	if(valIt != configs.second.end())
	{
		if(valIt->second == "simple")
		{
			valIt = configs.second.find("reply");
			if(valIt == configs.second.end() || valIt->second.empty())
				throw ConfigException("reply missing");
			std::string reply = valIt->second;
			
			newHandler = new SimpleEvent(NULL, reply);
		}
		else if(valIt->second == "static")
		{
			valIt = configs.second.find("handler");
			if(valIt == configs.second.end() || valIt->second.empty())
				throw ConfigException("static handler type missint");
			if(valIt->second == "restart")
				newHandler = new RestartHandler();
			else if(valIt->second == "exit")
				newHandler = new ExitHandler();
			else if(valIt->second == "help")
				newHandler = new HelpHandler(man);
			else if(valIt->second == "stattracker")
				newHandler = new StatTracker(configs.second);
			else
				throw ConfigException("static handler type not known");
		}
		else
			throw ConfigException("event handler type not known");
	}

	newHandler->SetAlias(alias);
	valIt = configs.second.find("description");
	if(valIt != configs.second.end())
		newHandler->SetDescription(valIt->second);

	valIt = configs.second.find("show");
	if(valIt != configs.second.end()
	   && ( valIt->second == "0" || valIt->second == "no" || valIt->second == "false"))
		newHandler->SetShown(false);
	else
		newHandler->SetShown(true);

	return newHandler;
}

bool EventHandler::DoesHandle(ServerInterface& srv,
                const std::string& event,
                const std::string& origin,
                const ParamList& args)
{
	if(filter != NULL)
		return filter->DoesHandle(srv, event, origin, args);
	else
		return true;
}
