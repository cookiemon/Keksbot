#include <stdlib.h>
#include "exceptions.h"
#include "logging.h"
#include "eventmanager.h"
#include <errno.h>
#include <iostream>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

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
			EventManager evtMan("keksbot.cfg");
			while(true)
				evtMan.DoSelect();
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
		catch(std::exception& e)
		{
			Log(LOG_ERR, "Caught unknown exception: %s", e.what());
			return 0;
		}
	}
}
