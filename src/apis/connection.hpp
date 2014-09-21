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

#ifndef LIBSLIRC_HDR_APIS_CONNECTION_HPP_INCLUDED
#define LIBSLIRC_HDR_APIS_CONNECTION_HPP_INCLUDED

#include <string>

#include "../event.hpp"
#include "../module_api.hpp"

namespace slirc {
namespace apis {

/**
 * \brief Module API for IRC connections.
 */
struct connection: module_api<slirc::apis::connection> {
	using module_api::module_api;

	/**
	 * \brief Connection status
	 *
	 * Valid transistions:
	 * - disconnected -> connecting (attempting to establish a connection)
	 * - connecting -> connected (connection attempt successful)
	 * - connecting -> disconnecting (connection attempt was aborted)
	 * - connecting -> disconnected (connection attempt failed)
	 * - connected -> disconnecting (established connection is shut down)
	 * - connected -> disconnected (unexpected connection loss, e.g. timeout)
	 * - disconnecting -> disconnected (connection shutdown complete)
	 */
	enum class connection_status {
		disconnected, ///< No connection exists.
		connecting, ///< The connection is currently being established.
		connected, ///< Connection is established.
		disconnecting ///< Connection is shutting down.
	};

	/**
	 * \brief Connect to the IRC server.
	 */
	virtual void connect() = 0;

	/**
	 * \brief Disconnect from the IRC server.
	 */
	virtual void disconnect() = 0;

	/**
	 * \brief Check the status of the connection.
	 *
	 * \return The current connection status.
	 */
	virtual connection_status status() const = 0;

	/**
	 * \brief Send some data over the connection.
	 *
	 * \param data The data to send.
	 */
	virtual void send(const std::string &data) = 0;

	/**
	 * \brief Event that is raised when the connection status changes.
	 *
	 * The details are attached in a status_change tag.
	 */
	struct status_change_event: event::type {};

	/**
	 * \brief Event tag containing the status change details.
	 *
	 * Attached to status_change_event.
	 */
	struct status_change {
		connection_status old_status; ///< The previous connection status.
		connection_status new_status; ///< The new connection status.
	};

	/**
	 * \brief Event that is raised when a line is received.
	 *
	 * The raw IRC line is attached in a raw_irc_line tag.
	 */
	struct raw_irc_line_event: event::type {};

	/**
	 * \brief Event tag containing raw network data.
	 *
	 * Attached to raw_irc_line_event.
	 */
	struct raw_irc_line {
		/**
		 * The raw IRC line, only stripped of leading white space as well as
		 * the line ending delimiters.
		 */
		std::string line;
	};
};

}
}

#endif // LIBSLIRC_HDR_APIS_CONNECTION_HPP_INCLUDED
