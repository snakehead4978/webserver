/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientSocket.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jeremie <jeremie@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/06 17:27:51 by jeremie           #+#    #+#             */
/*   Updated: 2026/02/23 03:50:51 by jeremie          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ClientSocket.hpp"

ClientSocket::ClientSocket(int soc) : ACustomSocket(CLIENT)
{
	header = true;
	bigHeader = false;
	sock = soc;
	nRead = 0;
	readDone = false;
	nWrite = 0;
	writeSize = -1;
	writeFile = -1;
	port = -1;
	check = 0;
	message = 0;
	myServer = 0;
	error = 0;
	time = std::time(0);
	timedOut = false;
	cgi = 0;
	if (sock == -1)
	{
		perror("accept");
		return ;
	}
	if (setNonblock() || addToEpoll())
	{
		resetSocket();
		return ;
	}
	std::cout << "Client created and my server is and my socket is " << sock << std::endl;
}

ClientSocket::~ClientSocket()
{
	if (writeFile != -1)
	{
		close(writeFile);
	}
	epo.data.fd = sock;
	epoll_ctl(epol, EPOLL_CTL_DEL, sock, &epo);
	if (cgi)
		delete cgi;
}

void	ClientSocket::addToRequest(std::string &str)
{
	request.append(str);
}

int	secondparse(std::string &s, Message &msg)
{
	std::stringstream content (s);
	std::string name;
	std::string value;
	content >> name;
	if (*(name.end() - 1) != ':')
	{
		std::cout << "unknown name: " << name << std::endl;
		return ;
	}
	// std::cout << "Key: " << name.substr(0, name.size() - 1);
	content >> value;
	// std::cout << "     |     Data: " << name << std::endl;
	name.resize(name.size() - 1);
	msg.setHeaders(name, value);
}

int	ClientSocket::fillFirstBody()
{
	if (message->getHost().empty())
		return (setError(400));
	secondCheck();
	if (message->getChunked() && message->getSize() != -1)
		return (setError(400));
	if (!(message->getMethod() & POST))
		return (0);
	int checks = myServer->initialChecks(message->getSize(), message->getMethod(), message->getTarget(), maxBodySize);
	if (checks)
		return (setError(checks));
	readTo += 4;
	if (!(message->getMethod() & POST))
	{
		readDone = true;
		return (0);
	}
	return (fillBody());
}

int	ClientSocket::fillHeaders()
{
	std::string::size_type n = request.find("\r\n\r\n");
	if (n != request.npos)
		header = false;
	if (readTo == n)
		return (setError(400));
	std::string::size_type readNext;
	int	err;
	while (readNext = request.find("\r\n", readTo + 2) <= n)
	{
		err = message->parseLine(request.substr(readTo + 2, readNext));
		if (err)
			return (setError(err));
		readTo = readNext;
	}
	if (!header)
		fillFirstBody();
	return (0);
}

int	ClientSocket::resetRead()
{
	request.clear();
	request.resize(0);
	body.clear();
	body.resize(0);
	header = true;
	bigHeader = false;
	readDone = false;
	nRead = 0;
	check = -1;
	return (switchToWrite());
}

int	ClientSocket::resetWrite(bool all, bool read)
{
	if (all)
	{
		answer.clear();
		answer.resize(0);
		nWrite = 0;
		if (writeFile != -1)
		{
			if (close(writeFile))
				return (1);
		}
		writeFile = -1;
		writeSize = -1;
		if (read)
			return (switchToRead());
	}
	else
	{
		writeSize = -1;
		answer.clear();
		answer.resize(0);
		nWrite = 0;
	}
	return (0);
}

int	ClientSocket::switchToRead()
{
	epo.data.fd = sock;
	epo.events = EPOLLIN;
	if (epoll_ctl(epol, EPOLL_CTL_MOD, sock, &epo))
		return (perror("epoll_ctl"), 1);
	return (0);
}

int	ClientSocket::switchToWrite()
{
	epo.data.fd = sock;
	epo.events = EPOLLOUT;
	if (epoll_ctl(epol, EPOLL_CTL_MOD, sock, &epo))
		return (perror("epoll_ctl"), 1);
	return (0);
}

int	ClientSocket::handleWrite()
{
	if (error)
		myServer->fillError(error, message->getConnection(), message->getTarget(), answer);
	check = 0;
	if (writeFile != -1 && !answer.size())
	{
		std::cout << "im here" << std::endl;
		static char outBuff[OUTPUT_BUFF];
		bzero(outBuff, OUTPUT_BUFF);
		int n = read(writeFile, outBuff, OUTPUT_BUFF);
		if (n == -1)
			perror("read");
		write(sock, outBuff, n);
		if (n < BUFSIZ)
			return (resetWrite(true, true));
	}
	else
	{
		if (writeSize == -1)
			writeSize = answer.size();
		nWrite += write(sock, &answer.at(nWrite), writeSize - nWrite);
		if (nWrite == writeSize)
			return (resetWrite(writeFile == -1, true));
	}
	return (0);
}

static std::string	convert(int num)
{
	static std::stringstream ss;
	ss.str("");
	ss.clear();
	ss << num;
	return (ss.str());
}

int	ClientSocket::setError(int num)
{
	error = num;
	return (1);
}

int	ClientSocket::handleFirstLine()
{
	std::string firstLine = request.substr(0, readTo);
	std::string word;
	static std::stringstream	ss;
	ss.clear();
	ss.str(firstLine);
	if (ss.fail())
		return (setError(500));
	if (!(ss >> word))
		return (setError(400));
	try
	{
		message->setMethod(word);
	}
	catch(const std::exception& e)
	{
		return (setError(501));
	}
	if (!(ss >> word) || word.front() != '/')
		return (setError(400));
	message->setTarget(word);
	if (!(ss >> word))
		return (setError(400));
	if (word != "HTTP/1.1")
		return (setError(505));
	if (ss >> word)
		return (setError(400));
	return (0);
}

static int	getHexa(std::string num)
{
	long	final = 0;
	char c;
	for (int i = 0; c = num[i]; i++)
	{
		if (final >= 10000000)
			return (-1);
		if (c >= '0' && c <= '9')
			final = final * 16 + (c - '0');
		else if (c <= 'f' && c >= 'a')
			final = final * 16 + (c - 'a' + 10);
		else
			final = final * 16 + (c - 'A' + 10);
	}
	return ((int)final);
}

static int unChunk(std::string line, int &size, int &skipTo)
{
	int i = 0;
	char c = line[i];
	while ((c <= '9' && c >= '0') || (c <= 'f' && c >= 'a') || (c <= 'F' && c >= 'A') && line[i])
		c = line[++i];
	if (!line[i] || (line[i] == '\r' && !line[i + 1]))
		return (1);
	if (line[i] != '\r' || line[i + 1] != '\n')
		return (400);
	size = getHexa(line.substr(0, i));
	if (size == -1)
		return (413);
	skipTo = i + 2;
	return (0);
}

int	ClientSocket::fillBody()
{
	if (message->getChunked())
	{
		int size;
		int skipTo;
		int check;
		while (body.size() < maxBodySize)
		{
			check = unChunk(request.substr(readTo, request.size() - readTo), size, skipTo);
			if (check > 1)
				return (setError(check));
			else if (!check)
				return (0);
			else
			{
				if (request.size() >= readTo + skipTo + size + 2)
				{
					readTo += skipTo;
					body.append(request.substr(readTo, size));
					if (!size)
					{
						readDone = true;
						return (0);
					}
					readTo += size + 2;
				}
				else
					return (0);
			}
		}
	}
	else
		body.append(request.substr(readTo, request.size() - readTo));
	if (body.size() > maxBodySize)
		return (setError(413));
	return (0);
}

int ClientSocket::answerError(int err)
{
	int error;
	error = err;
	resetRead();
	return (0);
}

int	ClientSocket::handleRequest()
{
	std::string::size_type	n = request.find("\r\n");
	if (!nRead)
	{
		if (n == request.npos)
		{
			bigHeader = true;
			return (0);
		}
		else
		{
			message = new Message;
			if (!message)
				return (answerError(500));
			readTo = n;
			handleFirstLine();
			fillHeaders();
		}
	}
	else if (n != request.npos)
		fillHeaders();
	if (error)
		return (answerError(error));
	if (header)
		nRead++;
	else if (!readDone)
		fillBody();
	if (error)
		return (answerError(error));
	if (readDone)
	{
		int errCheck;
		if (message->getMethod() & GET)
			errCheck = myServer->fillGet(message, answer, writeFile, port);
		else if (message->getMethod() & POST)
			errCheck = myServer->fillPost(message, answer, writeFile, port);
		else
			errCheck = myServer->fillDelete(message, answer);
		if (errCheck)
			return (answerError(errCheck));
		// answer.append("HTTP/1.1");
		std::cout << "Number of reads done: " << nRead << std::endl;
		std::cout << "request is :" << request.substr(0, n) << "$" << std::endl;
		writeFile = open("./default_pages/404.html", O_RDONLY);
		if (writeFile == -1)
		{
			std::cout << "wrong file path" << std::endl;
			return (1);
		}
		struct stat statBuf;
		fstat(writeFile, &statBuf);
		answer.append("HTTP/1.1 200 OK\r\nContent-Length: ");
		answer.append(convert(statBuf.st_size));
		answer.append("\r\n\r\n");
		// answer.append("HTTP/1.1 200 OK\r\nContent-Length: 12\r\nContent-Type: text/plain; charset=utf-8\r\n\r\nHello World!");
		return (resetRead());
	}
	return (0);
}

bool	ClientSocket::isHeader() const
{
	return (header);
}

bool	ClientSocket::isBigHeader() const
{
	return (bigHeader);
}

void	ClientSocket::firstCheck(int _port, std::list<ACustomSocket *> &allSockets)
{
	if (check)
		return ;
	port = _port;
	check = 1;	
	for (std::list<ACustomSocket *>::iterator i = allSockets.begin(); i != allSockets.end(); i++)
	{
		if (!(*i)->isClient() && ((ServerSocket *)(*i))->portInfo(port))
				possibleServers.push_back((ServerSocket *)(*i));			
	}
	if (possibleServers.size() == 1)
	{
		check = 2;
		myServer = possibleServers.front();
		possibleServers.clear();
	}
}

void	ClientSocket::secondCheck()
{
	if (check == 2)
		return ;
	check = 2;
	std::string host = message->getHost();
	for (std::list<ServerSocket *>::iterator i = possibleServers.begin(); i != possibleServers.end(); i++)
	{
		if ((*i)->checkHostname(host))
		{
			myServer = *i;
			possibleServers.clear();
			return ;
		}
	}
	myServer = possibleServers.front();
	possibleServers.clear();
}

int	ClientSocket::checkTime()
{
	static std::time_t t;
	if (timedOut)
		return (0);
	t = std::time(0);
	if (!nRead)
	{
		if (std::difftime(t, time) >= 75)
			answerError(408);
	}
	else if (std::difftime(t, time) >= 60)
	{
		if (check == -1)
			return (1);
		answerError(408);
	}
	return (0);
}
