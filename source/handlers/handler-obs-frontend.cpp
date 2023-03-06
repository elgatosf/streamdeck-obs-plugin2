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

#include "handler-obs-frontend.hpp"
#include <memory>
#include <mutex>
#include "module.hpp"
#include "server.hpp"
#include "handler-obs-source.hpp"

/* clang-format off */
#define DLOG(LEVEL, ...) streamdeck::message(streamdeck::log_level:: LEVEL, "[Handler::OBS::Frontend] " __VA_ARGS__)
/* clang-format on */

// Converts a sync function into an async function
static std::function<void(std::weak_ptr<void>, std::shared_ptr<streamdeck::jsonrpc::request>)> make_async(
	obs_task_type task_type,
	std::function<void(std::shared_ptr<streamdeck::jsonrpc::request>, std::shared_ptr<streamdeck::jsonrpc::response>)>
		callback)
{
	// TODO: Make function signature less insane

	return [callback](std::weak_ptr<void> handle, std::shared_ptr<streamdeck::jsonrpc::request> req) {
		struct passed_data {
			std::weak_ptr<void>                           handle;
			std::shared_ptr<streamdeck::jsonrpc::request> req;
			std::function<void(std::shared_ptr<streamdeck::jsonrpc::request>,
							   std::shared_ptr<streamdeck::jsonrpc::response>)>
				callback;
		};
		passed_data* pd = new passed_data();
		pd->handle      = handle;
		pd->req         = req;
		pd->callback    = callback;

		obs_queue_task(
			obs_task_type::OBS_TASK_UI,
			[](void* ptr) {
				nlohmann::json params;

				// Retrieve passed things and clean up immediately.
				passed_data*                                  pd       = static_cast<passed_data*>(ptr);
				std::weak_ptr<void>                           handle   = pd->handle;
				std::shared_ptr<streamdeck::jsonrpc::request> req      = pd->req;
				auto                                          callback = pd->callback;
				delete pd;

				auto res = std::make_shared<streamdeck::jsonrpc::response>();
				res->copy_id(*req);

				callback(req, res);

				streamdeck::server::instance()->reply(handle, res);
			},
			pd, false);
	};
}

std::shared_ptr<streamdeck::handlers::obs_frontend> streamdeck::handlers::obs_frontend::instance()
{
	static std::weak_ptr<streamdeck::handlers::obs_frontend> _instance;
	static std::mutex                                        _lock;

	std::unique_lock<std::mutex> lock(_lock);
	if (_instance.expired()) {
		auto instance = std::make_shared<streamdeck::handlers::obs_frontend>();
		_instance     = instance;
		return instance;
	}

	return _instance.lock();
}

streamdeck::handlers::obs_frontend::~obs_frontend()
{
	os_cpu_usage_info_destroy(cpu_info);
	obs_frontend_remove_event_callback(streamdeck::handlers::obs_frontend::handle_frontend_event, this);
}

streamdeck::handlers::obs_frontend::obs_frontend()
{
	cpu_info = os_cpu_usage_info_start();
	is_loaded = false;

	obs_frontend_add_event_callback(streamdeck::handlers::obs_frontend::handle_frontend_event, this);

	auto server = streamdeck::server::instance();

	server->handle_connect([this]() {
		// Ensure the plugin gets a synthetic loaded event if the frontend is already loaded
		if (this->loaded()) {
			streamdeck::server::instance()->notify("obs.frontend.event.loaded", nlohmann::json::object());
		}
	});

	server->handle_sync("obs.frontend.streaming.start", std::bind(&streamdeck::handlers::obs_frontend::streaming_start,
																  this, std::placeholders::_1, std::placeholders::_2));
	server->handle_sync("obs.frontend.streaming.stop", std::bind(&streamdeck::handlers::obs_frontend::streaming_stop,
																 this, std::placeholders::_1, std::placeholders::_2));
	server->handle_sync("obs.frontend.streaming.active",
						std::bind(&streamdeck::handlers::obs_frontend::streaming_active, this, std::placeholders::_1,
								  std::placeholders::_2));

	server->handle_sync("obs.frontend.recording.start", std::bind(&streamdeck::handlers::obs_frontend::recording_start,
																  this, std::placeholders::_1, std::placeholders::_2));
	server->handle_sync("obs.frontend.recording.stop", std::bind(&streamdeck::handlers::obs_frontend::recording_stop,
																 this, std::placeholders::_1, std::placeholders::_2));
	server->handle_sync("obs.frontend.recording.active",
						std::bind(&streamdeck::handlers::obs_frontend::recording_active, this, std::placeholders::_1,
								  std::placeholders::_2));

	server->handle_sync("obs.frontend.recording.pause", std::bind(&streamdeck::handlers::obs_frontend::recording_pause,
																  this, std::placeholders::_1, std::placeholders::_2));
	server->handle_sync("obs.frontend.recording.unpause",
						std::bind(&streamdeck::handlers::obs_frontend::recording_unpause, this, std::placeholders::_1,
								  std::placeholders::_2));
	server->handle_sync("obs.frontend.recording.paused",
						std::bind(&streamdeck::handlers::obs_frontend::recording_paused, this, std::placeholders::_1,
								  std::placeholders::_2));

	server->handle_async(
		"obs.frontend.replaybuffer.enabled",
		make_async(obs_task_type::OBS_TASK_UI, std::bind(&streamdeck::handlers::obs_frontend::replaybuffer_enabled,
														 this, std::placeholders::_1, std::placeholders::_2)));
	server->handle_async(
		"obs.frontend.replaybuffer.start",
		make_async(obs_task_type::OBS_TASK_UI, std::bind(&streamdeck::handlers::obs_frontend::replaybuffer_start, this,
														 std::placeholders::_1, std::placeholders::_2)));
	server->handle_async(
		"obs.frontend.replaybuffer.save",
		make_async(obs_task_type::OBS_TASK_UI, std::bind(&streamdeck::handlers::obs_frontend::replaybuffer_save, this,
														 std::placeholders::_1, std::placeholders::_2)));
	server->handle_async(
		"obs.frontend.replaybuffer.stop",
		make_async(obs_task_type::OBS_TASK_UI, std::bind(&streamdeck::handlers::obs_frontend::replaybuffer_stop, this,
														 std::placeholders::_1, std::placeholders::_2)));
	server->handle_async(
		"obs.frontend.replaybuffer.active",
		make_async(obs_task_type::OBS_TASK_UI, std::bind(&streamdeck::handlers::obs_frontend::replaybuffer_active, this,
														 std::placeholders::_1, std::placeholders::_2)));

	server->handle_async("obs.frontend.studiomode", std::bind(&streamdeck::handlers::obs_frontend::studiomode, this,
															  std::placeholders::_1, std::placeholders::_2));
	server->handle_async("obs.frontend.studiomode.enable",
						 std::bind(&streamdeck::handlers::obs_frontend::studiomode_enable, this, std::placeholders::_1,
								   std::placeholders::_2));
	server->handle_async("obs.frontend.studiomode.disable",
						 std::bind(&streamdeck::handlers::obs_frontend::studiomode_disable, this, std::placeholders::_1,
								   std::placeholders::_2));
	server->handle_async("obs.frontend.studiomode.active",
						 std::bind(&streamdeck::handlers::obs_frontend::studiomode_active, this, std::placeholders::_1,
								   std::placeholders::_2));

	server->handle_async("obs.frontend.virtualcam", std::bind(&streamdeck::handlers::obs_frontend::virtualcam, this,
															  std::placeholders::_1, std::placeholders::_2));
	server->handle_async("obs.frontend.virtualcam.start",
							std::bind(&streamdeck::handlers::obs_frontend::virtualcam_start, this, std::placeholders::_1,
									std::placeholders::_2));
	server->handle_async("obs.frontend.virtualcam.stop",
						 std::bind(&streamdeck::handlers::obs_frontend::virtualcam_stop, this, std::placeholders::_1,
								   std::placeholders::_2));
	server->handle_async("obs.frontend.virtualcam.active",
						 std::bind(&streamdeck::handlers::obs_frontend::virtualcam_active, this, std::placeholders::_1,
								   std::placeholders::_2));

	server->handle_async("obs.frontend.transition_studio",
						 std::bind(&streamdeck::handlers::obs_frontend::transition_studio, this, std::placeholders::_1,
								   std::placeholders::_2));

	server->handle_async("obs.frontend.scenecollection", std::bind(&streamdeck::handlers::obs_frontend::scenecollection,
																   this, std::placeholders::_1, std::placeholders::_2));
	server->handle_sync("obs.frontend.scenecollection.list",
						std::bind(&streamdeck::handlers::obs_frontend::scenecollection_list, this,
								  std::placeholders::_1, std::placeholders::_2));

	server->handle_async("obs.frontend.profile", std::bind(&streamdeck::handlers::obs_frontend::profile,
																   this, std::placeholders::_1, std::placeholders::_2));
	server->handle_sync("obs.frontend.profile.list",
						std::bind(&streamdeck::handlers::obs_frontend::profile_list, this,
								  std::placeholders::_1, std::placeholders::_2));

	server->handle_async("obs.frontend.scene", std::bind(&streamdeck::handlers::obs_frontend::scene, this,
														 std::placeholders::_1, std::placeholders::_2));
	server->handle_sync("obs.frontend.scene.list", std::bind(&streamdeck::handlers::obs_frontend::scene_list, this,
															 std::placeholders::_1, std::placeholders::_2));

	server->handle_async("obs.frontend.transition", std::bind(&streamdeck::handlers::obs_frontend::transition, this,
															  std::placeholders::_1, std::placeholders::_2));
	server->handle_async("obs.frontend.transition.list", std::bind(&streamdeck::handlers::obs_frontend::transition_list,
																   this, std::placeholders::_1, std::placeholders::_2));

	server->handle_sync("obs.frontend.screenshot", std::bind(&streamdeck::handlers::obs_frontend::screenshot, this,
															 std::placeholders::_1, std::placeholders::_2));

	server->handle_sync("obs.frontend.stats", std::bind(&streamdeck::handlers::obs_frontend::stats, this,
														std::placeholders::_1, std::placeholders::_2));

	server->handle_async("obs.frontend.tbar", std::bind(&streamdeck::handlers::obs_frontend::tbar, this,
														 std::placeholders::_1, std::placeholders::_2));
}

bool streamdeck::handlers::obs_frontend::loaded() const {
	return is_loaded;
}

void streamdeck::handlers::obs_frontend::handle_frontend_event(enum obs_frontend_event event, void* private_data)
{
	static_cast<streamdeck::handlers::obs_frontend*>(private_data)->handle(event);
}

void streamdeck::handlers::obs_frontend::handle(enum obs_frontend_event event)
{
	try {
		std::string    method;
		nlohmann::json reply = nlohmann::json::object();

		switch (event) {
		case OBS_FRONTEND_EVENT_FINISHED_LOADING:
			method = "obs.frontend.event.loaded";
			is_loaded = true;
			{
				char* name     = obs_frontend_get_current_profile();
				active_profile = name;
				bfree(name);

				name                    = obs_frontend_get_current_scene_collection();
				active_scene_collection = name;
				bfree(name);

				struct obs_frontend_source_list list = {0};
				obs_frontend_get_transitions(&list);
				for (size_t idx = 0; idx < list.sources.num; idx++) {
					obs_source_t* source = *(list.sources.array + idx);
					auto          osh    = obs_source_get_signal_handler(source);
					signal_handler_connect(osh, "rename", &streamdeck::handlers::obs_frontend::on_rename_transition,
										   this);
				}
				obs_frontend_source_list_free(&list);
			}
			break;

		case OBS_FRONTEND_EVENT_STREAMING_STARTING:
		case OBS_FRONTEND_EVENT_STREAMING_STARTED:
		case OBS_FRONTEND_EVENT_STREAMING_STOPPING:
		case OBS_FRONTEND_EVENT_STREAMING_STOPPED:
			method = "obs.frontend.event.streaming";
			break;

		case OBS_FRONTEND_EVENT_RECORDING_STARTING:
		case OBS_FRONTEND_EVENT_RECORDING_STARTED:
		case OBS_FRONTEND_EVENT_RECORDING_STOPPING:
		case OBS_FRONTEND_EVENT_RECORDING_STOPPED:
		case OBS_FRONTEND_EVENT_RECORDING_PAUSED:
		case OBS_FRONTEND_EVENT_RECORDING_UNPAUSED:
			method = "obs.frontend.event.recording";
			break;

		case OBS_FRONTEND_EVENT_REPLAY_BUFFER_STARTING:
		case OBS_FRONTEND_EVENT_REPLAY_BUFFER_STARTED:
		case OBS_FRONTEND_EVENT_REPLAY_BUFFER_STOPPING:
		case OBS_FRONTEND_EVENT_REPLAY_BUFFER_STOPPED:
			method = "obs.frontend.event.replaybuffer";
			break;

		case OBS_FRONTEND_EVENT_STUDIO_MODE_ENABLED:
		case OBS_FRONTEND_EVENT_STUDIO_MODE_DISABLED:
			method = "obs.frontend.event.studiomode";
			break;

		case OBS_FRONTEND_EVENT_VIRTUALCAM_STARTED:
		case OBS_FRONTEND_EVENT_VIRTUALCAM_STOPPED:
			method = "obs.frontend.event.virtualcam";
			break;

			
		#if OBS_MINIMUM_SUPPORT >= 280000
		case OBS_FRONTEND_EVENT_PROFILE_RENAMED:
			method         = "obs.frontend.event.profile.renamed";
			{
				char* profile_name = obs_frontend_get_current_profile();
				reply["from"]       = active_profile;
				active_profile     = profile_name;
				reply["to"]       = active_profile;
				bfree(profile_name);
			}
			break;
		#endif


		case OBS_FRONTEND_EVENT_PROFILE_CHANGED:
			method = "obs.frontend.event.profile";
			{
				char* profile_name = obs_frontend_get_current_profile();
				active_profile     = profile_name;
				reply["profile"]   = profile_name;
				bfree(profile_name);
			}
			break;

		case OBS_FRONTEND_EVENT_PROFILE_LIST_CHANGED:
			method = "obs.frontend.event.profiles";
			{
				reply["profiles"] = nlohmann::json::array();
				char **profile_names = obs_frontend_get_profiles();
				char** profile       = profile_names;
				while (*profile) {
					reply["profiles"].push_back(*profile);
					profile++;
				}
				bfree(profile_names);
			}
			break;

		case OBS_FRONTEND_EVENT_SCENE_CHANGED:
		case OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED:
			method = "obs.frontend.event.scene";
			{
				obs_source_t* source = obs_frontend_get_current_scene();
				const char*   data   = obs_source_get_name(source);
				reply["program"]     = data ? nlohmann::json(data) : nlohmann::json(nullptr);
				obs_source_release(source);
			}
			if (obs_frontend_preview_program_mode_active()) {
				obs_source_t* source = obs_frontend_get_current_preview_scene();
				const char*   data   = obs_source_get_name(source);
				reply["preview"]     = data ? nlohmann::json(data) : nlohmann::json(nullptr);
				obs_source_release(source);
			} else {
				reply["preview"] = reply["program"];
			}
			break;

		case OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED:
			method          = "obs.frontend.event.scenes";
			reply["scenes"] = nlohmann::json::array();
			{
				struct obs_frontend_source_list list = {0};
				obs_frontend_get_scenes(&list);
				for (size_t idx = 0; idx < list.sources.num; idx++) {
					obs_source_t* source = *(list.sources.array + idx);
					const char*   data   = obs_source_get_name(source);
					reply["scenes"].push_back(data ? data : "");
				}
				obs_frontend_source_list_free(&list);
			}
			break;

		#if OBS_MINIMUM_SUPPORT >= 280000
		case OBS_FRONTEND_EVENT_SCENE_COLLECTION_RENAMED:
			method = "obs.frontend.event.scenecollection.renamed";
			reply["from"]            = active_scene_collection;
			active_scene_collection = obs_frontend_get_current_scene_collection();
			reply["to"]            = active_scene_collection;
			break;
		#endif

		case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED:
			method                  = "obs.frontend.event.scenecollection";
			active_scene_collection = obs_frontend_get_current_scene_collection();
			reply["collection"] = active_scene_collection;
			break;

		case OBS_FRONTEND_EVENT_SCENE_COLLECTION_LIST_CHANGED:
			method               = "obs.frontend.event.scenecollections";
			reply["collections"] = nlohmann::json::array();
			{
				char** collections = obs_frontend_get_scene_collections();
				for (char** ptr = collections; *ptr != nullptr; ptr++) {
					reply["collections"].push_back(*ptr);
				}
				bfree(collections);
			}
			break;

		case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CLEANUP:
			method = "obs.frontend.event.scenecollectioncleanup";
			break;

		case OBS_FRONTEND_EVENT_TRANSITION_CHANGED:
			method = "obs.frontend.event.transition";
			{
				obs_source_t* source = obs_frontend_get_current_transition();
				reply["transition"]  = obs_source_get_name(source);
				obs_source_release(source);
			}
			break;

		case OBS_FRONTEND_EVENT_TRANSITION_STOPPED:
			method = "obs.frontend.event.transition.stopped";
			break;

		case OBS_FRONTEND_EVENT_TRANSITION_DURATION_CHANGED:
			method            = "obs.frontend.event.transition.duration";
			reply["duration"] = obs_frontend_get_transition_duration();
			break;

		case OBS_FRONTEND_EVENT_TRANSITION_LIST_CHANGED:
			method               = "obs.frontend.event.transitions";
			reply["transitions"] = nlohmann::json::array();
			{
				auto source_api = obs_source::instance();

				struct obs_frontend_source_list list = {0};
				obs_frontend_get_transitions(&list);
				for (size_t idx = 0; idx < list.sources.num; idx++) {
					obs_source_t* source = *(list.sources.array + idx);
					const char*   name   = obs_source_get_name(source);
					bool          fixed  = obs_transition_fixed(source);
					auto          trans  = nlohmann::json::object();
					trans["name"]        = name ? name : "";
					trans["fixed"]       = fixed;

					reply["transitions"].push_back(trans);
					auto osh = obs_source_get_signal_handler(source);
					signal_handler_disconnect(osh, "rename", &streamdeck::handlers::obs_frontend::on_rename_transition,
											  this);
					signal_handler_connect(osh, "rename", &streamdeck::handlers::obs_frontend::on_rename_transition,
										   this);
				}
				obs_frontend_source_list_free(&list);
			}
			break;

		case OBS_FRONTEND_EVENT_TBAR_VALUE_CHANGED:
			method = "obs.frontend.event.tbar";
			reply["position"] = obs_frontend_get_tbar_position();
			break;
		}

		switch (event) {
		case OBS_FRONTEND_EVENT_STREAMING_STARTING:
		case OBS_FRONTEND_EVENT_RECORDING_STARTING:
		case OBS_FRONTEND_EVENT_REPLAY_BUFFER_STARTING:
			reply["state"] = "STARTING";
			break;

		case OBS_FRONTEND_EVENT_STREAMING_STARTED:
		case OBS_FRONTEND_EVENT_RECORDING_STARTED:
		case OBS_FRONTEND_EVENT_REPLAY_BUFFER_STARTED:
			reply["state"] = "STARTED";
			break;

		case OBS_FRONTEND_EVENT_STREAMING_STOPPING:
		case OBS_FRONTEND_EVENT_RECORDING_STOPPING:
		case OBS_FRONTEND_EVENT_REPLAY_BUFFER_STOPPING:
			reply["state"] = "STOPPING";
			break;

		case OBS_FRONTEND_EVENT_STREAMING_STOPPED:
		case OBS_FRONTEND_EVENT_RECORDING_STOPPED:
		case OBS_FRONTEND_EVENT_REPLAY_BUFFER_STOPPED:
			reply["state"] = "STOPPED";
			break;

		case OBS_FRONTEND_EVENT_RECORDING_PAUSED:
			reply["state"] = "PAUSED";
			break;

		case OBS_FRONTEND_EVENT_RECORDING_UNPAUSED:
			reply["state"] = "UNPAUSED";
			break;

		case OBS_FRONTEND_EVENT_STUDIO_MODE_ENABLED:
			reply["state"] = "ENABLED";
			break;

		case OBS_FRONTEND_EVENT_STUDIO_MODE_DISABLED:
			reply["state"] = "DISABLED";
			break;

		case OBS_FRONTEND_EVENT_VIRTUALCAM_STARTED:
			reply["state"] = "STARTED";
			break;

		case OBS_FRONTEND_EVENT_VIRTUALCAM_STOPPED:
			reply["state"] = "STOPPED";
			break;

		}

		streamdeck::server::instance()->notify(method, reply);
	} catch (std::exception const& ex) {
		DLOG(LOG_DEBUG, "Error: %s", ex.what());
	}
}

void streamdeck::handlers::obs_frontend::on_rename_transition(void*, calldata_t* calldata)
{
	// Retrieve information.
	obs_source_t* source;
	const char*   old_name;
	const char*   new_name;
	if (!calldata_get_ptr(calldata, "source", &source)) {
		DLOG(LOG_WARNING,
			 "Failed to retrieve 'source' entry from call data in 'source_rename' signal. This is a bug in OBS "
			 "Studio.");
		return;
	} else if (!calldata_get_string(calldata, "prev_name", &old_name)) {
		DLOG(LOG_WARNING,
			 "Failed to retrieve 'prev_name' entry from call data in 'source_rename' signal. This is a bug in "
			 "OBS "
			 "Studio.");
		return;
	} else if (!calldata_get_string(calldata, "new_name", &new_name)) {
		DLOG(LOG_WARNING,
			 "Failed to retrieve 'new_name' entry from call data in 'source_rename' signal. This is a bug in "
			 "OBS "
			 "Studio.");
		return;
	}

	// Do our WebSocket work.
	nlohmann::json reply = nlohmann::json::object();
	reply["from"]        = old_name ? old_name : "";
	reply["to"]          = new_name ? new_name : "";

	streamdeck::server::instance()->notify("obs.frontend.event.transition.rename", reply);
}

void streamdeck::handlers::obs_frontend::streaming_start(std::shared_ptr<streamdeck::jsonrpc::request>,
														 std::shared_ptr<streamdeck::jsonrpc::response> res)
{
	/** obs.frontend.streaming.stop
	 *
	 * @return {bool} `true` if Streaming was started, otherwise `false`.
	 */

	if (!obs_frontend_streaming_active()) {
		obs_frontend_streaming_start();
		res->set_result(true);
	} else {
		res->set_result(false);
	}
}

void streamdeck::handlers::obs_frontend::streaming_stop(std::shared_ptr<streamdeck::jsonrpc::request>,
														std::shared_ptr<streamdeck::jsonrpc::response> res)
{
	/** obs.frontend.streaming.stop
	 *
	 * @return {bool} `true` if Streaming was stopped, otherwise `false`.
	 */

	if (obs_frontend_streaming_active()) {
		obs_frontend_streaming_stop();
		res->set_result(true);
	} else {
		res->set_result(false);
	}
}

void streamdeck::handlers::obs_frontend::streaming_active(std::shared_ptr<streamdeck::jsonrpc::request>,
														  std::shared_ptr<streamdeck::jsonrpc::response> res)
{
	/** obs.frontend.streaming.active
	 *
	 * @return {bool} `true` if Streaming is active, otherwise `false`.
	 */

	res->set_result(obs_frontend_streaming_active());
}

void streamdeck::handlers::obs_frontend::recording_start(std::shared_ptr<streamdeck::jsonrpc::request>,
														 std::shared_ptr<streamdeck::jsonrpc::response> res)
{
	/** obs.frontend.recording.start
	 *
	 * @return {bool} `true` if Recording was started, otherwise `false`.
	 */

	if (!obs_frontend_recording_active()) {
		obs_frontend_recording_start();
		res->set_result(true);
	} else {
		res->set_result(false);
	}
}

void streamdeck::handlers::obs_frontend::recording_stop(std::shared_ptr<streamdeck::jsonrpc::request>,
														std::shared_ptr<streamdeck::jsonrpc::response> res)
{
	/** obs.frontend.recording.stop
	 *
	 * @return {bool} `true` if Recording was stopped, otherwise `false`.
	 */

	if (obs_frontend_recording_active()) {
		obs_frontend_recording_stop();
		res->set_result(true);
	} else {
		res->set_result(false);
	}
}

void streamdeck::handlers::obs_frontend::recording_active(std::shared_ptr<streamdeck::jsonrpc::request>,
														  std::shared_ptr<streamdeck::jsonrpc::response> res)
{
	/** obs.frontend.recording.active
	 *
	 * @return {bool} `true` if Recording is active, otherwise `false`.
	 */

	res->set_result(obs_frontend_recording_active());
}

void streamdeck::handlers::obs_frontend::recording_pause(std::shared_ptr<streamdeck::jsonrpc::request>,
														 std::shared_ptr<streamdeck::jsonrpc::response> res)
{
	/** obs.frontend.recording.pause
	 *
	 * @return {bool} `true` if Recording was paused, otherwise `false`.
	 */

	if (obs_frontend_recording_active()) {
		obs_frontend_recording_pause(true);
		res->set_result(obs_frontend_recording_paused());
	} else {
		res->set_result(false);
	}
}

void streamdeck::handlers::obs_frontend::recording_unpause(std::shared_ptr<streamdeck::jsonrpc::request>,
														   std::shared_ptr<streamdeck::jsonrpc::response> res)
{
	/** obs.frontend.recording.unpause
	 *
	 * @return {bool} `true` if Recording was unpaused, otherwise `false`.
	 */

	if (obs_frontend_recording_active()) {
		obs_frontend_recording_pause(false);
		res->set_result(!obs_frontend_recording_paused());
	} else {
		res->set_result(false);
	}
}

void streamdeck::handlers::obs_frontend::recording_paused(std::shared_ptr<streamdeck::jsonrpc::request>,
														  std::shared_ptr<streamdeck::jsonrpc::response> res)
{
	/** obs.frontend.recording.paused
	 *
	 * @return {bool} `true` if Recording is paused, otherwise `false`.
	 */

	res->set_result(obs_frontend_recording_paused());
}

bool is_replaybuffer_enabled()
{
	// SDOI-273: obs_frontend_get_replay_buffer_output can only be called in a UI deferred action
	auto out = obs_frontend_get_replay_buffer_output();
	bool res = out != nullptr;
	obs_output_release(out);
	return res;
}

void streamdeck::handlers::obs_frontend::replaybuffer_enabled(std::shared_ptr<streamdeck::jsonrpc::request>,
															  std::shared_ptr<streamdeck::jsonrpc::response> res)
{
	/** obs.frontend.replaybuffer.enabled
	 *
	 * @return {bool} `true` if Replay Buffer is enabled, otherwise `false`.
	 */

	res->set_result(is_replaybuffer_enabled());
}

void streamdeck::handlers::obs_frontend::replaybuffer_start(std::shared_ptr<streamdeck::jsonrpc::request>,
															std::shared_ptr<streamdeck::jsonrpc::response> res)
{
	/** obs.frontend.replaybuffer.start
	 *
	 * @return {bool} `true` if Replay Buffer was started, otherwise `false`.
	 */

	if (!is_replaybuffer_enabled() || obs_frontend_replay_buffer_active()) {
		res->set_result(false);
	}

	obs_frontend_replay_buffer_start();
	res->set_result(obs_frontend_replay_buffer_active());
}

void streamdeck::handlers::obs_frontend::replaybuffer_save(std::shared_ptr<streamdeck::jsonrpc::request>,
														   std::shared_ptr<streamdeck::jsonrpc::response> res)
{
	/** obs.frontend.replaybuffer.save
	 *
	 * @return {bool} `true` if Replay Buffer was saved, otherwise `false`.
	 */

	if (!is_replaybuffer_enabled() || !obs_frontend_replay_buffer_active()) {
		res->set_result(false);
	}

	obs_frontend_replay_buffer_save();
	res->set_result(true);
}

void streamdeck::handlers::obs_frontend::replaybuffer_stop(std::shared_ptr<streamdeck::jsonrpc::request>,
														   std::shared_ptr<streamdeck::jsonrpc::response> res)
{
	/** obs.frontend.replaybuffer.stop
	 *
	 * @return {bool} `true` if Replay Buffer was stopped, otherwise `false`.
	 */

	if (!is_replaybuffer_enabled() || !obs_frontend_replay_buffer_active()) {
		res->set_result(false);
	}

	obs_frontend_replay_buffer_stop();
	res->set_result(!obs_frontend_replay_buffer_active());
}

void streamdeck::handlers::obs_frontend::replaybuffer_active(std::shared_ptr<streamdeck::jsonrpc::request>,
															 std::shared_ptr<streamdeck::jsonrpc::response> res)
{
	/** obs.frontend.replaybuffer.active
	 *
	 * @return {bool} `true` if Replay Buffer is active, otherwise `false`.
	 */

	res->set_result(obs_frontend_replay_buffer_active());
}

void streamdeck::handlers::obs_frontend::studiomode(std::weak_ptr<void>                           handle,
													std::shared_ptr<streamdeck::jsonrpc::request> req)
{
	/** obs.frontend.studiomode
	 *
	 * @param {bool} enabled [Optional] `true` to enable studio mode, or `false` to disable it.
	 * 
	 * @return {bool} `true` if Studio Mode is enabled, otherwise `false`.
	 */

	struct passed_data {
		std::weak_ptr<void>                           handle;
		std::shared_ptr<streamdeck::jsonrpc::request> req;
	};
	passed_data* pd = new passed_data();
	pd->handle      = handle;
	pd->req         = req;

	// Studio Mode affects UI directly, so we need to perform this in the UI thread.
	obs_queue_task(
		obs_task_type::OBS_TASK_UI,
		[](void* ptr) {
			nlohmann::json params;

			// Retrieve passed things and clean up immediately.
			passed_data*                                  pd     = static_cast<passed_data*>(ptr);
			std::weak_ptr<void>                           handle = pd->handle;
			std::shared_ptr<streamdeck::jsonrpc::request> req    = pd->req;
			delete pd;

			// If there was a request to change mode, do it.
			if (req->get_params(params)) {
				auto p = params.find("enabled");
				if (p != params.end()) {
					if (!p->is_boolean()) {
						throw jsonrpc::invalid_params_error("Parameter 'enabled' must be a Boolean.");
					}

					obs_frontend_set_preview_program_mode(p->get<bool>());
				}
			}

			// Reply with current state.
			auto res = std::make_shared<streamdeck::jsonrpc::response>();
			res->copy_id(*req);
			res->set_result(obs_frontend_preview_program_mode_active());
			streamdeck::server::instance()->reply(handle, res);
		},
		pd, false);
}

void streamdeck::handlers::obs_frontend::studiomode_enable(std::weak_ptr<void>                           handle,
														   std::shared_ptr<streamdeck::jsonrpc::request> req)
{
	/** obs.frontend.studiomode.enable
	 *
	 * Alias of obs.frontend.studiomode(enabled=true).
	 */

	nlohmann::json params;
	params["enabled"] = true;
	req->set_params(params);
	studiomode(handle, req);
}

void streamdeck::handlers::obs_frontend::studiomode_disable(std::weak_ptr<void>                           handle,
															std::shared_ptr<streamdeck::jsonrpc::request> req)
{
	/** obs.frontend.studiomode.disable
	 *
	 * Alias of obs.frontend.studiomode(enabled=false).
	 */

	nlohmann::json params;
	params["enabled"] = false;
	req->set_params(params);
	studiomode(handle, req);
}

void streamdeck::handlers::obs_frontend::studiomode_active(std::weak_ptr<void>                           handle,
														   std::shared_ptr<streamdeck::jsonrpc::request> req)
{
	/** obs.frontend.studiomode.active
	 *
	 * Alias of obs.frontend.studiomode().
	 */
	studiomode(handle, req);
}


void streamdeck::handlers::obs_frontend::virtualcam(std::weak_ptr<void>                           handle,
													std::shared_ptr<streamdeck::jsonrpc::request> req)
{
	/** obs.frontend.virtualcam
	 *
	 * @param {bool} enabled [Optional] `true` to start the virtual cam, or `false` to disable it.
	 * 
	 * @return {bool} `true` if the virtual cam is enabled, otherwise `false`.
	 */

	struct passed_data {
		std::weak_ptr<void>                           handle;
		std::shared_ptr<streamdeck::jsonrpc::request> req;
	};
	passed_data* pd = new passed_data();
	pd->handle      = handle;
	pd->req         = req;

	obs_queue_task(
		obs_task_type::OBS_TASK_UI,
		[](void* ptr) {
			nlohmann::json params;

			// Retrieve passed things and clean up immediately.
			passed_data*                                  pd     = static_cast<passed_data*>(ptr);
			std::weak_ptr<void>                           handle = pd->handle;
			std::shared_ptr<streamdeck::jsonrpc::request> req    = pd->req;
			delete pd;

			bool current_state = obs_frontend_virtualcam_active();

			// If there was a request to change mode, do it.
			if (req->get_params(params)) {
				auto p = params.find("enabled");
				if (p != params.end()) {
					if (!p->is_boolean()) {
						throw jsonrpc::invalid_params_error("Parameter 'enabled' must be a Boolean.");
					}

					auto new_state = p->get<bool>();
					if (new_state != current_state) {
						if (new_state) {
							obs_frontend_start_virtualcam();
						} else {
							obs_frontend_stop_virtualcam();
						}
					}
				}
			}

			// Reply with current state.
			auto res = std::make_shared<streamdeck::jsonrpc::response>();
			res->copy_id(*req);
			res->set_result(obs_frontend_virtualcam_active());
			streamdeck::server::instance()->reply(handle, res);
		},
		pd, false);
}

void streamdeck::handlers::obs_frontend::virtualcam_start(std::weak_ptr<void>                           handle,
														   std::shared_ptr<streamdeck::jsonrpc::request> req)
{
	/** obs.frontend.virtualcam.start
	 *
	 * Alias of obs.frontend.virtualcam(enabled=true).
	 */

	nlohmann::json params;
	params["enabled"] = true;
	req->set_params(params);
	virtualcam(handle, req);
}

void streamdeck::handlers::obs_frontend::virtualcam_stop(std::weak_ptr<void>                           handle,
															std::shared_ptr<streamdeck::jsonrpc::request> req)
{
	/** obs.frontend.virtualcam.stop
	 *
	 * Alias of obs.frontend.virtualcam(enabled=false).
	 */

	nlohmann::json params;
	params["enabled"] = false;
	req->set_params(params);
	virtualcam(handle, req);
}

void streamdeck::handlers::obs_frontend::virtualcam_active(std::weak_ptr<void>                           handle,
														   std::shared_ptr<streamdeck::jsonrpc::request> req)
{
	/** obs.frontend.studiomode.active
	 *
	 * Alias of obs.frontend.virtualcam().
	 */
	virtualcam(handle, req);
}

void streamdeck::handlers::obs_frontend::transition_studio(std::weak_ptr<void>                           handle,
														   std::shared_ptr<streamdeck::jsonrpc::request> req)
{
	/** obs.frontend.transition_studio
	 *
	 * Transitions the "preview" scene into the "program" scene
	 */

	struct passed_data {
		std::weak_ptr<void>                           handle;
		std::shared_ptr<streamdeck::jsonrpc::request> req;
	};
	passed_data* pd = new passed_data();
	pd->handle      = handle;
	pd->req         = req;

	obs_queue_task(
		obs_task_type::OBS_TASK_UI,
		[](void* ptr) {
			// Retrieve passed things and clean up immediately.
			passed_data*                                  pd     = static_cast<passed_data*>(ptr);
			std::weak_ptr<void>                           handle = pd->handle;
			std::shared_ptr<streamdeck::jsonrpc::request> req    = pd->req;
			delete pd;

			std::shared_ptr<streamdeck::jsonrpc::response> res = std::make_shared<streamdeck::jsonrpc::response>();

			obs_frontend_preview_program_trigger_transition();
			res->copy_id(*req);
			res->set_result("");

			try {
				res->validate();
				streamdeck::server::instance()->reply(handle, res);
			} catch (std::exception const& ex) {
				DLOG(LOG_ERROR, "Failed to send reply: %s", ex.what());
			}
		},
		pd, false);
}

void streamdeck::handlers::obs_frontend::scenecollection(std::weak_ptr<void>                           handle,
														 std::shared_ptr<streamdeck::jsonrpc::request> req)
{
	/** obs.frontend.scenecollection
	 *
	 * @param collection {string} [Optional] Name of the scene collection to which to switch to.
	 *
	 * @return {string} The name of the current scene collection.
	 */

	// Validate parameters.
	nlohmann::json params;
	if (req->get_params(params)) {
		auto p = params.find("collection");
		if (p != params.end()) {
			if (!p->is_string()) {
				throw jsonrpc::invalid_params_error("Parameter 'collection' must be a string or String.");
			}
		}
	}

	struct passed_data {
		std::weak_ptr<void>                           handle;
		std::shared_ptr<streamdeck::jsonrpc::request> req;
	};
	passed_data* pd = new passed_data();
	pd->handle      = handle;
	pd->req         = req;

	// Some Frontend interaction requires a frontend task.
	obs_queue_task(
		obs_task_type::OBS_TASK_UI,
		[](void* ptr) {
			// Retrieve passed things and clean up immediately.
			passed_data*                                  pd     = static_cast<passed_data*>(ptr);
			std::weak_ptr<void>                           handle = pd->handle;
			std::shared_ptr<streamdeck::jsonrpc::request> req    = pd->req;
			delete pd;

			std::shared_ptr<streamdeck::jsonrpc::response> res = std::make_shared<streamdeck::jsonrpc::response>();
			res->copy_id(*req);
			try {
				nlohmann::json params;
				req->get_params(params);
				if (params.size() > 0) {
					auto kv = params.find("collection");
					if ((kv != params.end()) && (kv->is_string())) {
						std::string collection;
						kv->get_to(collection);
						obs_frontend_set_current_scene_collection(collection.c_str());
					} else {
						throw streamdeck::jsonrpc::invalid_params_error("'collection' must be a string value.");
					}
				}

				const char* col = obs_frontend_get_current_scene_collection();
				res->set_result(col ? col : "");
			} catch (streamdeck::jsonrpc::error const& ex) {
				res->set_error(ex.id(), ex.what() ? ex.what() : "Unknown error.");
			}
			try {
				res->validate();
				streamdeck::server::instance()->reply(handle, res);
			} catch (std::exception const& ex) {
				DLOG(LOG_ERROR, "Failed to send reply: %s", ex.what());
			}
		},
		pd, false);
}

void streamdeck::handlers::obs_frontend::scenecollection_list(std::shared_ptr<streamdeck::jsonrpc::request>,
															  std::shared_ptr<streamdeck::jsonrpc::response> res)
{
	/** obs.frontend.scenecollection.list
	 *
	 * @return {Array(string)} An array of strings containing all available scene collections.
	 */

	nlohmann::json collections = nlohmann::json::array();

	char** list = obs_frontend_get_scene_collections();
	for (char** ptr = list; *ptr != nullptr; ptr++) {
		collections.push_back(*ptr);
	}
	bfree(list);

	res->set_result(collections);
}


void streamdeck::handlers::obs_frontend::profile(std::weak_ptr<void>                           handle,
														 std::shared_ptr<streamdeck::jsonrpc::request> req)
{
	/** obs.frontend.profile
	 *
	 * @param profile {string} [Optional] Name of the profile to which to switch to.
	 *
	 * @return {string} The name of the current profile.
	 */

	// Validate parameters.
	nlohmann::json params;
	if (req->get_params(params)) {
		auto p = params.find("profile");
		if (p != params.end()) {
			if (!p->is_string()) {
				throw jsonrpc::invalid_params_error("Parameter 'profile' must be a string or String.");
			}
		}
	}

	struct passed_data {
		std::weak_ptr<void>                           handle;
		std::shared_ptr<streamdeck::jsonrpc::request> req;
	};
	passed_data* pd = new passed_data();
	pd->handle      = handle;
	pd->req         = req;

	// Some Frontend interaction requires a frontend task.
	obs_queue_task(
		obs_task_type::OBS_TASK_UI,
		[](void* ptr) {
			// Retrieve passed things and clean up immediately.
			passed_data*                                  pd     = static_cast<passed_data*>(ptr);
			std::weak_ptr<void>                           handle = pd->handle;
			std::shared_ptr<streamdeck::jsonrpc::request> req    = pd->req;
			delete pd;

			std::shared_ptr<streamdeck::jsonrpc::response> res = std::make_shared<streamdeck::jsonrpc::response>();
			res->copy_id(*req);
			try {
				nlohmann::json params;
				req->get_params(params);
				if (params.size() > 0) {
					auto kv = params.find("profile");
					if ((kv != params.end()) && (kv->is_string())) {
						std::string collection;
						kv->get_to(collection);
						obs_frontend_set_current_profile(collection.c_str());
					} else {
						throw streamdeck::jsonrpc::invalid_params_error("'profile' must be a string value.");
					}
				}

				char* profile = obs_frontend_get_current_profile();
				res->set_result(profile ? profile : "");
				bfree(profile);
			} catch (streamdeck::jsonrpc::error const& ex) {
				res->set_error(ex.id(), ex.what() ? ex.what() : "Unknown error.");
			}
			try {
				res->validate();
				streamdeck::server::instance()->reply(handle, res);
			} catch (std::exception const& ex) {
				DLOG(LOG_ERROR, "Failed to send reply: %s", ex.what());
			}
		},
		pd, false);
}

void streamdeck::handlers::obs_frontend::profile_list(std::shared_ptr<streamdeck::jsonrpc::request>,
															  std::shared_ptr<streamdeck::jsonrpc::response> res)
{
	/** obs.frontend.profile.list
	 *
	 * @return {Array(string)} An array of strings containing all available profiles.
	 */

	nlohmann::json profiles = nlohmann::json::array();

	char** list = obs_frontend_get_profiles();
	for (char** ptr = list; *ptr != nullptr; ptr++) {
		profiles.push_back(*ptr);
	}
	bfree(list);

	res->set_result(profiles);
}

void streamdeck::handlers::obs_frontend::scene(std::weak_ptr<void>                           handle,
											   std::shared_ptr<streamdeck::jsonrpc::request> req)
{
	/** obs.frontend.scene
	 *
	 * @param program {bool} [Optional] Which output should be targeted? `true` means 'Program', `false` means 'Preview'. Default is `false`.
	 * @param scene {string} [Optional] Name of the scene to which to switch to.
	 *
	 * @return {string} The current scene in the selected output.
	 */

	// Validate parameters.
	nlohmann::json params;
	if (req->get_params(params)) {
		{
			auto p = params.find("program");
			if (p != params.end()) {
				if (!p->is_boolean()) {
					throw jsonrpc::invalid_params_error("Parameter 'program' must be a Boolean.");
				}
			}
		}

		{
			auto p = params.find("scene");
			if (p != params.end()) {
				if (!p->is_string()) {
					throw jsonrpc::invalid_params_error("Parameter 'scene' must be a string or String.");
				}
			}
		}
	}

	struct passed_data {
		std::weak_ptr<void>                           handle;
		std::shared_ptr<streamdeck::jsonrpc::request> req;
	};
	passed_data* pd = new passed_data();
	pd->handle      = handle;
	pd->req         = req;

	// Some Frontend interaction requires a frontend task.
	obs_queue_task(
		obs_task_type::OBS_TASK_UI,
		[](void* ptr) {
			// Retrieve passed things and clean up immediately.
			passed_data*                                  pd     = static_cast<passed_data*>(ptr);
			std::weak_ptr<void>                           handle = pd->handle;
			std::shared_ptr<streamdeck::jsonrpc::request> req    = pd->req;
			delete pd;

			std::shared_ptr<streamdeck::jsonrpc::response> res = std::make_shared<streamdeck::jsonrpc::response>();
			res->copy_id(*req);
			try {
				nlohmann::json params;
				req->get_params(params);

				bool is_program_scene = false;
				{
					auto key = params.find("program");
					if (key != params.end()) {
						is_program_scene = key->get<bool>();
					}
				}

				{
					auto key = params.find("scene");
					if (key != params.end()) {
						std::string scene_name;
						key->get_to(scene_name);

						obs_source_t* scene = obs_get_source_by_name(scene_name.c_str());
						if (!is_program_scene && obs_frontend_preview_program_mode_active()) {
							obs_frontend_set_current_preview_scene(scene);
						} else {
							obs_frontend_set_current_scene(scene);
						}
						obs_source_release(scene);
					}
				}

				obs_source_t* source = nullptr;
				if (!is_program_scene && obs_frontend_preview_program_mode_active()) {
					source = obs_frontend_get_current_preview_scene();
				} else {
					source = obs_frontend_get_current_scene();
				}
				const char* name = obs_source_get_name(source);
				obs_source_release(source);

				res->set_result(name ? name : "");
			} catch (streamdeck::jsonrpc::error const& ex) {
				res->set_error(ex.id(), ex.what() ? ex.what() : "Unknown error.");
			}

			try {
				res->validate();
				streamdeck::server::instance()->reply(handle, res);
			} catch (std::exception const& ex) {
				DLOG(LOG_ERROR, "Failed to send reply: %s", ex.what());
			}
		},
		pd, false);
}

void streamdeck::handlers::obs_frontend::scene_list(std::shared_ptr<streamdeck::jsonrpc::request>,
													std::shared_ptr<streamdeck::jsonrpc::response> res)
{
	/** obs.frontend.scene.list
	 *
	 * @return {Array(string)} An array of strings containing all available scenes.
	 */

	nlohmann::json scenes = nlohmann::json::array();

	struct obs_frontend_source_list list = {0};
	obs_frontend_get_scenes(&list);
	for (size_t idx = 0; idx < list.sources.num; idx++) {
		obs_source_t* scene = *(list.sources.array + idx);
		scenes.push_back(obs_source_get_name(scene));
	}
	obs_frontend_source_list_free(&list);

	res->set_result(scenes);
}

void streamdeck::handlers::obs_frontend::transition(std::weak_ptr<void>                           handle,
													std::shared_ptr<streamdeck::jsonrpc::request> req)
{
	/** obs.frontend.transition
	 *
	 * @param transition {string} [Optional] 
	 * @param duration {int} [Optional] 
	 * @return {{transition: string, duration: int}} The unique name of the current transition and the duration.
	 */

	struct task_data {
		std::weak_ptr<void>                           handle;
		std::shared_ptr<streamdeck::jsonrpc::request> req;
	};
	auto td    = new task_data;
	td->req    = req;
	td->handle = handle;

	obs_queue_task(
		obs_task_type::OBS_TASK_UI,
		[](void* ptr) {
			nlohmann::json params;
			nlohmann::json p_transition;
			nlohmann::json p_duration;
			auto           td = reinterpret_cast<task_data*>(ptr);
			try {
				auto res = std::make_shared<streamdeck::jsonrpc::response>();
				res->copy_id(*td->req);

				if (td->req->get_params(params)) {
					nlohmann::json::iterator param;

					param = params.find("transition");
					if (param != params.end()) {
						if (param->is_string()) {
							p_transition = *param;
						} else {
							throw jsonrpc::invalid_params_error(
								"The parameter 'transition' must be of type 'string' if present.");
						}
					}

					param = params.find("duration");
					if (param != params.end()) {
						if (param->is_number_integer()) {
							p_duration = *param;
						} else {
							throw jsonrpc::invalid_params_error(
								"The parameter 'duration' must be of type 'integer' if present.");
						}
					}
				}

				if (p_transition.is_string()) {
					auto fname = p_transition.get<std::string>();
					auto list  = std::shared_ptr<obs_frontend_source_list>(
                        new obs_frontend_source_list(),
                        [](obs_frontend_source_list* v) { obs_frontend_source_list_free(v); });
					obs_frontend_get_transitions(list.get());

					bool found = false;
					for (size_t idx = 0; idx < list->sources.num; idx++) {
						const char* name = obs_source_get_name(list->sources.array[idx]);
						if (fname == name) {
							obs_frontend_set_current_transition(list->sources.array[idx]);
							found = true;
							break;
						}
					}

					if (!found) {
						throw jsonrpc::invalid_params_error(
							"The parameter 'transition' must describe an existing transition.");
					}
				}

				if (p_duration.is_number_integer()) {
					obs_frontend_set_transition_duration(p_duration.get<int>());
				}

				{
					auto source   = std::shared_ptr<obs_source_t>(obs_frontend_get_current_transition(),
                                                                [](obs_source_t* v) { obs_source_release(v); });
					auto name     = obs_source_get_name(source.get());
					auto duration = obs_frontend_get_transition_duration();

					auto res_obj          = nlohmann::json::object();
					res_obj["transition"] = name ? name : "";
					res_obj["duration"]   = duration;
					res->set_result(res_obj);
				}

				streamdeck::server::instance()->reply(td->handle, res);
			} catch (const std::exception& ex) {
				try {
					auto res = std::make_shared<streamdeck::jsonrpc::response>();
					res->copy_id(*td->req);
					res->set_error(jsonrpc::SERVER_ERROR, ex.what());
					streamdeck::server::instance()->reply(td->handle, res);
				} catch (...) {
				}
			}
		},
		td, false);
}

void streamdeck::handlers::obs_frontend::transition_list(std::weak_ptr<void>                           handle,
														 std::shared_ptr<streamdeck::jsonrpc::request> req)
{
	/** obs.frontend.transition.list
	 *
	 * @return {Array({name: string, fixed: bool})} A list of unique names of transitions and whether their duration is fixed.
	 */

	struct task_data {
		std::weak_ptr<void>                           handle;
		std::shared_ptr<streamdeck::jsonrpc::request> req;
	};
	auto td    = new task_data;
	td->req    = req;
	td->handle = handle;

	obs_queue_task(
		obs_task_type::OBS_TASK_UI,
		[](void* ptr) {
			auto td = reinterpret_cast<task_data*>(ptr);
			try {
				auto list = std::shared_ptr<obs_frontend_source_list>(
					new obs_frontend_source_list(),
					[](obs_frontend_source_list* v) { obs_frontend_source_list_free(v); });
				obs_frontend_get_transitions(list.get());

				nlohmann::json data = nlohmann::json::array();
				for (size_t idx = 0; idx < list->sources.num; idx++) {
					const char* name  = obs_source_get_name(list->sources.array[idx]);
					bool        fixed = obs_transition_fixed(list->sources.array[idx]);
					auto        trans = nlohmann::json::object();
					trans["name"]     = name ? name : "";
					trans["fixed"]    = fixed;

					data.push_back(trans);
				}

				auto res = std::make_shared<streamdeck::jsonrpc::response>();
				res->copy_id(*td->req);
				res->set_result(data);
				streamdeck::server::instance()->reply(td->handle, res);
			} catch (const std::exception& ex) {
				try {
					auto res = std::make_shared<streamdeck::jsonrpc::response>();
					res->copy_id(*td->req);
					res->set_error(jsonrpc::SERVER_ERROR, ex.what());
					streamdeck::server::instance()->reply(td->handle, res);
				} catch (...) {
				}
			}
		},
		td, false);
}

void streamdeck::handlers::obs_frontend::screenshot(std::shared_ptr<streamdeck::jsonrpc::request>  req,
													std::shared_ptr<streamdeck::jsonrpc::response> res)
{
	/** obs.frontend.screenshot
	 *
	 * @param source {string} [Optional] A source reference to take a screenshot of.
	 */

	nlohmann::json                params;
	std::shared_ptr<obs_source_t> source;

	if (req->get_params(params)) { // Validate parameters.
		nlohmann::json::iterator param;

		param = params.find("source");
		if (param != params.end()) {
			if (!param->is_string()) {
				throw jsonrpc::invalid_params_error("The parameter 'source' must be of type 'string' if provided.");
			} else {
				source = std::shared_ptr<obs_source_t>(obs_get_source_by_name(param->get<std::string>().c_str()),
													   [](obs_source_t* v) { obs_source_release(v); });
			}
		}
	}

	if (source) {
		obs_frontend_take_source_screenshot(source.get());
	} else {
		obs_frontend_take_screenshot();
	}

	res->set_result(true);
}


void streamdeck::handlers::obs_frontend::stats(std::shared_ptr<streamdeck::jsonrpc::request>  req,
													std::shared_ptr<streamdeck::jsonrpc::response> res)
{
	/** obs.frontend.stats
	 */

	nlohmann::json                result = nlohmann::json::object();

	// Heavily modified from OBSBasicStats::Update() @ window-basic-stats.cpp

	struct obs_video_info ovi = {};
	obs_get_video_info(&ovi);

	auto stream = obs_frontend_get_streaming_output();
	auto recording = obs_frontend_get_recording_output();

	result["fpsTarget"] = obs_get_active_fps();
	result["obsFps"] = (double)ovi.fps_num / (double)ovi.fps_den;
	result["cpu"]       = os_cpu_usage_info_query(cpu_info);
	result["recording"] = nlohmann::json::object();
	result["stream"]    = nlohmann::json::object();
	#if OBS_MINIMUM_SUPPORT >= 280000
	char* path          = obs_frontend_get_current_record_output_path();
	result["recording"]["path"] = path;
	result["recording"]["freeSpace"] = os_get_free_disk_space(path);
	bfree(path);
	#endif
	result["memRSS"] = os_get_proc_resident_size();
	result["frameTimeNS"] = obs_get_average_frame_time_ns();

	video_t* video          = obs_get_video();
	result["framesEncoded"] = video_output_get_total_frames(video);
	result["framesSkipped"] = video_output_get_skipped_frames(video);
	result["framesTotal"] = obs_get_total_frames();
	result["framesLagged"] = obs_get_lagged_frames();
	

	if (stream) {
		result["stream"]["id"]           = obs_output_get_id(stream);
		result["stream"]["name"]         = obs_output_get_name(stream);
		result["stream"]["bytes"]        = obs_output_get_total_bytes(stream);
		result["stream"]["frames"]       = obs_output_get_total_frames(stream);
		result["stream"]["drops"]        = obs_output_get_frames_dropped(stream);
		result["stream"]["paused"]       = obs_output_paused(stream);
		result["stream"]["active"]       = obs_output_active(stream);
		result["stream"]["delay"]        = obs_output_get_delay(stream);
		result["stream"]["congestion"]   = obs_output_get_congestion(stream);
		result["stream"]["connectTime"] = obs_output_get_connect_time_ms(stream);
		result["stream"]["reconnecting"] = obs_output_reconnecting(stream);

		obs_output_release(stream);
	}
	if (recording) {
		result["recording"]["id"]     = obs_output_get_id(recording);
		result["recording"]["name"]   = obs_output_get_name(recording);
		result["recording"]["bytes"]  = obs_output_get_total_bytes(recording);
		result["recording"]["frames"] = obs_output_get_total_frames(recording);
		result["recording"]["drops"]  = obs_output_get_frames_dropped(recording);
		result["recording"]["paused"] = obs_output_paused(recording);
		result["recording"]["active"] = obs_output_active(recording);
		result["recording"]["delay"]  = obs_output_get_delay(recording);

		obs_output_release(recording);
	}

	// CPU Usage							.cpu (Percentage 0.0..100.0)
	// Disk space available					.recording.freeSpace (bytes)
	// Disk full in							Calculate with .recording.freeSpace * 8 / recording bitrate, use average bitrate over 10+ seconds
	// Memory Usage							.memRSS (bytes)
	// FPS									.fpsTarget 
	// Average time to render frame			.frameTimeNS (integer nanoseconds)
	// Frames missed due to rendering lag	.framesSkipped / .framesEncoded
	// Skipped frames due to encoding lag	.framesLagged / .framesTotal

	// Outputs
	// Name					.type.name
	// Status				.type.paused ? "Paused" : .type.active ? "Active / Streaming / Recording" : "Inactive"
	// Dropped Frames		.type.drops
	// Total Data Output	.type.bytes
	// Bitrate				Calculate based on delta between stats calls using .type.bytes


	res->set_result(result);
}


void streamdeck::handlers::obs_frontend::tbar(std::weak_ptr<void>                           handle,
													std::shared_ptr<streamdeck::jsonrpc::request> req)
{
	/** obs.frontend.tbar
	 *
	 * @param position {int} [Optional] 
	 * @param offset {int} [Optional] 
	 * @param release {bool} [Optional]
	 * @return {int} The current tbar position
	 */

	streamdeck::queue_task(obs_task_type::OBS_TASK_UI, false, [handle, req]() {
		nlohmann::json params;
		nlohmann::json position;
		nlohmann::json offset;
		nlohmann::json release;

		try {
			auto res = std::make_shared<streamdeck::jsonrpc::response>();
			res->copy_id(*req);

			auto tbar_pos = obs_frontend_get_tbar_position();
			auto res_obj          = nlohmann::json::object();
			res_obj       = tbar_pos;

			if (!req->get_params(params)) {
				res->set_result(res_obj);
				return;
			}
			nlohmann::json::iterator param;

			param = params.find("position");
			if (param != params.end()) {
				if (param->is_number_integer()) {
					position = *param;
				} else {
					throw jsonrpc::invalid_params_error("The parameter 'position' must be of type 'int' if present.");
				}
			}

			param = params.find("offset");
			if (param != params.end()) {
				if (param->is_number_integer()) {
					offset = *param;
				} else {
					throw jsonrpc::invalid_params_error("The parameter 'offset' must be of type 'int' if present.");
				}
			}

			param = params.find("release");
			if (param != params.end()) {
				if (param->is_boolean()) {
					release = *param;
				} else {
					throw jsonrpc::invalid_params_error(
						"The parameter 'release' must be of type 'boolean' if present.");
				}
			}

			if (!position.is_null() && !offset.is_null()) {
				throw jsonrpc::invalid_params_error("The parameters 'offset' and 'position' are mutually exclusive. Please provide only one.");	
			}

			if (position.is_number_integer()) {
				int value = position;
				if (value >= 0 && value <= 1023) {
					obs_frontend_set_tbar_position(value);
				} else {
					throw jsonrpc::invalid_params_error("The parameter 'position' is out of range 0..1023.");
				}
			}

			if (offset.is_number_integer()) {
				int value = offset;
				if (value >= -1023 && value <= 1023) {
					value = value + tbar_pos;
					//value = value < 0    ? 0 : value;
					//value = value > 1023 ? 1023 : value;
					obs_frontend_set_tbar_position(value);
				} else {
					throw jsonrpc::invalid_params_error("The parameter 'offset' is out of range -1023..1023.");
				}
			}

			if (release.is_boolean() && release.get<bool>()) {
				obs_frontend_release_tbar();
			}

			res_obj = obs_frontend_get_tbar_position();
			res->set_result(res_obj);
			streamdeck::server::instance()->reply(handle, res);
		} catch (const std::exception& ex) {
			try {
				auto res = std::make_shared<streamdeck::jsonrpc::response>();
				res->copy_id(*req);
				res->set_error(jsonrpc::SERVER_ERROR, ex.what());
				streamdeck::server::instance()->reply(handle, res);
			} catch (...) {
			}
		}
	});
}
