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
		class obs_source {
			public /* Singleton */:
			static std::shared_ptr<streamdeck::handlers::obs_source> instance();

			public:
			~obs_source();
			obs_source();

			public:
			static void on_source_create(void* ptr, calldata_t* calldata);
			static void on_destroy(void* ptr, calldata_t* calldata);
			static void on_rename(void* ptr, calldata_t* calldata);

			static void on_enable(void* ptr, calldata_t* calldata);
			static void on_activate_deactivate(void* ptr, calldata_t* calldata);
			static void on_show_hide(void* ptr, calldata_t* calldata);

			static void on_mute(void* ptr, calldata_t* calldata);
			static void on_volume(void* ptr, calldata_t* calldata);

			static void on_filter_add(void* ptr, calldata_t* calldata);
			static void on_filter_remove(void* ptr, calldata_t* calldata);
			static void on_filter_reorder(void* ptr, calldata_t* calldata);

			static void on_media_play(void* ptr, calldata_t* calldata);
			static void on_media_pause(void* ptr, calldata_t* calldata);
			static void on_media_restart(void* ptr, calldata_t* calldata);
			static void on_media_stopped(void* ptr, calldata_t* calldata);
			static void on_media_next(void* ptr, calldata_t* calldata);
			static void on_media_previous(void* ptr, calldata_t* calldata);
			static void on_media_started(void* ptr, calldata_t* calldata);
			static void on_media_ended(void* ptr, calldata_t* calldata);

			private /* Sources */:
			void enumerate(std::shared_ptr<streamdeck::jsonrpc::request>,
						   std::shared_ptr<streamdeck::jsonrpc::response>);

			void state(std::shared_ptr<streamdeck::jsonrpc::request>, std::shared_ptr<streamdeck::jsonrpc::response>);

			void settings(std::shared_ptr<streamdeck::jsonrpc::request>,
						  std::shared_ptr<streamdeck::jsonrpc::response>);

			void media(std::shared_ptr<streamdeck::jsonrpc::request>, std::shared_ptr<streamdeck::jsonrpc::response>);

			void properties(std::shared_ptr<streamdeck::jsonrpc::request>,
							std::shared_ptr<streamdeck::jsonrpc::response>);

			private /* Filters */:
			void filters(std::shared_ptr<streamdeck::jsonrpc::request>, std::shared_ptr<streamdeck::jsonrpc::response>);
		};
	} // namespace handlers
} // namespace streamdeck
