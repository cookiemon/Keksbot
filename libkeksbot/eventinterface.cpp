#include "eventinterface.h"
#include "exceptions.h"
#include "simpleevent.h"
#include "statichandlers.h"
#include "stattracker.h"
#include "stats.h"
#include "classifiedhandler.h"
#include "udsserver.h"
#include <assert.h>

EventHandler* CreateEventHandler(const Configs& configs, EventManager* man)
{
	assert(man != NULL);

	std::string alias;
	configs.GetValue("alias", alias);

	std::string type;
	configs.GetValue("type", type);

	EventHandler* newHandler = NULL;
	if(type == "simple")
	{
		std::string reply;
		configs.GetValue("reply", reply);
		newHandler = new SimpleEvent(reply);
	}
	else if(type == "static")
	{
		std::string handler;
		configs.GetValue("handler", handler);
		if(handler == "restart")
			newHandler = new RestartHandler();
		else if(handler == "exit")
			newHandler = new ExitHandler();
		else if(handler == "help")
			newHandler = new HelpHandler(man);
		else if(handler == "stattracker")
			newHandler = new StatTracker(configs);
		else if(handler == "stats")
			newHandler = new Stats(configs);
		else if(handler == "classified")
			newHandler = new ClassifiedHandler();
		else if(handler == "uds")
		{
			UdsServer* hand = new UdsServer(man, configs);
			man->AddNetworklistener(hand);
			newHandler = hand;
		}
		else
			throw ConfigException("static handler type not known");
	}
	else
		throw ConfigException("event handler type not known");

	newHandler->SetAlias(alias);
	std::string desc;
	configs.GetValueOrDefault("description", desc);
	newHandler->SetDescription(desc);

	bool show;
	configs.GetValueOrDefault("show", show, true);
	newHandler->SetShown(show);

	return newHandler;
}

bool EventHandler::DoesHandle(ServerInterface& srv,
                const std::string& event,
                const std::string& origin,
                const ParamList& args)
{
	return true;
}
