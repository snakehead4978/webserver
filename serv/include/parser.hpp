/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jeremie <jeremie@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 19:12:53 by jeremie           #+#    #+#             */
/*   Updated: 2026/02/23 16:05:16 by jeremie          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PARSER_H
# define PARSER_H
#include "Message.hpp"

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <cstring>
#include <unistd.h>
#include <csignal>
#include <sys/epoll.h>
#include <set>
#include <fcntl.h>
#include <list>
#include "ServerSocket.hpp"
// int	server(Settings &setting);

class ServerSocket;

int	server(std::list<ServerSocket *>&);
bool isEmptyLine(std::string &line);
int getNextLine(std::ifstream &conf, std::string &line);
int announceError(std::string &, bool line = true);
int announceError(const char *, bool line = true);
void	clearServers(std::list<ServerSocket *> &servers);






#endif