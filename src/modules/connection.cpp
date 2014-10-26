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

#include "connection.hpp"

#include <cassert>

#include "../irc.hpp"
#include "../network/connection.hpp"

namespace {
	const std::string whitespace("\0\t\r\n ", 5);
	const std::string lineending("\r\n", 2);
}

slirc::modules::connection::connection(slirc::irc &context, const std::string &hostport)
: apis::connection(context)
, conn()
, connstat(connection_status::disconnected)
, hostname(hostport)
, port(6667) {
	if (hostname.substr(0, 6) == "irc://") {
		hostname.erase(0, 6);
	}
	else if (hostname.substr(0, 7) == "ircs://") {
		hostname.erase(0, 7);
		// todo: enable SSL
	}

	std::string::size_type pos = hostname.find_last_not_of("0123456789");
	if (pos != hostname.npos && pos != hostname.size()-1 && hostname[pos] == ':') {
		// Yay! port number!
		port = std::stoul(hostname.substr(pos+1));
		hostname.erase(pos);
	}
}

void slirc::modules::connection::connect() {
	boost::mutex::scoped_lock lock(api_mutex);
	if (connstat != connection_status::disconnected) {
		return;
	}
	assert(!conn);
	change_status(connection_status::connecting, lock);

	conn.reset(new network::connection());
	conn->on_status([&](const boost::system::error_code &error) {
		boost::mutex::scoped_lock lock(api_mutex);
		if (error) {
			change_status(connection_status::disconnected, lock);
		}
		else if (connstat == connection_status::connecting) {
			change_status(connection_status::connected, lock);
		}
	});
	conn->on_recv([&](const std::string &netdata){
		read_buffer += netdata;
		std::string::size_type pos;
		while (read_buffer.npos != (pos = read_buffer.find_first_of(lineending))) {
			std::string line = read_buffer.substr(0, pos);
			read_buffer.erase(0, pos+1);

			pos = line.find_first_not_of(whitespace);
			if (pos != line.npos) {
				line.erase(0, pos);

				event::pointer pe = event::create<raw_irc_line_event>();
				{ raw_irc_line tag_ril;
					tag_ril.line = line;
					pe->data.set(tag_ril);
				}
				irc.queue_event(pe);
			}
		}
	});
	conn->connect(hostname, port);
}

void slirc::modules::connection::disconnect() {
	boost::mutex::scoped_lock lock(api_mutex);
	if (conn) {
		conn->disconnect();
	}
}

slirc::apis::connection::connection_status slirc::modules::connection::status() const {
	boost::mutex::scoped_lock lock(api_mutex);
	return connstat;
}

void slirc::modules::connection::send(const std::string &data) {
	boost::mutex::scoped_lock lock(api_mutex);
	if (conn && connstat == connection_status::connected) {
		conn->send(data);
	}
}

void slirc::modules::connection::change_status(connection_status newstatus, boost::mutex::scoped_lock &api_mutex_lock) {
	static_cast<void>(api_mutex_lock); // possibly unused parameter in NDEBUG
	assert(api_mutex_lock);

	if (newstatus != connstat) {
		event::pointer pe = event::create<status_change_event>();
		{ status_change tag_sc;
			tag_sc.old_status = connstat;
			tag_sc.new_status = newstatus;
			pe->data.set(tag_sc);
		}
		connstat = newstatus;
		irc.queue_event(pe);
	}
}
