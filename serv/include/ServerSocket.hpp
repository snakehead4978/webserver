/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerSocket.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jeremie <jeremie@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/06 17:05:52 by jeremie           #+#    #+#             */
/*   Updated: 2026/02/25 08:49:07 by jeremie          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVERSOCKET_H
# define SERVERSOCKET_H

# include "ACustomSocket.hpp"
# include <iostream>
#include <netinet/ip.h>
#include <string.h>
#include <list>
#include "parser.hpp"
#include <set>
#include "Message.hpp"
#include <sys/stat.h>
#include <dirent.h>

class ClientSocket;
class Cgi;

typedef struct s_locations
{
	int	client_max_body;
	std::string root;
	int	methods;
	std::pair<int, std::string> redirection;
	std::map<std::string, std::string> cgi;
	int autoindex;
	std::list<std::string> index;
	std::string uploads;
	std::string path;
	std::list<std::pair<int, std::string> > errors;
}	t_locations;

class ServerSocket : public ACustomSocket
{
	private:
		std::set<int>	port;
		std::set<std::string>	hostname;
		t_locations serverSettings;
		std::list<std::pair<std::string, t_locations *> > locations;
		std::set<int>	portIgnored;
		std::set<int>	sockets;
		t_locations *findLocation(std::string &);
		int	handleDirectory(std::string &, Message *, std::string &, int &, t_locations *, bool &);
	public:
		ServerSocket();
		~ServerSocket();
		void printServer();
		int	startServer(std::list<ACustomSocket *> &socketsToFree, std::map<int, ACustomSocket *> &allSockets);
		int	fillServer(std::ifstream &conf);
		int	fillLocation(std::ifstream &conf, std::stringstream &line);
		int	parseServerLine(std::ifstream &conf, std::string &line, bool &);
		int	parseLocationLine(std::string &line, bool &, t_locations *);
		void	checkPorts(std::set<int>&);
		void	resetServerSocket();
		int		portInfo(int) const;
		int		checkHostname(std::string &);
		int		initialChecks(int, int, size_t &, t_locations *);
		int		isCGI(Message *, t_locations *, std::string &, std::string &);
		void	getTime(std::string &);
		int		fillGet(Message *, std::string &, int &, t_locations *, bool &);
		int		fillPost(Message *, std::string &, std::string &, t_locations *, bool &);
		int		fillDelete(Message *, std::string &, t_locations *, bool &);
		int		fillError(int, Message *, std::string &, int &, t_locations **, bool &);
		int 	fillAnswer(int status, bool connection, std::string &answer, bool &conn, int contentLength = 0, std::string contentType = "", std::string location = "");
		int		setLocation(Message *message, t_locations **location);
		int		fillCgi(t_locations *, std::string &Cgipath, std::string &rootPath, std::string &answer, ClientSocket *, bool &);
		int		redirected(t_locations *location, std::string &answer, Message *message, bool &);
};

#endif
