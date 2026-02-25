/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ACustomSocket.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jeremie <jeremie@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/06 16:49:24 by jeremie           #+#    #+#             */
/*   Updated: 2026/02/24 16:04:57 by jeremie          ###   ########.fr       */
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
# include <ctime>
# include <list>
# include <map>

enum
{
	OWNER,
	CLIENT,
	CGI
};

class ACustomSocket
{
	protected:
		int type;
		int	sock;
	public:
		static std::map<int, ACustomSocket *> allSockets;
		static std::list<ACustomSocket *>socketsToFree;
		static int epol; 
		ACustomSocket(int typ);
		virtual ~ACustomSocket();
		int		getSock(void) const;
		int		isWhat() const;
		int 	setNonblock();
		int		addToEpoll();
		void	resetSocket();
		

};

#endif
