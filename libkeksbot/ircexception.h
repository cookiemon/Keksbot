#ifndef IRCEXCEPTION_H
#define IRCEXCEPTION_H

#include <exception>
#include <libircclient/libircclient.h>

class IrcException : public std::exception
{
private:
	const int errNum;
public:
	IrcException(int errNum) : errNum(errNum)
	{
	}

	int ErrorNumber() const
	{
		return errNum;
	}

	const char* what()
	{
		return irc_strerror(errNum);
	}
};

#endif
