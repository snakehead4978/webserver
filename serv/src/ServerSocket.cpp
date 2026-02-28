/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerSocket.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jla-chon <jla-chon@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/06 17:08:40 by jeremie           #+#    #+#             */
/*   Updated: 2026/02/28 18:55:09 by jla-chon         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ServerSocket.hpp"
#include "ClientSocket.hpp"
#include "Cgi.hpp"

static void	initLocation(t_locations &location)
{
	location.client_max_body = -1;
	location.methods = -1;
	location.autoindex = -1;
	location.redirection.first = -1;
}

ServerSocket::ServerSocket() : ACustomSocket(OWNER)
{
	initLocation(serverSettings);
}

static void	clearLocation(std::list<std::pair<std::string, t_locations *> > &locations)
{
	if (locations.empty())
		return ;
	for (std::list<std::pair<std::string, t_locations *> >::iterator i = locations.begin(); i != locations.end(); i++)
	{
		i->second->errors.clear();
		delete i->second;
	}
}

ServerSocket::~ServerSocket()
{
	clearLocation(locations);
	serverSettings.errors.clear();
	resetServerSocket();
	if (sock != -1)
		close(sock);
	sock = -1;
}

static const std::string	&getStatusText(int status)
{
	static std::map<int, std::string> codes;
	if (codes.empty())
	{
		codes[200] = " OK\r\n";
		codes[201] = " Created\r\n";
		codes[204] = " No Content\r\n";
		codes[301] = " Moved Permanently\r\n";
		codes[302] = " Found\r\n";
		codes[307] = " Temporary Redirect\r\n";
		codes[308] = " Permanent Redirect\r\n";
		codes[400] = " Bad Request\r\n";
		codes[403] = " Forbidden\r\n";
		codes[404] = " Not Found\r\n";
		codes[405] = " Method Not Allowed\r\n";
		codes[408] = " Request Timeout\r\n";
		codes[409] = " Conflict\r\n";
		codes[413] = " Content Too Large\r\n";
		codes[415] = " Unsupported Media Type\r\n";
		codes[500] = " Internal Server Error\r\n";
		codes[501] = " Not Implemented\r\n";
		codes[502] = " Bad Gateway\r\n";
		codes[504] = " Gateway Timeout\r\n";
		codes[505] = " HTTP Version Not Supported\r\n";
	}
	static std::string unknown(" Unknown\r\n");
	if (codes.count(status))
		return (codes[status]);
	return (unknown);
}

static bool	hasDot(std::string &path)
{
	std::stringstream ss(path);
	std::string segment;
	while (std::getline(ss, segment, '/'))
	{
		if (!segment.empty() && segment[0] == '.')
			return (true);
	}
	return (false);
}

int	ServerSocket::redirected(t_locations *location, std::string &answer, Message *message, bool &conn)
{
	if (location->redirection.second.empty())
		return (location->redirection.first);
	fillAnswer(location->redirection.first, message->getConnection(), answer, conn, 0, "", location->redirection.second);
	return (0);
}

static int	extractLastWord(std::stringstream &line, std::string &word, bool slashTerminate)
{
	if (!(line >> word))
		return (announceError(word.assign("Config error: expected something after " + word)));
	if (word[word.size() -1] != ';')
		return (announceError(word.assign("Config error: " + word + " must be terminated by ';'")));	
	word.erase(word.end() - 1);
	if (word.empty())
		return (announceError("Config error: isolated ';'"));
	if (slashTerminate && word[word.size() -1] != '/')
		word.push_back('/');
	return (0);
}

static int extractWord(std::stringstream &line, std::string &word)
{
	if (!(line >> word) || word.empty())
		return (announceError(word.assign("Config error: expected something after " + word)));
	if (word[word.size() -1] == ';')
		return (announceError(word.assign("Config error: " + word + " must not be terminated by ';'")));
	return (0);
}

static int parseRoot(std::stringstream &s, std::string &word, t_locations *location)
{
	if (!location->root.empty())
		return (announceError("Config error: multiple roots"));
	if (extractLastWord(s, word, true))
		return (1);
	location->root = word;
	return (0);
}

static int parseAutoindex(std::stringstream &s, std::string &word, t_locations *location)
{
	if (!(location->autoindex == -1))
		return (announceError("Config error: multiple autoindexes"));
	if (extractLastWord(s, word, false))
		return (1);
	if (word == "on")
		location->autoindex = 1;
	else if (word == "off")
		location->autoindex = 0;
	else
		return (announceError("Config error: autoindex must be on or off"));
	return (0);
}

static int	getNum(std::string &word, int &num)
{
	if (word.empty())
		return (announceError("empty num"));
	int size = word.size();
	for (int i = 0; i < size; i++)
	{
		if (word[i] < '0' || word[i] > '9')
			return (announceError(word.assign(word + " is not a number")));
	}
	long number;
	std::stringstream ss(word);
	ss >> number;
	if (number < 0 || number >= 2147483647)
		return (announceError("number must be between 1 and 100000000"));
	num = (int)number;
	return (0);
}

static int parseMaxbody(std::stringstream &s, std::string &word, t_locations *location)
{
	if (!(location->client_max_body == -1))
		return (announceError("Config error: multiple max body sizes"));
	if (extractLastWord(s, word, false))
		return (1);
	int size;
	if (getNum(word, size))
		return (1);
	location->client_max_body = size;
	return (0);
}

static int parseIndex(std::stringstream &s, std::string &word, t_locations *location)
{
	if (extractLastWord(s, word, false))
		return (1);
	location->index.push_back(word);
	return (0);
}

static int	parseErrorPage(std::stringstream &s, std::string &word, t_locations *location)
{
	std::pair<int, std::string> temp;
	if (extractWord(s, word))
		return (1);
	if (getNum	(word, temp.first))
		return (1);
	if (temp.first < 200 || temp.first > 600)
		return (announceError("Config error: error returns must be between 200 and 600"));	
	if (extractLastWord(s, word, false))
		return (1);
	temp.second = word;
	location->errors.push_back(temp);
	return (0);
}

static int	parseCgi(std::stringstream &s, std::string &word, t_locations *location)
{
	std::string extension;
	if (extractWord(s, word))
		return (1);
	if (word != ".py" && word != ".c" && word != ".php")
		return (announceError("Config error: cgi only allows .py and .c and .php"));
	if (!location->cgi.empty() && location->cgi.count(word))
		return (announceError(word.assign("Config error: duplicate cgi of extension " + word)));
	extension = word;
	if (extractLastWord(s, word, false))
		return (1);
	location->cgi[extension] = word;
	return (0);
}

static int getMethod(std::string &word, int &method)
{
	int m;
	if (word == "GET")
		m = GET;
	else if (word == "POST")
		m = POST;
	else if (word == "DELETE")
		m = DELETE;
	else
		return (announceError(word.assign("Config error: " + word + " is not a valid method")));
	if (method == -1)
		method = m;
	else if (method & m)
		return (announceError("Config error: duplicate methods"));
	else
		method |= m;
	return (0);
}

static int	parseMethod(std::stringstream &s, std::string &word, t_locations *location)
{
	if (location->methods != -1)
		return (announceError("Config error: limit_except already defined"));
	if (!(s >> word))
		return (announceError("Config error: expected methods after limit_except"));
	if (word[word.size() -1] == ';')
	{
		word.erase(word.end() - 1);
		return (getMethod(word, location->methods));
	}
	if (getMethod(word, location->methods))
		return (1);
	if (!(s >> word))
		return (announceError("Config error: expected ';'"));
	if (word[word.size() -1] == ';')
	{
		word.erase(word.end() - 1);
		return (getMethod(word, location->methods));
	}
	if (getMethod(word, location->methods))
		return (1);
	if (!(s >> word))
		return (announceError("Config error: expected ';'"));
	if (word[word.size() -1] != ';')
		return (announceError("Config error: max 3 methods"));
	word.erase(word.end() - 1);
	if (getMethod(word, location->methods))
		return (1);
	if (s >> word)
		return (announceError(word.assign("Config error: expected nothing after ';' but found " + word)));
	return (0);
}

static int	parseUpload(std::stringstream &s, std::string &word, t_locations *location)
{
	if (!location->uploads.empty())
		return (announceError("Config error: upload already defined"));
	if (extractLastWord(s, word, true))
		return (1);
	location->uploads = word;
	return (0);
}

static int	parseRedirection(std::stringstream &s, std::string &word, t_locations *location)
{
	if (!(location->redirection.first == -1))
		return (announceError("Config error: return already defined"));
	if (!(s >> word))
		return (announceError("Config error: expected error code or URL after return"));
	if (word[word.size() -1] == ';')
	{
		word.erase(word.end() - 1);
		if (!getNumSoft(word, location->redirection.first))
		{
			if (location->redirection.first < 200 || location->redirection.first > 600)
				return (announceError("Config error: return error code must be between 200 and 600"));
		}
		else
		{
			location->redirection.first = 302;
			location->redirection.second = word;
		}			
		return (0);
	}
	if (getNumSoft(word, location->redirection.first))
		return (announceError(word.assign("Config error: expected error code but found " + word)));
	if (location->redirection.first < 200 || location->redirection.first > 600)
		return (announceError("Config error: return error code must be between 200 and 600"));
	if (extractLastWord(s, word, false))
		return (1);
	location->redirection.second = word;
	return (0);
}

int	ServerSocket::parseLocationLine(std::string &line, bool &endLocation, t_locations *location)
{
	std::stringstream s(line);
	std::string word;
	s >> word;
	if (word == "}")
	{
		if (s >> word)
			return (announceError("Config error: '}' must be by itself"));
		endLocation = 1;
		return (0);
	}
	if (word == "location")
		return (announceError("Config error: location cannot be in another location"));
	if (word == "root")
		return (parseRoot(s, word, location));
	if (word == "autoindex")
		return (parseAutoindex(s, word, location));
	if (word == "client_max_body")
		return (parseMaxbody(s, word, location));
	if (word == "index")
		return (parseIndex(s, word, location));
	if (word == "error_page")
		return (parseErrorPage(s, word, location));
	if (word == "cgi")
		return (parseCgi(s, word, location));
	if (word == "limit_except")
		return (parseMethod(s, word, location));
	if (word == "uploads")
		return (parseUpload(s, word, location));
	if (word == "return")
		return (parseRedirection(s, word, location));
	return (announceError(word.assign("Config error: unknown module " + word)));
}

static int	checkLocation(std::stringstream &line, std::pair<std::string, t_locations *> &temp)
{
	std::string word;
	std::string path("/");
	if (!(line >> word))
		return (announceError("Config error: expected '{' but found nothing"));
	if (word != "{")
	{
		if (word[0] != '/')
			return (announceError("Config error: location must start with '/'"));
		if (word[word.size() -1] != '/')
			word.push_back('/');
		path = word;
		if (!(line >> word))
			return (announceError("Config error: expected '{' but found nothing"));
	}
	if (word == "{")
	{
		if (line >> word)
			return (announceError("Config error: expected nothing after '{' but found "));
	}
	else
		return (announceError(word.assign("Config error: expected '{' but found " + word)));
	temp.first = path;
	temp.second = new t_locations;
	initLocation(*temp.second);
	return (0);
}

int ServerSocket::fillLocation(std::ifstream &conf, std::stringstream &line)
{
	std::string word;
	bool endLocation = 0;
	std::pair<std::string, t_locations *>temp;
	if (checkLocation(line, temp))
		return (1);
	while (!endLocation)
	{
		if (getNextLine(conf, word) || parseLocationLine(word, endLocation, temp.second))
		{
			delete temp.second;
			return (1);
		}
	}
	temp.second->path = temp.first;
	locations.push_back(temp);
	return (0);
}

static int	parseListen(std::stringstream &s, std::string &word, std::set<int> &port)
{
	if (extractLastWord(s, word, false))
		return (1);
	int num;
	if (getNum(word, num))
		return (1);
	if (num < 0 || num > 65535)
		return (announceError("Config error: port number must be between 0 and 65,535"));
	if (port.find(num) != port.end())
		return (announceError("Config error: duplicate listen ports"));
	port.insert(num);
	return (0);
}

static int	parseServername(std::stringstream &s, std::string &word, std::set<std::string>&hostname)
{
	bool done = 0;
	while (!done)
	{
		if (!(s >> word))
			return (announceError("Config error: expected ';' but found nothing"));
		if (word[word.size() -1] == ';')
		{
			done = 1;
			word.erase(word.end() - 1);
			if (s >> word)
				return (announceError("Config error: expected nothing but found something after ';'"));
		}
		if (hostname.find(word) != hostname.end())
			return (announceError("Config error: duplicate hostname"));
		hostname.insert(word);
	}
	return (0);
}

int	ServerSocket::parseServerLine(std::ifstream &conf, std::string &line, bool &endServer)
{
	std::stringstream s(line);
	std::string word;
	s >> word;
	if (word == "}")
	{
		if (s >> word)
			return (announceError("Config error: '}' must be by itself"));
		endServer = 1;
		return (0);
	}
	if (word == "server")
		return (announceError("Config error: server cannot be in another server"));
	if (word == "location")
		return (fillLocation(conf, s));
	if (word == "root")
		return (parseRoot(s, word, &serverSettings));
	if (word == "autoindex")
		return (parseAutoindex(s, word, &serverSettings));
	if (word == "client_max_body")
		return (parseMaxbody(s, word, &serverSettings));
	if (word == "index")
		return (parseIndex(s, word, &serverSettings));
	if (word == "error_page")
		return (parseErrorPage(s, word, &serverSettings));
	if (word == "cgi")
		return (parseCgi(s, word, &serverSettings));
	if (word == "listen")
		return (parseListen(s, word, port));
	if (word == "server_name")
		return (parseServername(s, word, hostname));
	return (announceError(word.assign("Config error: unknown module " + word)));	
}

int	ServerSocket::fillServer(std::ifstream &conf)
{
	std::string line;
	bool endServer = 0;
	while (!endServer)
	{
		if (getNextLine(conf, line) || parseServerLine(conf, line, endServer))
			return (1);		
	}
	return (0);
}

static void	printLocations(t_locations &location)
{
	std::cout << "Client body " << location.client_max_body << std::endl;
	std::cout << "Methods " << location.methods << std::endl;
	std::cout << "Autoindex " << location.autoindex << std::endl;
	std::cout << "Root: " << (location.root.empty() ? "no" : location.root) << std::endl;
	std::cout << "Uploads: " << (location.uploads.empty() ? "no" : location.uploads) << std::endl;
	std::cout << "Redirection " << location.redirection.first << " and :" << (location.redirection.second.empty() ? "no" : location.redirection.second) << std::endl;
	if (!location.cgi.empty())
	{
		for (std::map<std::string, std::string>::iterator i = location.cgi.begin(); i != location.cgi.end(); i++)
			std::cout << "CGI " << i->first << " " << i->second << std::endl;
	}
	if (!location.index.empty())
	{
		for (std::list<std::string>::iterator i = location.index.begin(); i != location.index.end(); i++)
			std::cout << "Index " << *i << std::endl;
	}
	if (!location.errors.empty())
	{
		for (std::list<std::pair<int, std::string> >::iterator i = location.errors.begin(); i != location.errors.end(); i++)
			std::cout << "Error " << i->first << " " << i->second << std::endl;
	}
}

void	ServerSocket::printServer()
{
	std::cout << "--------------------------------------\\" << std::endl;
	if (!port.empty())
	{
		for (std::set<int>::iterator i = port.begin(); i != port.end(); i++)
			std::cout << "Port " << *i << std::endl;
	}
	if (!hostname.empty())
	{
		for (std::set<std::string>::iterator i = hostname.begin(); i != hostname.end(); i++)
			std::cout << "Hostname " << *i << std::endl;
	}
	printLocations(serverSettings);
	if (!locations.empty())
	{
		for (std::list<std::pair<std::string, t_locations *> >::iterator i = locations.begin(); i != locations.end(); i++)
		{
			std::cout << "Location " << i->first << std::endl;
			std::cout << "*****************************************************" << std::endl;
			printLocations(*i->second);
			std::cout << "*****************************************************" << std::endl << std::endl;
		}
	}
	std::cout << "--------------------------------------/" << std::endl << std::endl;
}

void	ServerSocket::resetServerSocket()
{
	if (sockets.empty())
		return ;
	for (std::set<int>::iterator i = sockets.begin(); i != sockets.end(); i++)
		close(*i);
	sockets.clear();
	sock = -1;
}

int	ServerSocket::startServer(std::list<ACustomSocket *> &socketsToFree, std::map<int, ACustomSocket *> &allSockets)
{
	if (port.empty())
		return (0);
	for (std::set<int>::iterator i = port.begin(); i != port.end(); i++)
	{
		if (portIgnored.count(*i))
			continue ;
		sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock == -1)
			return (perror("socket"), 1);
		if (setNonblock())
			return (resetServerSocket(), 1);
		int option = 1;
		if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) == -1)
			return (perror("setsockopt"), resetServerSocket(), 1);
		struct sockaddr_in	info;
		memset(&info, 0, sizeof(info));
		info.sin_family = AF_INET;
		info.sin_addr.s_addr = htonl(INADDR_ANY);
		info.sin_port = htons(*i);
		if (bind(sock, (struct sockaddr *)&info, sizeof(info)) == -1)
		{
			std::cerr << "port: " << *i << std::endl;
			return (resetServerSocket(), perror("bind"), 1);
		}
		if (listen(sock, SOMAXCONN) == -1)
			return (resetServerSocket(), perror("listen"), 1);
		if (addToEpoll())
			return (resetServerSocket(), 1);
		sockets.insert(sock);
		allSockets[sock] = this;
	}
	socketsToFree.push_back(this);
	return (0);
}

void	ServerSocket::checkPorts(std::set<int> &portsInUse)
{
	std::set<int>::iterator end = portsInUse.end();
	if (port.empty())
	{
		port.insert(DEFAULT_PORT);
		if (portsInUse.find(DEFAULT_PORT) == end)
			portsInUse.insert(DEFAULT_PORT);
		else
			portIgnored.insert(DEFAULT_PORT);
		return ;
	}
	for (std::set<int>::iterator i = port.begin(); i != port.end(); i++)
	{
		if (portsInUse.find(*i) == end)
			portsInUse.insert(*i);
		else
			portIgnored.insert(*i);
		end = portsInUse.end();
	}
	if (portIgnored.empty())
		return ;
}

int	ServerSocket::portInfo(int _port) const
{
	if (port.find(_port) != port.end())
		return (1);
	if (portIgnored.find(_port) != portIgnored.end())
		return (2);
	return (0);
}

int	ServerSocket::checkHostname(std::string &host)
{
	if (hostname.find(host) != hostname.end())
		return (1);
	return (0);
}

t_locations	*ServerSocket::findLocation(std::string &target)
{
	size_t	maxsize = 0;
	t_locations *found = 0;
	std::string normTarget = target;
	if (normTarget.empty() || normTarget[normTarget.size() - 1] != '/')
		normTarget += "/";
	if (!locations.empty())
	{
		for (std::list<std::pair<std::string, t_locations *> >::iterator i = locations.begin(); i != locations.end(); i++)
		{
			if (normTarget.find(i->first) == 0 && i->first.size() > maxsize)
			{
				maxsize = i->first.size();
				found = i->second;
			}
		}
	}
	if (!found && !serverSettings.root.empty())
		found = &serverSettings;
	return (found);
}

int	ServerSocket::initialChecks(int size, int method, size_t &maxbodysize, t_locations *toCheck)
{
	if (!toCheck )
		return (404);
	int			tmp = 0;
	if (toCheck->client_max_body == -1)
	{
		if (serverSettings.client_max_body == -1)
			tmp = MAX_BODY;
		else
			tmp = serverSettings.client_max_body;
	}
	else
		tmp = toCheck->client_max_body;
	if (size >= 0 && size > tmp)
		return (413);
	maxbodysize = tmp;
	if (toCheck->methods == -1)
	{
		if (serverSettings.methods == -1)
			tmp = GET;
		else
			tmp = serverSettings.methods;
	}
	else
		tmp = toCheck->methods;
	if (!(tmp & method))
		return (403);
	return (0);
}

void	ServerSocket::getTime(std::string &answer)
{
	static std::time_t t;
	static char timeString[38]; //std::size("Date: aaa, dd bbb YYYY HH:MM:SS GMT\r\n")];
	t = std::time(0);
	std::strftime(timeString, sizeof(timeString), "Date: %a, %d %b %Y %H:%M:%S GMT\r\n", std::gmtime(&t));
	answer.append(timeString);
}

static std::string	getDefaultPage(int status)
{
	std::string	path = "./default_pages/";
	path.append(convert(status));
	path.append(".html");
	return (path);
}

static int	getRoot(t_locations *location, t_locations &serverSettings, std::string &root)
{
	if (location && !location->root.empty())
		root = location->root;
	else
		root = serverSettings.root;
	if (root.empty())
		root.append("./");
	return (0);
}

int	ServerSocket::fillError(int error, Message *message, std::string &answer, int &writeFile, t_locations **location, bool &conn)
{
	static std::list<std::string> errors;
	struct stat	statBuf;
	errors.clear();
	errors.resize(0);
	answer.clear();
	answer.resize(0);
	if (!*location)
	{
		if (message)
			*location = findLocation(message->getTarget());
	}
	if (!*location)
		*location = &serverSettings;
	std::string root;
	getRoot(*location, serverSettings, root);
	if (location && !(*location)->errors.empty())
	{
		for (std::list<std::pair<int, std::string> >::iterator i = (*location)->errors.begin(); i != (*location)->errors.end(); i++)
		{
			if (i->first == error)
				errors.push_back(i->second);
		}
	}
	if (!serverSettings.errors.empty())
	{
		for (std::list<std::pair<int, std::string> >::iterator i = serverSettings.errors.begin(); i != serverSettings.errors.end(); i++)
		{
			if (i->first == error)
				errors.push_back(i->second);
		}
	}
	int fd;
	if (!errors.empty())
	{
		for (std::list<std::string>::iterator i = errors.begin(); i != errors.end(); i++)
		{
			fd = open((root + i->substr(1)).c_str(), O_RDONLY);
			if (fd != -1 && !fstat(fd, &statBuf))
			{
				if (!message)
					fillAnswer(error, false, answer, conn, (int)statBuf.st_size, "text/html");
				else	
					fillAnswer(error, message->getConnection(), answer, conn, (int)statBuf.st_size, "text/html");
				writeFile = fd;
				return (0);
			}
			if (fd != -1)
				close(fd);
		}
	}
	// std::cout << "page: " << getDefaultPage(error).c_str() << std::endl;
	fd = open(getDefaultPage(error).c_str(), O_RDONLY);
	if (fd != -1 && fstat(fd, &statBuf) == 0)
	{
		if (!message)
			fillAnswer(error, false, answer, conn, (int)statBuf.st_size, "text/html");
		else
			fillAnswer(error, message->getConnection(), answer, conn, (int)statBuf.st_size, "text/html");
		writeFile = fd;
		return (0);
	}
	if (fd != -1)
		close(fd);
	std::string	body = "<html><body><h1>";
	body.append(convert(error));
	body.append(getStatusText(error));
	body.append("</h1></body></html>");
	if (!message)
		fillAnswer(error, false, answer, conn, (int)body.size(), "text/html");
	else
		fillAnswer(error, message->getConnection(), answer, conn, (int)body.size(), "text/html");
	answer.append(body);
	return (0);
}

static std::string sanitizePath(std::string path)
{
	std::vector<std::string> segments;
	std::stringstream ss(path);
	std::string segment;
	while (std::getline(ss, segment, '/'))
	{
		if (segment.empty() || segment == ".")
			continue ;
		else if (segment == "..")
		{
			if (!segments.empty())
				segments.pop_back();
		}
		else
			segments.push_back(segment);
	}
	std::string result;
	for (std::vector<std::string>::iterator i = segments.begin(); i != segments.end(); i++)
		result += "/" + *i;
	if (result.empty())
		result = "/";
	return (result);
}

static std::string	buildFilepath(std::string &root, std::string &target, std::string &locationPath)
{
	std::string clean = target;
	std::string::size_type q = target.find('?');
	if (q != target.npos)
		clean = target.substr(0, q);
	clean = sanitizePath(clean);
	if (clean.size() >= locationPath.size()
		&& clean.substr(0, locationPath.size()) == locationPath)
		return (root + clean.substr(locationPath.size()));
	if (clean.size() + 1 == locationPath.size()
		&& locationPath.substr(0, clean.size()) == clean)
		return (root);
	return (root + clean.substr(1));
}

int	ServerSocket::isCGI(Message *message, t_locations *location, std::string &cgiPath, std::string &rootPath)
{
	if (!location)
		return (0);
	std::string target = message->getTarget();
	size_t q = target.find('?');
	if (q != std::string::npos)
		target = target.substr(0, q);
	if (hasDot(target))
		return (0);
	size_t dot = target.rfind('.');
	if (dot == std::string::npos)
		return (0);
	std::string ext = target.substr(dot);
	std::string rootDir;
	getRoot(location, serverSettings, rootDir);
	rootPath = buildFilepath(rootDir, message->getTarget(), location->path);
	char *checkRoot = realpath(rootDir.c_str(), 0);
	char *checkPath = realpath(rootPath.c_str(), 0);
	if (!checkRoot || !checkPath)
	{
		free(checkRoot);
		free(checkPath);
		return (0);
	}
	size_t rootLen = strlen(checkRoot);
	if (strncmp(checkPath, checkRoot, rootLen) != 0 || checkPath[rootLen] != '/')
	{
		free(checkRoot);
		free(checkPath);
		return (0);
	}
	free(checkRoot);
	free(checkPath);
	if (location->cgi.count(ext))
	{
		cgiPath = location->cgi[ext];
		return (1);
	}
	if (serverSettings.cgi.count(ext))
	{
		cgiPath = serverSettings.cgi[ext];
		return (1);
	}
	return (0);
}

static int	extractHeader(std::string &partHeaders, const std::string &key, std::string &value)
{
	size_t	start = partHeaders.find(key + ":");
	if (start == partHeaders.npos)
		return (1);
	start += key.size() + 1;
	size_t	end = partHeaders.find("\r\n", start);
	std::string	raw = partHeaders.substr(start, end - start);
	int	first = firstChar(raw);
	value = raw.substr(first);
	return (0);
}

static int	extractFilename(std::string &partHeaders, std::string &filename)
{
	std::string	disposition;
	if (extractHeader(partHeaders, "Content-Disposition", disposition))
		return (400);
	size_t	start = disposition.find("filename=");
	if (start == disposition.npos)
		return (400);
	start += 9;
	size_t	end;
	if (disposition[start] == '"')
	{
		end = disposition.find("\"", ++start);
		if (end == disposition.npos)
			return (400);
		filename = disposition.substr(start, end - start);
	}
	else
	{
		end = disposition.find_first_of(";\r\n ", start);
		if (end == disposition.npos)
			filename = disposition.substr(start);
		else
			filename = disposition.substr(start, end - start);
	}
	if (filename.empty())
		return (400);
	return (0);
}

static int	checkTransferEncoding(std::string &partHeaders)
{
	std::string	header;
	if (extractHeader(partHeaders, "Content-Transfer-Encoding", header))
		return (0);
	if (header != "8bit" && header != "binary" && header != "7bit")
		return (415);
	return (0);
}

static int	extractPartHeaders(std::string &body, std::string &boundary, std::string &partHeaders, size_t &dataStart, size_t searchFrom)
{
	size_t	start = body.find(boundary + "\r\n", searchFrom);
	if (start == body.npos)
		return (-1);
	start += boundary.size() + 2;
	size_t	end = body.find("\r\n\r\n", start);
	if (end == body.npos)
		return (400);
	partHeaders = body.substr(start, end - start);
	dataStart = end + 4;
	return (checkTransferEncoding(partHeaders));
}

static int	extractDataAt(std::string &body, std::string &boundary, std::string &filename, size_t &dataStart, size_t &dataEnd, size_t searchFrom)
{
	std::string	partHeaders;
	int	err = extractPartHeaders(body, boundary, partHeaders, dataStart, searchFrom);
	if (err)
		return (err);
	err = extractFilename(partHeaders, filename);
	if (err)
		return (err);
	dataEnd = body.find("\r\n" + boundary, dataStart);
	if (dataEnd == body.npos)
		return (400);
	return (0);
}

static int	writeToFile(std::string &filepath, std::string &body, size_t dataStart, size_t dataEnd)
{
	struct stat	statBuf;
	if (stat(filepath.c_str(), &statBuf) == 0)
		return (409);
	int	fd = open(filepath.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0644);
	if (fd == -1)
		return (500);
	if (write(fd, body.c_str() + dataStart, dataEnd - dataStart) == -1)
	{
		close(fd);
		return (500);
	}
	close(fd);
	return (0);
}

int	ServerSocket::fillPost(Message *message, std::string &answer, std::string &body, t_locations *location, bool &conn)
{
	if (!location)
		return (404);
	if (location->redirection.first != -1)
		return (redirected(location, answer, message, conn));
	if (location->uploads.empty())
		return (405);
	if (!message->headerExists("content-type"))
		return (400);
	if (message->getHeader("content-type") != "multipart/form-data")
		return (415);
	if (message->getContentTypeSize() != 2)
		return (400);
	std::string boundary = message->getBoundary();
	if (setBoundary(boundary))
		return (400);
	boundary = "--" + boundary;
	std::string	closingBoundary = boundary + "--";
	size_t		searchFrom = 0;
	size_t		dataStart;
	size_t		dataEnd;
	std::string	filename;
	int			fileCount = 0;
	int			err;
	while (body.find(closingBoundary, searchFrom) != body.npos)
	{
		err = extractDataAt(body, boundary, filename, dataStart, dataEnd, searchFrom);
		if (err == -1)
			break ;
		if (err)
			return (err);
		if (hasDot(filename))
			return (400);
		if (filename.find('/') != filename.npos)
			return (400);
		std::string	filepath = location->uploads + filename;
		err = writeToFile(filepath, body, dataStart, dataEnd);
		if (err)
			return (err);
		fileCount++;
		searchFrom = dataEnd + 2;
	}
	if (!fileCount)
		return (400);
	fillAnswer(201, message->getConnection(), answer, conn);
	return (0);
}

static std::string	getMimeType(const std::string &path)
{
	size_t	dot = path.rfind('.');
	if (dot == path.npos)
		return ("application/octet-stream");
	std::string	ext = path.substr(dot);
	if (ext == ".html" || ext == ".htm")
		return ("text/html");
	if (ext == ".css")
		return ("text/css");
	if (ext == ".js")
		return ("application/javascript");
	if (ext == ".jpg" || ext == ".jpeg")
		return ("image/jpeg");
	if (ext == ".png")
		return ("image/png");
	if (ext == ".gif")
		return ("image/gif");
	if (ext == ".txt")
		return ("text/plain");
	if (ext == ".pdf")
		return ("application/pdf");
	return ("application/octet-stream");
}

static int	generateAutoindex(const std::string &dirPath, const std::string &target, std::string &html)
{
	DIR	*dir = opendir(dirPath.c_str());
	if (!dir)
		return (1);
	html.append("<html><head><title>Index of ");
	html.append(target);
	html.append("</title></head><body><h1>Index of ");
	html.append(target);
	html.append("</h1><hr><pre>");
	struct dirent	*entry;
	while ((entry = readdir(dir)))
	{
		std::string	name(entry->d_name);
		if (name == ".")
			continue ;
		std::string	href = target;
		if (href[href.size() - 1] != '/')
			href.push_back('/');
		href.append(name);
		html.append("<a href=\"");
		html.append(href);
		html.append("\">");
		html.append(name);
		html.append("</a>\n");
	}
	closedir(dir);
	html.append("</pre><hr></body></html>");
	return (0);
}

int	ServerSocket::handleDirectory(std::string &filepath, Message *message, std::string &answer, int &writeFile, t_locations *location, bool &conn)
{
	if (filepath[filepath.size() - 1] != '/')
		filepath.push_back('/');
	std::list<std::string>	indexFiles;
	if (!location->index.empty())
	{
		for (std::list<std::string>::iterator i = location->index.begin(); i != location->index.end(); i++)
			indexFiles.push_back(*i);
	}
	if (!serverSettings.index.empty())
	{
		for (std::list<std::string>::iterator i = serverSettings.index.begin(); i != serverSettings.index.end(); i++)
			indexFiles.push_back(*i);
	}
	struct stat	statBuf;
	if (!indexFiles.empty())
	{
		for (std::list<std::string>::iterator i = indexFiles.begin(); i != indexFiles.end(); i++)
		{
			std::string	indexPath = filepath + *i;
			if (stat(indexPath.c_str(), &statBuf) == 0 && S_ISREG(statBuf.st_mode))
			{
				int	fd = open(indexPath.c_str(), O_RDONLY);
				if (fd == -1)
					return (403);
				fillAnswer(200, message->getConnection(), answer, conn, (int)statBuf.st_size, getMimeType(indexPath));
				writeFile = fd;
				return (0);
			}
		}
	}
	if (location->autoindex == 1 || (location->autoindex == -1 && serverSettings.autoindex == 1))
	{
		std::string	html;
		generateAutoindex(filepath, message->getTarget(), html);
		if (html.empty())
			return (403);
		fillAnswer(200, message->getConnection(), answer, conn, (int)html.size(), "text/html");
		answer.append(html);
		return (0);
	}
	return (403);
}

int	ServerSocket::fillGet(Message *message, std::string &answer, int &writeFile, t_locations *location, bool &conn)
{
	if (!location)
		return (404);
	if (location->redirection.first != -1)
		return (redirected(location, answer, message, conn));
	std::string root;
	std::string target = message->getTarget();
	getRoot(location, serverSettings, root);
	if (root[root.size() - 1] != '/')
		root += "/";
	struct stat statBuf;
	if (hasDot(target))
		return (404);
	std::string filepath = buildFilepath(root, target, location->path);
	if (stat(filepath.c_str(), &statBuf) == -1)
		return (404);
	char *checkRoot = realpath(root.c_str(), 0);
	char *checkPath = realpath(filepath.c_str(), 0);
	if (!checkRoot || !checkPath)
	{
		free(checkRoot);
		free(checkPath);
		return (403);
	}
	size_t rootLen = strlen(checkRoot);
	if (strncmp(checkPath, checkRoot, rootLen) != 0
		|| (checkPath[rootLen] != '/' && checkPath[rootLen] != '\0'))
	{
		free(checkRoot);
		free(checkPath);
		return (403);
	}
	free(checkRoot);
	free(checkPath);
	if (S_ISDIR(statBuf.st_mode))
	{
		if (target[target.size() - 1] != '/')
			return (fillAnswer(301, message->getConnection(), answer, conn, 0, "", target + "/"));
		return (handleDirectory(filepath, message, answer, writeFile, location, conn));
	}
	if (!S_ISREG(statBuf.st_mode))
		return (403);
	int fd = open(filepath.c_str(), O_RDONLY);
	if (fd == -1)
		return (403);
	fillAnswer(200, message->getConnection(), answer, conn, (int)statBuf.st_size, getMimeType(filepath));
	writeFile = fd;
	return (0);
}

static bool	forceClose(int error)
{
	static std::set<int> errors;
	if (errors.empty())
	{
		errors.insert(400);
		errors.insert(408);
		errors.insert(413);
		errors.insert(500);
		errors.insert(501);
		errors.insert(505);
	}
	if (errors.find(error) == errors.end())
		return (false);
	return (true);
}

int ServerSocket::fillAnswer(int status, bool connection, std::string &answer, bool &conn, int contentLength, std::string contentType, std::string location)
{
	answer.append("HTTP/1.1 ");
	answer.append(convert(status));
	answer.append(getStatusText(status));
	answer.append("Server: webserver/1.0\r\n");
	getTime(answer);
	if (forceClose(status))
	{
		conn = false;
		connection = false;
	}
	else
		conn = connection;
	if (connection)
		answer.append("Connection: keep-alive\r\n");
	else
		answer.append("Connection: close\r\n");
	if (!location.empty())
	{
		answer.append("Location: ");
		answer.append(location);
		answer.append("\r\n");
	}
	if (status == 204)
	{
		answer.append("\r\n");
		return (0);
	}
	if (!contentType.empty())
	{
		answer.append("Content-Type: ");
		answer.append(contentType);
		answer.append("\r\n");
	}
	answer.append("Content-Length: ");
	answer.append(convert(contentLength));
	answer.append("\r\n\r\n");
	return (0);
}

int	ServerSocket::setLocation(Message *message, t_locations **location)
{
	*location = findLocation(message->getTarget());
	return (0);
}

int	ServerSocket::fillDelete(Message *message, std::string &answer, t_locations *location, bool &conn)
{
	if (!location)
		return (404);
	if (location->redirection.first != -1)
		return (redirected(location, answer, message, conn));
	std::string	root;
	getRoot(location, serverSettings, root);
	if (hasDot(message->getTarget()))
		return (404);
	std::string filepath = buildFilepath(root, message->getTarget(), location->path);
	struct stat	statBuf;
	if (stat(filepath.c_str(), &statBuf) == -1)
		return (404);
	if (!S_ISREG(statBuf.st_mode))
		return (403);
	if (access(filepath.c_str(), W_OK) == -1)
		return (403);
	if (unlink(filepath.c_str()) == -1)
		return (500);
	fillAnswer(204, message->getConnection(), answer, conn);
	return (0);
}

int		ServerSocket::fillCgi(t_locations *location, std::string &Cgipath, std::string &rootPath, std::string &answer, ClientSocket *client, bool &conn)
{
	if (!location)
		return (404);
	if (location->redirection.first != -1)
		return (redirected(location, answer, client->getMessage(), conn));
	client->createCgi(Cgipath, rootPath);
	int err = client->startCgi();
	if (err == -10)
		return (-10);
	if (err)
		return (500);
	return (0);
}
