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
	 * \brief Event tag specifying CTCPs types.
	 */
	struct ctcp {
		/// The type of the CTCP.
		std::string type;
		/// The original (binary) message attached with the event.
		std::string raw;
		// TODO: text
	};

	/**
	 * \brief Event tag specifying attached CTCPs.
	 */
	struct ctcp_list {
		/// The old nickname of the user.
		std::string old_nick;
		/// The new nickname of the user.
		std::string new_nick;
	};

	/**
	 * \brief Event tag containing the text message.
	 */
	struct message {
		/// Initializes a message as type "other".
		inline message(): type(other) {};

		/// The original (binary) message attached with the event.
		std::string raw;
		// TODO: text
		/// The type of this message.
		enum {
			other, ///< Other types of messages (the default).
			privmsg, ///< This message comes from a PRIVMSG.
			notice ///< This message comes from a NOTICE.
		} type;
	};

	/**
	 * \brief Event tag specifying a nick change.
	 */
	struct nick_change {
		/// The old nickname of the user.
		std::string old_nick;
		/// The new nickname of the user.
		std::string new_nick;
	};

	/**
	 * \brief Event tag specifying a numeric.
	 */
	struct numeric {
		/// The number of the numeric.
		unsigned number;
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
		// TODO:
		// /// A pointer to the user object of the sender (if any).
		// user::pointer origin_user;
	};

	/**
	 * \brief Event tag containing the parameters of the raw message.
	 */
	struct parameters {
		/// The parameters extracted from the message according to the protocol.
		std::vector<std::string> params;
	};

	/**
	 * \brief Event tag specifying the recipient of a message.
	 *
	 * The recipient of a message is the user or channel it is addressed to.
	 */
	struct recipient {
		/// The verbatim name of the recipient.
		std::string recipient_string;
		// TODO:
		// /// A pointer to the channel object receiving the message.
		// channel::pointer recipient_channel;
		// /// A pointer to the user object receiving the message.
		// \note This will most likely be the "me" user on the connection.
		// user::pointer recipient_user;
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

	/**
	 * \brief Event that is raised when a user changes his nickname
	 *
	 * - Always has a \ref parameters tag attached containing the split
	 *   parameters.
	 * - Always has an \ref origin tag attached denoting the sender.
	 * - Always has a \ref recipient attached tag attached specifying the
	 *   recipient.
	 * - Always has a \ref message tag attached containing the message and type
	 *   of message (privmsg vs notice).
	 */
	struct message_event: event::requires_tags<parameters, origin, recipient, message> {};
//	struct mode_event: event::requires_tags<parameters, origin> {};

	/**
	 * \brief Event that is raised when a user changes his nickname
	 *
	 * - Always has a \ref parameters tag attached containing the split
	 *   parameters.
	 * - Always has an \ref origin tag attached denoting the sender.
	 * - Always has a \ref nick_change tag attached specifying the old and new
	 *   nicknames respectively.
	 *
	 * \note The nickname referred to by the \ref origin tag may change at an
	 *       unspecified time during this event and should not be relied on.
	 *       When reacting to this event in a manner that requires an origin
	 *       nickname, use the respective field from the \ref nick_change tag
	 *       instead.
	 */
	struct nick_event: event::requires_tags<parameters, origin, nick_change> {};

	/**
	 * \brief Event that is raised when a numeric is received.
	 *
	 * - Always has a \ref parameters tag attached containing the split
	 *   parameters.
	 * - Always has an \ref origin tag attached denoting the sender.
	 * - Always has a \ref numeric tag attached specifying the numerics number.
	 */
	struct numeric_event: event::requires_tags<parameters, origin, numeric> {};

	/**
	 * \brief Event that is raised when a user parts a channel.
	 *
	 * - Always has a \ref parameters tag attached containing the split
	 *   parameters.
	 * - Always has an \ref origin tag attached denoting the leaving user.
	 * - Always has a \ref recipient tag attached specifying the channel that
	 *   is being left.
	 * - Has a \ref message tag attached containing the part message iff a part
	 *   message was sent.
	 */
	struct part_event: event::requires_tags<parameters, origin, recipient> {};

	/**
	 * \brief Event that is raised when receiving a PING command from the
	 *        server.
	 *
	 * - Always has a \ref parameters tag attached containing the split
	 *   parameters.
	 * - Always has a \ref message tag attached containing the message received
	 *   by and to return to the sender.
	 */
	struct ping_event: event::requires_tags<message> {};

	/**
	 * \brief Event that is raised when a user quits.
	 *
	 * - Always has a \ref parameters tag attached containing the split
	 *   parameters.
	 * - Always has an \ref origin tag attached denoting the user quitting.
	 * - Has a \ref message tag attached containing the quit message iff a quit
	 *   message was sent.
	 */
	struct quit_event: event::requires_tags<parameters, origin> {};

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

