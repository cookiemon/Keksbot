#include "simpleevent.h"
#include "serverinterface.h"
#include <fstream>
#include <map>
#include <stdlib.h>

typedef std::map<std::string, std::vector<std::string> > AnswerMap;
typedef std::vector<std::string> StringList;

SimpleEvent::SimpleEvent(const std::string& answer)
	: answerString(answer)
{
}

void SimpleEvent::OnEvent(ServerInterface& srv,
                          const std::string& event,
                          const std::string& origin,
                          const std::vector<std::string>& params)
{
	if(params.size() < 2)
		return;

	std::string answer = answerString;
	AnswerMap choices;
	choices.insert(AnswerMap::value_type("USER", StringList(1, origin)));
	choices.insert(AnswerMap::value_type("CHAN", StringList(1, params[0])));
	const std::string& msg = params[params.size() - 1];
	choices.insert(AnswerMap::value_type("MSG", StringList(1, msg)));
	choices.insert(AnswerMap::value_type("NICK", StringList(1, srv.GetNick())));

	bool foundReplacement;
	do
	{
		foundReplacement = false;
		size_t nextVariable = 0;
		while((nextVariable = answer.find("${", nextVariable)) != std::string::npos)
		{
			nextVariable += 2;
			size_t variableEnd = answer.find('}', nextVariable);
			if(variableEnd != std::string::npos)
			{
				std::string variableName = answer.substr(nextVariable,
				                                        variableEnd - nextVariable);
				AnswerMap::iterator it = choices.find(variableName);
				if(it == choices.end())
				{
					StringList loadedAnswers;
					LoadAnswers(variableName, loadedAnswers);
					std::pair<AnswerMap::iterator, bool> inserted
					                        = choices.insert(AnswerMap::value_type(variableName,
					                                         loadedAnswers));
					it = inserted.first;
				}
				answer.replace(nextVariable - 2,
				               variableEnd - nextVariable + 3,
							   GetRandomString(it->second)); 
				foundReplacement = true;
			}
		}
	} while(foundReplacement);
	
	if(answer.substr(0, 4) == "/me ")
		srv.SendAction(params[0], answer.substr(4));
	else
		srv.SendMsg(params[0], answer);
}

void SimpleEvent::LoadAnswers(const std::string& name, std::vector<std::string>& out)
{
	std::ifstream file((name + ".txt").c_str());
	std::string line;
	while(std::getline(file, line))
		out.push_back(line);
	if(out.size() == 0)
		out.push_back("${" + name + "}");
}

std::string SimpleEvent::GetRandomString(std::vector<std::string>& list)
{
	if(list.size() > 1)
	{
		int randNr = rand() % list.size();
		std::vector<std::string>::iterator it = list.begin();
		std::advance(it, randNr);
		std::string retVal = *it;
		list.erase(it);
		return retVal;
	}
	else
	{
		return list[0];
	}
}
