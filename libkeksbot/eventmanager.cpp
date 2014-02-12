#include "eventmanager.h"
#include "exceptions.h"
#include "logging.h"
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
			serverlist.push_back(new Server(it->first, it->second));
		}
		catch(ConfigException& e)
		{
			Log(LOG_ERR, "Config section server/%s malformed", it->first.c_str());
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
