// Copyright (C) 2022, Corsair Memory Inc. All rights reserved.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once
#include <string>

#include "config.hpp"
#include "version.hpp"

namespace streamdeck {
	std::string get_translated_text(const std::string& text);

	enum class log_level : int {
		LOG_CRITICAL = 0,
		LOG_ERROR    = 100,
		LOG_WARNING  = 200,
		LOG_INFO     = 300,
		LOG_DEBUG    = 400,
	};

	void message(log_level level, const char* format, ...);
} // namespace streamdeck
