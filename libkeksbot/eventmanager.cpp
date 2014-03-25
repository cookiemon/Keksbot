#include "eventmanager.h"
#include "exceptions.h"
#include "logging.h"
#include "server.h"
#include "simpleevent.h"
#include <algorithm>
#include <errno.h>
#include <string.h>

EventManager::EventManager(const std::string& cfgfile)
{
	Configs cfgs(cfgfile);

	const Configs& serverSection = cfgs.GetSubsection("server");
	SubsectionMap::const_iterator it = serverSection.FirstSubsection();
	if(it == serverSection.EndSubsection())
		throw ConfigException("Could not find any servers.");
	for(;
		it != serverSection.EndSubsection();
		++it)
	{
		try
		{
			Server* srv = new Server(it->second, this);
			serverlist.push_back(srv);
			AddNetworklistener(srv);
		}
		catch(ConfigException& e)
		{
			Log(LOG_ERR, "Config section server/%s malformed", it->first.c_str());
		}
	}

	const Configs& handlerSection = cfgs.GetSubsection("handler");
	it = handlerSection.FirstSubsection();
	if(it == handlerSection.EndSubsection())
		Log(LOG_WARNING, "Could not find any handlers in config file");
	else
	{
		for(;
		    it != handlerSection.EndSubsection();
			++it)
		{
			try
			{
				EventHandler* newHandler = CreateEventHandler(it->second, this);
				switch(newHandler->GetType())
				{
				case TYPE_SIMPLE:
					aliasedEvents.insert(AliasedMap::value_type(newHandler->GetAlias(),
					                                            newHandler));
					break;
				case TYPE_MISC:
					miscEvents.push_back(newHandler);
					break;
				default:
					delete newHandler;
					Log(LOG_ERR, "handler with unknown handler type");
					break;
				}
			}
			catch(ConfigException& e)
			{
				Log(LOG_ERR, "Config section handler/%s malformed", it->first.c_str());
			}
		}
	}
}

EventManager::~EventManager(void)
{
	for(AliasedMap::iterator it = aliasedEvents.begin(); it != aliasedEvents.end(); ++ it)
		delete it->second;
	aliasedEvents.clear();
	for(size_t i = 0; i < miscEvents.size(); ++i)
		delete miscEvents[i];
	miscEvents.clear();
	for(size_t i = 0; i < serverlist.size(); ++i)
		delete serverlist[i];
	serverlist.clear();
}

void EventManager::DoSelect(void)
{
	struct timeval tv;
	tv.tv_usec = 250000;
	fd_set inSet;
	fd_set outSet;
	int maxFd = 0;
	FD_ZERO(&inSet);
	FD_ZERO(&outSet);

	for(size_t i = 0; i < networklisteners.size(); ++i)
	{
		try
		{
			networklisteners[i]->AddSelectDescriptors(inSet, outSet, maxFd);
		}
		catch(NumericErrorException& e)
		{
			Log(LOG_ERR, "Server failed to register select descriptors: [%d] %s",
				e.ErrorNumber(), e.what());
		}
		catch(std::exception& e)
		{
			Log(LOG_ERR, "Server failed to register select descriptors: %s",
				e.what());
		}
	}

	if(select(maxFd+1, &inSet, &outSet, NULL, &tv) < 0)
		Log(LOG_ERR, "Select error: [%d] %s", errno, strerror(errno));

	for(size_t i = 0; i < networklisteners.size(); ++i)
	{
		try
		{
			networklisteners[i]->SelectDescriptors(inSet, outSet);
		}
		catch(NumericErrorException& e)
		{
			Log(LOG_ERR, "Failed on select descriptor: [%d] %s",
				e.ErrorNumber(), e.what());
		}
		catch(std::exception& e)
		{
			Log(LOG_ERR, "Failed on select descriptor: %s",
				e.what());
		}
	}
}

void EventManager::DistributeEvent(Server& source,
                                   const std::string& event,
                                   const std::string& origin,
                                   const ParamList& badParams)
{
	ParamList params = badParams;
	if(event == "PRIVMSG")
		params[0] = origin;
	else if(event == "ACTION" && params.size() > 1)
		if(!params[0].empty() && params[0][0] != '#')
			params[0] = origin;
	if(std::find(source.GetIgnored().begin(), source.GetIgnored().end(), User(origin))
		== source.GetIgnored().end())
	{
		if(event == "PRIVMSG" || event == "CHANNEL")
		{
			const std::string& message = *params.rbegin();
			if(message[0] == source.GetPrefix())
			{
				size_t aliasLen = message.find_first_of(" \r\t\n");
				if(aliasLen != std::string::npos)
					aliasLen -= 1;

				std::string keyword = message.substr(1, aliasLen);
				std::map<std::string, EventHandler*>::iterator it;
				it = aliasedEvents.find(keyword);
				if(it != aliasedEvents.end()
					&& it->second->DoesHandle(source, event, origin, params))
				{
					ParamList strippedParams(params.begin(), params.end());
					std::string realMsg;
					if(aliasLen != std::string::npos)
						aliasLen = message.find_first_not_of(" \r\t\n", aliasLen + 1);
					if(aliasLen != std::string::npos)
						realMsg = message.substr(aliasLen);
					strippedParams[strippedParams.size() - 1] = realMsg;
					it->second->OnEvent(source, event, origin, strippedParams);
				}
			}
		}
	}

	for(std::vector<EventHandler*>::iterator it = miscEvents.begin();
	    it != miscEvents.end();
		++it)
	{
		if((*it)->DoesHandle(source, event, origin, params))
			(*it)->OnEvent(source, event, origin, params);
	}
}

const std::vector<Server*>& EventManager::GetServers()
{
	return serverlist;
}

class SecondGetter
{
public:
	EventHandler* operator()(AliasedMap::value_type& param)
	{
		return param.second;
	}
};

std::vector<EventHandler*> EventManager::GetEvents()
{
	std::vector<EventHandler*> evtlist;
	for(AliasedMap::iterator it = aliasedEvents.begin();
	    it != aliasedEvents.end();
	    ++it)
		evtlist.push_back(it->second);
	evtlist.insert(evtlist.end(), miscEvents.begin(), miscEvents.end());
	return evtlist;
}

void EventManager::AddNetworklistener(SelectingInterface* listener)
{
	networklisteners.push_back(listener);
}

void EventManager::DelNetworklistener(SelectingInterface* listener)
{
	std::vector<SelectingInterface*>::iterator it = std::find(networklisteners.begin(), networklisteners.end(), listener);
	if(it == networklisteners.end())
		throw IllegalArgumentException("Deleted network listener was not in interface");

	std::swap(*it, *(networklisteners.end() - 1));
	networklisteners.pop_back();
}

ServerInterface* EventManager::GetServer(const std::string& name)
{
	for(std::vector<Server*>::iterator it = serverlist.begin();
		it != serverlist.end();
		++it)
	{
		if((*it)->GetName() == name)
			return *it;
	}
	throw IllegalArgumentException("Server " + name + " not found");
}
