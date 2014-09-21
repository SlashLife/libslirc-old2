/***************************************************************************
**  Copyright 2014-2014 by Simon "SlashLife" Stienen                      **
**  http://projects.slashlife.org/libslirc/                               **
**  libslirc@projects.slashlife.org                                       **
**                                                                        **
**  This file is part of libslIRC.                                        **
**                                                                        **
**  libslIRC is free software: you can redistribute it and/or modify      **
**  it under the terms of the GNU Lesser General Public License as        **
**  published by the Free Software Foundation, either version 3 of the    **
**  License, or (at your option) any later version.                       **
**                                                                        **
**  libslIRC is distributed in the hope that it will be useful,           **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of        **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
**  GNU General Public License for more details.                          **
**                                                                        **
**  You should have received a copy of the GNU General Public License     **
**  and the GNU Lesser General Public License along with libslIRC.        **
**  If not, see <http://www.gnu.org/licenses/>.                           **
***************************************************************************/

#ifndef LIBSLIRC_HDR_MODULE_API_HPP_INCLUDED
#define LIBSLIRC_HDR_MODULE_API_HPP_INCLUDED

#include "module.hpp"

namespace slirc {

/**
 * \brief Base class to mark module API interfaces consistently through CRTP.
 *
 * All implementations sharing the same module_api<> base class are mutually
 * exclusive on a single IRC context; only one of them can be loaded at any
 * time.
 *
 * Use: <tt>struct my_api_interface: slirc::module_api<my_api_interface> { ... };</tt>
 */
template<typename ApiInterface>
struct module_api: module {
	/**
	 * \brief The tag
	 */
	typedef ApiInterface module_api_type;

	/**
	 * \brief Constructor forward to common base class.
	 */
	// DO NOT REMOVE! This is here to force documentation to be generated
	inline
	module_api(slirc::irc &irc): module(irc) {}
};

}

#endif // LIBSLIRC_HDR_MODULE_API_HPP_INCLUDED
