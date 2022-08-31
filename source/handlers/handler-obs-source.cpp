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

#include "handler-obs-source.hpp"
#include <mutex>
#include <thread>
#include "module.hpp"
#include "server.hpp"

#include <callback/signal.h>
#include <obs.h>

/* clang-format off */
#define DLOG(LEVEL, ...) streamdeck::message(streamdeck::log_level:: LEVEL, "[Handler::OBS::Source] " __VA_ARGS__)
/* clang-format on */

static void obs_source_deleter(obs_source_t* v)
{
	obs_source_release(v);
}

static std::map<obs_source_type, std::string> type_map = {
	{OBS_SOURCE_TYPE_INPUT, "input"},
	{OBS_SOURCE_TYPE_FILTER, "filter"},
	{OBS_SOURCE_TYPE_TRANSITION, "transition"},
	{OBS_SOURCE_TYPE_SCENE, "scene"},
};
static std::map<uint32_t, std::string> output_flag_map = {
	{OBS_SOURCE_VIDEO, "video"},
	{OBS_SOURCE_AUDIO, "audio"},
	{OBS_SOURCE_ASYNC, "async"},
	{OBS_SOURCE_CUSTOM_DRAW, "custom_draw"},
	{OBS_SOURCE_INTERACTION, "interaction"},
	{OBS_SOURCE_COMPOSITE, "composite"},
	{OBS_SOURCE_DO_NOT_DUPLICATE, "no_duplicate"},
	{OBS_SOURCE_DEPRECATED, "deprecated"},
	{OBS_SOURCE_DO_NOT_SELF_MONITOR, "no_monitor"},
	{OBS_SOURCE_CAP_DISABLED, "disabled"},
	{OBS_SOURCE_MONITOR_BY_DEFAULT, "auto_monitor"},
	{OBS_SOURCE_SUBMIX, "submix"},
	{OBS_SOURCE_CONTROLLABLE_MEDIA, "controllable_media"},
};
static std::map<uint32_t, std::string> flag_map = {
	{OBS_SOURCE_FLAG_UNUSED_1, "unused_1"},
	{OBS_SOURCE_FLAG_FORCE_MONO, "force_mono"},
};
static std::map<speaker_layout, std::string> speaker_layout_map = {
	{SPEAKERS_UNKNOWN, "unknown"}, {SPEAKERS_MONO, "1.0"},    {SPEAKERS_STEREO, "2.0"},  {SPEAKERS_2POINT1, "2.1"},
	{SPEAKERS_4POINT0, "4.0"},     {SPEAKERS_4POINT1, "4.1"}, {SPEAKERS_5POINT1, "5.1"}, {SPEAKERS_7POINT1, "7.1"},
};
static std::map<obs_media_state, std::string> media_state_map = {
	{OBS_MEDIA_STATE_NONE, "none"},       {OBS_MEDIA_STATE_PLAYING, "playing"},
	{OBS_MEDIA_STATE_OPENING, "opening"}, {OBS_MEDIA_STATE_BUFFERING, "buffering"},
	{OBS_MEDIA_STATE_PAUSED, "paused"},   {OBS_MEDIA_STATE_STOPPED, "stopped"},
	{OBS_MEDIA_STATE_ENDED, "ended"},     {OBS_MEDIA_STATE_ERROR, "error"},
};
static std::map<obs_property_type, std::string> property_type_map = {
	{OBS_PROPERTY_INVALID, ""},
	{OBS_PROPERTY_BOOL, "bool"},
	{OBS_PROPERTY_INT, "int"},
	{OBS_PROPERTY_FLOAT, "float"},
	{OBS_PROPERTY_TEXT, "text"},
	{OBS_PROPERTY_PATH, "path"},
	{OBS_PROPERTY_LIST, "list"},
	{OBS_PROPERTY_COLOR, "color"},
	{OBS_PROPERTY_BUTTON, "button"},
	{OBS_PROPERTY_FONT, "font"},
	{OBS_PROPERTY_EDITABLE_LIST, "editable_list"},
	{OBS_PROPERTY_FRAME_RATE, "framerate"},
	{OBS_PROPERTY_GROUP, "group"},
	{OBS_PROPERTY_COLOR_ALPHA, "color_alpha"},
};
static std::map<obs_combo_format, std::string> combo_format_map = {
	{OBS_COMBO_FORMAT_INVALID, ""},
	{OBS_COMBO_FORMAT_INT, "int"},
	{OBS_COMBO_FORMAT_FLOAT, "float"},
	{OBS_COMBO_FORMAT_STRING, "string"},
};
static std::map<obs_combo_type, std::string> combo_type_map = {
	{OBS_COMBO_TYPE_INVALID, ""},
	{OBS_COMBO_TYPE_EDITABLE, "editable"},
	{OBS_COMBO_TYPE_LIST, "list"},
};
static std::map<obs_editable_list_type, std::string> editable_list_type_map = {
	{OBS_EDITABLE_LIST_TYPE_STRINGS, "strings"},
	{OBS_EDITABLE_LIST_TYPE_FILES, "files"},
	{OBS_EDITABLE_LIST_TYPE_FILES_AND_URLS, "files_and_urls"},
};
static std::map<obs_path_type, std::string> path_type_map = {
	{OBS_PATH_FILE, "file"},
	{OBS_PATH_FILE_SAVE, "file_save"},
	{OBS_PATH_DIRECTORY, "string"},
};
static std::map<obs_text_type, std::string> text_type_map = {
	{OBS_TEXT_DEFAULT, "default"},
	{OBS_TEXT_PASSWORD, "password"},
	{OBS_TEXT_MULTILINE, "multiline"},
};
static std::map<obs_number_type, std::string> number_type_map = {
	{OBS_NUMBER_SCROLLER, "scroller"},
	{OBS_NUMBER_SLIDER, "slider"},
};
static std::map<obs_group_type, std::string> group_type_map = {
	{OBS_COMBO_INVALID, ""}, // Not a bug in our code, this is what is actually in obs-properties.h. - Xaymar
	{OBS_GROUP_NORMAL, "normal"},
	{OBS_GROUP_CHECKABLE, "checkable"},
};

static std::shared_ptr<obs_source> resolve_source_reference(nlohmann::json value)
{
	if (value.is_array()) {
		if (value.size() == 0) {
			throw streamdeck::jsonrpc::invalid_params_error("Invalid array size for source.");
		}

		for (size_t n = 0; n < value.size(); n++) {
			if (!value.at(n).is_string()) {
				throw streamdeck::jsonrpc::invalid_params_error("Invalid type for source.");
			}
		}

		std::shared_ptr<obs_source> ref = {obs_get_source_by_name(value.at(0).get<std::string>().c_str()),
										   obs_source_deleter};
		for (size_t n = 1; n < value.size(); n++) {
			ref = {obs_source_get_filter_by_name(ref.get(), value.at(n).get<std::string>().c_str()),
				   obs_source_deleter};
		}

		return ref;
	} else if (value.is_string()) {
		return {obs_get_source_by_name(value.get<std::string>().c_str()), obs_source_deleter};
	} else {
		throw streamdeck::jsonrpc::invalid_params_error("Invalid type for source.");
	}
}

static nlohmann::json build_source_reference(obs_source_t* source)
{
	if (obs_source_get_type(source) == OBS_SOURCE_TYPE_FILTER) {
		nlohmann::json value = nlohmann::json::array();
		value.push_back(obs_source_get_name(source));
		while (obs_source_get_type(source) == OBS_SOURCE_TYPE_FILTER) {
			source = obs_filter_get_parent(source);
			value.insert(value.begin(), obs_source_get_name(source));
		}
		return value;
	} else {
		return nlohmann::json(obs_source_get_name(source));
	}
}

static nlohmann::json build_source_media_metadata(obs_source_t* source)
{
	nlohmann::json res = nlohmann::json::object();

	{
		auto kv = media_state_map.find(obs_source_media_get_state(source));
		if (kv != media_state_map.end()) {
			res["status"] = kv->second;
		} else {
			res["status"] = nlohmann::json();
		}
	}

	{
		auto o = nlohmann::json::array();
		o.push_back(obs_source_media_get_time(source));
		o.push_back(obs_source_media_get_duration(source));
		res["time"] = o;
	}

	return res;
}

static nlohmann::json build_source_metadata(obs_source_t* source)
{
	nlohmann::json res  = nlohmann::json::object();
	const char*    name = obs_source_get_name(source);

	// The current name of the source.
	res["id"]             = obs_source_get_id(source);
	res["id_unversioned"] = obs_source_get_unversioned_id(source);
	res["name"]           = name ? name : "";

	{
		auto kv = type_map.find(obs_source_get_type(source));
		if (kv != type_map.end()) {
			res["type"] = kv->second;
		} else {
			res["type"] = nlohmann::json();
		}
	}

	res["enabled"] = obs_source_enabled(source);
	res["active"]  = obs_source_active(source);
	res["visible"] = obs_source_showing(source);

	{ // Output Flags
		auto o  = nlohmann::json::object();
		auto of = obs_source_get_output_flags(source);
		for (auto kv : output_flag_map) {
			o[kv.second] = (of & kv.first) ? true : false;
		}
		res["output_flags"] = o;
		res["outputflags"]  = o; // Deprecated
	}

	{ // Size and Base Size
		auto o = nlohmann::json::array();
		o.push_back(obs_source_get_width(source));
		o.push_back(obs_source_get_height(source));
		o.push_back(obs_source_get_base_width(source));
		o.push_back(obs_source_get_base_height(source));
		res["size"] = o;
	}

	{ // Flags
		auto o  = nlohmann::json::object();
		auto of = obs_source_get_flags(source);
		for (auto kv : flag_map) {
			o[kv.second] = (of & kv.first) ? true : false;
		}
		res["flags"] = o;
	}

	{ // Audio
		auto o = nlohmann::json::object();
		{
			auto kv = speaker_layout_map.find(obs_source_get_speaker_layout(source));
			if (kv != speaker_layout_map.end()) {
				o["layout"] = kv->second;
			} else {
				o["layout"] = nlohmann::json();
			}
		}
		o["muted"]       = obs_source_muted(source);
		o["volume"]      = obs_source_get_volume(source);
		o["balance"]     = obs_source_get_balance_value(source);
		o["sync_offset"] = obs_source_get_sync_offset(source);
		o["mixers"]      = obs_source_get_audio_mixers(source);
		res["audio"]     = o;
	}

	res["media"] = build_source_media_metadata(source);

	return res;
}

nlohmann::json build_properties_metadata(obs_properties_t* props)
{
	nlohmann::json res = nlohmann::json::array();
	for (obs_property_t* prop = obs_properties_first(props); prop != nullptr; obs_property_next(&prop)) {
		nlohmann::json res2 = nlohmann::json::object();
		res2["setting"]     = obs_property_name(prop);
		res2["name"]        = obs_property_description(prop);
		res2["description"] = obs_property_long_description(prop);
		res2["enabled"]     = obs_property_enabled(prop);
		res2["visible"]     = obs_property_visible(prop);

		auto proptype = obs_property_get_type(prop);
		{ // type
			auto kv = property_type_map.find(proptype);
			if (kv != property_type_map.end()) {
				res2["type"] = kv->second;
			}
		}

		switch (proptype) {
		case OBS_PROPERTY_BOOL:
			break;
		case OBS_PROPERTY_INT: {
			{ // subtype
				auto kv = number_type_map.find(obs_property_int_type(prop));
				if (kv != number_type_map.end()) {
					res2["subtype"] = kv->second;
				}
			}
			{ // limits
				auto o    = nlohmann::json::object();
				o["min"]  = obs_property_int_min(prop);
				o["max"]  = obs_property_int_max(prop);
				o["step"] = obs_property_int_step(prop);

				res2["limits"] = o;
			}
			res2["suffix"] = obs_property_int_suffix(prop);
			break;
		}
		case OBS_PROPERTY_FLOAT: {
			{ // subtype
				auto kv = number_type_map.find(obs_property_float_type(prop));
				if (kv != number_type_map.end()) {
					res2["subtype"] = kv->second;
				}
			}
			{ // limits
				auto o    = nlohmann::json::object();
				o["min"]  = obs_property_float_min(prop);
				o["max"]  = obs_property_float_max(prop);
				o["step"] = obs_property_float_step(prop);

				res2["limits"] = o;
			}
			res2["suffix"] = obs_property_float_suffix(prop);
			break;
		}
		case OBS_PROPERTY_TEXT: {
			{ // subtype
				auto kv = text_type_map.find(obs_property_text_type(prop));
				if (kv != text_type_map.end()) {
					res2["subtype"] = kv->second;
				}
			}
			// Upstream Bug: This function returns obs_text_type, but wants to return bool. Type confusion through Ctrl-C + Ctrl-V without double checking things.
			res2["monospace"] = !!obs_property_text_monospace(prop);
			break;
		}
		case OBS_PROPERTY_LIST: {
			{ // subtype
				auto kv = combo_type_map.find(obs_property_list_type(prop));
				if (kv != combo_type_map.end()) {
					res2["subtype"] = kv->second;
				}
			}
			obs_combo_format format = obs_property_list_format(prop);
			{ // format
				auto kv = combo_format_map.find(format);
				if (kv != combo_format_map.end()) {
					res2["format"] = kv->second;
				}
			}
			{
				auto o = nlohmann::json::array();
				for (size_t idx = 0, edx = obs_property_list_item_count(prop); idx < edx; idx++) {
					auto p       = nlohmann::json::object();
					p["name"]    = obs_property_list_item_name(prop, idx);
					p["enabled"] = !obs_property_list_item_disabled(prop, idx);
					switch (format) {
					case OBS_COMBO_FORMAT_STRING:
						p["value"] = obs_property_list_item_string(prop, idx);
						break;
					case OBS_COMBO_FORMAT_INT:
						p["value"] = obs_property_list_item_int(prop, idx);
						break;
					case OBS_COMBO_FORMAT_FLOAT:
						p["value"] = obs_property_list_item_float(prop, idx);
						break;
					default:
						break;
					}
				}
				res2["items"] = o;
			}

			break;
		}
		case OBS_PROPERTY_PATH: {
			{ // subtype
				auto kv = path_type_map.find(obs_property_path_type(prop));
				if (kv != path_type_map.end()) {
					res2["subtype"] = kv->second;
				}
			}
			res2["filter"]       = obs_property_path_filter(prop);
			res2["default_path"] = obs_property_path_default_path(prop);
			break;
		}
		case OBS_PROPERTY_EDITABLE_LIST: {
			{ // subtype
				auto kv = editable_list_type_map.find(obs_property_editable_list_type(prop));
				if (kv != editable_list_type_map.end()) {
					res2["subtype"] = kv->second;
				}
			}
			res2["filter"]       = obs_property_editable_list_filter(prop);
			res2["default_path"] = obs_property_editable_list_default_path(prop);
			{
				auto o = nlohmann::json::array();
				for (size_t idx = 0, edx = obs_property_list_item_count(prop); idx < edx; idx++) {
					auto p       = nlohmann::json::object();
					p["name"]    = obs_property_list_item_name(prop, idx);
					p["enabled"] = !obs_property_list_item_disabled(prop, idx);
					p["value"]   = obs_property_list_item_string(prop, idx);
					o.push_back(p);
				}
				res2["items"] = o;
			}
			break;
		}
		case OBS_PROPERTY_FRAME_RATE: {
			{
				auto o = nlohmann::json::array();
				for (size_t idx = 0, edx = obs_property_frame_rate_options_count(prop); idx < edx; idx++) {
					auto p     = nlohmann::json::object();
					p["name"]  = obs_property_frame_rate_option_description(prop, idx);
					p["value"] = obs_property_frame_rate_option_name(prop, idx);
					o.push_back(p);
				}
				res2["options"] = o;
			}
			{
				auto o = nlohmann::json::object();
				for (size_t idx = 0, edx = obs_property_frame_rate_fps_ranges_count(prop); idx < edx; idx++) {
					auto min_fps = obs_property_frame_rate_fps_range_min(prop, idx);
					auto max_fps = obs_property_frame_rate_fps_range_max(prop, idx);

					auto p = nlohmann::json::object();
					{
						auto m = nlohmann::json::array();
						m.push_back(min_fps.numerator);
						m.push_back(min_fps.denominator);
						p["min"] = m;
					}
					{
						auto m = nlohmann::json::array();
						m.push_back(max_fps.numerator);
						m.push_back(max_fps.denominator);
						p["max"] = m;
					}
					o.push_back(p);
				}
				res2["ranges"] = o;
			}
			break;
		}
		case OBS_PROPERTY_GROUP: {
			{ // subtype
				auto kv = group_type_map.find(obs_property_group_type(prop));
				if (kv != group_type_map.end()) {
					res2["subtype"] = kv->second;
				}
			}
			auto subprops   = obs_property_group_content(prop); // Don't call obs_properties_destroy on this.
			res2["content"] = build_properties_metadata(subprops);
			break;
		}
		default:
			break;
		}
		res.push_back(res2);
	}
	return res;
}

static void listen_source_signals(obs_source_t* source, void* ptr)
{
	auto osh = obs_source_get_signal_handler(source);
	signal_handler_connect(osh, "rename", &streamdeck::handlers::obs_source::on_rename, ptr);
	signal_handler_connect(osh, "destroy", &streamdeck::handlers::obs_source::on_destroy, ptr);

	signal_handler_connect(osh, "enable", &streamdeck::handlers::obs_source::on_enable, ptr);
	signal_handler_connect(osh, "activate", &streamdeck::handlers::obs_source::on_activate_deactivate, ptr);
	signal_handler_connect(osh, "deactivate", &streamdeck::handlers::obs_source::on_activate_deactivate, ptr);
	signal_handler_connect(osh, "show", &streamdeck::handlers::obs_source::on_show_hide, ptr);
	signal_handler_connect(osh, "hide", &streamdeck::handlers::obs_source::on_show_hide, ptr);

	// Audio
	signal_handler_connect(osh, "mute", &streamdeck::handlers::obs_source::on_mute, ptr);
	signal_handler_connect(osh, "volume", &streamdeck::handlers::obs_source::on_volume, ptr);

	// Filters
	signal_handler_connect(osh, "filter_add", &streamdeck::handlers::obs_source::on_filter_add, ptr);
	signal_handler_connect(osh, "filter_remove", &streamdeck::handlers::obs_source::on_filter_remove, ptr);
	signal_handler_connect(osh, "reorder_filters", &streamdeck::handlers::obs_source::on_filter_reorder, ptr);

	// Media Controls
	signal_handler_connect(osh, "media_play", &streamdeck::handlers::obs_source::on_media_play, ptr);
	signal_handler_connect(osh, "media_pause", &streamdeck::handlers::obs_source::on_media_pause, ptr);
	signal_handler_connect(osh, "media_restart", &streamdeck::handlers::obs_source::on_media_restart, ptr);
	signal_handler_connect(osh, "media_stopped", &streamdeck::handlers::obs_source::on_media_stopped, ptr);
	signal_handler_connect(osh, "media_next", &streamdeck::handlers::obs_source::on_media_next, ptr);
	signal_handler_connect(osh, "media_previous", &streamdeck::handlers::obs_source::on_media_previous, ptr);
	signal_handler_connect(osh, "media_started", &streamdeck::handlers::obs_source::on_media_started, ptr);
	signal_handler_connect(osh, "media_ended", &streamdeck::handlers::obs_source::on_media_ended, ptr);
}

static void silence_source_signals(obs_source_t* source, void* ptr)
{
	auto osh = obs_source_get_signal_handler(source);
	signal_handler_disconnect(osh, "rename", &streamdeck::handlers::obs_source::on_rename, ptr);
	signal_handler_disconnect(osh, "destroy", &streamdeck::handlers::obs_source::on_destroy, ptr);

	signal_handler_disconnect(osh, "enable", &streamdeck::handlers::obs_source::on_enable, ptr);
	signal_handler_disconnect(osh, "activate", &streamdeck::handlers::obs_source::on_activate_deactivate, ptr);
	signal_handler_disconnect(osh, "deactivate", &streamdeck::handlers::obs_source::on_activate_deactivate, ptr);
	signal_handler_disconnect(osh, "show", &streamdeck::handlers::obs_source::on_show_hide, ptr);
	signal_handler_disconnect(osh, "hide", &streamdeck::handlers::obs_source::on_show_hide, ptr);

	// Audio
	signal_handler_disconnect(osh, "mute", &streamdeck::handlers::obs_source::on_mute, ptr);
	signal_handler_disconnect(osh, "volume", &streamdeck::handlers::obs_source::on_volume, ptr);

	// Filters
	signal_handler_disconnect(osh, "filter_add", &streamdeck::handlers::obs_source::on_filter_add, ptr);
	signal_handler_disconnect(osh, "filter_remove", &streamdeck::handlers::obs_source::on_filter_remove, ptr);

	// Media Controls
	signal_handler_disconnect(osh, "media_play", &streamdeck::handlers::obs_source::on_media_play, ptr);
	signal_handler_disconnect(osh, "media_pause", &streamdeck::handlers::obs_source::on_media_pause, ptr);
	signal_handler_disconnect(osh, "media_restart", &streamdeck::handlers::obs_source::on_media_restart, ptr);
	signal_handler_disconnect(osh, "media_stopped", &streamdeck::handlers::obs_source::on_media_stopped, ptr);
	signal_handler_disconnect(osh, "media_next", &streamdeck::handlers::obs_source::on_media_next, ptr);
	signal_handler_disconnect(osh, "media_previous", &streamdeck::handlers::obs_source::on_media_previous, ptr);
	signal_handler_disconnect(osh, "media_started", &streamdeck::handlers::obs_source::on_media_started, ptr);
	signal_handler_disconnect(osh, "media_ended", &streamdeck::handlers::obs_source::on_media_ended, ptr);
}

std::shared_ptr<streamdeck::handlers::obs_source> streamdeck::handlers::obs_source::instance()
{
	static std::weak_ptr<streamdeck::handlers::obs_source> _instance;
	static std::mutex                                      _lock;

	std::unique_lock<std::mutex> lock(_lock);
	if (_instance.expired()) {
		auto instance = std::make_shared<streamdeck::handlers::obs_source>();
		_instance     = instance;
		return instance;
	}

	return _instance.lock();
}

streamdeck::handlers::obs_source::~obs_source()
{
	{
		auto osh = obs_get_signal_handler();
		signal_handler_disconnect(osh, "source_create", &on_source_create, this);
	}
}

streamdeck::handlers::obs_source::obs_source()
{
	{
		auto osh = obs_get_signal_handler();
		signal_handler_connect(osh, "source_create", &on_source_create, this);
	}

	auto server = streamdeck::server::instance();
	server->handle_sync("obs.source.enumerate", std::bind(&streamdeck::handlers::obs_source::enumerate, this,
														  std::placeholders::_1, std::placeholders::_2));
	server->handle_sync("obs.source.state", std::bind(&streamdeck::handlers::obs_source::state, this,
													  std::placeholders::_1, std::placeholders::_2));
	server->handle_sync("obs.source.filters", std::bind(&streamdeck::handlers::obs_source::filters, this,
														std::placeholders::_1, std::placeholders::_2));
	server->handle_sync("obs.source.settings", std::bind(&streamdeck::handlers::obs_source::settings, this,
														 std::placeholders::_1, std::placeholders::_2));
	server->handle_sync("obs.source.media", std::bind(&streamdeck::handlers::obs_source::media, this,
													  std::placeholders::_1, std::placeholders::_2));
	server->handle_sync("obs.source.properties", std::bind(&streamdeck::handlers::obs_source::properties, this,
														   std::placeholders::_1, std::placeholders::_2));
}

void streamdeck::handlers::obs_source::on_source_create(void* ptr, calldata_t* calldata)
{
	// Retrieve information.
	obs_source_t* source;
	if (!calldata_get_ptr(calldata, "source", &source)) {
		DLOG(LOG_WARNING,
			 "Failed to retrieve 'source' entry from call data in 'source_create' signal. This is a bug in OBS "
			 "Studio.");
		return;
	}

	// Do our WebSocket work.
	nlohmann::json reply = build_source_metadata(source);
	streamdeck::server::instance()->notify("obs.source.event.create", reply);

	// Add listeners for other signals.
	listen_source_signals(source, ptr);
}

void streamdeck::handlers::obs_source::on_destroy(void* ptr, calldata_t* calldata)
{
	// Retrieve information.
	obs_source_t* source;
	if (!calldata_get_ptr(calldata, "source", &source)) {
		DLOG(LOG_WARNING,
			 "Failed to retrieve 'source' entry from call data in 'source_destroy' signal. This is a bug in OBS "
			 "Studio.");
		return;
	}

	// Do our WebSocket work.
	nlohmann::json reply = nlohmann::json::object();
	reply["source"]      = build_source_reference(source);

	streamdeck::server::instance()->notify("obs.source.event.destroy", reply);

	// Remove listeners for other signals.
	silence_source_signals(source, ptr);
}

void streamdeck::handlers::obs_source::on_rename(void*, calldata_t* calldata)
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
	reply["source"]      = build_source_reference(source);
	reply["from"]        = old_name ? old_name : "";
	reply["to"]          = new_name ? new_name : "";

	streamdeck::server::instance()->notify("obs.source.event.rename", reply);
}

void streamdeck::handlers::obs_source::on_enable(void*, calldata_t* calldata)
{
	// Handles "activate" and "deactivate" as one event.

	// Retrieve information.
	obs_source_t* source;
	if (!calldata_get_ptr(calldata, "source", &source)) {
		DLOG(LOG_WARNING,
			 "Failed to retrieve 'source' entry from call data in 'enable' signal. This is a bug in OBS "
			 "Studio.");
		return;
	}

	{ // Deprecated: obs.source.event.enable
		nlohmann::json reply = nlohmann::json::object();
		reply["source"]      = build_source_reference(source);
		reply["enabled"]     = obs_source_enabled(source);

		streamdeck::server::instance()->notify("obs.source.event.enable", reply);
	}

	{ // obs.source.event.state
		nlohmann::json reply = nlohmann::json::object();
		reply["source"]      = build_source_reference(source);
		reply["state"]       = build_source_metadata(source);

		streamdeck::server::instance()->notify("obs.source.event.state", reply);
	}
}

void streamdeck::handlers::obs_source::on_activate_deactivate(void*, calldata_t* calldata)
{
	// Handles "activate" and "deactivate" as one event.

	// Retrieve information.
	obs_source_t* source;
	if (!calldata_get_ptr(calldata, "source", &source)) {
		DLOG(LOG_WARNING,
			 "Failed to retrieve 'source' entry from call data in 'activate'/'deactivate' signal. This is a "
			 "bug in "
			 "OBS "
			 "Studio.");
		return;
	}

	{ // Deprecated: obs.source.event.activate
		nlohmann::json reply = nlohmann::json::object();
		reply["source"]      = build_source_reference(source);
		reply["active"]      = obs_source_active(source);

		streamdeck::server::instance()->notify("obs.source.event.activate", reply);
	}

	{ // obs.source.event.state
		nlohmann::json reply = nlohmann::json::object();
		reply["source"]      = build_source_reference(source);
		reply["state"]       = build_source_metadata(source);

		streamdeck::server::instance()->notify("obs.source.event.state", reply);
	}
}

void streamdeck::handlers::obs_source::on_show_hide(void*, calldata_t* calldata)
{
	// Handles "show" and "hide" as one event.

	// Retrieve information.
	obs_source_t* source;
	if (!calldata_get_ptr(calldata, "source", &source)) {
		DLOG(LOG_WARNING,
			 "Failed to retrieve 'source' entry from call data in 'show'/'hide' signal. This is a bug in OBS "
			 "Studio.");
		return;
	}

	{ // Deprecated: obs.source.event.show
		nlohmann::json reply = nlohmann::json::object();
		reply["source"]      = build_source_reference(source);
		reply["visible"]     = obs_source_showing(source);

		streamdeck::server::instance()->notify("obs.source.event.show", reply);
	}

	{ // obs.source.event.state
		nlohmann::json reply = nlohmann::json::object();
		reply["source"]      = build_source_reference(source);
		reply["state"]       = build_source_metadata(source);

		streamdeck::server::instance()->notify("obs.source.event.state", reply);
	}
}

void streamdeck::handlers::obs_source::on_mute(void*, calldata_t* calldata)
{
	// Retrieve information.
	obs_source_t* source;
	bool          muted;
	if (!calldata_get_ptr(calldata, "source", &source)) {
		DLOG(LOG_WARNING,
			 "Failed to retrieve 'source' entry from call data in 'mute' signal. This is a bug in OBS "
			 "Studio.");
		return;
	} else if (!calldata_get_bool(calldata, "muted", &muted)) {
		DLOG(LOG_WARNING,
			 "Failed to retrieve 'muted' entry from call data in 'mute' signal. This is a bug in OBS "
			 "Studio.");
		return;
	}

	{ // Deprecated: obs.source.event.mute
		nlohmann::json reply = nlohmann::json::object();
		reply["source"]      = build_source_reference(source);
		reply["muted"]       = muted;

		streamdeck::server::instance()->notify("obs.source.event.mute", reply);
	}

	{ // obs.source.event.state
		nlohmann::json reply = nlohmann::json::object();
		reply["source"]      = build_source_reference(source);
		reply["state"]       = build_source_metadata(source);

		streamdeck::server::instance()->notify("obs.source.event.state", reply);
	}
}

void streamdeck::handlers::obs_source::on_volume(void*, calldata_t* calldata)
{
	// Retrieve information.
	obs_source_t* source;
	double        volume;
	if (!calldata_get_ptr(calldata, "source", &source)) {
		DLOG(LOG_WARNING,
			 "Failed to retrieve 'source' entry from call data in 'mute' signal. This is a bug in OBS "
			 "Studio.");
		return;
	} else if (!calldata_get_float(calldata, "volume", &volume)) {
		DLOG(LOG_WARNING,
			 "Failed to retrieve 'volume' entry from call data in 'volume' signal. This is a bug in OBS "
			 "Studio.");
		return;
	}

	{ // obs.source.event.state
		nlohmann::json reply = nlohmann::json::object();
		reply["source"]      = build_source_reference(source);
		reply["state"]       = build_source_metadata(source);

		streamdeck::server::instance()->notify("obs.source.event.state", reply);
	}
}

void streamdeck::handlers::obs_source::on_filter_add(void* ptr, calldata_t* calldata)
{
	// Retrieve information.
	obs_source_t* source;
	obs_source_t* filter;
	if (!calldata_get_ptr(calldata, "source", &source)) {
		DLOG(LOG_WARNING,
			 "Failed to retrieve 'source' entry from call data in 'filter_add' signal. This is a bug in OBS "
			 "Studio.");
		return;
	} else if (!calldata_get_ptr(calldata, "filter", &filter)) {
		DLOG(LOG_WARNING,
			 "Failed to retrieve 'filter' entry from call data in 'filter_add' signal. This is a bug in OBS "
			 "Studio.");
		return;
	}

	// Do our WebSocket work.
	nlohmann::json reply = nlohmann::json::object();
	reply["source"]      = build_source_reference(source);
	reply["filter"]      = build_source_metadata(filter);
	streamdeck::server::instance()->notify("obs.source.event.filter.add", reply);

	// Filters are private sources, we need to listen to them as well.
	listen_source_signals(filter, ptr);
}

void streamdeck::handlers::obs_source::on_filter_remove(void* ptr, calldata_t* calldata)
{
	// Retrieve information.
	obs_source_t* source;
	obs_source_t* filter;
	if (!calldata_get_ptr(calldata, "source", &source)) {
		DLOG(LOG_WARNING,
			 "Failed to retrieve 'source' entry from call data in 'filter_remove' signal. This is a bug in OBS "
			 "Studio.");
		return;
	} else if (!calldata_get_ptr(calldata, "filter", &filter)) {
		DLOG(LOG_WARNING,
			 "Failed to retrieve 'filter' entry from call data in 'filter_remove' signal. This is a bug in OBS "
			 "Studio.");
		return;
	}

	// Do our WebSocket work.
	nlohmann::json reply = nlohmann::json::object();
	reply["source"]      = build_source_reference(filter);
	streamdeck::server::instance()->notify("obs.source.event.filter.remove", reply);

	// Filters are private sources, we need to silence the listened signals.
	silence_source_signals(filter, ptr);
}

void streamdeck::handlers::obs_source::on_filter_reorder(void* ptr, calldata_t* calldata)
{
	// Retrieve information.
	obs_source_t* source;
	obs_source_t* filter;
	if (!calldata_get_ptr(calldata, "source", &source)) {
		DLOG(LOG_WARNING,
			 "Failed to retrieve 'source' entry from call data in 'filter_remove' signal. This is a bug in OBS "
			 "Studio.");
		return;
	} else if (!calldata_get_ptr(calldata, "filter", &filter)) {
		DLOG(LOG_WARNING,
			 "Failed to retrieve 'filter' entry from call data in 'filter_remove' signal. This is a bug in OBS "
			 "Studio.");
		return;
	}

	// Do our WebSocket work.
	nlohmann::json reply = nlohmann::json::object();
	reply["source"]      = build_source_reference(filter);
	{
		nlohmann::json result = nlohmann::json::array();
		obs_source_enum_filters(
			source,
			[](obs_source_t*, obs_source_t* filter, void* ptr) {
				nlohmann::json* result = static_cast<nlohmann::json*>(ptr);
				result->push_back(build_source_metadata(filter));
			},
			&result);
		reply["order"] = result;
	}
	streamdeck::server::instance()->notify("obs.source.event.filter.reorder", reply);
}

void streamdeck::handlers::obs_source::on_media_play(void*, calldata_t* calldata)
{
	// Retrieve information.
	obs_source_t* source;
	if (!calldata_get_ptr(calldata, "source", &source)) {
		DLOG(LOG_WARNING,
			 "Failed to retrieve 'source' entry from call data in 'media_play' signal. This is a bug in OBS "
			 "Studio.");
		return;
	}

	// Do our WebSocket work.
	nlohmann::json reply = nlohmann::json::object();
	reply["source"]      = build_source_reference(source);
	reply["signal"]      = "play";
	reply["media"]       = build_source_media_metadata(source);

	streamdeck::server::instance()->notify("obs.source.event.media", reply);
}

void streamdeck::handlers::obs_source::on_media_pause(void*, calldata_t* calldata)
{
	// Retrieve information.
	obs_source_t* source;
	if (!calldata_get_ptr(calldata, "source", &source)) {
		DLOG(LOG_WARNING,
			 "Failed to retrieve 'source' entry from call data in 'media_pause' signal. This is a bug in OBS "
			 "Studio.");
		return;
	}

	// Do our WebSocket work.
	nlohmann::json reply = nlohmann::json::object();
	reply["source"]      = build_source_reference(source);
	reply["signal"]      = "pause";
	reply["media"]       = build_source_media_metadata(source);

	streamdeck::server::instance()->notify("obs.source.event.media", reply);
}

void streamdeck::handlers::obs_source::on_media_restart(void*, calldata_t* calldata)
{
	// Retrieve information.
	obs_source_t* source;
	if (!calldata_get_ptr(calldata, "source", &source)) {
		DLOG(LOG_WARNING,
			 "Failed to retrieve 'source' entry from call data in 'media_restart' signal. This is a bug in OBS "
			 "Studio.");
		return;
	}

	// Do our WebSocket work.
	nlohmann::json reply = nlohmann::json::object();
	reply["source"]      = build_source_reference(source);
	reply["signal"]      = "restart";
	reply["media"]       = build_source_media_metadata(source);

	streamdeck::server::instance()->notify("obs.source.event.media", reply);
}

void streamdeck::handlers::obs_source::on_media_stopped(void*, calldata_t* calldata)
{
	// Retrieve information.
	obs_source_t* source;
	if (!calldata_get_ptr(calldata, "source", &source)) {
		DLOG(LOG_WARNING,
			 "Failed to retrieve 'source' entry from call data in 'media_stop' signal. This is a bug in OBS "
			 "Studio.");
		return;
	}

	// Do our WebSocket work.
	nlohmann::json reply = nlohmann::json::object();
	reply["source"]      = build_source_reference(source);
	reply["signal"]      = "stopped";
	reply["media"]       = build_source_media_metadata(source);

	streamdeck::server::instance()->notify("obs.source.event.media", reply);
}

void streamdeck::handlers::obs_source::on_media_next(void*, calldata_t* calldata)
{
	// Retrieve information.
	obs_source_t* source;
	if (!calldata_get_ptr(calldata, "source", &source)) {
		DLOG(LOG_WARNING,
			 "Failed to retrieve 'source' entry from call data in 'media_next' signal. This is a bug in OBS "
			 "Studio.");
		return;
	}

	// Do our WebSocket work.
	nlohmann::json reply = nlohmann::json::object();
	reply["source"]      = build_source_reference(source);
	reply["signal"]      = "next";
	reply["media"]       = build_source_media_metadata(source);

	streamdeck::server::instance()->notify("obs.source.event.media", reply);
}

void streamdeck::handlers::obs_source::on_media_previous(void*, calldata_t* calldata)
{
	// Retrieve information.
	obs_source_t* source;
	if (!calldata_get_ptr(calldata, "source", &source)) {
		DLOG(LOG_WARNING,
			 "Failed to retrieve 'source' entry from call data in 'media_previous' signal. This is a bug in OBS "
			 "Studio.");
		return;
	}

	// Do our WebSocket work.
	nlohmann::json reply = nlohmann::json::object();
	reply["source"]      = build_source_reference(source);
	reply["signal"]      = "previous";
	reply["media"]       = build_source_media_metadata(source);

	streamdeck::server::instance()->notify("obs.source.event.media", reply);
}

void streamdeck::handlers::obs_source::on_media_started(void*, calldata_t* calldata)
{
	// Retrieve information.
	obs_source_t* source;
	if (!calldata_get_ptr(calldata, "source", &source)) {
		DLOG(LOG_WARNING,
			 "Failed to retrieve 'source' entry from call data in 'media_started' signal. This is a bug in OBS "
			 "Studio.");
		return;
	}

	// Do our WebSocket work.
	nlohmann::json reply = nlohmann::json::object();
	reply["source"]      = build_source_reference(source);
	reply["signal"]      = "started";
	reply["media"]       = build_source_media_metadata(source);

	streamdeck::server::instance()->notify("obs.source.event.media", reply);
}

void streamdeck::handlers::obs_source::on_media_ended(void*, calldata_t* calldata)
{
	// Retrieve information.
	obs_source_t* source;
	if (!calldata_get_ptr(calldata, "source", &source)) {
		DLOG(LOG_WARNING,
			 "Failed to retrieve 'source' entry from call data in 'media_ended' signal. This is a bug in OBS "
			 "Studio.");
		return;
	}

	// Do our WebSocket work.
	nlohmann::json reply = nlohmann::json::object();
	reply["source"]      = build_source_reference(source);
	reply["signal"]      = "ended";
	reply["media"]       = build_source_media_metadata(source);

	streamdeck::server::instance()->notify("obs.source.event.media", reply);
}

void streamdeck::handlers::obs_source::enumerate(std::shared_ptr<streamdeck::jsonrpc::request>,
												 std::shared_ptr<streamdeck::jsonrpc::response> res)
{
	nlohmann::json result = nlohmann::json::array();

	obs_enum_sources(
		[](void* ptr, obs_source_t* source) {
			nlohmann::json* result = static_cast<nlohmann::json*>(ptr);
			result->push_back(build_source_metadata(source));
			return true;
		},
		&result);
	obs_enum_scenes(
		[](void* ptr, obs_source_t* source) {
			nlohmann::json* result = static_cast<nlohmann::json*>(ptr);
			result->push_back(build_source_metadata(source));
			return true;
		},
		&result);

	res->set_result(result);
}

void streamdeck::handlers::obs_source::state(std::shared_ptr<streamdeck::jsonrpc::request>  req,
											 std::shared_ptr<streamdeck::jsonrpc::response> res)
{
	/** obs.source.state
	 *
	 * @param source {string|Array(string)} The source (or source + filter) to check or change the state of.
	 *
	 * @param enabled {bool} [Optional] `true` to enable, `false` to disable.
	 * @param volume {number} [Optional] Volume in percent, from 0.00 to 1.00.
	 * @param muted {bool} [Optional] `true` to mute, `false` to unmute.
	 * @param balance {number} [Optional] Balance as a float from 0.00 (left) to 1.00 (right).
	 *
	 * @return {object} An object containing the current state of the source.
	 */

	nlohmann::json params;
	if (!req->get_params(params)) {
		throw jsonrpc::invalid_request_error("Method requires parameters.");
	}

	// Figure out which source we are modifying.
	std::shared_ptr<obs_source_t> source;
	{
		auto p = params.find("source");
		if (p != params.end()) {
			source = resolve_source_reference(*p);
			if (!source) {
				throw jsonrpc::invalid_params_error("'source' does not exist.");
			}
		} else {
			throw jsonrpc::invalid_params_error("'source' must be present.");
		}
	}

	// Apply enabled state if present.
	{
		auto p = params.find("enabled");
		if (p != params.end()) {
			if (!p->is_boolean()) {
				throw jsonrpc::invalid_params_error("'enabled' must be a Boolean.");
			}

			obs_source_set_enabled(source.get(), p->get<bool>());
		}
	}

	// Apply volume if present.
	{
		auto p = params.find("volume");
		if (p != params.end()) {
			if (!p->is_number()) {
				throw jsonrpc::invalid_params_error("'volume' must be a number.");
			}
			float v = p->get<float>();
			if ((v < 0.) || (v > 1.)) {
				throw jsonrpc::invalid_params_error("'volume' can't be lower than 0 or higher than 1.");
			}

			obs_source_set_volume(source.get(), v);
		}
	}

	// Apply muted state if present.
	{
		auto p = params.find("muted");
		if (p != params.end()) {
			if (!p->is_boolean()) {
				throw jsonrpc::invalid_params_error("'muted' must be a Boolean.");
			}

			obs_source_set_muted(source.get(), p->get<bool>());
		}
	}

	// Apply balance if present.
	{
		auto p = params.find("balance");
		if (p != params.end()) {
			if (!p->is_number()) {
				throw jsonrpc::invalid_params_error("'balance' must be a number.");
			}
			float v = p->get<float>();
			if ((v < 0.) || (v > 1.)) {
				throw jsonrpc::invalid_params_error("'balance' can't be lower than 0 or higher than 1.");
			}

			obs_source_set_balance_value(source.get(), v);
		}
	}

	// Return the currently known information.
	res->set_result(build_source_metadata(source.get()));
}

void streamdeck::handlers::obs_source::settings(std::shared_ptr<streamdeck::jsonrpc::request>  req,
												std::shared_ptr<streamdeck::jsonrpc::response> res)
{
	/** obs.source.settings
	 *
	 * @param source {string|Array(string)} The source (or source + filter) to check or change the state of.
	 *
	 * @param settings {array|object} [Optional] A RFC 6902 or RFC 7386 patch to apply to the settings of a source.
	 *
	 * @return {object} An object containing the current settings of the source.
	 */

	// Validate parameters.
	nlohmann::json parameters;
	if (!req->get_params(parameters)) {
		throw jsonrpc::invalid_params_error("Missing parameters.");
	}

	// - 'source'.
	auto p_source = parameters.find("source");
	if (p_source == parameters.end()) {
		throw jsonrpc::invalid_params_error("Missing 'source' parameter.");
	} else if (!(p_source->is_array() || p_source->is_string())) {
		throw jsonrpc::invalid_params_error("Parameter 'source' must be a string or array.");
	}

	// - 'settings'.
	auto p_settings = parameters.find("settings");
	if (p_settings != parameters.end()) {
		if (!(p_settings->is_array() || p_settings->is_object())) {
			throw jsonrpc::invalid_params_error(
				"Parameter 'settings' must either be an RFC 7386 object or an RFC 6902 array.");
		}
	}

	// Try and resolve the source reference to an actual source.
	auto source = resolve_source_reference(*p_source);
	if (!source) {
		throw jsonrpc::invalid_params_error("Parameter 'source' does not describe an existing source.");
	}

	// Grab the source's settings object.
	std::shared_ptr<obs_data_t> data{obs_source_get_settings(source.get()), [](obs_data_t* v) { obs_data_release(v); }};

	// If settings are specified, update the source.
	if (p_settings != parameters.end()) {
		// Apply the patch.
		auto original_data = nlohmann::json::parse(obs_data_get_json(data.get()));
		auto patched_data  = original_data;
		if (p_settings->is_object()) {
			// RFC 7386: https://tools.ietf.org/html/rfc7386
			try {
				original_data.merge_patch(*p_settings);
			} catch (std::exception const& ex) {
				throw jsonrpc::invalid_params_error(ex.what());
			}
		} else if (p_settings->is_array()) {
			// RFC 6902: https://datatracker.ietf.org/doc/html/rfc6902
			// RFC 6901: https://datatracker.ietf.org/doc/html/rfc6901
			try {
				patched_data = original_data.patch(*p_settings);
			} catch (std::exception const& ex) {
				throw jsonrpc::invalid_params_error(ex.what());
			}
		}

		// If the patch went without errors, update the source.
		data = std::shared_ptr<obs_data_t>(obs_data_create_from_json(patched_data.dump().c_str()),
										   [](obs_data_t* v) { obs_data_release(v); });
		obs_source_update(source.get(), data.get());
	}

	// Reply with the current source settings.
	res->set_result(nlohmann::json::parse(obs_data_get_json(data.get())));
}

void streamdeck::handlers::obs_source::media(std::shared_ptr<streamdeck::jsonrpc::request>  req,
											 std::shared_ptr<streamdeck::jsonrpc::response> res)
{
	/** obs.source.media
	 *
	 * @param source {string|Array(string)} The source (or source + filter) to control media on.
	 * @param action {string} [Optional] One of 'play', 'pause', 'restart', 'stop', 'next', 'previous'.
	 * @param time {integer|number} [Optional] Time to seek to. If float, is specified in seconds. If integer, is specified in milliseconds.
	 *
	 * @return {object} An object containing the current state of the source.
	 */

	// Need to:
	// - time get and set
	// - state get
	// - started get, ended get

	nlohmann::json                args;
	nlohmann::json                out;
	std::shared_ptr<obs_source_t> source;

	{ // Validate the request for all required information.
		if (!req->get_params(args)) {
			throw jsonrpc::invalid_params_error("No parameters.");
		}

		if (!args.contains("source")) {
			throw jsonrpc::invalid_params_error("'source' not specified.");
		}
	}

	if (args.contains("source")) { // Find Source
		auto arg = args.find("source");
		source   = resolve_source_reference(*arg);
		if (!source) {
			throw jsonrpc::invalid_params_error("'source' does not exist.");
		}
	}

	if (args.contains("action")) {
		auto arg = args.find("action");
		if (!arg->is_string()) {
			throw jsonrpc::invalid_params_error("'action' must be a string if provided.");
		} else {
			std::string action = arg->get<std::string>();
			if (action == "play") {
				obs_source_media_play_pause(source.get(), false);
			} else if (action == "pause") {
				obs_source_media_play_pause(source.get(), true);
			} else if (action == "restart") {
				obs_source_media_restart(source.get());
			} else if (action == "stop") {
				obs_source_media_stop(source.get());
			} else if (action == "next") {
				obs_source_media_next(source.get());
			} else if (action == "previous") {
				obs_source_media_previous(source.get());
			} else {
				throw jsonrpc::invalid_params_error("'action' is not one of the accepted strings.");
			}
		}
	}

	if (args.contains("time")) {
		auto    arg      = args.find("time");
		int64_t duration = obs_source_media_get_duration(source.get());
		int64_t time     = 0;

		if (!arg->is_number()) {
			throw jsonrpc::invalid_params_error("'time' must be a number if provided.");
		} else if (arg->is_number_float()) {
			time = std::lroundf(arg->get<float>() * 1000.f);
		} else if (arg->is_number_integer()) {
			time = arg->get<int64_t>() * 1000;
		}

		if (time > duration) {
			throw jsonrpc::invalid_params_error("'time' is larger than duration.");
		} else {
			obs_source_media_set_time(source.get(), time);
		}
	}

	{ // Generate output.
		out = build_source_metadata(source.get());
	}

	res->set_result(out);
}

void streamdeck::handlers::obs_source::properties(std::shared_ptr<streamdeck::jsonrpc::request>  req,
												  std::shared_ptr<streamdeck::jsonrpc::response> res)
{
	/** obs.source.properties
	 *
	 * @param source {string|Array(string)} The source (or source + filter) to check or change the state of.
	 *
	 * @return {object} An object containing the current properties of the source.
	 */

	// Validate parameters.
	nlohmann::json parameters;
	if (!req->get_params(parameters)) {
		throw jsonrpc::invalid_params_error("Missing parameters.");
	}

	// - 'source'.
	auto p_source = parameters.find("source");
	if (p_source == parameters.end()) {
		throw jsonrpc::invalid_params_error("Missing 'source' parameter.");
	} else if (!(p_source->is_array() || p_source->is_string())) {
		throw jsonrpc::invalid_params_error("Parameter 'source' must be a string or array.");
	}

	// Try and resolve the source reference to an actual source.
	auto source = resolve_source_reference(*p_source);
	if (!source) {
		throw jsonrpc::invalid_params_error("Parameter 'source' does not describe an existing source.");
	}

	// Convert properties into useful JSON objects.
	std::shared_ptr<obs_properties_t> properties{obs_get_source_properties(obs_source_get_id(source.get())),
												 [](obs_properties_t* v) { obs_properties_destroy(v); }};
	res->set_result(build_properties_metadata(properties.get()));
}

void streamdeck::handlers::obs_source::filters(std::shared_ptr<streamdeck::jsonrpc::request>  req,
											   std::shared_ptr<streamdeck::jsonrpc::response> res)
{
	// Validate parameters.
	nlohmann::json parameters;
	if (!req->get_params(parameters)) {
		throw jsonrpc::invalid_params_error("Missing parameters.");
	}

	auto p_source = parameters.find("source");

	if (p_source == parameters.end()) {
		throw jsonrpc::invalid_params_error("'source' must be present.");
	} else if (!p_source->is_string()) {
		throw jsonrpc::invalid_params_error("'source' must be of type 'string'.");
	}

	// Try and find things in question.
	std::shared_ptr<obs_source_t> source;

	{ // Try and retrieve the described source from the name.
		std::string name = p_source->get<std::string>();
		source           = std::shared_ptr<obs_source_t>(obs_get_source_by_name(name.c_str()), obs_source_deleter);
		if (!source) {
			throw jsonrpc::invalid_params_error("'source' does not describe an existing source.");
		}
	}

	// Enumerate over scene items.
	nlohmann::json result = nlohmann::json::array();
	obs_source_enum_filters(
		source.get(),
		[](obs_source_t*, obs_source_t* filter, void* ptr) {
			nlohmann::json* result = static_cast<nlohmann::json*>(ptr);
			result->push_back(build_source_metadata(filter));
		},
		&result);

	res->set_result(result);
}
