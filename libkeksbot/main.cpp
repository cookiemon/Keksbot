#include <stdlib.h>
#include "ircexception.h"
#include "logging.h"
#include "server.h"
#include <iostream>

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
