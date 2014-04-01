#include "httprelay.h"
#include "configs.h"
#include "eventmanager.h"
#include "logging.h"
#include "serverinterface.h"
#include <algorithm>
#include <curl/curl.h>

HttpRelay::HttpRelay(const Configs& cfg)
{
	multiHandle = curl_multi_init();

	cfg.GetValue("cburl", cbUrl);
	std::string hooks;
	cfg.GetValueOrDefault("hooks", hooks,
		std::string("CHANNEL,JOIN,PART,QUIT,TOPIC"));
	cfg.GetValueOrDefault("server", server, std::string(""));
	size_t pos = 0;
	while(pos != std::string::npos)
	{
		size_t nextPos = hooks.find(',', pos);
		handledEvents.insert(hooks.substr(pos, nextPos - pos));
		if(nextPos == std::string::npos)
			pos = std::string::npos;
		else
			pos = nextPos + 1;
	}
}

HttpRelay::~HttpRelay()
{
	curl_multi_cleanup(multiHandle);
}

static size_t NoMemoryCallback(void* cont, size_t size, size_t nmemb, void* usrp)
{
	size_t* readBytes = reinterpret_cast<size_t*>(usrp);
	*readBytes += size * nmemb;
	return *readBytes;
}

void HttpRelay::OnEvent(ServerInterface& srv,
	const std::string& event,
	const std::string& origin,
	const std::vector<std::string>& params)
{
	typedef std::map<std::string, std::string> StrMap;
	StrMap postParams;
	postParams["user"] = origin;
	postParams["bot"] = origin == srv.GetNick() ? "1" : "0";
	if(event == "CHANNEL")
	{
		postParams.insert(StrMap::value_type("hook", "message"));
	}
	else
	{
		std::pair<StrMap::iterator, bool> it = postParams.insert( StrMap::value_type(
			"hook", event));
		std::transform(it.first->second.begin(), it.first->second.end(),
			it.first->second.begin(),
			&tolower);
	}
	if(event == "QUIT")
	{
		if(!params.empty())
			postParams["message"] = params[0];
	}
	else if(event == "TOPIC")
	{
		postParams["channel"] = params[0];
		postParams["topic"] = params[1];
	}
	else
	{
		size_t paramsSize = params.size();
		switch(paramsSize)
		{
			default:
			case 2:
				postParams["message"] = params[1];
			case 1:
				postParams["channel"] = params[0];
			case 0:
				break;
		}
	}

	CURL* handle = curl_easy_init();

	curl_easy_setopt(handle, CURLOPT_URL, cbUrl.c_str());
	curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
	size_t s = 0;
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, &NoMemoryCallback);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &s);
	curl_easy_setopt(handle, CURLOPT_USERAGENT, "keksbot/1.3-beta");
	curl_easy_setopt(handle, CURLOPT_POSTREDIR, CURL_REDIR_POST_ALL);
	curl_easy_setopt(handle, CURLOPT_POST, 1L);

	std::string encodedParams = GetUrlParamList(handle, postParams);
	curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, encodedParams.size());
	curl_easy_setopt(handle, CURLOPT_COPYPOSTFIELDS, encodedParams.c_str());

	curl_multi_add_handle(multiHandle, handle);
	Log(LOG_DEBUG, "Sent http request to %s. Post: %s",
		cbUrl.c_str(), encodedParams.c_str());
}

std::string HttpRelay::GetUrlParamList(CURL* handle,
	const std::map<std::string, std::string>& params)
{
	std::string retVal;
	for(std::map<std::string, std::string>::const_iterator it = params.begin();
		it != params.end();
		++it)
	{
		char* encoded = curl_easy_escape(handle, it->first.c_str(), 0);
		retVal += encoded;
		curl_free(encoded);
		retVal += "=";
		encoded = curl_easy_escape(handle, it->second.c_str(), 0);
		retVal += encoded;
		retVal += "&";
		curl_free(encoded);
	}
	if(!retVal.empty())
		retVal.erase(retVal.size() - 1);
	return retVal;
}

bool HttpRelay::DoesHandle(ServerInterface& srv,
	const std::string& event,
	const std::string& origin,
	const std::vector<std::string>& params)
{
	if(!server.empty() && server != srv.GetName())
		return false;
	return handledEvents.find(event) != handledEvents.end();
}

void HttpRelay::AddSelectDescriptors(fd_set& inSet,
	fd_set& outSet,
	fd_set& excSet,
	int& maxFD)
{
	int max = 0;
	curl_multi_fdset(multiHandle, &inSet, &outSet, &excSet, &max);
	maxFD = std::max(maxFD, max);
}

void HttpRelay::SelectDescriptors(fd_set& inSet, fd_set& outSet, fd_set& excSet)
{
	int running;
	curl_multi_perform(multiHandle, &running);
	struct CURLMsg* msg;
	while((msg = curl_multi_info_read(multiHandle, &running)))
	{
		CURL* handle = msg->easy_handle;
		Log(LOG_ERR, "Result of curl request: %ld", static_cast<long>(msg->data.result));
		curl_multi_remove_handle(multiHandle, handle);
		curl_easy_cleanup(handle);
	}
}
