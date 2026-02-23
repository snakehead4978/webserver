/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ACustomSocket.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jeremie <jeremie@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/06 16:49:24 by jeremie           #+#    #+#             */
/*   Updated: 2026/02/21 02:12:24 by jeremie          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ACUSTOMSOCKET_H
# define ACUSTOMSOCKET_H

# include <iostream>
# include <fcntl.h>
# include <unistd.h>
# include <stdio.h>
# include <sys/socket.h>
# include <sys/epoll.h>
# include "server.hpp"
# include <ctime>

enum
{
	OWNER,
	CLIENT
};

class ACustomSocket
{
	protected:
		int type;
		int	sock;
	public:
		static struct epoll_event epo;
		static int epol; 
		ACustomSocket(int typ);
		ACustomSocket(const ACustomSocket& t);
		ACustomSocket&	operator=(const ACustomSocket& t);
		virtual ~ACustomSocket();
		int		getSock(void) const;
		int		isClient() const;
		int 	setNonblock();
		int		addToEpoll();
		void	resetSocket();
		

};

#endif
