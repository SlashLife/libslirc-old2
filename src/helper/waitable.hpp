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

#ifndef LIBSLIRC_HDR_HELPER_WAITABLE_HPP_INCLUDED
#define LIBSLIRC_HDR_HELPER_WAITABLE_HPP_INCLUDED

#include <algorithm>
#include <atomic>
#include <chrono>
#include <memory>
#include <type_traits>

#include <boost/chrono.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/utility.hpp>

namespace slirc {
namespace helper {

namespace detail {

// Some ugly magic to get *std*::chrono working with condition variables as
// well as unifying the wait call (timed_wait/wait_for/wait_until)

// generic
template<typename WaitObject, typename Lock, typename Timeout, typename Predicate>
bool cv_timed_wait(WaitObject &obj, Lock &lock, const Timeout &timeout, Predicate &&predicate) {
	return obj.timed_wait(lock, timeout, std::forward<Predicate>(predicate));
}

// duration, boost
template<typename WaitObject, typename Lock, typename TimeoutRep, typename TimeoutPeriod, typename Predicate>
bool cv_timed_wait(WaitObject &obj, Lock &lock, boost::chrono::duration<TimeoutRep, TimeoutPeriod> &&timeout, Predicate &&predicate) {
	return obj.wait_for(lock, timeout, std::forward<Predicate>(predicate));
}

// duration, std
template<typename WaitObject, typename Lock, typename TimeoutRep, typename TimeoutPeriod, typename Predicate>
bool cv_timed_wait(WaitObject &obj, Lock &lock, std::chrono::duration<TimeoutRep, TimeoutPeriod> &&timeout, Predicate &&predicate) {
	return obj.wait_for(lock, boost::chrono::duration<
		TimeoutRep, boost::ratio<TimeoutPeriod::num, TimeoutPeriod::den>
	>(timeout.count()), std::forward<Predicate>(predicate));
}

// time point, boost
template<typename WaitObject, typename Lock, typename TimeoutClock, typename TimeoutDuration, typename Predicate>
bool cv_timed_wait(WaitObject &obj, Lock &lock, const boost::chrono::time_point<TimeoutClock, TimeoutDuration> &timeout, Predicate &&predicate) {
	return obj.wait_until(lock, timeout, std::forward<Predicate>(predicate));
}

// time point, std
template<typename WaitObject, typename Lock, typename TimeoutClock, typename TimeoutDuration, typename Predicate>
bool cv_timed_wait(WaitObject &obj, Lock &lock, const std::chrono::time_point<TimeoutClock, TimeoutDuration> &timeout, Predicate &&predicate) {
	return cv_timed_wait(obj, lock, timeout - TimeoutClock::now(), std::forward<Predicate>(predicate));
}

}

/**
 * \brief A helper class to be able to wait for
 */
struct waitable: private boost::noncopyable {
private:
	typedef std::function<void()> callback_type;
	typedef std::vector<callback_type> callback_list_type;

	mutable boost::mutex callback_list_mutex;
		mutable callback_list_type callbacks;
		bool is_open;

	bool add_callback(const callback_type &callback) const;

public:
	/**
	 * \brief Constructs a waitable object.
	 *
	 * The newly created object is in open state.
	 */
	waitable();

	/**
	 * \brief Destructs a waitable object.
	 *
	 * If necessary, pending waits will be woken before destruction.
	 */
	~waitable();

	/**
	 * \brief Opens the waitable.
	 *
	 * When opening, pending waits will be woken up.
	 *
	 * While open, new waits will return instantly.
	 */
	void open();

	/**
	 * \brief Closes the waitable.
	 *
	 * While closed, waits will block until any waitable they are waiting on
	 * is opened, or until the wait times out.
	 */
	void close();

	/**
	 * \brief Wait with timeout for a range of waitables.
	 *
	 * Waits for at most the specified timeout for one of the waitables
	 * referred to by the range [begin, end).
	 *
	 * \tparam Iterator An iterator type that satisfies at least the
	 *                  ForwardIterator concept.
	 * \tparam Timeout A type representing either a duration or a point in time.
	 *
	 * \param begin An iterator to the first waitable object.
	 * \param end An iterator one past the last waitable object.
	 * \param timeout A duration to wait for or a point in time to wait until,
	 *                unless a waitable becomes available earlier.
	 *
	 * \return An iterator to the waitable object which has become available or
	 *         the end iterator if the wait timed out.
	 */
	template<typename Iterator, typename Timeout>
	static Iterator wait(Iterator begin, Iterator end, Timeout timeout) {
		if (begin == end) {
			// There is nothing to wait for, no need for all this trouble.
			return end;
		}

		struct shared_data_type {
			boost::mutex mutex;
				boost::condition_variable cond;
				Iterator retval;
		};
		std::shared_ptr<shared_data_type> shared_data = std::make_shared<shared_data_type>();
		std::weak_ptr<shared_data_type> weak_data = shared_data;

		shared_data->retval = end;
		auto pred = [=]() -> bool { return shared_data->retval != end; };

		{ boost::mutex::scoped_lock lock(shared_data->mutex);
			while(begin != end) {
				if (static_cast<const waitable &>(*begin).add_callback([weak_data, begin](){
					std::shared_ptr<shared_data_type> shared_data = weak_data.lock();
					if (shared_data) {
						boost::mutex::scoped_lock lock(shared_data->mutex);
						shared_data->retval = begin;
						shared_data->cond.notify_all();
					}
				})) {
					return begin;
				}
				++begin;
			}
			detail::cv_timed_wait(shared_data->cond, lock, timeout, pred);
			return shared_data->retval;
		}
	}

	/**
	 * \brief Wait for a range of waitables.
	 *
	 * Waits without timeout for one of the waitables referred to by the range
	 * [begin, end).
	 *
	 * \tparam Iterator An iterator type that satisfies at least the
	 *                  ForwardIterator concept.
	 *
	 * \param begin An iterator to the first waitable object.
	 * \param end An iterator one past the last waitable object.
	 *
	 * \return An iterator to the waitable object which has become available.
	 */
	template<typename Iterator>
	inline static Iterator wait(Iterator begin, Iterator end) {
		return wait(begin, end,
			std::chrono::time_point<std::chrono::steady_clock>::max());
	}

	/**
	 * \brief Wait with timeout for this waitable.
	 *
	 * Waits for at most the specified timeout for this waitable to become
	 * available.
	 *
	 * \tparam Timeout A type representing either a duration or a point in time.
	 *
	 * \param timeout A duration to wait for or a point in time to wait until,
	 *                unless a waitable becomes available earlier.
	 *
	 * \return @c true if the waitable has become available or @c false if the
	 *         wait timed out.
	 */
	template<typename Timeout>
	inline bool wait(Timeout timeout) const {
		return this == wait(this, this+1, timeout);
	}

	/**
	 * \brief Wait for this waitable.
	 *
	 * Waits without timeout for the waitables to become available.
	 */
	inline void wait() const {
		wait(std::chrono::time_point<std::chrono::steady_clock>::max());
	}
};

}
}

#endif // LIBSLIRC_HDR_HELPER_WAITABLE_HPP_INCLUDED
