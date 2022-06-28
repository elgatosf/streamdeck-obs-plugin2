// Copyright (C) 2020 - 2021 Corsair GmbH
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
#include <memory>
#include "json-rpc.hpp"

namespace streamdeck {
	namespace handlers {
		class system {
			public /* Singleton */:
			static std::shared_ptr<streamdeck::handlers::system> instance();

			public:
			system();

			private:
			std::shared_ptr<streamdeck::jsonrpc::response> _ping(std::shared_ptr<streamdeck::jsonrpc::request>);
			std::shared_ptr<streamdeck::jsonrpc::response> _version(std::shared_ptr<streamdeck::jsonrpc::request>);
		};
	} // namespace handlers
} // namespace streamdeck
