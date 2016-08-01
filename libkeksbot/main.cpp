#include <stdlib.h>
#include "common.h"
#include "exceptions.h"
#include "logging.h"
#include "eventmanager.h"
#include "server.h"
#include <errno.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

extern "C"
{
	void take_over_world(void)
	{
		exit(1);
	}
	void kill_all_humans(void)
	{
		exit(1);
	}

	KEKSBOT_API int Run(void)
	{
		struct timeval tv;
		int err;
		err = gettimeofday(&tv, NULL);
		if(err == 0)
			srand(static_cast<unsigned int>(tv.tv_sec ^ tv.tv_usec));
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
		catch(...)
		{
			Log(LOG_ERR, "Oh shit, caught something totally unknown");
			return 0;
		}
	}
}
