/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerSocket.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jeremie <jeremie@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/06 17:05:52 by jeremie           #+#    #+#             */
/*   Updated: 2026/02/23 03:53:10 by jeremie          ###   ########.fr       */
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
#include <iterator>
#include "Message.hpp"

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
		int		initialChecks(int, int, std::string, int &);
		int		isCGI(int, std::string &, std::string &);
		int		fillAnswer(int, bool, std::string &, int);
		void	getTime(std::string &);

		int		fillGet(Message *, std::string &, int &);
		int		fillPost(Message *, std::string &, int &);
		int		fillDelete(Message *, std::string &);
		int		fillError(int, bool, std::string , std::string &);
};

int	getNumSoft(std::string &word, int &num);


#endif
