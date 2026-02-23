/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerSocket.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jeremie <jeremie@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/06 17:08:40 by jeremie           #+#    #+#             */
/*   Updated: 2026/02/23 04:11:44 by jeremie          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ServerSocket.hpp"

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
	sock = -1;
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
	if (word != ".py" && word != ".c")
		return (announceError("Config error: cgi only allows .py and .c"));
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
	if (!(s >> word))
		return (announceError("Config error: expected ';'"));
	if (word[word.size() -1] == ';')
	{
		word.erase(word.end() - 1);
		return (getMethod(word, location->methods));
	}
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

int	getNumSoft(std::string &word, int &num)
{
	int size = word.size();
	for (int i = 0; i < size; i++)
	{
		if (word[i] < '0' || word[i] > '9')
			return (1);
	}
	long number;
	static std::stringstream ss;
	ss.clear();
	ss.str(word);
	ss >> number;
	if (number < 0 || number >= 2147483647)
		return (1);
	num = (int)number;
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
	{
		epo.data.fd = sock;
		epoll_ctl(epol, EPOLL_CTL_DEL, sock, &epo);	
		close(*i);
	}
	sockets.clear();
}

std::list<ACustomSocket *> socketsToFree;
	std::map<int, ACustomSocket *> allSockets;

int	ServerSocket::startServer(std::list<ACustomSocket *> &socketsToFree, std::map<int, ACustomSocket *> &allSockets)
{
	if (port.empty())
		return (0);
	for (std::set<int>::iterator i = port.begin(); i != port.end(); i++)
	{
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
			return (resetServerSocket(), perror("bind"), 1);
		if (listen(sock, 5) == -1)
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
		if (portsInUse.find(80) == end)
			portsInUse.insert(80);
		else
			portIgnored.insert(80);
		return ;
	}
	for (std::set<int>::iterator i = port.begin(); i != port.end(); i++)
	{
		if (portsInUse.find(*i) == end)
			portsInUse.insert(*i);
		else
			portIgnored.insert(*i);
	}
	for (std::set<int>::iterator i = portIgnored.begin(); i != portIgnored.end(); i++)
		portsInUse.erase(*i);
}

int	ServerSocket::portInfo(int _port) const
{
	if (port.find(_port) != port.end());
		return (1);
	if (portIgnored.find(_port) != port.end())
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
	int maxsize = 0;
	t_locations *found = 0;
	for (std::list<std::pair<std::string, t_locations *> >::iterator i = locations.begin(); i != locations.end(); i++)
	{
		if (target.find(i->first) == 0 && i->first.size() > maxsize)
		{
			maxsize = i->first.size();
			found = i->second;
		}
	}
	if (!found && !serverSettings.root.empty())
		found = &serverSettings;
	return (found);
}

int	ServerSocket::initialChecks(int size, int method, std::string target, int &maxbodysize)
{
	t_locations *toCheck = findLocation(target);
	if (!toCheck )
		return (404);
	int			tmp;
	if (size > 0)
	{
		if (toCheck->client_max_body == -1)
		{
			if (serverSettings.client_max_body == -1)
				tmp = MAX_BODY;
			else
				tmp = serverSettings.client_max_body;
		}
		else
			tmp = toCheck->client_max_body;
		if (size > tmp)
			return (413);
	}
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
		return (405);
	return (0);
}

void	ServerSocket::getTime(std::string &answer)
{
	static std::time_t t;
	static char timeString[std::size("Date: aaa, dd bbb YYYY HH:MM:SS GMT\r\n")];
	static char *data;
	static std::size_t size;
	if (!data)
	{
		data = std::data(timeString);
		size = std::size(timeString);
	}
	t = std::time(0);
	std::strftime(data, size, "Date: %a, %d %b %Y %H:%M:%S GMT\r\n", std::gmtime(&t));
	answer.append(timeString);
}

int	ServerSocket::isCGI(int method, std::string &target, std::string &cgi, std::string &answer)
{
	t_locations	*location = findLocation(target);
	if (!location)
		return (404);
	if ();
}

int	ServerSocket::

int	ServerSocket::fillGet(Message *message, std::string &target, int &writeFile)
{
	t_locations *location = findLocation(target);
	if (!location)
		return (404);
	std::string toSearch;
}

int	ServerSocket::fillAnswer(int method, bool connection, std::string &target, int writeFile, std::string &answer)
{
	
}
	//	check = myServer.fillAnswer(message->getMethod(), message->getConnection(), message->getTarget(), writeFile);

