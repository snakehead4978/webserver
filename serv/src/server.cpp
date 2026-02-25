/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jeremie <jeremie@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/04 12:00:55 by jeremie           #+#    #+#             */
/*   Updated: 2026/02/25 09:34:01 by jeremie          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parser.hpp"
#include "ServerSocket.hpp"
#include "ClientSocket.hpp"
#include "Cgi.hpp"

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

static int handleClient(ClientSocket *client)
{
	static std::string	headData(HEAD_BUFF, 0);
	static std::string	maxHeadData(MAX_HEAD_BUFF, 0);
	static std::string	bodyData(BODY_BUFF, 0);
	int errcheck = 1;
	int	readSize;
	std::string *readBuff;
	if (client->timedOut)
		return (client->timeout());
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
	client->addToRequest(*readBuff, errcheck);
	if (client->handleRequest())
	{
		std::cout << "Client out" << std::endl;
		return (1);
	}
	return (0);
}

static void	closeAllSockets(std::list<ACustomSocket *> &allSockets)
{
	if (allSockets.empty())
		return ;
	for (std::list<ACustomSocket *>::iterator i = allSockets.begin(); i != allSockets.end(); i++)
		delete (*i);
}

static int	addClient(ServerSocket *server, std::list<ACustomSocket *> &socketsToFree, int serverFd)
{
	struct sockaddr_in	addr;
	socklen_t	addrlen = sizeof(addr);
	
	ClientSocket *firefox = new ClientSocket(accept(serverFd, (struct sockaddr *)&addr, &addrlen));
	if (firefox->getSock() == -1)
	{
		delete firefox;
		return (1);
	}
	getsockname(serverFd, (struct sockaddr *)&addr, &addrlen);
	int port = ntohs(addr.sin_port);
	firefox->firstCheck(port, socketsToFree, server);
	server->allSockets[firefox->getSock()] = firefox;
	socketsToFree.push_back(firefox);
	return (0);
}

int	ACustomSocket::epol = -1;

static int removeSocketFromLists(ACustomSocket *socket, std::list<ACustomSocket *> &socketsToFree)
{
	std::cerr << "removing fd=" << socket->getSock() << " type=" << socket->isWhat() << std::endl;
	ACustomSocket::allSockets.erase(socket->getSock());
	socketsToFree.remove(socket);
	delete socket;
	return (1);
}

static void	timeoutCheck(std::list<ACustomSocket *> &socketsToFree)
{
	std::vector<ACustomSocket *> toRemove;
	for (std::list<ACustomSocket *>::iterator i = socketsToFree.begin(); i != socketsToFree.end(); i++)
	{
		if ((*i)->isWhat() == CLIENT && ((ClientSocket *)(*i))->checkTime())
			toRemove.push_back(*i);
	}
	for (std::vector<ACustomSocket *>::iterator i = toRemove.begin(); i != toRemove.end(); i++)
		removeSocketFromLists(*i, socketsToFree);
	toRemove.clear();
	for (std::map<int, ACustomSocket *>::iterator i = ACustomSocket::allSockets.begin(); i != ACustomSocket::allSockets.end(); i++)
	{
		if (i->second->isWhat() == CGI && ((Cgi *)i->second)->checkTime())
			toRemove.push_back(i->second);
	}
	for (std::vector<ACustomSocket *>::iterator i = toRemove.begin(); i != toRemove.end(); i++)
	{
		((Cgi *)*i)->sendTimeout();
		delete *i;
	}
}

int	server(std::list<ServerSocket *>&servers)
{
	signalSetup();
	ACustomSocket::epol = epoll_create(1);
	if (ACustomSocket::epol == -1)
		return (perror("epoll_create"), 1);
	if (servers.empty())
		return (1);
	for (std::list<ServerSocket *>::iterator i = servers.begin(); i != servers.end(); i++)
	{
		if ((*i)->startServer(ACustomSocket::socketsToFree, ACustomSocket::allSockets))
			return (clearServers(servers), 1);
	}
	struct epoll_event events[10];
	int nEvents;
	std::map<int, ACustomSocket *>::iterator found;
	while (1)
	{
		nEvents = epoll_wait(ACustomSocket::epol, events, MAX_EVENTS, 100);
		if (signum == 1)
			break ;
		for (int i = 0; i < nEvents; i++)
		{
			found = ACustomSocket::allSockets.find(events[i].data.fd);
			if (found == ACustomSocket::allSockets.end())
				continue ;
			if (found->second->isWhat() == CLIENT)
			{
				if (events[i].events & (EPOLLHUP | EPOLLERR))
				{
					removeSocketFromLists(found->second, ACustomSocket::socketsToFree);
					continue ;
				}
				if (events[i].events & EPOLLIN)
				{
					if (handleClient((ClientSocket *)found->second))
						removeSocketFromLists(found->second, ACustomSocket::socketsToFree);
				}
				else
				{
					if (((ClientSocket *)found->second)->handleWrite())
					{
						std::cout << "Client out" << std::endl;
						removeSocketFromLists(found->second, ACustomSocket::socketsToFree);
					}
				}
			}
			else if (found->second->isWhat() == OWNER)
			{
				if (addClient((ServerSocket *)found->second, ACustomSocket::socketsToFree, found->first))
					perror("epoll_ctl");
			}
			else
			{
				Cgi *cgi = (Cgi *)found->second;
				if (events[i].events & (EPOLLIN | EPOLLHUP | EPOLLERR))
				{
					if (cgi->handleRead())
						delete cgi;
				}
				else if (events[i].events & EPOLLOUT)
				{
					if (cgi->handleWrite())
						delete cgi;	
				}
			}
		}
		timeoutCheck(ACustomSocket::socketsToFree);
	}
	closeAllSockets(ACustomSocket::socketsToFree);
	return (1);
}
