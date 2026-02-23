/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jeremie <jeremie@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/04 12:00:55 by jeremie           #+#    #+#             */
/*   Updated: 2026/02/21 10:15:52 by jeremie          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser.hpp"
#include "ServerSocket.hpp"
#include "ClientSocket.hpp"
#include "server.hpp"

int signum = 0;

void killServer(int sig)
{
	std::cout << "Successfully killed" << std::endl;
	(void)sig;
	signum = 1;
}

void signalSetup()
{
	signal(SIGINT, killServer);
}

int handleClient(ClientSocket *client, std::map<int, ACustomSocket *> allSockets, std::list<ACustomSocket *> &socketsToFree)
{
	static std::string	headData(HEAD_BUFF, 0);
	static std::string	maxHeadData(MAX_HEAD_BUFF, 0);
	static std::string	bodyData(BODY_BUFF, 0);
	int errcheck = 1;
	int	readSize;
	std::string *readBuff;
	if (client->timedOut)
		return (client->timeout());
	std::cout << "Hi my socket is " << client->getSock() << std::endl;
	if (!client->isHeader())
	{
		readBuff = &bodyData;	
		readSize = BODY_BUFF - 1;
	}
	else if (client->isBigHeader())
	{
		readBuff = &maxHeadData;
		readSize = MAX_HEAD_BUFF - 1;
	}
	else
	{
		readBuff = &headData;	
		readSize = HEAD_BUFF - 1;
	}
	std::cout << "head buff count is " << readSize << std::endl;
	readBuff->assign(readSize, 0);
	errcheck = read(client->getSock(), &(*readBuff)[0], readSize);
	if (errcheck <= 0)
	{
		if (errcheck == -1)
			std::cerr << "Read error: Client closed" << std::endl;
		else
			std::cerr << "Client has disconnected" << std::endl;
		return (1);
	}
	client->addToRequest(*readBuff);
	if (client->handleRequest())
	{
		std::cout << "Client out" << std::endl;
		return (1);
	}
	return (0);
}

void	closeAllSockets(std::list<ACustomSocket *> &allSockets)
{
	for (std::list<ACustomSocket *>::iterator i = allSockets.begin(); i != allSockets.end(); i++)
		delete (*i);
}

int	addClient(ServerSocket *server, std::map<int, ACustomSocket *> &allSockets, std::list<ACustomSocket *> &socketsToFree)
{
	struct sockaddr_in	addr;
	socklen_t	addrlen = sizeof(addr);
	
	ClientSocket *firefox = new ClientSocket(accept(server->getSock(), (struct sockaddr *)&addr, &addrlen));
	if (firefox->getSock() == -1)
	{
		delete firefox;
		return (1);
	}
	int port = ntohs(addr.sin_port);
	firefox->firstCheck(port, socketsToFree);
	allSockets[firefox->getSock()] = firefox;
	socketsToFree.push_back(firefox);
	return (0);
}

int	ACustomSocket::epol = -1;
struct epoll_event ACustomSocket::epo = {events: 0, data: 0};

static int removeSocketFromLists(ACustomSocket *socket, std::list<ACustomSocket *> &socketsToFree, std::map<int, ACustomSocket *> &allSockets)
{
	allSockets.erase(socket->getSock());
	socketsToFree.remove(socket);
	delete socket;
	return (1);
}

static void	timeoutCheck(std::map<int, ACustomSocket *> &allSockets, std::list<ACustomSocket *> &socketsToFree)
{
	for (std::list<ACustomSocket *>::iterator i = socketsToFree.begin(); i != socketsToFree.end(); i++)
	{
		if ((*i)->isClient() && ((ClientSocket *)(*i))->checkTime())
		{
			removeSocketFromLists(*i, socketsToFree, allSockets);
			timeoutCheck(allSockets, socketsToFree);
			return ;
		}
	}
}

int	server(std::list<ServerSocket *>&servers)
{
	signalSetup();
	ACustomSocket::epol = epoll_create(1);
	if (ACustomSocket::epol == -1)
		return (perror("epoll_create"), 1);
	std::list<ACustomSocket *> socketsToFree;
	std::map<int, ACustomSocket *> allSockets;
	for (std::list<ServerSocket *>::iterator i = servers.begin(); i != servers.end(); i++)
	{
		if ((*i)->startServer(socketsToFree, allSockets))
			return (clearServers(servers), 1);
	}
	struct epoll_event events[5];
	int nEvents;
	std::map<int, ACustomSocket *>::iterator found;
	while (1)
	{
		nEvents = epoll_wait(ACustomSocket::epol, events, 5, 0);
		if (signum == 1)
			break ;
		for (int i = 0; i < nEvents; i++)
		{
			found = allSockets.find(events[i].data.fd);
			if (found->second->isClient())
			{
				if (events[i].events & EPOLLIN)
				{
					if (handleClient((ClientSocket *)found->second, allSockets, socketsToFree))
						removeSocketFromLists(found->second, socketsToFree, allSockets);
				}
				else
				{
					std::cout << "IM writing" << std::endl;
					if (((ClientSocket *)found->second)->handleWrite())
					{
						std::cout << "Client out" << std::endl;
						removeSocketFromLists(found->second, socketsToFree, allSockets);
					}
				}
			}
			else
			{
				if (addClient((ServerSocket *)found->second, allSockets, socketsToFree))
					perror("epoll_ctl");
			}
		}
		timeoutCheck(allSockets, socketsToFree);
	}
	closeAllSockets(socketsToFree);
	return (1);
}
