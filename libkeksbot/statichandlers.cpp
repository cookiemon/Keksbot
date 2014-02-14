#include "statichandlers.h"
#include "eventinterface.h"
#include "eventmanager.h"
#include "exceptions.h"
#include "logging.h"
#include "server.h"

void HelpHandler::OnEvent(Server& srv,
             const std::string& evt,
             const std::string& origin,
             const ParamList& args)
{
	if(manager == NULL)
	{
		Log(LOG_ERR, "Help handler has no associated manager");
		return;
	}
	std::string searchString = args[args.size() - 1];
	std::vector<EventHandler*> events = manager->GetEvents();
	std::vector<EventHandler*>::iterator it = events.begin();
	while(it != events.end() && (*it)->GetAlias() !=searchString)
		++it;

	std::string reply;
	if(it != events.end())
	{
		reply = (*it)->GetAlias();
		reply += ": ";
		reply += (*it)->GetDescription();
	}
	else
	{
		reply = "Further topics:";
		for(it = events.begin(); it != events.end(); ++it)
			if((*it)->GetShown())
				reply += " " + (*it)->GetAlias();
	}
	srv.SendMsg(GetChannel(origin, args), reply);
}
