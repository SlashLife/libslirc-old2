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

#ifndef LIBSLIRC_HDR_IRC_HPP_INCLUDED
#define LIBSLIRC_HDR_IRC_HPP_INCLUDED

#include <deque>
#include <type_traits>
#include <utility>

#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/signals2.hpp>

#include "event.hpp"
#include "exceptions/no_module.hpp"
#include "exceptions/no_tag.hpp"
#include "helper/tag_container.hpp"
#include "helper/waitable.hpp"
#include "module.hpp" // necessary for default deleter of unique_ptr<module>

/// \namespace slirc \brief So much IRC in a single topmost namespace!
/// \namespace slirc::helper \brief Helper classes with no semantic connection to IRC

namespace slirc {

struct module;

/**
 * \brief The main context for any IRC connection.
 *
 * The IRC context is the combining piece for managing an IRC connection.
 *
 * The functions provided by this class can be distinguished in three
 * categories:
 * - Event queue management
 * - Event handler management
 * - Module management
 *
 * \note Unless explicitly specified otherwise, all APIs on a context should
 *       be treated as not thread safe and should be used from a single
 *       threaded environment.
 * \note On notable exception is the event queue API: Events can be added
 *       safely from any thread and will correctly unblock a worker thread
 *       waiting on event_ready.
 */
struct irc: private boost::noncopyable {
private:
	typedef std::map<std::type_index, std::unique_ptr<module>> module_container_t;
	module_container_t modules;

	boost::mutex event_queue_mutex;
		std::deque<event::pointer> event_queue;
		helper::waitable event_ready_internal;

	struct signal_type {
		boost::signals2::signal<void(event::pointer)> signal;
		bool (*check)(event &);
	};
	std::map<std::type_index, signal_type> signals;

public:
	/**
	 * \brief Creates an empty IRC context.
	 */
	irc();

	///////////////////////////////////////////////////////////////////////////
	// Event Queue API

	/**
	 * \brief A waitable object to wait for events to become available in the
	 *        event queue.
	 *
	 * This waitable will be open as long as there are events in the queue.
	 *
	 * \note If multiple threads wait on the same queue, they will all be
	 *       woken up when an event becomes available; however only one thread
	 *       is guaranteed to be able to fetch an event. (Then again it is a
	 *       bad idea to have multiple threads concurrently wait for events on
	 *       the same IRC context.)
	 */
	const helper::waitable &event_ready;

	/**
	 * \brief Queue an event to the event queue.
	 *
	 * \param newevent The event to add to the queue.
	 *
	 * \note This function is thread safe.
	 */
	void queue_event(event::pointer newevent);

	/**
	 * \brief Queue an event to the begin of the event queue.
	 *
	 * \param newevent The event to add to the queue.
	 *
	 * \note This function is thread safe.
	 */
	void queue_event_front(event::pointer newevent);

	/**
	 * \brief Tries to fetch an event from the queue.
	 *
	 * \return A pointer to the next event or @c nullptr if no event is
	 *         available.
	 *
	 * \note This function does not block. If you want to wait for an event,
	 *       try waiting on event_ready before fetching an event.
	 *
	 * \note This function is thread safe.
	 */
	event::pointer fetch_event();



	///////////////////////////////////////////////////////////////////////////
	// Event handler API



	///////////////////////////////////////////////////////////////////////////
	// Module API

	/**
	 * \brief Fetches a module.
	 *
	 * \throw exceptions::no_module if the requested module is not loaded.
	 * \return A reference to the requested module.
	 */
	template<typename Module>
	Module &module() {
		static_assert(std::is_base_of<slirc::module, Module>::value,
			"The passed argument is not derived from slirc::module!");

		Module *module;
		module_container_t::iterator it = modules.find(typeid(typename Module::module_api_type));
		if (it != modules.end() && (module = dynamic_cast<Module*>(it->second.get()))) {
			return *module;
		}

		throw exceptions::no_module();
	}

	/**
	 * \brief Fetches a module.
	 *
	 * \throw exceptions::no_module if the requested module is not loaded.
	 * \return A reference to the requested module.
	 */
	template<typename Module>
	const Module &module() const {
		return const_cast<irc&>(*this).module<Module>();
	}

	/**
	 * \brief Unloads module from the context.
	 *
	 * If a module of the same module API is already loaded, it is unloaded
	 * automatically before the new module is loaded.
	 */
	template<typename Module>
	void unload() {
		static_assert(std::is_base_of<slirc::module, Module>::value,
			"The passed argument is not derived from slirc::module!");

		module_container_t::iterator it = modules.find(typeid(typename Module::module_api_type));
		if (it != modules.end()) {
			modules.erase(it);
		}
		else {
			throw exceptions::no_module();
		}
	}

	/**
	 * \brief Loads a new module into the context.
	 *
	 * If a module of the same module API is already loaded, it is unloaded
	 * automatically before the new module is loaded.
	 *
	 * \param params The parameters (without the leading reference to the
	 *               IRC context) to be passed to the modules constructor.
	 * \return A reference to the newly loaded module.
	 */
	template<typename Module, typename... Params>
	Module &load(Params&&... params) {
		static_assert(std::is_base_of<slirc::module, Module>::value,
			"The passed argument is not derived from slirc::module!");

		try {
			unload<typename Module::module_api_type>();
		}
		catch(exceptions::no_module &) {
			// ok - no need to unload module
		}

		Module *newmod = new Module(*this, std::forward<Params>(params)...);
		modules[typeid(typename Module::module_api_type)] = module_container_t::mapped_type(newmod);
		return *newmod;
	}
};

template<typename Iterator>
struct irc_wait_iterator {
	typedef Iterator base_iterator;
	typedef helper::waitable value_type;
	typedef value_type &reference;
	typedef value_type *pointer;

	irc_wait_iterator &operator++() {
		++it;
		return *this;
	}

	irc_wait_iterator operator++(int) const {
		irc_wait_iterator temp = *this;
		++it;
		return *this;
	}

	reference operator*() const {
		return static_cast<irc&>(*it);
	}

	pointer operator->() const {
		return &static_cast<irc&>(*it);
	}

	bool operator==(const Iterator &other) const {
		return it == other;
	}

	bool operator!=(const Iterator &other) const {
		return it != other;
	}

private:
	base_iterator it;
};

}

#endif // LIBSLIRC_HDR_IRC_HPP_INCLUDED
