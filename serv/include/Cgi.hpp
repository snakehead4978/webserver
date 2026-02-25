/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jeremie <jeremie@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/23 01:00:25 by jeremie           #+#    #+#             */
/*   Updated: 2026/02/24 14:31:34 by jeremie          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGI_HPP
# define CGI_HPP

# include <iostream>
# include "ACustomSocket.hpp"
# include <unistd.h>
# include <signal.h>
# include <sys/wait.h>
# include "misc.hpp"
# include <ctime>
# include <vector>
# include <cstdlib>

class ClientSocket;

class Cgi : public ACustomSocket
{
	private:
		ClientSocket	*client;
		pid_t pid;
		int	pipeIn[2];
		int	pipeOut[2];
		int	sockWrite;
		size_t	offset;
		std::string inter;
		std::string path;
		std::time_t time;
		std::string port;
		bool	cleaned;
		int	buildEnv();
	public:
		Cgi(ClientSocket *client, std::string &, std::string &, std::string &);
		~Cgi();
		int	execute();
		int handleRead();
		int	handleWrite();
		int		checkTime();
		void	sendTimeout();
		void	buildHeaders(std::vector<std::string>&);
		int		cgiExit(bool ex = true);
};

#endif
