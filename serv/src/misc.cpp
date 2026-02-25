/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   misc.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jeremie <jeremie@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/23 10:45:33 by jeremie           #+#    #+#             */
/*   Updated: 2026/02/25 07:11:44 by jeremie          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "misc.hpp"

std::string	convert(int num)
{
	static std::stringstream ss;
	ss.str("");
	ss.clear();
	ss << num;
	return (ss.str());
}

int	getNumSoft(std::string &word, int &num)
{
	int size = word.size();
	for (int i = 0; i < size; i++)
	{
		if (word[i] < '0' || word[i] > '9')
			return (1);
	}
	long number;
	static std::stringstream ss;
	ss.clear();
	ss.str(word);
	ss >> number;
	if (number < 0 || number >= 2147483647)
		return (1);
	num = (int)number;
	return (0);
}

bool isWhiteSpace(char c)
{
	if (c == ' ' || (c <= 13 && c >= 9))
		return (1);
	return (0);
}

int firstChar(std::string &line)
{
	int n = 0;
	while (line[n])
	{
		if (isWhiteSpace(line[n]))
			n++;
		else
			break ;
	}
	return (n);
}

static bool validBoundaryChar(char c)
{
	if (c >= 'a' && c <= 'z')
		return (true);
	if (c >= 'A' && c <= 'Z')
		return (true);
	if (c >= '0' && c <= '9')
		return (true);
	std::string special("'()+_,-./:=? ");
	return (special.find(c) != std::string::npos);
}

int	setBoundary(std::string &boundary)
{
	if (boundary.find("boundary=") != 0)
		return (1);
	boundary = boundary.substr(9);
	if (boundary.empty() || boundary.size() > 70)
		return (1);
	if (isWhiteSpace(boundary[boundary.size() - 1]))
		return (1);
	for (size_t i = 0; i < boundary.size(); i++)
	{
		if (!validBoundaryChar(boundary[i]))
			return (1);
	}
	return (0);
}
