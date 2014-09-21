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

#ifndef LIBSLIRC_HDR_NETWORK_HPP_INCLUDED
#define LIBSLIRC_HDR_NETWORK_HPP_INCLUDED

namespace boost { namespace asio {
	struct io_service;
}}

/// \namespace slirc::network General network handling
namespace slirc {
namespace network {

/**
 * \brief Enumeration for the network handling modes.
 */
enum class handling_mode {
	automatic, ///< libslirc handles network i/o processing itself
	manual ///< network i/o processing has to be manually invoked by the user
};

/**
 * \brief Manually run network tasks.
 *
 * \note This request will be ignored if the handling mode is currently set to
 *       handling_mode::automatic.
 */
void run();

/**
 * \brief Sets whether network should be handled manually or automatically.
 *
 * \param mode Set to handling_mode::manual to handle the network yourself.
 *             Network events are only handled when you call run() manually.
 *             Set to handling_mode::automatic (default) to have libslirc
 *             handle the network in a separate thread.
 *
 * \note May not be called from within the internal network thread.
 */
void set_handling_mode(handling_mode mode = handling_mode::manual);

/**
 * \brief Returns the current state of the handling mode.
 *
 * This may need some time to update after the handling mode has been changed.
 *
 * \return \c handling_mode::automatic if networking is currently handled
 *         automatically, \c handling_mode::manual if the network is currently
 *         handled manually.
 */
handling_mode current_handling_mode();

/**
 * \brief A reference to internally used the Boost.ASIO io_service object.
 */
extern boost::asio::io_service &service;

}
}

#endif // LIBSLIRC_HDR_NETWORK_HPP_INCLUDED
