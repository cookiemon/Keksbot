#include "date.h"
#include "configs.h"
#include "server.h"
#include <iomanip>
#include <sstream>
#include <time.h>

Date::Date(const Configs& cfg)
{
}

Date::~Date()
{
}

void Date::OnEvent(Server& srv,
	const std::string& event,
	const std::string& origin,
	const std::vector<std::string>& params)
{
	auto now = time(NULL);
	struct tm localnow;
	localtime_r(&now, &localnow);
	std::stringstream sstr;
	sstr << std::put_time(&localnow, "%Y-%m-%dT%H:%M:%S%Z");
	srv.SendMsg(params[0], sstr.str());
}


