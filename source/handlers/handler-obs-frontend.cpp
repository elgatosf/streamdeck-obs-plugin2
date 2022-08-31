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
	obs_frontend_remove_event_callback(streamdeck::handlers::obs_frontend::handle_frontend_event, this);
}

streamdeck::handlers::obs_frontend::obs_frontend()
{
	obs_frontend_add_event_callback(streamdeck::handlers::obs_frontend::handle_frontend_event, this);

	auto server = streamdeck::server::instance();
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

	server->handle_async("obs.frontend.transition_studio",
						 std::bind(&streamdeck::handlers::obs_frontend::transition_studio, this, std::placeholders::_1,
								   std::placeholders::_2));

	server->handle_async("obs.frontend.scenecollection", std::bind(&streamdeck::handlers::obs_frontend::scenecollection,
																   this, std::placeholders::_1, std::placeholders::_2));
	server->handle_sync("obs.frontend.scenecollection.list",
						std::bind(&streamdeck::handlers::obs_frontend::scenecollection_list, this,
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

		case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED:
			method              = "obs.frontend.event.scenecollection";
			reply["collection"] = obs_frontend_get_current_scene_collection();
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
				}
				obs_frontend_source_list_free(&list);
			}
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
		}

		streamdeck::server::instance()->notify(method, reply);
	} catch (std::exception const& ex) {
		DLOG(LOG_DEBUG, "Error: %s", ex.what());
	}
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
