#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <string.h>
#include <stdexcept>
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

class NumericErrorException : public std::exception
{
private:
	const int errNum;
public:
	NumericErrorException(int errNum) : errNum(errNum)
	{
	}

	int ErrorNumber() const throw()
	{
		return errNum;
	}
};

class IrcException : public NumericErrorException
{
public:
	IrcException(int errNum) : NumericErrorException(errNum)
	{
	}

	const char* what() const throw()
	{
		return irc_strerror(ErrorNumber());
	}
};

class SystemException : public NumericErrorException
{
public:
	SystemException(int errNum) : NumericErrorException(errNum)
	{
	}

	const char* what() const throw()
	{
		return strerror(ErrorNumber());
	}
};

class ConfigException : public std::runtime_error
{
public:
	ConfigException(const std::string& msg)
		: std::runtime_error(msg)
	{
	}
};

class IllegalArgumentException : public std::runtime_error
{
public:
	IllegalArgumentException(const std::string& msg)
		: std::runtime_error(msg)
	{
	}
};

#endif
