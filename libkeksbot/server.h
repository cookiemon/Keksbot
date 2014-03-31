#ifndef SERVER_H
#define SERVER_H

#include "channel.h"
#include "configs.h"
#include "selectinginterface.h"
#include "serverinterface.h"
#include <string>
#include <vector>
#include <libircclient/libircclient.h>

typedef std::vector<std::string> ParamList;

class EventManager;

class Server : public ServerInterface, public SelectingInterface
{
private:
	irc_callbacks_t callbacks;
	irc_session_t* session;

	std::string name;
	std::string srv;
	bool ipv6;
	unsigned short port;
	std::string passwd;
	std::string nick;
	std::string username;
	std::string realname;
	std::string prefix;
	ChannelListType channels;
	UserListType ignored;

	EventManager* manager;
public:
	Server(const Configs& settings, EventManager* man);

	~Server(void);

	void Connect(void);
	void Disconnect(void);
	void AddSelectDescriptors(fd_set& inSet,
		fd_set& outSet,
		fd_set& excSet,
		int& maxFd);
	void SelectDescriptors(fd_set& inSet, fd_set& outSet, fd_set& excSet);
	void EventConnect(const std::string& evt, const std::string& origin, const ParamList& args);
	void EventNumeric(unsigned int       evt, const std::string& origin, const ParamList& args);
	void EventMisc   (const std::string& rvt, const std::string& origin, const ParamList& args);

	bool IsConnected(void);
	void Join(const std::string& chan, const std::string& pw = "");
	void SendMsg(const std::string& chan, const std::string& msg);
	void SendAction(const std::string& chan, const std::string& msg);

	std::string GetName(void);
	std::string GetLocation(void);
	unsigned short GetPort(void);
	std::string GetNick(void);
	void SetNick(const std::string& newNick);
	std::string GetUsername(void);
	std::string GetRealname(void);
	std::string GetPrefix(void);
	const UserListType& GetIgnored();

	const Channel& GetChannel(const std::string& chan);

private:
	void Init(void);
	void LogIrcEvent(const std::string& evt, const std::string& origin, const ParamList& params);
};

#endif
