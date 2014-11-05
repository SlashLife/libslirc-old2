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

#include "client_to_server.hpp"

#include "../apis/connection.hpp"

#include <utility>

namespace arg = std::placeholders;

slirc::modules::client_to_server::client_to_server(slirc::irc &context)
: apis::protocol(context)
, parserconn(context.attach<apis::connection::raw_irc_line_event>(
	std::bind(&client_to_server::parser, this, arg::_1))) {
}

slirc::modules::client_to_server::~client_to_server() {
	parserconn.disconnect();
}

void slirc::modules::client_to_server::parser(event::pointer ep) {
	const std::string &line = ep->data.get<apis::connection::raw_irc_line>().line;

	ep->queue_as<parsed_event>();

	parameters &prm = ep->data.set(parameters());
		prm.params = irc_split(line);

	if (prm.params.empty()) {
		return;
	}

	do {
		if (
			// !prm.params[0].empty() && // Check unnecessary: Only the last
			//   parameter can be empty if it is the literal extended parameter ":"
			//   The first parameter cannot be an extended parameter, though.
			prm.params[0][0] == ':'
		) {
			origin &org = ep->data.set(origin());
				org.origin_string = prm.params[0].substr(1);

			if (prm.params.size() < 2) break;
			else if (
				prm.params[1].size() == 3 &&
				('0' <= prm.params[1][0] && prm.params[1][0] <= '9') &&
				('0' <= prm.params[1][1] && prm.params[1][1] <= '9') &&
				('0' <= prm.params[1][2] && prm.params[1][2] <= '9')
			) {
				// NUMERIC
				numeric &num = ep->data.set(numeric());
					num.number =
						(prm.params[1][0] - '0') * 100 +
						(prm.params[1][1] - '0') * 10 +
						(prm.params[1][2] - '0') * 1;
				ep->queue_as<numeric_event>();
			}
			else if (prm.params[1] == "QUIT") {
				if (2 < prm.params.size()) {
					message &msg = ep->data.set(message());
						msg.raw = prm.params[2];
				}
				ep->queue_as<quit_event>();
			}

			else if (prm.params.size() < 3) break;
			else if (prm.params[1] == "NICK") {
				nick_change &nch = ep->data.set(nick_change());
					nch.old_nick = prm.params[0].substr(0, prm.params[0].find('!'));
					nch.new_nick = prm.params[2];
				ep->queue_as<nick_event>();
			}
			else if (prm.params[1] == "PART") {
				recipient &rcp = ep->data.set(recipient());
					rcp.recipient_string = prm.params[2];
				if (3 < prm.params.size()) {
					message &msg = ep->data.set(message());
						msg.raw = prm.params[3];
				}
				ep->queue_as<quit_event>();
			}
		}
		else {
			// Check for commands ... well ... at least for what we know:

			if (prm.params.size() < 2) break;
			else if (prm.params[0] == "PING" && prm.params) {
				message &msg = ep->data.set(message());
					msg.raw = prm.params[1];
				ep->queue_as<ping_event>();
			}
		}
	} while(0);
}
