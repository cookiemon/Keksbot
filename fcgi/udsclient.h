#ifndef UDSCLIENT_H
#define UDSCLIENT_H

#include "exceptions.h"
#include <errno.h>
#include <stdexcept>
#include <string>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/un.h>

class UdsClient
{
private:
	struct sockaddr_un addr;
	int fd;
public:
	UdsClient(const std::string& sockFile)
	{
		addr.sun_family = AF_UNIX;
		strncpy(addr.sun_path, sockFile.c_str(), sizeof(addr.sun_path));
		if(addr.sun_path[sizeof(addr.sun_path)-1] != '\0')
			throw std::runtime_error("Path for unix domain socket too long");

		fd = socket(AF_UNIX, SOCK_STREAM, 0);
		if(fd == -1)
			throw SystemException(errno);

		int res = connect(fd,
			reinterpret_cast<struct sockaddr*>(&addr),
			sizeof(struct sockaddr_un));
		if(res == -1)
			throw SystemException(errno);
	}

	void Send(const std::string& msg)
	{
		size_t num = 0;
		errno = 0;
		do
		{
			ssize_t sent = write(fd, msg.c_str() + num, msg.size() - num);
			if(sent > -1)
				num += static_cast<size_t>(sent);
		} while(num < msg.size()
				&& (errno == 0 || errno == EAGAIN || errno == EWOULDBLOCK));

		if(num < msg.size())
			throw SystemException(errno);
	}

	std::string Read(void)
	{
		std::string str;
		char buf[512];
		ssize_t num;
		while((num = read(fd, buf, 512)))
			str += std::string(buf, buf + num);

		return str;
	}
};

#endif
