#include "simpleevent.h"
#include "server.h"
#include <fstream>
#include <map>
#include <stdlib.h>

typedef std::map<std::string, std::vector<std::string> > AnswerMap;
typedef std::vector<std::string> StringList;

SimpleEvent::SimpleEvent(const std::string& answer)
	: answerString(answer)
{
}

void SimpleEvent::OnEvent(Server& srv,
                          const std::string& event,
                          const std::string& origin,
                          const std::vector<std::string>& params)
{
	if(params.size() < 2)
		return;
	auto additionalVariables = std::multimap<std::string, std::string>{
		{"USER", origin},
		{"CHAN", params.front()},
		{"MSG", params.back()},
		{"NICK", srv.GetNick()},
	};
	std::string answer = answerString;

	srv.SendMsg(params[0], RandomReplace(answerString, additionalVariables));
}

