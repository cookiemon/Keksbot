#include <stdlib.h>
#include "server.h"
#include "eventmanager.h"
#include "exceptions.h"
#include "logging.h"
#include <assert.h>
#include <map>
#include <sstream>
#include <stdexcept>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <libircclient/libircclient.h>
#include <libircclient/libirc_rfcnumeric.h>

typedef std::map<irc_session_t*, Server*> ServerSessionMapType;
typedef std::pair<ServerSessionMapType::iterator, bool> InsertedIt;
static ServerSessionMapType serverSessionMap;

void TransformParams(const char** params, unsigned int count, ParamList& args)
{
	for(unsigned int i = 0; i < count; ++i)
		args.push_back(params[i]);
}

void event_connect(irc_session_t* session,
	const char* event,
	const char* origin,
	const char** params,
	unsigned int count)
{
	ServerSessionMapType::iterator it = serverSessionMap.find(session);
	if(it == serverSessionMap.end())
	{
		Log(LOG_ERR, "Received event \"%s\" for unregistered session", event);
		return;
	}
	
	ParamList args;
	TransformParams(params, count, args);

	if(origin == NULL)
		origin = "";

	it->second->EventConnect(event, origin, args);
}

void event_misc(irc_session_t* session,
	const char* event,
	const char* origin,
	const char** params,
	unsigned int count)
{
	ServerSessionMapType::iterator it = serverSessionMap.find(session);
	if(it == serverSessionMap.end())
	{
		Log(LOG_ERR, "Received event \"%s\" for unregistered session", event);
		return;
	}
	ParamList args;
	TransformParams(params, count, args);

	if(origin == NULL)
		origin = "";

	it->second->EventMisc(event, origin, args);
}

void event_nick(irc_session_t* session,
	const char* event,
	const char* origin,
	const char** params,
	unsigned int count)
{
	ServerSessionMapType::iterator it = serverSessionMap.find(session);
	if(it == serverSessionMap.end())
	{
		Log(LOG_ERR, "Received event \"%s\" for unregistered session", event);
		return;
	}
	ParamList args;
	TransformParams(params, count, args);

	if(origin == NULL)
		origin = "";

	if(it->second->GetNick() == origin && count > 0)
		it->second->SetNick(*params);

	it->second->EventMisc(event, origin, args);
}

void event_numeric(irc_session_t* session,
	unsigned int event,
	const char* origin,
	const char** params,
	unsigned int count)
{
	ServerSessionMapType::iterator it = serverSessionMap.find(session);
	if(it == serverSessionMap.end())
	{
		Log(LOG_ERR, "Received event %u for unregistered session", event);
		return;
	}

	ParamList args;
	TransformParams(params, count, args);

	it->second->EventNumeric(event, origin, args);
}

Server::Server(const std::string& name, const KeyValueMap& settings, EventManager* man)
	: name(name),
	manager(man)
{
	assert(manager != NULL);

	KeyValueMap::const_iterator end = settings.end();
	KeyValueMap::const_iterator it = settings.find("location");
	if(it == end)
		throw ConfigException("Server location not found");
	srv = it->second;

	port = srv[0] == '#' ? 6697 : 6667;
	it = settings.find("port");
	if(it != end)
	{
		char* end;
		long readPort = strtol(it->second.c_str(), &end, 10);
		if(end != it->second.c_str())
			port = readPort;
	}

	AddSettingOrDefault(settings, nick,     "nick", "nobody");
	AddSettingOrDefault(settings, username, "username", "toor");
	AddSettingOrDefault(settings, realname, "realname", "nobody");
	AddSettingOrDefault(settings, passwd,   "password", "");

	it = settings.find("prefix");
	if(it == end || it->second.empty())
		prefix = '-';
	else
		prefix = it->second[0];

	it = settings.find("channels");
	if(it != end)
	{
		std::string chan;
		std::istringstream sstr(it->second);
		while(std::getline(sstr, chan, ','))
			channels.push_back(chan);
	}

	it = settings.find("ignore");
	if(it != end)
	{
		std::string nick;
		std::istringstream sstr(it->second);
		while(std::getline(sstr, nick, ','))
			ignored.push_back(nick);
	}

	Init();
}

Server::~Server(void)
{
	irc_destroy_session(session);
	serverSessionMap.erase(session);
}

void Server::Init(void)
{
	memset(&callbacks, 0, sizeof(callbacks));
	callbacks.event_connect  = &event_connect;
	callbacks.event_numeric  = &event_numeric;
	callbacks.event_nick     = &event_nick;
	callbacks.event_quit     = &event_misc;
	callbacks.event_join     = &event_misc;
	callbacks.event_part     = &event_misc;
	callbacks.event_mode     = &event_misc;
	callbacks.event_umode    = &event_misc;
	callbacks.event_topic    = &event_misc;
	callbacks.event_kick     = &event_misc;
	callbacks.event_channel  = &event_misc;
	callbacks.event_privmsg  = &event_misc;
	callbacks.event_notice   = &event_misc;
	callbacks.event_channel_notice = &event_misc;
	callbacks.event_invite   = &event_misc;
	callbacks.event_ctcp_req = &event_misc;
	callbacks.event_ctcp_rep = &event_misc;
	callbacks.event_ctcp_action = &event_misc;
	callbacks.event_unknown  = &event_misc;
	//callbacks.event_dcc_chat_req = &event_misc;
	//callbacks.event_dcc_send_req = &event_misc;

	session = irc_create_session(&callbacks);

	if(session == NULL)
		throw std::runtime_error("Could not create irc session");

	irc_option_set(session, LIBIRC_OPTION_STRIPNICKS);
	irc_option_set(session, LIBIRC_OPTION_SSL_NO_VERIFY);

	InsertedIt inserted = serverSessionMap.insert(ServerSessionMapType::value_type(session, this));
	if(!inserted.second)
		Log(LOG_WARNING, "Session not unique while connecting to server: %s", srv.c_str());
}

void Server::AddSettingOrDefault(const KeyValueMap& settings,
	std::string& attribute,
	const std::string& key,
	const std::string& deflt)
{
	KeyValueMap::const_iterator it = settings.find(key);
	if(it == settings.end())
		attribute = deflt;
	else
		attribute = it->second;
}

void Server::Connect(void)
{
	const char* password = passwd.empty() ? NULL : passwd.c_str();
	int error = irc_connect(session, srv.c_str(), port, password,
		nick.c_str(), username.c_str(), realname.c_str());
	if(error != 0)
		throw IrcException(irc_errno(session));
}

void Server::AddSelectDescriptors(fd_set& inSet, fd_set& outSet, int& maxFd)
{
	int error = irc_add_select_descriptors(session, &inSet, &outSet, &maxFd);
	if(error != 0)
		throw IrcException(irc_errno(session));
}

void Server::SelectDescriptors(fd_set& inSet, fd_set& outSet)
{
	int error = irc_process_select_descriptors(session, &inSet, &outSet);
	if(error != 0)
		throw IrcException(irc_errno(session));
}

void Server::EventConnect(const std::string& evt, const std::string& origin, const ParamList& args)
{
	Log(LOG_INFO, "Connected to server: %s", origin.c_str());
	for(ChannelListType::iterator it = channels.begin(); it != channels.end(); ++it)
		Join(*it);
}

void Server::EventNumeric(unsigned int       evt, const std::string& origin, const ParamList& args)
{
	char evtString[20];
	snprintf(evtString, sizeof(evtString), "%d", evt);
	LogIrcEvent(evtString, origin, args);
	if(manager != NULL)
		manager->DistributeEvent(*this, evtString, origin, args);
}

void Server::EventMisc   (const std::string& evt, const std::string& origin, const ParamList& args)
{
	LogIrcEvent(evt, origin, args);
	if(manager != NULL)
		manager->DistributeEvent(*this, evt, origin, args);
}

bool Server::IsConnected(void)
{
	return irc_is_connected(session);
}

void Server::Join(const std::string& chan, const std::string& pw)
{
	int error = irc_cmd_join(session, chan.c_str(), pw.c_str());
	if(error != 0)
		throw IrcException(irc_errno(session));
}

void Server::SendMsg(const std::string& chan, const std::string& msg)
{
	int error = irc_cmd_msg(session, chan.c_str(), msg.c_str());
	if(error != 0)
		throw IrcException(irc_errno(session));
}

void Server::SendAction(const std::string& chan, const std::string& msg)
{
	int error = irc_cmd_me(session, chan.c_str(), msg.c_str());
	if(error != 0)
		throw IrcException(irc_errno(session));
}

std::string Server::GetName(void)
{
	return name;
}

std::string Server::GetLocation(void)
{
	return srv;
}

unsigned short Server::GetPort(void)
{
	return port;
}

std::string Server::GetNick(void)
{
	return nick;
}

void Server::SetNick(const std::string& newNick)
{
	nick = newNick;
}

std::string Server::GetUsername(void)
{
	return username;
}

std::string Server::GetRealname(void)
{
	return realname;
}

char Server::GetPrefix(void)
{
	return prefix;
}

const Server::NickListType& Server::GetIgnored(void)
{
	return ignored;
}

void Server::LogIrcEvent(const std::string& evt, const std::string& origin, const ParamList& args)
{
	std::string logmsg("Received irc event: ");
	logmsg += evt;
	logmsg += " origin: ";
	logmsg += origin;
	logmsg += " args: ";
	for(ParamList::const_iterator it = args.begin(); it != args.end(); ++it)
	{
		logmsg += (*it);
		logmsg += " | ";
	}
	Log(LOG_DEBUG, "[%s] %s", GetName().c_str(), logmsg.c_str());
}
