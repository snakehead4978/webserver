/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Message.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jeremie <jeremie@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/04 09:27:00 by jeremie           #+#    #+#             */
/*   Updated: 2026/02/25 07:13:09 by jeremie          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef __MESSAGE_H
# define __MESSAGE_H

# include <iostream>
# include <exception>
# include <map>
# include "misc.hpp"
#include <list>


class Message
{
	private:
		int	method;
		std::string target;
		std::map<std::string, std::list<std::string> > headers;
		int		size;
	public:
		int	getMethod() const;
		void		setMethod(std::string &);
		Message();
		~Message();
		void	setTarget(std::string );
		std::string	&getTarget();
		std::string	response;
		bool	getConnection();
		int		parseLine(std::string line);
		int		getSize() const;
		const std::string &getHeader(const std::string &key) const;
		bool	headerExists(const std::string &key);
		const std::list<std::string> &getHeaders(const std::string &key) const;
		int		finalChecks();
		std::string getBoundary();
		std::map<std::string, std::list<std::string> >&getHeaderMap();
		int	getContentTypeSize();
	
};

#endif

