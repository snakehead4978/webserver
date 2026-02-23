/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ACustomSocket.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jeremie <jeremie@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/06 16:52:30 by jeremie           #+#    #+#             */
/*   Updated: 2026/02/06 17:04:36 by jeremie          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ACustomSocket.hpp"

ACustomSocket::ACustomSocket(int typ) : type(typ), sock(-1) {}

ACustomSocket::~ACustomSocket()
{
	if (sock == -1)
		return ;
	close(sock);
}

ACustomSocket::ACustomSocket(const ACustomSocket& t) : type(t.type), sock(t.sock) {}

ACustomSocket&	 ACustomSocket::operator=(const ACustomSocket& t)
{
	if (this != &t)
	{
		sock = t.sock;
		type = t.type;
	}
	return (*this);
}

int	ACustomSocket::getSock(void) const
{
	return (sock);
}

int ACustomSocket::setNonblock()
{
	if (sock == -1)
		return (1);
	int flags = fcntl(sock, F_GETFL, 0);
	if (flags == -1)
		return (perror("fcntl"), 1);
	if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1)
		return (perror("fcntl"), 1);	
	return (0);
}

int	ACustomSocket::isClient() const
{
	return (type);
}

int	ACustomSocket::addToEpoll()
{
	epo.data.fd = sock;
	epo.events = EPOLLIN;
	if (epoll_ctl(epol, EPOLL_CTL_ADD, sock, &epo))
		return (perror("epoll_ctl"), 1);
	return (0);
}

void	ACustomSocket::resetSocket()
{
	if (sock == -1)
		return ;
	close(sock);
	sock = -1;
}

