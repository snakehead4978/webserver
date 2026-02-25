/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Message.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jeremie <jeremie@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/04 09:32:40 by jeremie           #+#    #+#             */
/*   Updated: 2026/02/25 07:14:09 by jeremie          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Message.hpp"
	
Message::Message() : size(-1) {}

Message::~Message()
{
	headers.clear();
}

void	Message::setMethod(std::string &s)
{
	if (s == "GET")
		method = GET;
	else if (s == "POST")
		method = POST;
	else if (s == "DELETE")
		method = DELETE;
	else
		throw std::invalid_argument("Not POST, GET or DELETE method!");
}

int	Message::getMethod() const
{
	return (method);
}

const std::string &Message::getHeader(const std::string &key) const
{
    return (headers.at(key).front());
}

bool	Message::headerExists(const std::string &key)
{
	return (headers.count(key));
}

const std::list<std::string> &Message::getHeaders(const std::string &key) const
{
    return (headers.at(key));
}

void	Message::setTarget(std::string str)
{
	target = str;
}

std::string	&Message::getTarget()
{
	return (target);
}

static void smallcase(std::string &string)
{
	if (string.empty())
		return ;
	for (std::string::iterator i = string.begin(); i != string.end(); i++)
	{
		if (*i >= 'A' && *i <= 'Z')
			*i += 32;
	}
}

static void splitValue(std::string &value, std::list<std::string> &values)
{
	static std::stringstream ss;
	static std::stringstream inner;
	ss.clear();
	ss.str(value);
	std::list<std::string> result;
	std::string token;
	while (std::getline(ss, token, ';'))
	{
		inner.clear();
		inner.str(token);
		std::string word;
		while (inner >> word)
			values.push_back(word);
	}
}

int Message::parseLine(std::string line)
{
	std::string word;
	std::string value;
	size_t colon = line.find(':');
	if ((colon == line.npos && isWhiteSpace(line[line.size() - 1])) || colon == 0)
		return (400);
	int first = firstChar(line);
	if (colon == line.npos)
		word = line.substr(first);
	else
		word = line.substr(first, colon - first);
	smallcase(word);
	if (word.empty())
		return (400);
	std::cerr << "$" << word <<  "$" << std::endl;
	if (colon == line.npos)
	{
		if (word == "host")
			return (400);
		else
		{
			headers[word];
			return (0);
		}
	}
	value = line.substr(colon + 1);
	value = value.substr(firstChar(value));
	if (word == "host" && headers.count("host"))
			return (400);
	if (word == "content-type" && headers.count("content-type"))
		return (400);
	if (word == "content-length" && headers.count("content-length"))
		return (400);
	if (word == "transfer-encoding" && headers.count("transfer-encoding"))
		return (400);
	splitValue(value, headers[word]);
	if (word == "host" && headers["host"].size() != 1)
		return (400);
	return (0);
}

int		Message::finalChecks()
{
	if (!headers.count("host"))
		return (400);
	if (headers.count("transfer-encoding") && !headers["transfer-encoding"].empty())
	{
		for (std::list<std::string>::iterator i = headers["transfer-encoding"].begin(); i != headers["transfer-encoding"].end(); i++)
		{
			if ((*i) != "chunked")
				return (501);	
		}
		if (headers["transfer-encoding"].size() != 1)
			return (400);
	}
	if (headers.count("content-length") && !headers["content-length"].empty())
	{
		if (headers["content-length"].size() > 1)
			return (400);
		std::string sizeString = getHeader("content-length");
		if (getNumSoft(sizeString, size))
			return (400);
	}
	return (0);
}

bool	Message::getConnection()
{
	bool connection = true;
	if (headers.empty())
		return (false);
	if (!headers.count("connection") || headers["connection"].empty())
		return (connection);
	for (std::list<std::string>::iterator i = headers["connection"].begin(); i != headers["connection"].end(); i++)
	{
		if (connection && i->find("close") != i->npos)
			connection = !connection;
		else if (!connection && i->find("keep-alive") != i->npos)
			connection = !connection;
	}
	return (connection);
}

std::string Message::getBoundary()
{
	return (headers["content-type"].back());
}

int	Message::getSize() const
{
	return (size);
}

std::map<std::string, std::list<std::string> >&Message::getHeaderMap()
{
	return (headers);
}

int	Message::getContentTypeSize()
{
	if (!headers.count("content-type"))
		return (0);
	return (headers.at("content-type").size());
}

