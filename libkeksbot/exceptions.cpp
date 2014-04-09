#include "exceptions.h"
#include <libircclient/libircclient.h>
#include <sqlite3.h>

const char* IrcException::what() const throw()
{
	return irc_strerror(ErrorNumber());
}

const char* SqliteException::what() const throw()
{
	return sqlite3_errstr(ErrorNumber());
}
