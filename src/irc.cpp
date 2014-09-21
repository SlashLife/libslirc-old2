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

#include "irc.hpp"

slirc::irc::irc()
: event_ready(event_ready_internal) {
	// The queue starts out empty.
	event_ready_internal.close();
}

void slirc::irc::queue_event(event::pointer newevent) {
	boost::mutex::scoped_lock lock(event_queue_mutex);
	event_queue.push_back(newevent);
	event_ready_internal.open();
}

void slirc::irc::queue_event_front(event::pointer newevent) {
	boost::mutex::scoped_lock lock(event_queue_mutex);
	event_queue.push_front(newevent);
	event_ready_internal.open();
}

slirc::event::pointer slirc::irc::fetch_event() {
	boost::mutex::scoped_lock lock(event_queue_mutex);
	slirc::event::pointer next = nullptr;
	if (!event_queue.empty()) {
		next = event_queue.front();
		event_queue.pop_front();
	}
	// no else!
	if (event_queue.empty()) {
		event_ready_internal.close();
	}
	return next;
}
