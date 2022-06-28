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

#include <callback/signal.h>

namespace streamdeck {
	namespace handlers {
		class obs_scene {
			public /* Singleton */:
			static std::shared_ptr<streamdeck::handlers::obs_scene> instance();

			public:
			~obs_scene();
			obs_scene();

			public:
			static void on_source_create(void* ptr, calldata_t* calldata);
			static void on_destroy(void* ptr, calldata_t* calldata);
			static void on_reorder(void* ptr, calldata_t* calldata);

			static void on_item_add(void* ptr, calldata_t* calldata);
			static void on_item_remove(void* ptr, calldata_t* calldata);
			static void on_item_visible(void* ptr, calldata_t* calldata);
			static void on_item_transform(void* ptr, calldata_t* calldata);

			private /* Scenes */:
			void items(std::shared_ptr<streamdeck::jsonrpc::request>, std::shared_ptr<streamdeck::jsonrpc::response>);

			void item_visible(std::shared_ptr<streamdeck::jsonrpc::request>,
							  std::shared_ptr<streamdeck::jsonrpc::response>);

		};
	} // namespace handlers
} // namespace streamdeck
