#include <stdlib.h>
#include "ircexception.h"
#include "logging.h"
#include "server.h"
#include <errno.h>
#include <iostream>
#include <string.h>
#include <sys/time.h>

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

				dummy.AddSelectDescriptors(inSet, outSet, maxFd);
				if(select(maxFd+1, &inSet, &outSet, NULL, &tv) < 0)
				{
					Log(LOG_ERR, "Select error: [%d] %s", errno, strerror(errno));
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
	}
}
