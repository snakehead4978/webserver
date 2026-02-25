/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   misc.hpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jeremie <jeremie@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/23 10:47:08 by jeremie           #+#    #+#             */
/*   Updated: 2026/02/25 10:58:30 by jeremie          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MISC_HPP
# define MISC_HPP


#include <string>
#include <sstream>

# define HEAD_BUFF 1000
# define MAX_HEAD_BUFF 8000
# define MAX_HEAD_NUM 4
# define BODY_BUFF 16000
# define MAX_BODY 1000000
# define MAX_EVENTS 64
# define DEFAULT_PORT 3030

enum
{
	GET = 1,
	POST = 2,
	DELETE = 4,
};

int firstChar(std::string &line);
int	getNumSoft(std::string &word, int &num);
std::string	convert(int num);
bool isWhiteSpace(char c);
int	setBoundary(std::string &boundary);







#endif