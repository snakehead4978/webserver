/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jeremie <jeremie@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/11/09 18:46:02 by jeremie           #+#    #+#             */
/*   Updated: 2026/02/25 10:40:06 by jeremie          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser.hpp"
#include "Message.hpp"
#include "ACustomSocket.hpp"
#include "ServerSocket.hpp"

int lineNum;

int	announceError(const char *error, bool line)
{
	if (line)
		std::cerr << error << " line " << lineNum << std::endl;
	else
		std::cerr << error  << std::endl;
	return (1);
}

int announceError(std::string &error, bool line)
{
	if (line)
		std::cerr << error << " line " << lineNum << std::endl;
	else
		std::cerr << error  << std::endl;
	return (1);
}

int	firstparse(std::string &s, Message &msg)
{
	std::stringstream content(s);
	std::string method;
	content >> method;
	// std::cout << "Method is " << method << std::endl;
	try
	{
		msg.setMethod(method);
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		return (1);
	}
	return (0);
}

bool isEmptyLine(std::string &line)
{
	int size = line.size();
	for (int i = 0; i < size; i++)
	{
		if (line[i] == ' ' || (line[i] >= 9 && line[i] <= 13))
			continue ;
		return (0);
	}
	return (1);
}

void	clearServers(std::list<ServerSocket *> &servers)
{
	if (servers.empty())
		return ;
	for (std::list<ServerSocket *>::iterator i = servers.begin(); i != servers.end(); i++)
		delete *i;
	servers.clear();
}

int	parseLine(std::string &line, std::ifstream &conf, std::list<ServerSocket *> &servers)
{
	std::stringstream s(line);
	std::string	word;
	s >> word;
	if (word == "server")
	{
		ServerSocket *server = new ServerSocket;
		if (server->fillServer(conf))
		{
			delete server;
			return (1);
		}
		servers.push_back(server);
		return (0);
	}
	std::cerr << "Config error: expected 'server' but found " << word << " instead" << std::endl; 
	return (1);
}

int getNextLine(std::ifstream &conf, std::string &line)
{
	while (!conf.eof())
	{
		std::getline(conf, line);
		lineNum++;
		if (conf.fail())
		{
			std::cerr << "Config error: Ifstream error" << std::endl;
			return (1);
		}
		if (!isEmptyLine(line))
			return (0);
	}
	std::cerr << "Config error: expected '}'" << std::endl;
	return (1);
}

int	parseConfig(std::string filename, std::list<ServerSocket *> &servers)
{
	std::ifstream conf(filename.c_str(), std::ios::binary);
	if (!conf.is_open())
		return (perror("ifstream"), 1);
	std::string line;
	while (std::getline(conf, line))
	{
		lineNum++;
		if (conf.fail())
			return (clearServers(servers), 1);
		if (isEmptyLine(line))
			continue ;
		if (parseLine(line, conf, servers))
			return (clearServers(servers), 1);
	}
	return (0);
}

void printServers(std::list<ServerSocket *> &servers)
{
	if (servers.empty())
		return ;
	for (std::list<ServerSocket *>::iterator i = servers.begin(); i != servers.end(); i++)
		(*i)->printServer();
}

static void pruneServers(std::list<ServerSocket *> &servers)
{
	std::set<int> ports_in_use;
	if (servers.empty())
		return ;
	for (std::list<ServerSocket *>::iterator i = servers.begin(); i != servers.end(); i++)
		(*i)->checkPorts(ports_in_use);	
}

int	main(int ac, char **av)
{
	std::cout << "Hi program has started, Good Luck!" << std::endl;
	if (!ac || ac > 2)
	{
		std::cerr << "Invalid arguments" << std::endl;
		return (1);
	}
	lineNum = 0;
	std::list<ServerSocket *> servers;
	std::string filepath;
	if (ac == 2)
		filepath = av[1];
	else
		filepath = "./conf/server2.conf";
	if (parseConfig(filepath, servers))
		return (1);
	printServers(servers);
	pruneServers(servers);
	int err = server(servers);
	if (ACustomSocket::epol != -1)
		close(ACustomSocket::epol);
	std::cout << std::endl << "Program has terminated." << std::endl;
	return (err);
}
