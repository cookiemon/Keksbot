#ifndef STATICHANDLERS_H
#define STATICHANDLERS_H

#include "eventinterface.h"
#include "eventmanager.h"
#include "exceptions.h"

class Server;

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

	EventType GetType()
	{
		return TYPE_SIMPLE;
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

	EventType GetType()
	{
		return TYPE_SIMPLE;
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
	             const ParamList& args);

	EventType GetType()
	{
		return TYPE_SIMPLE;
	}
};

#endif
