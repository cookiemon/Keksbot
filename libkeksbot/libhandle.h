#ifndef LIBHANDLE_H
#define LIBHANDLE_H

#include <curl/curl.h>

class LibCurlHandle
{
private:
	struct HandleCounter
	{
		size_t counter;
	};
	static HandleCounter* cnt;
public:
	LibCurlHandle()
	{
		if(cnt == NULL)
		{
			cnt = new HandleCounter();
			cnt->counter = 1;
			curl_global_init(CURL_GLOBAL_ALL);
		}
		else
		{
			cnt->counter += 1;
		}
	}
	~LibCurlHandle()
	{
		cnt->counter -= 1;
		if(cnt->counter == 0)
		{
			curl_global_cleanup();
			delete cnt;
			cnt = NULL;
		}
	}
};

#endif
