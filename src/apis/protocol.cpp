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

#include "protocol.hpp"

std::vector<std::string> slirc::apis::protocol::irc_split(const std::string &line) {
	std::vector<std::string> params;

	std::string::size_type begin, end=0;
	while(end != line.npos) {
		begin = line.find_first_not_of(' ', end);
		if (begin != line.npos) {
			if (line[begin]==':' && !params.empty()) {
				end = line.npos;
				++begin;
			}
			else {
				end = line.find_first_of(' ', begin);
			}
			params.emplace_back(line.substr(begin, end-begin));
		}
	}

	return params;
}
