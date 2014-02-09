#include "dynlib.h"
#include "logging.h"
#include <stdlib.h>

enum
{
	FALSE = 0,
	TRUE  = 1
};

#ifdef __cplusplus
extern "C"
#endif
typedef int (*RunProcType)(void);

int main(int argc, char** argv)
{
	OpenLogging("Keksbot");
	Log(LOG_INFO, "Starting Keksbot.");

	int run = TRUE;
	while(run)
	{
		DynLibHandle libHandle = OpenDynLib("./libkeksbot.so");
		if(libHandle == NULL)
		{
			Log(LOG_ERR, "Could not load libkeksbot.so: %s", LastDynLibError());
			exit(-1);
		}
		Log(LOG_INFO, "libkeksbot.so successfully loaded.");

		RunProcType runProc = (RunProcType) GetDynLibProc(libHandle, "Run");
		if(runProc == NULL)
		{
			Log(LOG_ERR, "Could not load libkeksbot.so/Run: %s", LastDynLibError());
			exit(-1);
		}

		Log(LOG_INFO, "Starting libkeksbot.so");
		run = runProc();
		CloseDynLib(libHandle);
	}

	CloseLogging();
	return 0;
}
