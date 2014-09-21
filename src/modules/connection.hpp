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

#ifndef LIBSLIRC_HDR_MODULES_CONNECTION_HPP_INCLUDED
#define LIBSLIRC_HDR_MODULES_CONNECTION_HPP_INCLUDED

#include "../apis/connection.hpp"

#include <memory>

#include <boost/thread/mutex.hpp>

namespace slirc { namespace network {
	struct connection;
}}

namespace slirc {
namespace modules {

/**
 * \brief Default module to handle IRC connections.
 */
struct connection: apis::connection {
	/**
	 * \brief Sets up a connection handler to a server.
	 *
	 * \param context The IRC context this module is loaded in. Will be passed
	 *                implicitly when loading the module.
	 * \param hostport The connection string in the form \<hostname\>:\<port\>.
	 *                 Can optionally be prefixed with either "irc://" or, to
	 *                 enable SSL on the connection, with "ircs://".
	 *
	 * \todo Implement SSL.
	 */
	connection(slirc::irc &context, const std::string &hostport);

	// inherited from API
	void connect() override;
	void disconnect() override;
	connection_status status() const override;
	void send(const std::string &data) override;

protected:
	/**
	 * \brief Changes internal status and queues a status change event.
	 *
	 * \param newstatus The status to change to. If this is the same as
	 *                  connstat, no event will be raised.
	 * \param api_mutex_lock A reference to the lock currently holding
	 *                       api_mutex.
	 */
	void change_status(connection_status newstatus, boost::mutex::scoped_lock &api_mutex_lock);

	mutable boost::mutex api_mutex; ///< \brief Mutex guarding conn, connstat and read_buffer.
		std::unique_ptr<network::connection> conn; ///< \brief The network::connection, if one is established.
		connection_status connstat; ///< \brief The current status of the connection.
		std::string read_buffer; ///< \brief The raw data which has been read, but not emitted as an event yet.

	std::string hostname; ///< \brief The hostname from the connection string.
	unsigned port; ///< \brief The port from the connection string.
};

}
}

#endif // LIBSLIRC_HDR_MODULES_CONNECTION_HPP_INCLUDED
