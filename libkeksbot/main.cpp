#include <stdlib.h>
#include "ircexception.h"
#include "logging.h"
#include "server.h"
#include <errno.h>
#include <iostream>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

class RestartException : public std::exception
{
};
class ExitException : public std::exception
{
};

void parseCmdLine(Server& srv, std::string line)
{
	if(line.compare(0, 7, "restart") == 0)
	{
		throw RestartException();
	}
	else if(line.compare(0, 4, "join") == 0)
	{
		srv.Join(line.substr(5));
	}
	else if(line.compare(0, 2, "me") == 0)
	{
		srv.SendAction("#tierwiese", line.substr(3));
	}
	else
	{
		srv.SendMsg("#tierwiese", line);
	}
}

extern "C"
{
	int Run(void)
	{
		try
		{
			Server dummy("#irc.freenode.net", 6697, "",
				"Keksbot", "Keksbot", "Keksbot");
			dummy.Connect();
			while(true)
			{
				struct timeval tv;
				tv.tv_usec = 250000;
				fd_set inSet;
				fd_set outSet;
				int maxFd = 0;
				FD_ZERO(&inSet);
				FD_ZERO(&outSet);
				FD_SET(STDIN_FILENO, &inSet);

				dummy.AddSelectDescriptors(inSet, outSet, maxFd);
				if(select(maxFd+1, &inSet, &outSet, NULL, &tv) < 0)
				{
					Log(LOG_ERR, "Select error: [%d] %s", errno, strerror(errno));
				}
				if(FD_ISSET(STDIN_FILENO, &inSet))
				{
					ssize_t numread;
					char buf[256];
					numread = read(STDIN_FILENO, buf, 256);
					if(numread > 0)
					{
						parseCmdLine(dummy, buf);
					}
				}
				dummy.SelectDescriptors(inSet, outSet);
			}
			return 1;
		}
		catch(IrcException& e)
		{
			Log(LOG_ERR, "Caught irc exception: [%d] %s", e.ErrorNumber(), e.what());
			return 0;
		}
		catch(RestartException& e)
		{
			return 1;
		}
		catch(ExitException& e)
		{
			return 0;
		}
	}
}
