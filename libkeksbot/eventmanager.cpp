#include "eventmanager.h"
#include "exceptions.h"
#include "logging.h"
#include "server.h"
#include "simpleevent.h"
#include "stringhelpers.h"
#include <algorithm>
#include <cctype>
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
				std::string keyword;

				switch(newHandler->GetType())
				{
				case TYPE_SIMPLE:
					keyword = MakeCaseInsensitiveKeyword(newHandler->GetAlias());
					aliasedEvents.insert(AliasedMap::value_type(keyword, newHandler));
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
	fd_set excSet;
	int maxFd = 0;
	FD_ZERO(&inSet);
	FD_ZERO(&outSet);
	FD_ZERO(&excSet);

	ExecuteDeletions();

	for(size_t i = 0; i < networklisteners.size(); ++i)
	{
		try
		{
			networklisteners[i]->AddSelectDescriptors(inSet, outSet, excSet, maxFd);
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

	if(select(maxFd+1, &inSet, &outSet, &excSet, &tv) < 0)
		Log(LOG_ERR, "Select error: [%d] %s", errno, strerror(errno));

	for(size_t i = 0; i < networklisteners.size(); ++i)
	{
		try
		{
			networklisteners[i]->SelectDescriptors(inSet, outSet, excSet);
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

void EventManager::DistributeSimpleEvent(Server& source,
                                         const std::string& event,
                                         const std::string& origin,
                                         const ParamList& params)
{
	const std::string& message = *params.rbegin();
	const std::string prefix = source.GetPrefix();
	if(message.compare(0, prefix.size(), prefix) == 0)
	{
		// Find alias
		size_t aliasEnd = message.find_first_of(" \r\t\n", prefix.size());
		aliasEnd = std::min(message.size(), aliasEnd);
		std::string keyword = message.substr(prefix.size(), aliasEnd - prefix.size());
		keyword = MakeCaseInsensitiveKeyword(keyword);

		// Strip alias from parameters
		ParamList strippedParams(params.begin(), --params.end());
		std::string realMsg = message.substr(aliasEnd);
		Trim(realMsg);
		strippedParams.push_back(realMsg);

		AliasedMap::iterator it = aliasedEvents.find(keyword);
		if(it != aliasedEvents.end()
			&& it->second->DoesHandle(source,event, origin, strippedParams))
		{
			try
			{
				it->second->OnEvent(source, event, origin, strippedParams);
			}
			catch(const NumericErrorException& e)
			{
				Log(LOG_ERR, "Error on event %s: [%d] %s",
					it->second->GetAlias().c_str(),
					e.ErrorNumber(),
					e.what());
			}
			catch(const std::exception& e)
			{
				Log(LOG_ERR, "Error on event %s: %s",
					it->second->GetAlias().c_str(),
					e.what());
			}
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
			DistributeSimpleEvent(source, event, origin, params);
	}

	for(std::vector<EventHandler*>::iterator it = miscEvents.begin();
	    it != miscEvents.end();
		++it)
	{
		try
		{
			if((*it)->DoesHandle(source, event, origin, params))
				(*it)->OnEvent(source, event, origin, params);
		}
		catch(const NumericErrorException& e)
		{
			Log(LOG_ERR, "Error on event %s: [%d] %s",
				(*it)->GetAlias().c_str(),
				e.ErrorNumber(), e.what());
		}
		catch(const std::exception& e)
		{
			Log(LOG_ERR, "Error on event %s: %s",
				(*it)->GetAlias().c_str(),
				e.what());
		}
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

void EventManager::AddEvent(EventHandler* evt)
{
	if(evt->GetType() == TYPE_SIMPLE)
	{
		std::string keyword = MakeCaseInsensitiveKeyword(evt->GetAlias());
		aliasedEvents.insert(AliasedMap::value_type(keyword, evt));
	}
	else
	{
		miscEvents.push_back(evt);
	}
}

void EventManager::DelEvent(EventHandler* evt)
{
	markedForDelete.push_back(evt);
}

void EventManager::ExecuteDeletions(void)
{
	std::vector<EventHandler*>::const_iterator end = markedForDelete.end();
	std::vector<EventHandler*>::reverse_iterator clr = miscEvents.rbegin();
	for(std::vector<EventHandler*>::const_iterator it = markedForDelete.begin();
		it != end;
		++it)
	{
		if((*it)->GetType() == TYPE_SIMPLE)
		{
			aliasedEvents.erase((*it)->GetAlias());
		}
		else
		{
			std::vector<EventHandler*>::iterator elem;
			elem = std::find(miscEvents.begin(), miscEvents.end(), *it);
			if(elem != miscEvents.end())
			{
				std::swap(*elem, *clr);
				++clr;
			}
		}
		delete *it;
	}
	markedForDelete.clear();
	miscEvents.erase(clr.base(), miscEvents.end());
}

Server* EventManager::GetServer(const std::string& name)
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

std::string EventManager::MakeCaseInsensitiveKeyword(const std::string& keyword)
{
	std::string lowerKeyword(keyword.length(), ' ');

	// cast is required because std::tolower is overloaded
	std::transform(keyword.begin(), keyword.end(), lowerKeyword.begin(),
		static_cast<int(*)(int)>(std::tolower));

	return lowerKeyword;
}
