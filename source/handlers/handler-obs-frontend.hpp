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
#include <memory>
#include "json-rpc.hpp"

#include "obs-frontend-api.h"
#include <util/platform.h>

namespace streamdeck {
	namespace handlers {
		class obs_frontend {
			public /* Singleton */:
			static std::shared_ptr<streamdeck::handlers::obs_frontend> instance();

			public:
			~obs_frontend();
			obs_frontend();

			bool loaded() const;

			private:
			static void handle_frontend_event(enum obs_frontend_event event, void* private_data);
			void        handle(enum obs_frontend_event event);

			static void on_rename_transition(void*, calldata_t* calldata);

			void streaming_start(std::shared_ptr<streamdeck::jsonrpc::request>,
								 std::shared_ptr<streamdeck::jsonrpc::response>);
			void streaming_stop(std::shared_ptr<streamdeck::jsonrpc::request>,
								std::shared_ptr<streamdeck::jsonrpc::response>);
			void streaming_active(std::shared_ptr<streamdeck::jsonrpc::request>,
								  std::shared_ptr<streamdeck::jsonrpc::response>);

			void recording_start(std::shared_ptr<streamdeck::jsonrpc::request>,
								 std::shared_ptr<streamdeck::jsonrpc::response>);
			void recording_stop(std::shared_ptr<streamdeck::jsonrpc::request>,
								std::shared_ptr<streamdeck::jsonrpc::response>);
			void recording_active(std::shared_ptr<streamdeck::jsonrpc::request>,
								  std::shared_ptr<streamdeck::jsonrpc::response>);

			void recording_pause(std::shared_ptr<streamdeck::jsonrpc::request>,
								 std::shared_ptr<streamdeck::jsonrpc::response>);
			void recording_unpause(std::shared_ptr<streamdeck::jsonrpc::request>,
								   std::shared_ptr<streamdeck::jsonrpc::response>);
			void recording_paused(std::shared_ptr<streamdeck::jsonrpc::request>,
								  std::shared_ptr<streamdeck::jsonrpc::response>);

			void replaybuffer_enabled(std::shared_ptr<streamdeck::jsonrpc::request>,
									  std::shared_ptr<streamdeck::jsonrpc::response>);
			void replaybuffer_start(std::shared_ptr<streamdeck::jsonrpc::request>,
									std::shared_ptr<streamdeck::jsonrpc::response>);
			void replaybuffer_save(std::shared_ptr<streamdeck::jsonrpc::request>,
								   std::shared_ptr<streamdeck::jsonrpc::response>);
			void replaybuffer_stop(std::shared_ptr<streamdeck::jsonrpc::request>,
								   std::shared_ptr<streamdeck::jsonrpc::response>);
			void replaybuffer_active(std::shared_ptr<streamdeck::jsonrpc::request>,
									 std::shared_ptr<streamdeck::jsonrpc::response>);

			void studiomode(std::weak_ptr<void>, std::shared_ptr<streamdeck::jsonrpc::request>);
			void studiomode_enable(std::weak_ptr<void>, std::shared_ptr<streamdeck::jsonrpc::request>);
			void studiomode_disable(std::weak_ptr<void>, std::shared_ptr<streamdeck::jsonrpc::request>);
			void studiomode_active(std::weak_ptr<void>, std::shared_ptr<streamdeck::jsonrpc::request>);

			void virtualcam(std::weak_ptr<void>, std::shared_ptr<streamdeck::jsonrpc::request>);
			void virtualcam_start(std::weak_ptr<void>, std::shared_ptr<streamdeck::jsonrpc::request>);
			void virtualcam_stop(std::weak_ptr<void>, std::shared_ptr<streamdeck::jsonrpc::request>);
			void virtualcam_active(std::weak_ptr<void>, std::shared_ptr<streamdeck::jsonrpc::request>);

			void transition_studio(std::weak_ptr<void>, std::shared_ptr<streamdeck::jsonrpc::request>);

			void scenecollection(std::weak_ptr<void>, std::shared_ptr<streamdeck::jsonrpc::request>);
			void scenecollection_list(std::shared_ptr<streamdeck::jsonrpc::request>,
									  std::shared_ptr<streamdeck::jsonrpc::response>);

			void profile(std::weak_ptr<void>, std::shared_ptr<streamdeck::jsonrpc::request>);
			void profile_list(std::shared_ptr<streamdeck::jsonrpc::request>,
									  std::shared_ptr<streamdeck::jsonrpc::response>);

			void scene(std::weak_ptr<void>, std::shared_ptr<streamdeck::jsonrpc::request>);
			void scene_list(std::shared_ptr<streamdeck::jsonrpc::request>,
							std::shared_ptr<streamdeck::jsonrpc::response>);

			void transition(std::weak_ptr<void>, std::shared_ptr<streamdeck::jsonrpc::request>);
			void transition_list(std::weak_ptr<void>, std::shared_ptr<streamdeck::jsonrpc::request>);

			void screenshot(std::shared_ptr<streamdeck::jsonrpc::request>,
							std::shared_ptr<streamdeck::jsonrpc::response>);

			void stats(std::shared_ptr<streamdeck::jsonrpc::request>, std::shared_ptr<streamdeck::jsonrpc::response>);

			void tbar(std::weak_ptr<void> handle, std::shared_ptr<streamdeck::jsonrpc::request> req);

			void recording_add_chapter(std::weak_ptr<void>, std::shared_ptr<streamdeck::jsonrpc::request> req);
			void get_output(std::weak_ptr<void>, std::shared_ptr<streamdeck::jsonrpc::request> req);

			private:
			os_cpu_usage_info_t* cpu_info;
			std::string active_scene_collection;
			std::string active_profile;
			bool                 is_loaded;
			static void*                frontend_library;

			static bool (*obs_frontend_recording_add_chapter)(const char*);
		};
	} // namespace handlers
} // namespace streamdeck
