#ifndef UDSSERVER_H
#define UDSSERVER_H

#include "configs.h"
#include "eventinterface.h"
#include "selectinginterface.h"
#include <string>
#include <vector>
#include <sys/socket.h>
#include <sys/types.h>
#include <linux/un.h>

class Server;
class EventManager;

class UdsServer : public EventHandler, public SelectingInterface
{
private:
	int srvSock;
	std::vector<int> clients;
	struct sockaddr_un addr;
	Server* srv;

public:
	UdsServer(EventManager* man, const Configs& configs);
	~UdsServer();

	void AddSelectDescriptors(fd_set& inFD,
		fd_set& outFD,
		fd_set& excFD,
		int& maxFD);
	void SelectDescriptors(fd_set& inFD, fd_set& outFD, fd_set& excFD);

	EventType GetType();

	void OnEvent(Server& srv,
				const std::string& event,
				const std::string& origin,
				const std::vector<std::string>& params);

private:
	void ParseMessage(int fd, std::string msg);
	void SendReply(int fd, const std::string& msg);
};

#endif
