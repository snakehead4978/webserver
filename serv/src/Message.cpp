/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Message.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jeremie <jeremie@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/04 09:32:40 by jeremie           #+#    #+#             */
/*   Updated: 2026/02/19 06:00:54 by jeremie          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Message.hpp"
	
Message::Message() : chunked(-1), connection(true), size(-1) {}

Message::~Message()
{
	std::cout << std::endl << "Destroying headers container." << std::endl;
	headers.clear();
}

Message::Message(const Message& t) : method(t.method), target(t.target), headers(t.headers) {}

Message&	 Message::operator=(const Message& t)
{
	if (this != &t)
	{
		method = t.method;
		target = t.target;
		headers = t.headers;
	}
	return (*this);
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

void	Message::setHeaders(const std::string &key, const std::string &value)
{
	if (headers.find(key) != headers.end())
		throw std::invalid_argument("Key already exists.");
	headers[key] = value;
}

const std::string	&Message::getHeader(const std::string &key) const
{
	return (headers.at(key));
}

void	Message::printHeaders()
{
	typename std::map<std::string, std::string>::iterator start;
	for (start = headers.begin(); start != headers.end(); ++start)
		std::cout << "Key: " << start->first << "  ||   Value: " << start->second << std::endl;
}

void	Message::setTarget(std::string str)
{
	target = str;
}

std::string	Message::getTarget() const
{
	return (target);
}

static void smallcase(std::string &string)
{
	for (std::string::iterator i = string.begin(); i != string.end(); i++)
	{
		if (*i >= 'a' && *i <= 'z')
			*i -= 32;
	}
}

int Message::changeConnection(std::string &word) const
{
	if (connection && word.find("close") != word.npos)
		return (1);
	if (!connection && word.find("keep-alive") != word.npos)
		return (1);
	return (0);	
}

int	Message::parseLine(std::string line)
{
	static std::stringstream ss;
	std::string word;
	std::string word2;
	ss.clear();
	ss.str(line);
	ss >> word;
	smallcase(word);
	if (!(ss >> word2))
	{
		if (word == "host:" || word == "host")
			return (400);
		if (word.back() != ':' && line.back() == ' ')
			return (400);	
		return (0);
	}
	if (word.back() != ':')
		return (400);
	word.erase(word.end() - 1);
	int swap;
	if (word == "host")
	{
		if (!host.empty() || ss >> word)
			return (400);
		host = word2;
	}
	else if (word == "connection")
	{
		swap = !changeConnection(word2);
		while (ss >> word  && swap)
			swap = !changeConnection(word);
		if (!swap)
			connection = ~connection;
	}
	else if (word == "transfer-encoding")
	{
		if (chunked != -1)
			return (400);
		swap =  (word2.find("chunked") != word2.npos);
		while (ss >> word && swap)
			swap = (word.find("chunked") != word.npos);
		if (swap)
			chunked = true;
	}
	else if (word == "content-length")
	{
		if (size != -1 || getNumSoft(word2, size))
			return (400);
	}
	return (0);
}

bool	Message::getChunked() const
{
	return ((chunked));
}

bool	Message::getConnection() const
{
	return (connection);
}

std::string	&Message::getHost()
{
	return (host);
}

int	Message::getSize() const
{
	return (size);
}
