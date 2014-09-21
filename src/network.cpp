/***************************************************************************
**  Copyright 2014-2014 by Simon "SlashLife" Stienen                      **
**  http://projects.slashlife.org/libslirc/                               **
**  libslirc@projects.slashlife.org                                       **
**                                                                        **
**  This file is part of libslIRC.                                        **
**                                                                        **
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

#include "network.hpp"

#include <algorithm>
#include <atomic>
#include <stdexcept>

#include <boost/asio.hpp>
#include <boost/thread.hpp>

namespace {
	boost::asio::io_service network_service;
	boost::thread network_service_async_worker;
	std::unique_ptr<boost::asio::io_service::work> work_loop;

	boost::mutex network_external_api;
		bool automatic_handling;

	void do_stop_thread() {
		network_service.stop();
		work_loop.reset();

		try {
			// May not be called from within the network thread!
			// join() terminates otherwise, so at least it is noticable
			network_service_async_worker.join();
		}
		catch(std::invalid_argument &) {
			// thread was not joinable, i.e. is not running anyway:
			// everything is fine!
		}
	}

	void do_start_thread() {
		if (!network_service.stopped()) {
			// service is still running - we got nothing to do here
			return;
		}

		// service is not running - but let's try to clean up an old thread
		do_stop_thread();
		// thread is now joined

		// Prepare atomic flag to check for correct thread startup
		std::atomic_flag thread_is_starting;
		thread_is_starting.test_and_set();
		// thread_is_starting is now true

		work_loop.reset(new boost::asio::io_service::work(network_service));

		boost::thread newthread([&](){
			network_service.reset();
			thread_is_starting.clear();
			network_service.run();
		});

		while(thread_is_starting.test_and_set()) {
			// will only turn false *one time* after the new thread has
			// passed reset() (and thus network_service will no longer
			// register as stopped())
			boost::thread::yield(); // wait for it ... just wait for it ...
		}

		std::swap(newthread, network_service_async_worker);
	}
}

void slirc::network::run() {
	boost::lock_guard<boost::mutex> extapi_lock(network_external_api);
	if (automatic_handling) {
		return;
	}
	service.poll();
}

void slirc::network::set_handling_mode(slirc::network::handling_mode mode) {
	boost::lock_guard<boost::mutex> extapi_lock(network_external_api);
	handling_mode current_mode = automatic_handling
		? handling_mode::automatic
		: handling_mode::manual;

	if (current_mode != mode) {
		if (mode == handling_mode::automatic) {
			network_service.stop();
			do_start_thread();
		}
		else {
			do_stop_thread();
		}

		// safe to do: guarded by network_external_api mutex
		automatic_handling = !automatic_handling;
	}
}

slirc::network::handling_mode slirc::network::current_handling_mode() {
	boost::lock_guard<boost::mutex> extapi_lock(network_external_api);
	return automatic_handling
		? handling_mode::automatic
		: handling_mode::manual;
}

boost::asio::io_service &slirc::network::service = network_service;

namespace {
	struct slirc_network_cleanup_type {
		slirc_network_cleanup_type() {
			automatic_handling = true;
			// Unfortunately setting this to automatic on startup will hang
			// the thread.
			slirc::network::set_handling_mode(slirc::network::handling_mode::manual);
		}
		~slirc_network_cleanup_type() {
			// no need to lock the mutex anymore - we're already going down and
			// it'll be gone in a moment anyway; no need to provoke a possible
			// deadlock before terminating by destructing a possibly still
			// locked mutex: At least tidy up the network before doing so.
			slirc::network::set_handling_mode(slirc::network::handling_mode::manual);
		}
	} slirc_network_cleanup;
}
