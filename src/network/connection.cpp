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

// Careful! This file is ugly!

#include "connection.hpp"

#include <cassert>

#include <boost/asio.hpp>
#include <boost/thread.hpp>

#ifndef LIBSLIRC_OPTION_WITHOUT_SSL
#	include <boost/asio/ssl.hpp>
#endif

#include "../network.hpp"

namespace slirc {
namespace network {
namespace detail {
	using boost::asio::ip::tcp;
	namespace ssl = boost::asio::ssl;

	struct connection_implementation {
		static const size_t arbitrary_buffer_length = 512;

		static inline boost::asio::io_service &service() {
			return network::service;
		}

		struct resolver {
			resolver()
			: res(service())
			{}

			tcp::resolver res;
			tcp::resolver::iterator it;
		};
		char recv_buffer[arbitrary_buffer_length];
		std::unique_ptr<resolver> resolver_context;
		boost::mutex socket_mutex;
			std::unique_ptr<tcp::socket> socket;
			bool send_in_progress;
			std::string send_buffer;
#ifndef LIBSLIRC_OPTION_WITHOUT_SSL
			std::unique_ptr<ssl::stream<tcp::socket>> ssl_stream;
			const ssl::context *ssl_context;
#endif

		slirc::network::connection::status_handler_type status_handler;
		slirc::network::connection::recv_handler_type   recv_handler;
		slirc::network::connection::send_handler_type  send_handler;

		connection_implementation()
		: send_in_progress(false)
		, status_handler([](const boost::system::error_code &){})
		, recv_handler([](const std::string &){})
		, send_handler([](std::size_t){})
		{}

		~connection_implementation() {
			if (
				// Careful! disconnect() locks itself, so ONLY protect the
				// condition of the if.
				boost::lock_guard<boost::mutex>(socket_mutex),
				socket && socket->is_open()
			) {
				disconnect();
			}
		}

		void connect(const std::string &addr, const std::string &service_port) {
			resolver_context.reset(new resolver());
			resolver_context->res.async_resolve(
				tcp::resolver::query(addr, service_port),
				[&](const boost::system::error_code& error, tcp::resolver::iterator it) {
					if (!error) {
						resolver_context->it = it;
						boost::lock_guard<boost::mutex> socket_lock(socket_mutex);
						assert(!socket);
						socket.reset(new tcp::socket(service()));
#ifndef LIBSLIRC_OPTION_WITHOUT_SSL
						if (ssl_context) {
							ssl_stream.reset(new ssl::stream<tcp::socket>(*socket, *ssl_context));
						}
#endif
						try_connect(socket_lock);
					}
					else {
						status_handler(error);
					}
				}
			);
		}

		void accept(unsigned port) {
			boost::lock_guard<boost::mutex> socket_lock(socket_mutex);
			assert(!socket);

			// TODO
			static_cast<void>(port);
		}

		void disconnect() {
			boost::lock_guard<boost::mutex> socket_lock(socket_mutex);
			assert(socket);
			boost::system::error_code ignored_error;
			socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_error);
			socket->close(ignored_error);
		}

		void send(const std::string &data) {
			boost::lock_guard<boost::mutex> socket_lock(socket_mutex);
			assert(socket);
			send_buffer += data;
			if (!send_in_progress) {
				send_in_progress = true;
				try_send(socket_lock);
			}
		}

	private:
		// attempts to send more data
		void try_send(boost::lock_guard<boost::mutex> &socket_lock_unused) {
			assert(socket);
			// assumes socket_mutex to be locked by the current thread
			static_cast<void>(socket_lock_unused); // just passed to reinforce
			// the caller to lock the mutex before calling this function

			if (send_buffer.empty()) {
				send_in_progress = false;
				return; // nothing to do
			}

			auto handler = [&](
				const boost::system::error_code& error, // Result of operation.
				std::size_t bytes_transferred
			) {
				if (bytes_transferred) {
					send_handler(bytes_transferred);
					boost::lock_guard<boost::mutex> socket_lock(socket_mutex);
					send_buffer.erase(0, bytes_transferred);
				}
				if (error) {
					status_handler(error);
				}
				else {
					boost::lock_guard<boost::mutex> socket_lock(socket_mutex);
					try_send(socket_lock);
				}
			};

#ifndef LIBSLIRC_OPTION_WITHOUT_SSL
			if (ssl_stream)
				boost::asio::async_write(*ssl_stream, boost::asio::buffer(send_buffer), handler);
			else
#endif
				boost::asio::async_write(*socket, boost::asio::buffer(send_buffer), handler);
		}

		// attempts a connection to the next endpoint
		void try_connect(boost::lock_guard<boost::mutex> &socket_lock_unused) {
			assert(socket);
			// assumes socket_mutex to be locked by the current thread
			static_cast<void>(socket_lock_unused); // just passed to reinforce
			// the caller to lock the mutex before calling this function

			static const tcp::resolver::iterator end;
			assert(resolver_context->it != end);

			tcp::resolver::iterator current = resolver_context->it++;

			socket->async_connect(
				*current,
				[&](const boost::system::error_code &error) {
					static const tcp::resolver::iterator end;
					if (error && end != ++(resolver_context->it)) {
						boost::lock_guard<boost::mutex> socket_lock(socket_mutex);
						try_connect(socket_lock);
					}
					else {
						// connection succeeded or
						// last endpoint failed connecting
						status_handler(error);
						if (!error) {
							// It actually succeeded! - start recving here.
							boost::lock_guard<boost::mutex> socket_lock(socket_mutex);
							try_recv(socket_lock);
						}
					}
				}
			);
		}

		// attempts recving from the socket
		void try_recv(boost::lock_guard<boost::mutex> &socket_lock_unused) {
			assert(socket);
			// assumes socket_mutex to be locked by the current thread
			static_cast<void>(socket_lock_unused); // just passed to reinforce
			// the caller to lock the mutex before calling this function

			if (socket) { // nowhere to recv from otherwise
				auto buffer = boost::asio::buffer(recv_buffer, arbitrary_buffer_length);
				auto reader = [&](
					const boost::system::error_code& error, // Result of operation.
					std::size_t bytes_transferred           // Number of bytes recv
				) {
					if (error) {
						status_handler(error);
					}
					else {
						recv_handler(std::string(recv_buffer, bytes_transferred));
						boost::lock_guard<boost::mutex> socket_lock(socket_mutex);
						try_recv(socket_lock); // and recv some more!
					}
				};

#ifndef LIBSLIRC_OPTION_WITHOUT_SSL
				if (ssl_stream)
					ssl_stream->async_receive(buffer, reader);
				else
#endif
					socket->async_receive(buffer, reader);
			}
		}
	};
}
}
}

slirc::network::connection::connection()
: impl(new slirc::network::detail::connection_implementation()) {}

slirc::network::connection::~connection() {
	// Necessary: Destruction involves destructing the implementation instance,
	// which is not known to a generated destructor.
}

void slirc::network::connection::on_status(status_handler_type status_handler) {
	impl->status_handler = status_handler;
}

void slirc::network::connection::on_recv(recv_handler_type recv_handler) {
	impl->recv_handler = recv_handler;
}

void slirc::network::connection::use_ssl(const boost::asio::ssl::context &ssl_context) {
#ifndef LIBSLIRC_OPTION_WITHOUT_SSL
	impl->ssl_context = &ssl_context;
#else
	static_cast<void>(ssl_context);
	assert(false && "use_ssl called, but libslirc was compiled without SSL support");
	throw std::logic_error("use_ssl called, but libslirc was compiled without SSL support");
#endif
}

void slirc::network::connection::no_ssl() {
#ifndef LIBSLIRC_OPTION_WITHOUT_SSL
	impl->ssl_context = nullptr;
#endif
}

void slirc::network::connection::send(const std::string &data) {
	impl->send(data);
}

void slirc::network::connection::connect(const std::string &hostname, unsigned port) {
	impl->connect(hostname, std::to_string(port));
}

void slirc::network::connection::disconnect() {
	impl->disconnect();
}
