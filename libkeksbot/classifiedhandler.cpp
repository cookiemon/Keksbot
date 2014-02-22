#include "classifiedhandler.h"

#include "serverinterface.h"
#include <ctype.h>
#include <string>
#include <algorithm>
#include <iostream>

void ClassifiedHandler::OnEvent(ServerInterface& srv,
			const std::string& event,
			const std::string& origin,
			const std::vector<std::string>& params)
{
	if(params.size() < 2)
		return;

	std::string msg;
	const std::string& lastParam = params[params.size() - 1];

	// Pre computed rot 13 start
	const std::string prefix = "XRXFOBGZNXRFNL";
	const std::string prefixlegacy = "XVGOBGZNXRFNL";
	if(lastParam.compare(0, prefix.size(), prefix) == 0)
		msg = lastParam.substr(prefix.size());
	else if(lastParam.compare(0, prefixlegacy.size(), prefixlegacy) == 0)
		msg = lastParam.substr(prefixlegacy.size());

	if(msg.size() == 0)
		return;

	std::transform(msg.begin(), msg.end(), msg.begin(), CaesarTransform());

	msg = Base64Decode()(msg);
	if(!msg.empty())
		srv.SendMsg(params[0], msg);
}
