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

#include "handler-obs-scene.hpp"
#include <mutex>
#include <thread>
#include "module.hpp"
#include "server.hpp"

#include <callback/signal.h>
#include <obs.h>

/* clang-format off */
#define DLOG(LEVEL, ...) streamdeck::message(streamdeck::log_level:: LEVEL, "[Handler::OBS::Scene] " __VA_ARGS__)
/* clang-format on */

static void obs_source_deleter(obs_source_t* v)
{
	obs_source_release(v);
}
static void obs_sceneitem_deleter(obs_sceneitem_t* v)
{
	obs_sceneitem_release(v);
}

static nlohmann::json build_sceneitem_transform(obs_sceneitem_t* item)
{
	obs_transform_info ti;
	nlohmann::json     o = nlohmann::json::object();
	obs_sceneitem_get_info(item, &ti);
	{
		auto p = nlohmann::json::array();
		p.push_back(ti.pos.x);
		p.push_back(ti.pos.y);
		o["position"] = p;
	}
	o["rotation"] = ti.rot;
	{
		auto p = nlohmann::json::array();
		p.push_back(ti.scale.x);
		p.push_back(ti.scale.y);
		o["scale"] = p;
	}
	o["alignment"] = ti.alignment;
	{
		auto p = nlohmann::json::object();

		o["type"]      = ti.bounds_type;
		o["alignment"] = ti.bounds_alignment;
		{
			auto q = nlohmann::json::array();
			q.push_back(ti.bounds.x);
			q.push_back(ti.bounds.y);
			p["size"] = q;
		}

		o["bounds"] = p;
	}

	return o;
}

static nlohmann::json build_sceneitem_info(obs_sceneitem_t* item)
{
	nlohmann::json o = nlohmann::json::object();
	o["id"]          = obs_sceneitem_get_id(item);
	o["name"]        = obs_source_get_name(obs_sceneitem_get_source(item));
	o["group"]       = obs_sceneitem_is_group(item);
	o["visible"]     = obs_sceneitem_visible(item);
	o["locked"]      = obs_sceneitem_locked(item);
	o["transform"]   = build_sceneitem_transform(item);

	return o;
};

static nlohmann::json build_sceneitem_reference(obs_sceneitem_t* item)
{
	obs_scene_t*  scene  = obs_sceneitem_get_scene(item);
	obs_source_t* source = obs_sceneitem_get_source(item);
	int64_t       id     = obs_sceneitem_get_id(item);

	nlohmann::json o = nlohmann::json::array();
	o.push_back(obs_source_get_name(obs_scene_get_source(scene)));
	o.push_back(obs_source_get_name(source));
	o.push_back(id);

	return o;
}

static std::shared_ptr<obs_sceneitem_t> resolve_sceneitem_reference(nlohmann::json data)
{
	// 1. Verify input parameters.
	if (!data.is_array()) {
		throw streamdeck::jsonrpc::invalid_params_error("Scene Item Reference must be an array.");
	}
	if (data.size() != 3) {
		throw streamdeck::jsonrpc::invalid_params_error(
			"Scene Item Reference must contain 3 elements of type [String/String/Number].");
	}
	if (!data[0].is_string()) {
		throw streamdeck::jsonrpc::invalid_params_error(
			"Scene Item Reference must contain 3 elements of type [String/String/Number].");
	}
	if (!data[1].is_string()) {
		throw streamdeck::jsonrpc::invalid_params_error(
			"Scene Item Reference must contain 3 elements of type [String/String/Number].");
	}
	if (!data[2].is_number()) {
		throw streamdeck::jsonrpc::invalid_params_error(
			"Scene Item Reference must contain 3 elements of type [String/String/Number].");
	}

	// 2. Gather data.
	std::string scene_  = data[0].get<std::string>();
	std::string source_ = data[1].get<std::string>();
	int64_t     id_     = data[2].get<int64_t>();

	// 3. Try and resolve the scene itself.
	std::shared_ptr<obs_source_t> scene_s =
		std::shared_ptr<obs_source_t>(obs_get_source_by_name(scene_.c_str()), obs_source_deleter);
	if (!scene_s)
		throw streamdeck::jsonrpc::internal_error("Failed to find scene.");

	// 4. Try and get the scene from the source.
	obs_scene_t* scene = obs_scene_from_source(scene_s.get());
	if (!scene) {
		scene = obs_group_from_source(scene_s.get());
		if (!scene) {
			throw streamdeck::jsonrpc::internal_error("Failed to get scene from referenced source.");
		}
	}

	// 5. Try and find the item inside the scene.
	std::shared_ptr<obs_sceneitem_t> item =
		std::shared_ptr<obs_sceneitem_t>(obs_scene_find_sceneitem_by_id(scene, id_), obs_sceneitem_deleter);
	if (!item)
		throw streamdeck::jsonrpc::internal_error("Failed to find item in scene.");
	obs_sceneitem_addref(item.get());

	// 6. Verify that the name still matches.
	const char* name = obs_source_get_name(obs_sceneitem_get_source(item.get()));
	if (source_ != name) {
		//throw streamdeck::jsonrpc::internal_error("Scene item name differs from parameters, aborting.");
		//TODO: Turn this into a warning.
	}

	return item;
}

static void listen_scene_signals(obs_source_t* source, void* ptr)
{
	auto osh = obs_source_get_signal_handler(source);
	signal_handler_connect(osh, "destroy", &streamdeck::handlers::obs_scene::on_destroy, ptr);
	signal_handler_connect(osh, "reorder", &streamdeck::handlers::obs_scene::on_reorder, ptr);
	signal_handler_connect(osh, "item_add", &streamdeck::handlers::obs_scene::on_item_add, ptr);
	signal_handler_connect(osh, "item_remove", &streamdeck::handlers::obs_scene::on_item_remove, ptr);
	signal_handler_connect(osh, "item_visible", &streamdeck::handlers::obs_scene::on_item_visible, ptr);
	signal_handler_connect(osh, "item_transform", &streamdeck::handlers::obs_scene::on_item_transform, ptr);
}

static void silence_scene_signals(obs_source_t* source, void* ptr)
{
	auto osh = obs_source_get_signal_handler(source);
	signal_handler_disconnect(osh, "destroy", &streamdeck::handlers::obs_scene::on_destroy, ptr);
	signal_handler_disconnect(osh, "reorder", &streamdeck::handlers::obs_scene::on_reorder, ptr);
	signal_handler_disconnect(osh, "item_add", &streamdeck::handlers::obs_scene::on_item_add, ptr);
	signal_handler_disconnect(osh, "item_remove", &streamdeck::handlers::obs_scene::on_item_remove, ptr);
	signal_handler_disconnect(osh, "item_visible", &streamdeck::handlers::obs_scene::on_item_visible, ptr);
	signal_handler_disconnect(osh, "item_transform", &streamdeck::handlers::obs_scene::on_item_transform, ptr);
}

std::shared_ptr<streamdeck::handlers::obs_scene> streamdeck::handlers::obs_scene::instance()
{
	static std::weak_ptr<streamdeck::handlers::obs_scene> _instance;
	static std::mutex                                     _lock;

	std::unique_lock<std::mutex> lock(_lock);
	if (_instance.expired()) {
		auto instance = std::make_shared<streamdeck::handlers::obs_scene>();
		_instance     = instance;
		return instance;
	}

	return _instance.lock();
}

streamdeck::handlers::obs_scene::~obs_scene()
{
	{
		auto osh = obs_get_signal_handler();
		signal_handler_disconnect(osh, "source_create", &on_source_create, this);
	}
}

streamdeck::handlers::obs_scene::obs_scene()
{
	{
		auto osh = obs_get_signal_handler();
		signal_handler_connect(osh, "source_create", &on_source_create, this);
	}

	auto server = streamdeck::server::instance();
	server->handle_sync("obs.scene.items", std::bind(&streamdeck::handlers::obs_scene::items, this,
													 std::placeholders::_1, std::placeholders::_2));
	server->handle_sync("obs.scene.item.visible", std::bind(&streamdeck::handlers::obs_scene::item_visible, this,
															std::placeholders::_1, std::placeholders::_2));
}

void streamdeck::handlers::obs_scene::on_source_create(void* ptr, calldata_t* calldata)
{
	obs_source_t* source;
	obs_scene*    self = static_cast<obs_scene*>(ptr);

	// 1. Validate that we are actually inside of a proper event.
	if (!calldata_get_ptr(calldata, "source", &source)) {
		return;
	}

	// 2. Verify that this is actually a scene.
	if (obs_source_get_type(source) != OBS_SOURCE_TYPE_SCENE) {
		return; // This is not a scene.
	}

	// 3. Listen to scene signals.
	listen_scene_signals(source, ptr);
}

void streamdeck::handlers::obs_scene::on_destroy(void* ptr, calldata_t* calldata)
{
	obs_source_t* source;
	obs_scene*    self = static_cast<obs_scene*>(ptr);

	// 1. Validate that we are actually inside of a proper event.
	if (!calldata_get_ptr(calldata, "source", &source)) {
		return;
	}

	// 2. Listen to scene signals.
	silence_scene_signals(source, ptr);
}

void streamdeck::handlers::obs_scene::on_item_add(void* ptr, calldata_t* calldata)
{
	obs_scene_t*     scene;
	obs_sceneitem_t* item;
	obs_scene*       self = static_cast<obs_scene*>(ptr);

	// 1. Validate that we are actually inside of a proper event.
	if (!calldata_get_ptr(calldata, "scene", &scene)) {
		DLOG(LOG_WARNING,
			 "Failed to retrieve 'scene' entry from call data in 'item_add' signal. This is a bug in OBS Studio.");
		return;
	} else if (!calldata_get_ptr(calldata, "item", &item)) {
		DLOG(LOG_WARNING,
			 "Failed to retrieve 'item' entry from call data in 'item_add' signal. This is a bug in OBS Studio.");
		return;
	}

	// 2. Signal remote about changes.
	nlohmann::json o = nlohmann::json::object();
	o["item"]        = build_sceneitem_reference(item);
	o["state"]       = build_sceneitem_info(item);
	streamdeck::server::instance()->notify("obs.scene.event.item.add", o);

}

void streamdeck::handlers::obs_scene::on_reorder(void*, calldata_t* calldata)
{
	obs_scene_t* scene;

	// Validate that we are actually inside of a proper event.
	if (!calldata_get_ptr(calldata, "scene", &scene)) {
		DLOG(LOG_WARNING,
			 "Failed to retrieve 'scene' entry from call data in 'item_add' signal. This is a bug in OBS Studio.");
		return;
	}

	// Enumerate the new structure of scene items.
	nlohmann::json result = nlohmann::json::array();
	obs_scene_enum_items(
		scene,
		[](obs_scene_t*, obs_sceneitem_t* item, void* ptr) {
			nlohmann::json* result = static_cast<nlohmann::json*>(ptr);

			if (obs_sceneitem_is_group(item)) {
				nlohmann::json subresult = nlohmann::json::array();
				subresult.push_back(obs_sceneitem_get_id(item));

				obs_scene_enum_items(
					obs_sceneitem_group_get_scene(item),
					[](obs_scene_t*, obs_sceneitem_t* item, void* ptr) {
						nlohmann::json* result = static_cast<nlohmann::json*>(ptr);
						result->push_back(obs_sceneitem_get_id(item));

						return true;
					},
					&subresult);

				result->push_back(subresult);

			} else {
				DLOG(LOG_WARNING, "Got item %d", obs_sceneitem_get_id(item));
				result->push_back(obs_sceneitem_get_id(item));
			}

			return true;
		},
		&result);

	// Signal remote about changes.
	nlohmann::json o = nlohmann::json::object();
	o["scene"]       = obs_source_get_name(obs_scene_get_source(scene));
	o["items"]       = result;
	streamdeck::server::instance()->notify("obs.scene.event.reorder", o);
}

void streamdeck::handlers::obs_scene::on_item_remove(void* ptr, calldata_t* calldata)
{
	obs_scene_t*     scene;
	obs_sceneitem_t* item;

	// 1. Validate that we are actually inside of a proper event.
	if (!calldata_get_ptr(calldata, "scene", &scene)) {
		DLOG(LOG_WARNING,
			 "Failed to retrieve 'scene' entry from call data in 'item_remove' signal. This is a bug in OBS Studio.");
		return;
	} else if (!calldata_get_ptr(calldata, "item", &item)) {
		DLOG(LOG_WARNING,
			 "Failed to retrieve 'item' entry from call data in 'item_remove' signal. This is a bug in OBS Studio.");
		return;
	}

	// 2. Signal remote about changes.
	nlohmann::json o = nlohmann::json::object();
	o["item"]        = build_sceneitem_reference(item);
	o["state"]       = build_sceneitem_info(item);
	streamdeck::server::instance()->notify("obs.scene.event.item.remove", o);
}

void streamdeck::handlers::obs_scene::on_item_visible(void*, calldata_t* calldata)
{
	obs_scene_t*     scene;
	obs_sceneitem_t* item;
	bool             visible;

	// 1. Validate that we are actually inside of a proper event.
	if (!calldata_get_ptr(calldata, "scene", &scene)) {
		DLOG(LOG_WARNING,
			 "Failed to retrieve 'scene' entry from call data in 'item_visible' signal. This is a bug in OBS Studio.");
		return;
	} else if (!calldata_get_ptr(calldata, "item", &item)) {
		DLOG(LOG_WARNING,
			 "Failed to retrieve 'item' entry from call data in 'item_visible' signal. This is a bug in OBS Studio.");
		return;
	} else if (!calldata_get_bool(calldata, "visible", &visible)) {
		DLOG(
			LOG_WARNING,
			"Failed to retrieve 'visible' entry from call data in 'item_visible' signal. This is a bug in OBS Studio.");
		return;
	}

	// 2. Signal remote about changes.
	nlohmann::json o = nlohmann::json::object();
	o["item"]        = build_sceneitem_reference(item);
	o["state"]       = build_sceneitem_info(item);
	streamdeck::server::instance()->notify("obs.scene.event.item.visible", o);
}

void streamdeck::handlers::obs_scene::on_item_transform(void* ptr, calldata_t* calldata)
{
	obs_scene_t*     scene;
	obs_sceneitem_t* item;
	obs_scene*       self = static_cast<obs_scene*>(ptr);

	// 1. Validate that we are actually inside of a proper event.
	if (!calldata_get_ptr(calldata, "scene", &scene)) {
		DLOG(
			LOG_WARNING,
			"Failed to retrieve 'scene' entry from call data in 'item_transform' signal. This is a bug in OBS Studio.");
		return;
	} else if (!calldata_get_ptr(calldata, "item", &item)) {
		DLOG(LOG_WARNING,
			 "Failed to retrieve 'item' entry from call data in 'item_transform' signal. This is a bug in OBS Studio.");
		return;
	}

	// 2. Signal remote about changes.
	nlohmann::json o = nlohmann::json::object();
	o["item"]        = build_sceneitem_reference(item);
	o["state"]       = build_sceneitem_info(item);
	streamdeck::server::instance()->notify("obs.scene.event.item.transform", o);
}

void streamdeck::handlers::obs_scene::items(std::shared_ptr<streamdeck::jsonrpc::request>  req,
											std::shared_ptr<streamdeck::jsonrpc::response> res)
{
	/** obs.scene.items
	 *
	 * @param {string} scene The name of the scene to enumerate items for.
	 * 
	 * @return {Array(string)} An array of strings of all available items in the scene.
	 */

	// 1. Validate parameters.
	nlohmann::json parameters;
	if (!req->get_params(parameters)) {
		throw jsonrpc::invalid_params_error("Missing parameters.");
	}

	// 2. Figure out which scene we want to look for.
	auto p_scene = parameters.find("scene");
	if (p_scene == parameters.end()) {
		throw jsonrpc::invalid_params_error("'scene' must be present.");
	} else if (!p_scene->is_string()) {
		throw jsonrpc::invalid_params_error("'scene' must be of type 'string'.");
	}

	// 3. Resolve the scene to a source.
	std::string                   scene_ = p_scene->get<std::string>();
	std::shared_ptr<obs_source_t> source =
		std::shared_ptr<obs_source_t>(obs_get_source_by_name(scene_.c_str()), obs_source_deleter);
	if (!source) {
		throw jsonrpc::invalid_params_error("'scene' does not describe an existing source or scene.");
	}

	// 4. Get the actual scene or group.
	obs_scene_t* scene = obs_scene_from_source(source.get());
	if (!scene) {
		scene = obs_group_from_source(source.get());
		if (!scene) {
			throw jsonrpc::invalid_params_error("'scene' describes a source not a scene.");
		}
	}

	// 5. Finally enumerate the items in said scene.
	nlohmann::json result = nlohmann::json::array();
	obs_scene_enum_items(
		scene,
		[](obs_scene_t*, obs_sceneitem_t* item, void* ptr) {
			nlohmann::json* result = static_cast<nlohmann::json*>(ptr);
			result->push_back(build_sceneitem_info(item));
			return true;
		},
		&result);
	res->set_result(result);
}

void streamdeck::handlers::obs_scene::item_visible(std::shared_ptr<streamdeck::jsonrpc::request>  req,
												   std::shared_ptr<streamdeck::jsonrpc::response> res)
{
	/** obs.scene.item.visible
	 *
	 * @param {array} item The reference to the item in question, as an Array(String, String, Number).
	 * @param {bool} visible [Optional] `true` to set the item to be visible, `false` to make it invisible.
	 * 
	 * @return {bool} `true` if visible, otherwise `false`.
	 */

	// 1. Validate parameters.
	nlohmann::json parameters;
	if (!req->get_params(parameters)) {
		throw jsonrpc::invalid_params_error("Missing parameters.");
	}

	// 2. Resolve the scene item.
	auto p_item = parameters.find("item");
	if (p_item == parameters.end()) {
		throw jsonrpc::invalid_params_error("'item' must be present.");
	}
	auto item = resolve_sceneitem_reference(p_item->get<nlohmann::json>());

	// 3. Update state according to parameters.
	auto p_visible = parameters.find("visible");
	if (p_visible != parameters.end()) {
		if (p_visible->is_boolean()) {
			obs_sceneitem_set_visible(item.get(), p_visible->get<bool>());
		} else {
			throw jsonrpc::invalid_params_error("'visible' must be of type 'boolean' if present.");
		}
	}

	res->set_result(obs_sceneitem_visible(item.get()));
}
