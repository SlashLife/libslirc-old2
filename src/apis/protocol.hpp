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

#ifndef LIBSLIRC_HDR_APIS_PROTOCOL_HPP_INCLUDED
#define LIBSLIRC_HDR_APIS_PROTOCOL_HPP_INCLUDED

#include <string>
#include <vector>

#include "../event.hpp"
#include "../module_api.hpp"

namespace slirc {
namespace apis {

/**
 * \brief Module API for protocol parsers.
 */
struct protocol: module_api<slirc::apis::protocol> {
	using module_api::module_api;

///////////////////////////////////////////////////////////////////////////////
// Defined tags

	/**
	 * \brief Event tag containing the parameters of the raw message.
	 */
	struct parameters {
		/// The parameters extracted from the message according to the protocol.
		std::vector<std::string> params;
	};

	/**
	 * \brief Event tag specifying the origin of a message.
	 *
	 * The origin of a message is the person (or server) who caused it.
	 *
	 * Commands are not tagged with an origin.
	 */
	struct origin {
		/// The verbatim user mask of the sender.
		std::string origin_string;
		// /// A pointer to the user object of the sender (if any).
		// user::pointer origin_user;
	};

	/**
	 * \brief Event tag containing the text message.
	 */
	struct message {
		/// The original (binary) message attached with the event.
		std::string raw;
		// TODO: parsed
	};



///////////////////////////////////////////////////////////////////////////////
// Defined events

	/**
	 * \brief Event that is raised after parsing a message.
	 *
	 * - Always has a \ref parameters tag attached containing the split
	 *   parameters.
	 */
	struct parsed_event: event::requires_tags<parameters> {};

//	struct ctcp_event: event::requires_tags<parameters, origin> {};
//	struct error_event: event::requires_tags<parameters> {};
//	struct invite_event: event::requires_tags<parameters, origin> {};
//	struct join_event: event::requires_tags<parameters, origin> {};
//	struct kick_event: event::requires_tags<parameters, origin> {};
//	struct message_event: event::requires_tags<parameters, origin> {};
//	struct mode_event: event::requires_tags<parameters, origin> {};
//	struct nick_event: event::requires_tags<parameters, origin> {};
//	struct numeric_event: event::requires_tags<parameters, origin> {};
//	struct part_event: event::requires_tags<parameters, origin> {};

	/**
	 * \brief Event that is raised when receiving a PING command from the
	 *        server.
	 *
	 * - Always has a \ref parameters tag attached containing the split
	 *   parameters.
	 * - Always has a \ref message tag attached which contains the message
	 *   received by and to return to the sender.
	 */
	struct ping_event: event::requires_tags<message> {};

	/**
	 * \brief Event that is raised when a user quits.
	 *
	 * - Always has a \ref parameters tag attached containing the split
	 *   parameters.
	 * - Always has an \ref origin tag attached to denote the user quitting.
	 * - Has a \ref message tag attached containing the quit message iff a quit
	 *   message was sent.
	 */
	struct quit_event: event::requires_tags<origin> {};

//	struct topic_event: event::requires_tags<parameters, origin> {};
//	struct wallops_event: event::requires_tags<parameters, origin> {};



///////////////////////////////////////////////////////////////////////////////
// Helper functions

	/**
	 * \brief Extracts the parameters from an IRC line according to RFC 1459.
	 *
	 * Leading whitespace characters as well as multiple whitespaces between
	 * parameters are ignored. The last parameter may contain or end with white
	 * spaces if it starts with a colon (':').
	 *
	 * \param raw The raw IRC line, end of line characters removed.
	 *
	 * \return A vector of the single parameters.
	 */
	static std::vector<std::string> irc_split(const std::string &raw);
};

}
}

#endif // LIBSLIRC_HDR_APIS_PROTOCOL_HPP_INCLUDED

