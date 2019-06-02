#include "eventinterface.h"
#include "date.h"
#include "exceptions.h"
#include "httprelay.h"
#include "logging.h"
#include "mensa.h"
#include "simpleevent.h"
#include "statichandlers.h"
#include "stattracker.h"
#include "stats.h"
#include "classifiedhandler.h"
#include "quizzer.h"
#include "udsserver.h"
#include "unicode.h"
#include <assert.h>
#include <iterator>

#include <random>

static std::random_device rng;

template<typename T>
T select_random(T begin, T end)
{
	if (begin == end)
		return end;

	auto max = std::distance(begin, end);
	auto dist = std::uniform_int_distribution<decltype(max)>(0, max-1);
	return std::next(begin, dist(rng));
}

std::string PopRandom(const std::string &key, std::multimap<std::string, std::string>& values)
{
	auto range = values.equal_range(key);
	auto elem = select_random(range.first, range.second);

	if (elem == range.second)
		return "";

	std::string value = elem->second;
	if (std::next(range.first) != range.second)
		values.erase(elem);
	return value;
}

void LoadAnswers(const std::string& name, std::multimap<std::string, std::string>& replacements)
{
	if (name.find('/') != std::string::npos) return;

	std::ifstream file((name + ".txt").c_str());
	std::string line;
	while (std::getline(file, line))
		replacements.insert({name, line});
}

/*!
 * Returns if \p str starts with a vowel sound. I.e. starts with any of 'aeiou' but has not a consonant-sounding first syllable
 */
bool StartsWithVowelSound(const std::string& str)
{
	if (str.empty()) return false;

	static const std::string vowels = "aeio";
	if (str[0] == 'u' && (str.size() < 2 || str[1] != 's')) return true;

	return vowels.find(str[0]) != std::string::npos;
}

/*!
 * Replace indefinite article 'A'/'a' in \p answer with 'an' if \p str starts with one of 'aeiou'.
 * \return true if 'a' has been replaced
 */
std::string& AToAn(std::string& answer, size_t start, size_t end, const std::string& str)
{
	if (start < 2)
		return answer;

	if (!StartsWithVowelSound(str))
		return answer;

	if (answer.substr(start - 2, 2) != "a " || answer.substr(start - 2, 2) != "A ")
		return answer;

	if (start > 3 && answer[start - 3] != ' ')
		return answer;

	answer.insert(start - 1, 1, 'n');
	return answer;
}

std::string EventHandler::RandomReplace(std::string answer, std::multimap<std::string, std::string> replacements) const
{
	size_t i = 0;
	while ( (i = answer.find("${", i)) < answer.size())
	{
		size_t i_end = answer.find('}', i);
		if (i_end == std::string::npos)
			break;

		std::string key = answer.substr(i + 2, i_end - i - 2);
		if (replacements.find(key) == replacements.end())
		{
			LoadAnswers(key, replacements);
			if (replacements.find(key) == replacements.end())
			{
				i += 2;
				continue;
			}
		}

		std::string r = PopRandom(key, replacements);
		AToAn(answer, i, i_end, r);
		answer.replace(i, i_end - i + 1, r);
	}
	return answer;
}

EventHandler* CreateEventHandler(const Configs& configs, EventManager* man)
{
	assert(man != NULL);

	std::string alias;
	configs.GetValue("alias", alias);

	std::string type;
	configs.GetValue("type", type);

	EventHandler* newHandler = NULL;
	if(type == "simple")
	{
		std::string reply;
		configs.GetValue("reply", reply);
		newHandler = new SimpleEvent(reply);
	}
	else if(type == "static")
	{
		std::string handler;
		configs.GetValue("handler", handler);
		Log(LOG_INFO, "Initializing %s", handler.c_str());
		if(handler == "restart")
			newHandler = new RestartHandler();
		else if(handler == "exit")
			newHandler = new ExitHandler();
		else if(handler == "help")
			newHandler = new HelpHandler(man);
		else if(handler == "stattracker")
			newHandler = new StatTracker(configs);
		else if(handler == "stats")
			newHandler = new Stats(configs);
		else if(handler == "classified")
			newHandler = new ClassifiedHandler();
		else if(handler == "uds")
		{
			UdsServer* hand = new UdsServer(man, configs);
			man->AddNetworklistener(hand);
			newHandler = hand;
		}
		else if(handler == "httprelay")
		{
			HttpRelay* hand = new HttpRelay(configs);
			man->AddNetworklistener(hand);
			newHandler = hand;
		}
		else if(handler == "mensa")
		{
			Mensa* hand = new Mensa(configs);
			man->AddNetworklistener(hand);
			newHandler = hand;
		}
		else if(handler == "unicode")
			newHandler = new Unicode(configs);
		else if(handler == "quizzer")
			newHandler = new Quizzer(configs);
		else if(handler == "date") {
			newHandler = new Date(configs);
			std::cout << "DATE!!!! \\p/\n";
			std::cout.flush();
		}
		else
			throw ConfigException("static handler type not known");
	}
	else
		throw ConfigException("event handler type not known");

	newHandler->SetAlias(alias);
	std::string desc;
	configs.GetValueOrDefault("description", desc);
	newHandler->SetDescription(desc);

	bool show;
	configs.GetValueOrDefault("show", show, true);
	newHandler->SetShown(show);

	return newHandler;
}

bool EventHandler::DoesHandle(Server& srv,
                const std::string& event,
                const std::string& origin,
                const ParamList& args)
{
	return true;
}

