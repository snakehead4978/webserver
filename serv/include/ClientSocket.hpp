/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientSocket.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jeremie <jeremie@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/06 17:27:11 by jeremie           #+#    #+#             */
/*   Updated: 2026/02/23 01:04:02 by jeremie          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENTSOCKET_HPP
# define CLIENTSOCKET_HPP

# include <iostream>
# include "ACustomSocket.hpp"
# include "ServerSocket.hpp"
# include <string.h>
# include <parser.hpp>
# include <sys/stat.h>
# include <cstdlib>
# include "Message.hpp"
# include "Cgi.hpp"

# define OUTPUT_BUFF 32000

class ClientSocket : public ACustomSocket
{
	private:
		ServerSocket	*myServer;
		std::string 	request;
		bool			header;
		bool			bigHeader;
		std::string		body;
		bool			readDone;
		int				nRead;
		int				nWrite;
		int				writeFile;
		std::string		answer;
		int				writeSize;
		int				port;
		int				check;
		int				maxBodySize;
		std::list<ServerSocket *>	possibleServers;
		Message			*message;
		int				 error;
		int				readTo;
		int				setError(int);
		int				resetRead();
		int				resetWrite(bool, bool);
		int				fillHeaders();
		int				switchToRead();
		int				switchToWrite();
		int				handleFirstLine();
		int				fillBody();
		int				fillFirstBody();
		int				answerError(int err);
		Cgi				*cgi;
		std::time_t		time;
	public:
		bool			timedOut;
		ClientSocket(int soc);
		void	addToRequest(std::string &str);
		int		handleRequest();
		bool	isHeader() const;
		bool	isBigHeader() const;
		int		handleWrite();
		void	firstCheck(int, std::list<ACustomSocket *> &);
		void	secondCheck();
		int		checkTime();
		int		timeout();
		~ClientSocket();
};


#endif
