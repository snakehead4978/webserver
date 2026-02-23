/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Message.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jeremie <jeremie@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/04 09:27:00 by jeremie           #+#    #+#             */
/*   Updated: 2026/02/19 06:00:50 by jeremie          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef __MESSAGE_H
# define __MESSAGE_H

# include <iostream>
# include <exception>
# include <string>
# include <map>
# include <cstdlib>
# include <sstream>
# include "ServerSocket.hpp"

enum
{
	GET = 1,
	POST = 2,
	DELETE = 4,
};

class Message
{
	private:
		int	method;
		std::string target;
		std::map<std::string, std::string> headers;
		int	chunked;
		bool	connection;
		std::string host;
		int		size;
		int	changeConnection(std::string &) const;
	public:
		int	getMethod() const;
		void		setMethod(std::string &);
		Message(/* args */);
		~Message();
		Message&	 operator=(const Message& t);
		Message(const Message& t);
		void	setHeaders(const std::string &key, const std::string &value);
		const std::string &getHeader(const std::string &key) const;
		void	printHeaders();
		void	setTarget(std::string );
		std::string	getTarget() const;
		std::string	response;
		bool	getConnection() const;
		bool	getChunked() const;
		std::string &getHost();
		int		parseLine(std::string line);
		int		getSize() const;
};


#endif

