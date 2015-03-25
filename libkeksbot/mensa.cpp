#include "mensa.h"

#include "logging.h"
#include "server.h"
#include <curl/curl.h>
#include <rapidjson/document.h>
#include <time.h>

void RoundToDay(struct tm* time)
{
	assert(time != NULL);
	time->tm_sec = 0;
	time->tm_min = 0;
	time->tm_hour = 0;
}

Mensa::Mensa(const Configs& cfg)
	: multiHandle(curl_multi_init()),
	lastupdate(0),
	updating(false)
{
	cfg.GetValue("menuurl", menuurl);
	cfg.GetValue("metaurl", metaurl);
	cfg.GetValue("login", login);
	cfg.GetValue("canteen", canteen);
	std::string rawlines;
	cfg.GetValue("lines", rawlines);
	std::istringstream sstr(rawlines);
	std::string newline;
	while(std::getline(sstr, newline, ','))
		lines.push_back(newline);
}

Mensa::~Mensa()
{
	curl_multi_cleanup(multiHandle);
}

void Mensa::OnEvent(Server& srv,
	const std::string& event,
	const std::string& origin,
	const std::vector<std::string>& params)
{
	if(params.size() == 0)
		throw std::logic_error("OnEvent should only be called for SENDMSG");
	int offset = 0;
	if(params.size() > 1 && !params[1].empty())
	{
		std::stringstream sstr(params[1]);
		sstr >> offset;
		if(sstr.fail())
		{
			srv.SendMsg(params[0], "Error: " + params[1] + " is not a number");
			return;
		}
	}
	time_t now = time(NULL);
	// No error handling, I don't care about overflow in time_t
	if(difftime(now, lastupdate) > (3600. * 24.))
	{
		originBuf.insert(QueuedResponse(&srv, params[0], offset));
		if(!updating)
			QueryMenuUpdate();
	}
	else
		SendMenu(srv, params[0], offset);
}

void Mensa::AddSelectDescriptors(fd_set& inSet,
	fd_set& outSet,
	fd_set& excSet,
	int& maxFD)
{
	int max = 0;
	curl_multi_fdset(multiHandle, &inSet, &outSet, &excSet, &max);
	maxFD = std::max(maxFD, max);
}

bool Mensa::UpdateMeta(rapidjson::Value& val)
{
	if(!val.IsObject() || val.IsNull())
		return true;

	rapidjson::Value& canteenjson = val[canteen.c_str()];
	if(!canteenjson.IsObject() || canteenjson.IsNull())
		return true;
	
	rapidjson::Value& linesjson = canteenjson["lines"];
	if(!linesjson.IsObject() || linesjson.IsNull())
		return true;

	for(size_t i = 0; i < lines.size(); ++i)
	{
		rapidjson::Value& linestr = linesjson[lines[i].c_str()];
		if(linestr.IsString())
		{
			lineMap[lines[i]] = linestr.GetString();
			Log(LOG_DEBUG, "Updated line name for %s(%s)",
				lines[i].c_str(), linestr.GetString());
		}
		else
		{
			Log(LOG_DEBUG, "Canteen line %s not found in metadata", lines[i].c_str());
		}
	}
	return false;
}

bool Mensa::UpdateMenu(rapidjson::Value& val)
{
	if(!val.IsObject() || val.IsNull())
		return true;
	menu = val;
	return false;
}

void Mensa::SelectDescriptors(fd_set& inSet, fd_set& outSet, fd_set& excSet)
{
	int running;
	curl_multi_perform(multiHandle, &running);
	struct CURLMsg* msg;
	int msgnum;
	bool hasfinished = false;
	while((msg = curl_multi_info_read(multiHandle, &msgnum)))
	{
		hasfinished = true;
		CURL* handle = msg->easy_handle;
		Log(LOG_DEBUG, "Result of curl request: %ld", static_cast<long>(msg->data.result));
		curl_multi_remove_handle(multiHandle, handle);
		curl_easy_cleanup(handle);
	}

	if(hasfinished && !running)
	{
		bool err = false;
		doc.Parse(metabuf.c_str());
		if(doc.IsObject() && doc.HasMember("mensa"))
			err = UpdateMeta(doc["mensa"]);
		doc.Parse(menubuf.c_str());
		if(doc.IsObject() && doc.HasMember(canteen.c_str()))
			err = err || UpdateMenu(doc[canteen.c_str()]);

		if(err)
		{
			for(std::set<QueuedResponse>::iterator it = originBuf.begin();
				it != originBuf.end();
				++it)
				it->srv->SendMsg(it->channel, "Error while receiving json");
		}
		else
		{
			for(std::set<QueuedResponse>::iterator it = originBuf.begin();
				it != originBuf.end();
				++it)
				SendMenu(*it->srv, it->channel, it->offset);
		}
		originBuf.clear();
		time_t now = time(NULL);
		struct tm tm;
		localtime_r(&now, &tm);
		RoundToDay(&tm);
		now = mktime(&tm);
		lastupdate = now;
		updating = false;
	}
}

void Mensa::RegisterCurlHandle(const std::string& url, std::string& buffer)
{
	buffer = std::string();
	CURL* handle = curl_easy_init();

	curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
	curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);

	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, &Mensa::PushData);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &buffer);
	curl_easy_setopt(handle, CURLOPT_USERAGENT, "$USERAGENTWITHMOZILLAGECKOSAFARIANDSHIT");

	curl_easy_setopt(handle, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
	curl_easy_setopt(handle, CURLOPT_USERPWD, login.c_str());

	curl_multi_add_handle(multiHandle, handle);

	Log(LOG_DEBUG, "Requested url %s", url.c_str());
}

void Mensa::QueryMenuUpdate()
{
	updating = true;

	RegisterCurlHandle(menuurl, menubuf);
	RegisterCurlHandle(metaurl, metabuf);
}

size_t Mensa::PushData(char* data, size_t size, size_t nmemb, void* userdata)
{
	std::string* buffer = static_cast<std::string*>(userdata);
	if(buffer != NULL)
		buffer->append(data, size*nmemb);
	return nmemb;
}

void Mensa::SendMenu(Server& srv, const std::string& channel, int offset)
{
	time_t now = time(NULL);
	struct tm localnow;
	localtime_r(&now, &localnow);
	RoundToDay(&localnow);
	if(offset)
		localnow.tm_mday += offset;
	else if(localnow.tm_wday % 6 == 0)
		localnow.tm_mday += localnow.tm_wday?2:1;

	now = mktime(&localnow);
	localtime_r(&now, &localnow);
	std::stringstream sstr;
	sstr << now;
	std::string strnow(sstr.str());

	if(!menu.HasMember(strnow.c_str()))
	{
		std::stringstream errmsg;
		errmsg << "Keine Daten vorhanden für " << localnow.tm_mday << "." << (localnow.tm_mon + 1) << "." << (localnow.tm_year % 100);
		srv.SendMsg(channel, errmsg.str());
		return;
	}

	rapidjson::Value& menunow = menu[strnow.c_str()];
	if(!menunow.IsObject() || menunow.IsNull())
		return;

	std::stringstream menumsg;
	menumsg << "Mensaeinheitsbrei am " << localnow.tm_mday << "." << (localnow.tm_mon + 1) << "." << (localnow.tm_year % 100);
	srv.SendMsg(channel, menumsg.str());
	for(size_t i = 0; i < lines.size(); ++i)
	{
		if(menunow.IsObject() && menunow.HasMember(lines[i].c_str()))
			SendLine(srv, channel, lines[i], menunow[lines[i].c_str()]);
	}
}

void Mensa::SendLine(Server& srv, const std::string& origin, const std::string& line, rapidjson::Value& value)
{
	if(!value.IsArray())
		return;
	std::string linename = lineMap[line];
	if(linename.empty())
		linename = line;

	std::stringstream strstr;
	strstr.setf(std::ios_base::fixed);
	strstr.precision(2);
	strstr << linename << ": ";
	for(rapidjson::Value::ValueIterator it = value.Begin();
		it != value.End();
		++it)
	{
		if(!it->IsObject())
			continue;
		if(!it->HasMember("meal"))
		{
			strstr << "Geschlossen";
			if(it->HasMember("closing_text"))
			{
				rapidjson::Value& closingtext = (*it)["closing_text"];
				if(closingtext.IsString())
				{
					strstr << " (" << closingtext.GetString() << ")";
				}
			}
			strstr << ", ";
		}
		else
		{
			if(!it->HasMember("price_1"))
				continue;
			rapidjson::Value& price = (*it)["price_1"];
			if(!price.IsDouble() || price.GetDouble() < 1.)
				continue;

			rapidjson::Value& meal = (*it)["meal"];
			if(!meal.IsString())
				return;
			strstr << meal.GetString();
			if(it->HasMember("dish"))
			{
				rapidjson::Value& dish = (*it)["dish"];
				if(dish.IsString() && dish.GetString()[0] != '\0')
					strstr << " " << dish.GetString();
				strstr << " (" << price.GetDouble() << "); ";
			}
		}
	}
	std::string str = strstr.str();
	str = str.substr(0, str.size() - 2);
	srv.SendMsg(origin, str);
}
