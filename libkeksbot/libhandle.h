#ifndef LIBHANDLE_H
#define LIBHANDLE_H

#include <assert.h>
#include <curl/curl.h>
#include <limits>

class LibCurlHandle
{
private:
	typedef unsigned int cnttype;
	static cnttype cnt;
public:
	LibCurlHandle()
	{
		assert(cnt != std::numeric_limits<cnttype>::max());
		if(cnt == 0)
			curl_global_init(CURL_GLOBAL_DEFAULT);
		cnt += 1;
	}
	~LibCurlHandle()
	{
		assert(cnt != 0);
		cnt -= 1;
		if(cnt == 0)
			curl_global_cleanup();
	}
};

#endif
