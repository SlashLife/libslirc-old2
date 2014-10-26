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

#ifndef LIBSLIRC_HDR_EVENT_HPP_INCLUDED
#define LIBSLIRC_HDR_EVENT_HPP_INCLUDED

#include <algorithm>
#include <functional>
#include <memory>
#include <typeindex>

#include "helper/tag_container.hpp"

namespace slirc {

struct irc;
struct event;

namespace detail {

struct check_event_tags_none {
	check_event_tags_none() = delete;
	inline static bool check(const event &) { return true; }
};

template<typename ...DataTags>
struct check_event_tags;

struct event_type_base {
	event_type_base() = delete;
	typedef check_event_tags_none execution_checks;
};

template<typename EventType>
struct event_type_check {
	static_assert(std::is_base_of<event_type_base, EventType>::value,
		"Event types must be derived from slirc::event::type!");
	static_assert(!std::is_same<event_type_base, EventType>::value,
		"slirc::event::type is not a valid event type. "
		"Use a type derived from it instead.");
	typedef EventType type;
};

template<typename EventType>
inline bool check_event_history(const std::type_index *begin, const std::type_index *end) {
	return end != std::find(begin, end,
		std::type_index(typeid(typename detail::event_type_check<EventType>::type)));
}

} // namespace detail

/**
 * \brief The event class.
 *
 * An event is any action happening in an IRC context.
 *
 * Events are specified by their event type and the data attached to it.
 * An event can change its type multiple times during its life.
 *
 * For example a raw network event may become a numeric_event after protocol
 * parsing and then become a rpl_welcome_event after parsing the specific
 * numeric.
 */
struct event {
	friend class slirc::irc;

private:
	typedef std::vector<std::type_index> event_type_history_type;
	event_type_history_type event_type_history;
	event_type_history_type::iterator current_type;

public:
	/**
	 * \brief The data attached with this event.
	 */
	helper::tag_container data;

	/**
	 * \brief Handle this event by its attached IRC context.
	 */
	std::function<void()> handle;

	/**
	 * \brief Base class for event type identifiers.
	 *
	 * If you define your own events, inherit from this class to allow them to
	 * be used as an event type.
	 *
	 * \note This type exists only for RTTI purposes and cannot be
	 *       instantiated.
	 */
	typedef detail::event_type_base type;

	/**
	 * \brief Base class for event type identifiers with required tags.
	 *
	 * Enables runtime checks for the given tag types when handling the event
	 * using
	 * If you define your own events, inherit from this class to allow them to
	 * be used as an event type and to enable runtime checks for required
	 * data tags in debug mode.
	 *
	 * \note This type exists only for RTTI purposes and cannot be
	 *       instantiated.
	 */
	template<typename ...DataTags>
	struct requires_tags: type {
		/**
		 * \brief The type holding the checking for this event type.
		 */
		typedef detail::check_event_tags<DataTags...> execution_checks;
	};

	/**
	 * \brief Storage type for handling events.
	 */
	typedef std::shared_ptr<event> pointer;

	/**
	 * \brief Create a new event.
	 *
	 * \tparam EventType The initial type of this event. This must be a type
	 *                   derived from (but not equal to) slirc::event::type
	 *
	 * \return Returns a pointer to the newly created event.
	 */
	template<typename EventType>
	static pointer create() {
		pointer pevent = std::make_shared<event>();
		pevent->current_type = pevent->event_type_history.begin();
		pevent->queue_as<EventType>(true);
		return pevent;
	}

	/**
	 * \brief Queues another event type for this event.
	 *
	 * Other event types will be handled after all handlers for the current
	 * type have been invoked.
	 *
	 * \tparam EventType The new type this event should be queued for.
	 *
	 * \param multiple If set to @c false (default), the new type will only be
	 *                 added to the queue if it is not in it already. If set to
	 *                 true, this check is skipped and the new type is always
	 *                 added.
	 *
	 * \return Returns @c true if the new type has been queued, @c false if the
	 *         type has already been queued and multiple is set to @c false.
	 */
	template<typename EventType>
	bool queue_as(bool multiple = false) {
		if (!multiple && will_be_a<EventType>()) {
			return false;
		}

		// store current offset, because event_type_history might reallocate
		// and invalidate the iterator.
		auto offset = current_type - event_type_history.begin();
		event_type_history.emplace_back(
			typeid(typename detail::event_type_check<EventType>::type));
		current_type = event_type_history.begin() + offset;

		return true;
	}

	/**
	 * \brief Checks whether this event has been handled as some event type.
	 *
	 * \tparam EventType The event type to check the event for.
	 *
	 * \return Returns whether this event has been handled as the given type.
	 */
	template<typename EventType>
	bool was_a() const {
		return detail::check_event_history<EventType>(
			&*(event_type_history.begin()), &*current_type);
	}

	/**
	 * \brief Checks whether this event is currently being handled as some
	 *        event type.
	 *
	 * \tparam EventType The event type to check the event for.
	 *
	 * \return Returns whether this event is being handled as the given type.
	 */
	template<typename EventType>
	bool is_a() const {
		return current_type != event_type_history.end() &&
			detail::check_event_history<EventType>(
				&*(current_type), &*(current_type+1));
	}

	/**
	 * \brief Checks whether this event is queued to be handled as some event
	 *        type.
	 *
	 * \tparam EventType The event type to check the event for.
	 *
	 * \return Returns whether this event will be handled as the given type.
	 */
	template<typename EventType>
	bool will_be_a() const {
		return current_type != event_type_history.end() &&
			detail::check_event_history<EventType>(
				&*(current_type+1), &*(event_type_history.end()));
	}
};

namespace detail {

template<typename FirstTag, typename ...DataTags>
struct check_event_tags<FirstTag, DataTags...> {
	check_event_tags() = delete;

	inline bool check(const event &e) {
		return
			nullptr != e.data.get_p<FirstTag>() &&
			check_event_tags<DataTags...>::check();
	}
};
template<>
struct check_event_tags<>: check_event_tags_none {};

}

}

#endif // LIBSLIRC_HDR_EVENT_HPP_INCLUDED
