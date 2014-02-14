#ifndef STATICHANDLERS_H
#define STATICHANDLERS_H

#include "eventinterface.h"
#include "eventmanager.h"
#include "exceptions.h"
#include "logging.h"
#include "server.h"

class RestartHandler : public EventHandler
{
public:
	void OnEvent(Server& srv,
	             const std::string& evt,
	             const std::string& origin,
	             const ParamList& args)
	{
		throw RestartException();
	}
};

class ExitHandler : public EventHandler
{
public:
	void OnEvent(Server& srv,
	             const std::string& evt,
	             const std::string& origin,
	             const ParamList& args)
	{
		throw ExitException();
	}
};

class HelpHandler : public EventHandler
{
private:
	EventManager* manager;
public:
	HelpHandler(EventManager* man)
		: manager(man)
	{
	}

	void OnEvent(Server& srv,
	             const std::string& evt,
	             const std::string& origin,
	             const ParamList& args)
	{
		if(manager == NULL)
		{
			Log(LOG_ERR, "Help handler has no associated manager");
			return;
		}
	}
};

#endif
