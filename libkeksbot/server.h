#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <vector>
#include <libircclient/libircclient.h>

typedef std::vector<std::string> ParamList;

class Server
{
private:
	irc_callbacks_t callbacks;
	irc_session_t* session;

	std::string srv;
	unsigned short port;
	std::string passwd;
	std::string nick;
	std::string username;
	std::string realname;
public:
	Server(const std::string& srv, unsigned short port,
		const std::string& passwd,
		const std::string& nick,
		const std::string& username,
		const std::string& realname);

	~Server(void);

	void Connect();
	void AddSelectDescriptors(fd_set& inSet, fd_set& outSet, int& maxFd);
	void SelectDescriptors(fd_set& inSet, fd_set& outSet);
	void EventConnect(const std::string& evt, const std::string& origin, const ParamList& args);
	void EventNumeric(unsigned int       evt, const std::string& origin, const ParamList& args);

	bool IsConnected(void);
	void Join(const std::string& chan, const std::string& pw = "");
};

#endif
