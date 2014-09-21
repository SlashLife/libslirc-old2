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

#ifndef LIBSLIRC_HDR_HELPER_TAG_CONTAINER_HPP_INCLUDED
#define LIBSLIRC_HDR_HELPER_TAG_CONTAINER_HPP_INCLUDED

#include <map>
#include <typeindex>
#include <utility>

#include <boost/any.hpp>

#include "../exceptions/no_tag.hpp"

namespace slirc {
namespace helper {

/**
 * \brief Container type for type safe storage and retrieval of multiple
 *        different types.
 *
 * - Can contain exactly 0 or 1 instances of every type.
 *
 * - Can access these instances type safely by type name.
 *
 * \note Due to its semantic of holding different types at the same time, this
 *       type does not meet the STL container specifications.
 */
struct tag_container {
	/**
	 * \brief Retrieves a pointer to the tag of specified type from container.
	 *
	 * \tparam T Type of the tag to be retrieved.
	 * \return A pointer to the tag or nullptr if no tag of the specified type
	 *         stored.
	 */
	template<typename T>
	inline T *get_p() {
		container_t::iterator it = data.find(typeid(T));
		if (it == data.end()) {
			return nullptr;
		}
		else {
			return boost::any_cast<T>(&(it->second));
		}
	}

	/**
	 * \brief Retrieves a pointer to the tag of specified type from container.
	 *
	 * \tparam T Type of the tag to be retrieved.
	 * \return A pointer to the tag or nullptr if no tag of the specified type
	 *         stored.
	 */
	template<typename T>
	inline T *get_p() const {
		return const_cast<tag_container&>(*this).get_p<T>();
	}

	/**
	 * \brief Retrieve a reference to the tag of specified type from container.
	 *
	 * \tparam T Type of the tag to be retrieved.
	 * \return A reference to the tag.
	 * \throw slirc::exceptions::no_tag if no tag of the specified type is
	 *                                  stored.
	 */
	template<typename T>
	inline T &get() {
		T *pobj = get_p<T>();
		if (pobj) {
			return *pobj;
		}
		else {
			throw exceptions::no_tag();
		}
	}

	/**
	 * \brief Retrieve a reference to the tag of specified type from container.
	 *
	 * \tparam T Type of the tag to be retrieved.
	 * \return A reference to the tag.
	 * \throw slirc::exceptions::no_tag if no tag of the specified type is
	 *                                  stored.
	 */
	template<typename T>
	inline const T &get() const {
		return const_cast<tag_container&>(*this).get<T>();
	}

	/**
	 * \brief Stores a tag.
	 * Overwrites a possibly existing tag of the same type.
	 *
	 * \tparam T Type of the tag to be stored. (Automatically retrieved from
	 *           the parameter.)
	 * \param tag The new value for the tag.
	 * \return A reference to the tag.
	 */
	template<typename T>
	inline T &set(T &&tag) {
		boost::any &any = data[typeid(T)] = std::move(tag);
		return boost::any_cast<T&>(any);
	}

	/**
	 * \brief Removes a tag.
	 *
	 * \tparam T Type of the tag to be removed.
	 */
	template<typename T>
	inline void unset() {
		container_t::iterator it = data.find(typeid(T));
		if (it != data.end()) {
			data.erase(it);
		}
		else {
			throw exceptions::no_tag();
		}
	}

private:
	typedef std::map<std::type_index, boost::any> container_t;
	container_t data;
};

}
}

#endif // LIBSLIRC_HDR_HELPER_TAG_CONTAINER_HPP_INCLUDED
