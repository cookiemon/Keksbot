#include <stdlib.h>
#include "server.h"
#include "eventmanager.h"
#include "exceptions.h"
#include "logging.h"
#include "stringhelpers.h"
#include <assert.h>
#include <map>
#include <sstream>
#include <stdexcept>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <libircclient.h>
#include <libirc_rfcnumeric.h>

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

Server::Server(const Configs& settings, EventManager* man)
	: name(settings.GetName()),
	manager(man)
{
	assert(manager != NULL);

	settings.GetValue("location", srv);
	settings.GetValueOrDefault("ipv6", ipv6);

	port = srv[0] == '#' ? 6697 : 6667;
	settings.GetValueOrDefault("port", port, port);

	settings.GetValueOrDefault("nick", nick, std::string("nobody"));
	settings.GetValueOrDefault("username", username, std::string("toor"));
	settings.GetValueOrDefault("realname", realname, std::string("nobody"));
	settings.GetValueOrDefault("password", passwd, std::string());
	
	settings.GetValueOrDefault("prefix", prefix, std::string("-"));

	std::string chanLine;
	settings.GetValueOrDefault("channels", chanLine, std::string());
	if(!chanLine.empty())
	{
		std::string chan;
		std::istringstream sstr(chanLine);
		while(std::getline(sstr, chan, ','))
			channels.insert(ChannelListType::value_type(chan, Channel()));
	}

	std::string ignoreLine;
	settings.GetValueOrDefault("ignore", ignoreLine, std::string());
	if(!ignoreLine.empty())
	{
		std::string nick;
		std::istringstream sstr(ignoreLine);
		while(std::getline(sstr, nick, ','))
			ignored.insert(User(nick));
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
	callbacks.event_nick     = &event_misc;
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

void Server::Connect(void)
{
	const char* password = passwd.empty() ? NULL : passwd.c_str();
	int error;
	if(ipv6)
		error = irc_connect6(session, srv.c_str(), port, password,
			nick.c_str(), username.c_str(), realname.c_str());
	else
		error = irc_connect(session, srv.c_str(), port, password,
			nick.c_str(), username.c_str(), realname.c_str());
	if(error != 0)
		throw IrcException(irc_errno(session));
}

void Server::Disconnect(void)
{
	irc_disconnect(session);
}

void Server::AddSelectDescriptors(fd_set& inSet,
	fd_set& outSet,
	fd_set& excSet,
	int& maxFd)
{
	if(!IsConnected())
	{
		try
		{
			Connect();
		}
		catch(IrcException& e)
		{
			Log(LOG_ERR, "Server \"%s\" failed to connect: [%d] %s",
				GetName().c_str(), e.ErrorNumber(), e.what());
		}
	}
	int max = 0;
	int error = irc_add_select_descriptors(session, &inSet, &outSet, &max);
	maxFd = std::max(maxFd, max);
	if(error != 0)
		Disconnect();
}

void Server::SelectDescriptors(fd_set& inSet, fd_set& outSet, fd_set& excSet)
{
	int error = irc_process_select_descriptors(session, &inSet, &outSet);
	if(error != 0)
		throw IrcException(irc_errno(session));
}

void Server::EventConnect(const std::string& evt, const std::string& origin, const ParamList& args)
{
	Log(LOG_INFO, "Connected to server: %s", origin.c_str());
	for(ChannelListType::iterator it = channels.begin(); it != channels.end(); ++it)
		Join(it->first);
}

void Server::EventNumeric(unsigned int       evt, const std::string& origin, const ParamList& args)
{
	char evtString[20];
	snprintf(evtString, sizeof(evtString), "%u", evt);

	switch(evt)
	{
		case LIBIRC_RFC_RPL_NAMREPLY:
		{
			if(args.size() < 4)
			{
				Log(LOG_ERR, "Libirc gave less than 4 parameters in RPL_NAMREPLY");
				break;
			}
			std::string nameList = args[3];
			std::string nick = CutFirstWord(nameList);
			Channel& chan = channels[args[2]];
			while(!nick.empty())
			{
				chan.users.insert(User(nick));
				nick = CutFirstWord(nameList);
			}
			return;
		}
		case LIBIRC_RFC_RPL_NOTOPIC:
		case LIBIRC_RFC_RPL_TOPIC:
			if(args.size() < 3)
			{
				Log(LOG_ERR, "Libirc gave less than 3 parameters in RPL_NOTOPIC");
				break;
			}
			channels[args[1]].topic = args[2];
			break;
	}

	if(manager != NULL)
		manager->DistributeEvent(*this, evtString, origin, args);
}

void Server::EventMisc (const std::string& evt, const std::string& origin, const ParamList& args)
{
	if(evt == "JOIN" && args.size() > 0)
	{
		channels[args[0]].users.insert(User(origin));
		if(origin == GetNick())
			irc_cmd_topic(session, args[0].c_str(), NULL);
	}
	else if(evt == "TOPIC" && args.size() > 0)
	{
		std::string topic;
		if(args.size() > 1)
			topic = args[1];
		channels[args[0]].topic = topic;
	}
	else if(evt == "NICK" && args.size() > 0)
	{
		for(ChannelListType::iterator it = channels.begin();
			it != channels.end();
			++it)
		{
			User usr(origin);
			User newUsr(args[0]);
			if(it->second.users.erase(usr) > 0)
				it->second.users.insert(newUsr);
		}
	}
	else if(evt == "QUIT")
	{
		User usr(origin);
		for(ChannelListType::iterator it = channels.begin();
			it != channels.end();
			++it)
		{
			it->second.users.erase(usr);
		}
	}
	else if(evt == "PART" && args.size() > 0)
	{
		User usr(origin);
		ChannelListType::iterator it = channels.find(args[0]);
		if(it != channels.end())
			it->second.users.erase(usr);
	}
	else if((evt == "CHANNEL" || evt == "ACTION")
		&& args[0][0] == '#' && args.size() > 1)
	{
		Channel& chan = channels[args[0]];
		chan.last.usr = User(origin);
		chan.last.msg = args[1];
		chan.last.date = time(NULL);
	}

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

void Server::SendSingleLineMsg(const std::string& chan, const std::string& msg)
{
	int error = 0;
	if(msg.substr(0, 4) == "/me ")
	{
		error = irc_cmd_me(session, chan.c_str(), msg.c_str() + 4);
	}
	else
	{
		error = irc_cmd_msg(session, chan.c_str(), msg.c_str());
	}
	if(error != 0)
		throw IrcException(irc_errno(session));
}

void Server::SendMsg(const std::string& chan, const std::string& msg)
{
	size_t idx = 0;
	size_t end = 0;
	do
	{
		end = msg.find("\\n", idx);
		std::string substr = msg.substr(idx, end - idx);
		SendSingleLineMsg(chan, substr);
		if(end != std::string::npos)
			end += 2;
	} while((idx = end) != std::string::npos);
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

std::string Server::GetPrefix(void)
{
	return prefix;
}

const UserListType& Server::GetIgnored(void)
{
	return ignored;
}

const Channel& Server::GetChannel(const std::string& chan)
{
	ChannelListType::iterator it = channels.find(chan);
	if(it == channels.end())
		throw IllegalArgumentException("Channel " + chan + " not found");
	return it->second;
}

