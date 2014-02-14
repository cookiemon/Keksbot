#include "eventmanager.h"
#include "exceptions.h"
#include "logging.h"
#include "simpleevent.h"
#include <errno.h>
#include <string.h>

EventManager::EventManager(const std::string& cfgfile)
{
	Configs cfgs(cfgfile);
	SectionSettings::const_iterator serverSettings = cfgs.GetSettings().find("server");
	if(serverSettings == cfgs.GetSettings().end())
		throw ConfigException("Could not find any servers.");
	for(SubsectionSettings::const_iterator it = serverSettings->second.begin();
		it != serverSettings->second.end();
		++it)
	{
		try
		{
			Server* srv = new Server(it->first, it->second);
			srv->SetManager(this);
			serverlist.push_back(srv);
		}
		catch(ConfigException& e)
		{
			Log(LOG_ERR, "Config section server/%s malformed", it->first.c_str());
		}
	}

	SectionSettings::const_iterator handlerSettings = cfgs.GetSettings().find("handler");
	if(handlerSettings == cfgs.GetSettings().end())
		Log(LOG_WARNING, "Could not find any handlers in config file");
	else
	{
		for(SubsectionSettings::const_iterator it = handlerSettings->second.begin();
		    it != handlerSettings->second.end();
			++it)
		{
			try
			{
				KeyValueMap::const_iterator valIt = it->second.find("type");
				if(valIt != it->second.end())
				{
					if(valIt->second == "simple")
					{
						valIt = it->second.find("alias");
						if(valIt == it->second.end() || valIt->second.empty())
							throw ConfigException("alias missing");
						std::string alias = valIt->second;
						valIt = it->second.find("reply");
						if(valIt == it->second.end())
							throw ConfigException("reply missing");
						std::string reply = valIt->second;
						EventHandler* newHandler = new SimpleEvent(NULL, reply);
						valIt = it->second.find("description");
						if(valIt != it->second.end())
							newHandler->SetDescription(valIt->second);
						aliasedEvents.insert(AliasedMap::value_type(alias, newHandler));
					}
					else
						throw ConfigException("handler type not known");
				}
				else
					throw(ConfigException("handler type missing"));
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

	for(size_t i = 0; i < serverlist.size(); ++i)
	{
		try
		{
			if(!serverlist[i]->IsConnected())
				serverlist[i]->Connect();
			serverlist[i]->AddSelectDescriptors(inSet, outSet, maxFd);
		}
		catch(IrcException& e)
		{
			Log(LOG_ERR, "Server \"%s\" failed to connect: [%d] %s",
				serverlist[i]->GetName().c_str(), e.ErrorNumber(), e.what());
				Log(LOG_ERR, "Hostname: %s", serverlist[i]->GetLocation().c_str());
		}
	}

	if(select(maxFd+1, &inSet, &outSet, NULL, &tv) < 0)
		Log(LOG_ERR, "Select error: [%d] %s", errno, strerror(errno));

	for(size_t i = 0; i < serverlist.size(); ++i)
	{
		try
		{
			serverlist[i]->SelectDescriptors(inSet, outSet);
		}
		catch(IrcException& e)
		{
			Log(LOG_ERR, "Failed on select descriptor: [%d] %s", e.ErrorNumber(), e.what());
		}
	}
}

void EventManager::DistributeEvent(Server& source,
                                   const std::string& event,
                                   const std::string& origin,
                                   const ParamList& params)
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
			if(it != aliasedEvents.end() && it->second->DoesHandle(source, event, origin, params))
			{
				it->second->OnEvent(source, event, origin, params);
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
