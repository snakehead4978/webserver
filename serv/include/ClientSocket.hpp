/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientSocket.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jeremie <jeremie@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/06 17:27:11 by jeremie           #+#    #+#             */
/*   Updated: 2026/02/25 03:43:29 by jeremie          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENTSOCKET_HPP
# define CLIENTSOCKET_HPP

# include <iostream>
# include "ACustomSocket.hpp"
# include "ServerSocket.hpp"
# include <string.h>
# include <parser.hpp>
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
		size_t			maxBodySize;
		std::list<ServerSocket *>	possibleServers;
		Message			*message;
		int				 error;
		size_t				readTo;
		int				setError(int);
		int				resetWrite(bool, bool);
		int				fillHeaders();
		int				switchToRead();
		int				handleFirstLine();
		int				fillBody();
		int				fillFirstBody();
		int				answerError(int err);
		Cgi				*cgi;
		std::time_t		time;
		t_locations		*location;
		bool			connection;
	public:
		bool			timedOut;
		ClientSocket(int soc);
		void	addToRequest(std::string &str, int);
		int		handleRequest();
		bool	isHeader() const;
		bool	isBigHeader() const;
		int		handleWrite();
		void	firstCheck(int, std::list<ACustomSocket *> &, ServerSocket *);
		void	secondCheck();
		int		checkTime();
		int		timeout();
		~ClientSocket();
		void	appendAnswer(std::string &str);
		int		switchToWrite();
		std::string &getBody();
		Message	*getMessage();
		int		prependAnswer(std::string);
		std::string &getAnswer();
		int		createCgi(std::string &, std::string &);
		int				resetRead();
		int		startCgi();
		int		turnCgi(bool);
		int		cgiError(int err);
		void	delCgi();
		bool	getConnection();
		void	setConnection(bool);


};


#endif
