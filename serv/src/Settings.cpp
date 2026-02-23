/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Settings.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jeremie <jeremie@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/04 12:07:27 by jeremie           #+#    #+#             */
/*   Updated: 2026/02/05 14:28:06 by jeremie          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Settings.hpp"

Settings::Settings() { std::cout << "Default constructor called\n"; }

Settings::~Settings() { std::cout << "Destructor called\n"; }

Settings::Settings(const Settings& t) { std::cout << "Copy constructor called\n"; }

Settings&	 Settings::operator=(const Settings& t)
{
	std::cout << "Copy assignment operator called\n";
	return (*this);
}


