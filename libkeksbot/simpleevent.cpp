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
			size_t variableName_start = nextVariable + 2;
			size_t variableEnd = answer.find('}', variableName_start);
			if(variableEnd != std::string::npos)
			{
				std::string variableName = answer.substr(variableName_start,
				                                         variableEnd - variableName_start);
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

				std::string rngString = GetRandomString(it->second);
				if(AToAn(answer, nextVariable, variableEnd+1, rngString))
				{
					nextVariable += 1;
					variableEnd += 1;
				}

				answer.replace(nextVariable, variableEnd - nextVariable + 1, rngString);

				foundReplacement = true;
				nextVariable += 2;
			}
		}
	} while(foundReplacement);

	if(answer.substr(0, 4) == "/me ")
		srv.SendAction(params[0], answer.substr(4));
	else
		srv.SendMsg(params[0], answer);
}

/*!
 * Replace indefinite article 'A'/'a' in \p answer with 'an' if \p str starts with one of 'aeiou'.
 * \return true if 'a' has been replaced
 */
bool SimpleEvent::AToAn(std::string& answer, const size_t start, const size_t end, const std::string& str) const
{
	// if article is "a" but replacement starts with "aeiou" change "A/a ${key}" to "An/an ${key}"
	bool indef = false;

	if(StartsWithVowelSound(str))
	{
		if(start > 3)
		{
			// only possibility is "x a ${key}" or "x A ${key}" where x is any char
			std::string indefArticle = answer.substr(start - 3, 3);
			if(indefArticle == " a " || indefArticle == " A ")
			{
				indef = true;
			}
		}
		else if(start == 2 && answer[0] == 'A' && answer[1] == ' ')
		{
			// edge-case "A ${key}"
			indef = true;
		}

		if(indef)
		{
			answer.insert(start - 1, 1, 'n');
		}
	}

	return indef;
}

/*!
 * Returns if \p str starts with a vowel sound. I.e. starts with any of 'aeiou' but has not a consonant-sounding first syllable
 */
bool SimpleEvent::StartsWithVowelSound(const std::string& str) const
{
	static const std::string vowels = "aeio"; // u is not always a vowel
	if(str[0] == 'u' && str[1] != 's') return true;
	return vowels.find(str[0]) != std::string::npos;
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
		size_t randNr = static_cast<size_t>(rand()) % list.size();
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
