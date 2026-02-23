/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Settings.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jeremie <jeremie@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/04 12:07:15 by jeremie           #+#    #+#             */
/*   Updated: 2026/02/04 12:07:37 by jeremie          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef __SETTINGS_H
# define __SETTINGS_H

# include <iostream>

class Settings
{
	private:
	public:
		Settings();
		Settings(const Settings& t);
		Settings&	operator=(const Settings& t);
		~Settings();
};

#endif
