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

#ifndef LIBSLIRC_HDR_EXCEPTIONS_NO_TAG_HPP_INCLUDED
#define LIBSLIRC_HDR_EXCEPTIONS_NO_TAG_HPP_INCLUDED

#include <stdexcept>

namespace slirc {
namespace exceptions {

/**
 * \brief Thrown by helper::tag_container::get() if the requested tag does not
 *        exist.
 */
struct no_tag : std::range_error {
	inline no_tag()
	: std::range_error("Supplied tag does not exist.") {}
};

}
}

#endif // LIBSLIRC_HDR_EXCEPTIONS_NO_TAG_HPP_INCLUDED
