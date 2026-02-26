/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientSocket.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jla-chon <jla-chon@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/06 17:27:51 by jeremie           #+#    #+#             */
/*   Updated: 2026/02/26 14:29:35 by jla-chon         ###   ########.fr       */
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
	location = 0;
	connection = false;
	maxBodySize = MAX_BODY;
	readTo = 0;
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
	std::cout << "Client " << sock << " connected" << std::endl;
}

ClientSocket::~ClientSocket()
{
	if (message)
		delete message;
	if (writeFile != -1)
	{
		close(writeFile);
	}
	if (cgi)
		delete cgi;
}

void	ClientSocket::addToRequest(std::string &str, int read)
{
	request.append(str, 0, read);
}

int	ClientSocket::fillFirstBody()
{
	int checks = message->finalChecks();
	if (checks)
		return (setError(checks));
	if (message->getHeader("host").empty())
		return (setError(400));
	secondCheck();
	if (!myServer)
		return (setError(500));
	if (message->headerExists("transfer-encoding") && !message->getHeader("transfer-encoding").empty() && message->getSize() != -1)
		return (setError(400));
	myServer->setLocation(message, &location);
	if (!location)
		return (setError(500));
	checks = myServer->initialChecks(message->getSize(), message->getMethod(), maxBodySize, location);
	if (checks)
		return (setError(checks));
	if (!(message->getMethod() & POST))
	{
		readDone = true;
		return (0);
	}
	readTo += 4;
	return (fillBody());
}

int	ClientSocket::fillHeaders()
{
	std::string::size_type n = request.find("\r\n\r\n");
	if (n != request.npos)
		header = false;
	if (readTo == n && !message->headerExists("host"))
		return (setError(400));
	std::string::size_type readNext;
	int	err;
	readNext = request.find("\r\n", readTo + 2);
	while (readNext <= n && readNext != request.npos)
	{
		std::string a = request.substr(readTo + 2, readNext - readTo - 2);
		err = message->parseLine(a);
		if (err)
			return (setError(err));
		readTo = readNext;
		if (request.size() <= readNext + 2)
			break ;
		readNext = request.find("\r\n", readTo + 2);
	}
	if (!header)
		return (fillFirstBody());
	return (0);
}

int	ClientSocket::resetRead()
{
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
			close(writeFile);
			writeFile = -1;
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
	if (message)
		delete message;
	message = 0;
	error = 0;
	location = 0;
	std::string start;
	if (readTo < request.size())
		start = request.substr(readTo);
	size_t s = start.find_first_not_of("\r\n");
	if (s != start.npos)
		start = start.substr(s);
	else
		start.clear();
	request = start;
	readTo = 0;
	nRead = 0;
	struct epoll_event ev;
	ev.data.fd = sock;
	ev.events = EPOLLIN;
	if (epoll_ctl(epol, EPOLL_CTL_MOD, sock, &ev) == -1)
	{
		if (errno == ENOENT)
			epoll_ctl(epol, EPOLL_CTL_ADD, sock, &ev);
		else
			return (perror("epoll_ctl"), 1);
	}
	if (!request.empty() && request.find("\r\n") != request.npos)
		return (handleRequest());
	request.clear();
	return (0);
}

void ClientSocket::setTime()
{
	time = std::time(0);
}

int	ClientSocket::turnCgi(bool on)
{
	struct epoll_event ev;
	ev.data.fd = sock;
	ev.events = EPOLLOUT;
	if (on)
	{
		int ret = epoll_ctl(epol, EPOLL_CTL_ADD, sock, &ev);
		if (ret)
		{
			if (errno == EEXIST)
				epoll_ctl(epol, EPOLL_CTL_MOD, sock, &ev);
			else
				return (perror("epoll_ctl"), 1);
		}
	}
	else
	{
		int ret = epoll_ctl(epol, EPOLL_CTL_DEL, sock, &ev);
		if (ret && errno != ENOENT)
			return (perror("epoll_ctl"), 1);		
	}
	return (0);
}

int	ClientSocket::switchToWrite()
{
	struct epoll_event ev;
	ev.data.fd = sock;
	ev.events = EPOLLOUT;
	if (epoll_ctl(epol, EPOLL_CTL_MOD, sock, &ev) == -1)
	{
		if (errno == ENOENT)
			epoll_ctl(epol, EPOLL_CTL_ADD, sock, &ev);
		else
			return (perror("epoll_ctl"), 1);
	}
	return (0);
}

int	ClientSocket::handleWrite()
{
	if (error)
	{
		if (!myServer)
		{
			resetWrite(true, true);
			return (1);
		}
		myServer->fillError(error, message, answer, writeFile, &location, connection);
		error = 0;
	}
	check = 0;
	if (writeFile != -1 && !answer.size())
	{
		static char outBuff[OUTPUT_BUFF];
		bzero(outBuff, OUTPUT_BUFF);
		int n = read(writeFile, outBuff, OUTPUT_BUFF);
		if (n == -1)
			return (resetWrite(true, false), 1);
		if (n == 0)
			return (resetWrite(true, true), 1);
		if (write(sock, outBuff, n) == -1)
			return (1);
		time = std::time(0);
		if (n < OUTPUT_BUFF)
			return (resetWrite(true, true), !connection);
	}
	else
	{
		if (writeSize == -1)
			writeSize = answer.size();
		if (writeSize == 0 || answer.empty() || nWrite >= (int)answer.size())
			return (resetWrite(true, true), !connection);
		int n = write(sock, answer.c_str() + nWrite, writeSize - nWrite);
		time = std::time(0);
		if (n == -1)
			return (1);
		nWrite += n;
		if (nWrite == writeSize)
			return (resetWrite(writeFile == -1, true), false);
	}
	return (0);
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
	if (!(ss >> word) || word[0] != '/')
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
	for (int i = 0; (c = num[i]); i++)
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
	while (((c <= '9' && c >= '0') || (c <= 'f' && c >= 'a') || (c <= 'F' && c >= 'A')) && line[i])
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
	if (message->headerExists("transfer-encoding") && !message->getHeader("transfer-encoding").empty())
	{
		int size;
		int skipTo;
		int check;
		while (!readDone)
		{
			check = unChunk(request.substr(readTo, request.size() - readTo), size, skipTo);
			if (check > 1)
				return (setError(check));
			else if (check == 1)
				return (0);
			else
			{
				if (request.size() >= readTo + (size_t)skipTo + (size_t)size + 2)
				{
					readTo += skipTo;
					body.append(request.substr(readTo, size));
					if (!size)
					{
						readTo += size + 2;
						readDone = true;
						return (0);
					}
					readTo += size + 2;
				}
				else
					return (0);
			}
			if (body.size() > (size_t)maxBodySize)
				return (setError(413));
		}
	}
	else
	{
		body.append(request.substr(readTo, request.size() - readTo));
		readTo = request.size();
		if (message->getSize() != -1 && (int)body.size() >= message->getSize())
		{
			readTo = readTo - (body.size() - message->getSize());
			body.resize(message->getSize());
			readDone = true;
			return (0);
		}
	}
	if (body.size() > (size_t)maxBodySize)
		return (setError(413));
	return (0);
}

int ClientSocket::answerError(int err)
{
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
			nRead = 1;
			readTo = n;
			if (handleFirstLine())
				return (answerError(error));
			fillHeaders();
		}
	}
	else if (n != request.npos)
	{
		if (header)
			fillHeaders();
		else if (!readDone)
			fillBody();
	}
	if (error)
		return (answerError(error));
	else if (!readDone && !header)
		fillBody();
	if (error)
		return (answerError(error));
	if (readDone)
	{
		int errCheck;
		std::string cgiPath;
		std::string rootPath;
		if (myServer->isCGI(message, location, cgiPath, rootPath))
		{
			errCheck = myServer->fillCgi(location, cgiPath, rootPath, answer, this, connection);
			if (errCheck == -10)
				return (-10);
			if (errCheck)
				return (answerError(errCheck));
			return (0);
		}
		else if (message->getMethod() & GET)
			errCheck = myServer->fillGet(message, answer, writeFile, location, connection);
		else if (message->getMethod() & POST)
			errCheck = myServer->fillPost(message, answer, body, location, connection);
		else
			errCheck = myServer->fillDelete(message, answer, location, connection);
		if (errCheck)
			return (answerError(errCheck));
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

void	ClientSocket::firstCheck(int _port, std::list<ACustomSocket *> &allSockets, ServerSocket *server)
{
	if (check)
		return ;
	port = _port;
	check = 1;
	myServer = server;
	if (allSockets.empty())
		return ;
	for (std::list<ACustomSocket *>::iterator i = allSockets.begin(); i != allSockets.end(); i++)
	{
		if ((*i)->isWhat() == OWNER && ((ServerSocket *)(*i))->portInfo(port))
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
	std::string host = message->getHeader("host");
	if (possibleServers.empty())
		return ;
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
		{
			time = std::time(0);			
			answerError(408);
		}
	}
	else if (std::difftime(t, time) >= 60)
	{
		time = std::time(0);			
		if (check == -1)
			return (1);
		answerError(408);
	}
	return (0);
}

void	ClientSocket::appendAnswer(std::string &str)
{
	answer.append(str);
}

std::string &ClientSocket::getBody()
{
	return (body);
}

Message	*ClientSocket::getMessage()
{
	return (message);
}

int	ClientSocket::prependAnswer(std::string str)
{
	std::string::size_type split = answer.find("\r\n\r\n");
	if (split == answer.npos)
		return (500);
	if (answer.find("Content-Length:") == answer.npos && answer.find("content-length:") == answer.npos)
	{
		std::string contentLength = "Content-Length: " + convert((int)(answer.size() - split - 4)) + "\r\n";
		answer.insert(split + 2, contentLength);
	}
	answer.insert(0, str);
	return (0);
}

int	ClientSocket::timeout()
{
	setError(408);
	resetRead();
	return (0);
}

std::string &ClientSocket::getAnswer()
{
	return (answer);
}

int		ClientSocket::createCgi(std::string &cgiPath, std::string &rootPath)
{
	std::string portaddr = convert(port);
	cgi = new Cgi(this, cgiPath, rootPath, portaddr);
	return (0);
}

int		ClientSocket::startCgi()
{
	if (!cgi)
		return (1);
	return (cgi->execute());
}

int		ClientSocket::cgiError(int err)
{
	error = err;
	resetRead();
	return (err);
}

void	ClientSocket::delCgi()
{
	cgi = 0;
}

bool	ClientSocket::getConnection()
{
	return (connection);
}

void	ClientSocket::setConnection(bool connect)
{
	connection = connect;
}
