/*
 * Regilo
 * Copyright (C) 2015-2016  Branislav Holý <branoholy@gmail.com>
 *
 * This file is part of Regilo.
 *
 * Regilo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Regilo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Regilo.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef REGILO_VERSION_HPP
#define REGILO_VERSION_HPP

#include <string>

namespace regilo {

/**
 * @brief The Version class is used to store version information.
 */
class Version
{
public:
    ///< The whole version number in format <major>[.<minor>[.<patch>]].
    static const std::string VERSION;

    ///< The major part of the version number (format <major>[.<minor>[.<patch>]]).
    static const std::string VERSION_MAJOR;

    ///< The minor part of the version number (format <major>[.<minor>[.<patch>]]).
    static const std::string VERSION_MINOR;

    ///< The patch part of the version number (format <major>[.<minor>[.<patch>]]).
    static const std::string VERSION_PATCH;
};

}

#endif // REGILO_VERSION_HPP
