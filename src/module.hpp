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

#ifndef LIBSLIRC_HDR_MODULE_HPP_INCLUDED
#define LIBSLIRC_HDR_MODULE_HPP_INCLUDED

#include <boost/noncopyable.hpp>

namespace slirc {

struct irc;

template<typename T>
class module_api;

/**
 * \brief Base class for all loadable modules.
 *
 * Modules are constructed and destroyed through the module API of the
 * IRC context (slirc::irc).
 *
 * \note Do not inherit from this class directly;
 *       inherit from module_api<T> instead.
 */
struct module: private boost::noncopyable {
protected:
	/**
	 * \brief Constructs the module and binds it to the given IRC context.
	 */
	inline
	module(slirc::irc &irc)
	: irc(irc) {}

public:
	/**
	 * \brief Virtual default destructor for polymorphic base class.
	 */
	virtual ~module() = default;

	/**
	 * \brief A reference to the IRC context this module is bound to.
	 */
	slirc::irc &irc;
};

}

#endif // LIBSLIRC_HDR_MODULE_HPP_INCLUDED
