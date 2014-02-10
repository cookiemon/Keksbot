#include "server.h"
#include "ircexception.h"
#include "logging.h"
#include <map>
#include <stdexcept>
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

	it->second->EventConnect(event, origin, args);
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

Server::Server(const std::string& srv, unsigned short port,
	const std::string& passwd,
	const std::string& nick,
	const std::string& username,
	const std::string& realname)
	: srv(srv),
	port(port),
	passwd(passwd),
	nick(nick),
	username(username),
	realname(realname)
{
	memset(&callbacks, 0, sizeof(callbacks));
	callbacks.event_connect = &event_connect;
	callbacks.event_numeric = &event_numeric;

	session = irc_create_session(&callbacks);

	if(session == NULL)
		throw std::runtime_error("Could not create irc session");

	irc_option_set(session, LIBIRC_OPTION_STRIPNICKS);
	irc_option_set(session, LIBIRC_OPTION_SSL_NO_VERIFY);

	InsertedIt inserted = serverSessionMap.insert(ServerSessionMapType::value_type(session, this));
	if(!inserted.second)
		Log(LOG_WARNING, "Session not unique while connecting to server: %s", srv.c_str());
}

Server::~Server(void)
{
	irc_destroy_session(session);
	serverSessionMap.erase(session);
}

void Server::Connect(void)
{
	int error = irc_connect(session, srv.c_str(), port, passwd.c_str(),
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
}

void Server::EventNumeric(unsigned int       evt, const std::string& origin, const ParamList& args)
{
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
