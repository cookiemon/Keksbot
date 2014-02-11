#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <exception>
#include <libircclient/libircclient.h>

class RestartException : public std::exception
{
public:
	const char* what() const throw()
	{
		return "Restarting";
	}
};

class ExitException : public std::exception
{
public:
	const char* what() const throw()
	{
		return "Exiting";
	}
};

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

	const char* what() const throw()
	{
		return irc_strerror(errNum);
	}
};

class ConfigException : public std::exception
{
private:
	const char* msg;
public:
	ConfigException(const char* msg) : msg(msg)
	{
	}
	const char* what() const throw()
	{
		return msg;
	}
};

#endif
