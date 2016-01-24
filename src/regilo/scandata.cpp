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

#include "regilo/scandata.hpp"

#include <iostream>

namespace regilo {

ScanData::ScanData() :
	scanId(-1), rotationSpeed(-1)
{
}

ScanData::ScanData(std::size_t scanId, double rotationSpeed) :
	scanId(scanId), rotationSpeed(rotationSpeed)
{
}

std::ostream& operator<<(std::ostream& out, const ScanData& data)
{
	out << "ScanData("
		<< data.scanId
		<< ", "
		<< data.rotationSpeed
		<< ", "
		<< data.size()
		<< ')'
		<< std::endl;

	for(const ScanRecord& record : data)
	{
		out << record << std::endl;
	}

	return out;
}

}
