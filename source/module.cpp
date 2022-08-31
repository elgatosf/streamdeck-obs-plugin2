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

#include "module.hpp"
#include <cstdarg>
#include <vector>
#include "server.hpp"
#include "version.hpp"
#include "details-popup.hpp"

#include "handlers/handler-obs-frontend.hpp"
#include "handlers/handler-obs-scene.hpp"
#include "handlers/handler-obs-source.hpp"
#include "handlers/handler-system.hpp"

#include "obs-module.h"

static std::shared_ptr<streamdeck::server>                 _server;
static std::shared_ptr<streamdeck::handlers::system>       _handler_system;
static std::shared_ptr<streamdeck::handlers::obs_frontend> _handler_obs_frontend;
static std::shared_ptr<streamdeck::handlers::obs_scene>    _handler_obs_scene;
static std::shared_ptr<streamdeck::handlers::obs_source>   _handler_obs_source;



MODULE_EXPORT bool obs_module_load(void)
{
	try {
		
		streamdeck::message(streamdeck::log_level::LOG_INFO, "Plugin version %s", STREAMDECK_VERSION);

		obs_frontend_add_tools_menu_item(obs_module_text("Streamdeck.Title"), plugin_show_details, nullptr);

		_server               = streamdeck::server::instance();
		_handler_system       = streamdeck::handlers::system::instance();
		_handler_obs_frontend = streamdeck::handlers::obs_frontend::instance();
		_handler_obs_scene    = streamdeck::handlers::obs_scene::instance();
		_handler_obs_source   = streamdeck::handlers::obs_source::instance();
		return true;
	} catch (...) {
		// If an exception occured, immediately abort everything.
		obs_module_unload();
		return false;
	}
}

MODULE_EXPORT void obs_module_unload(void)
{
	_server.reset();
	_handler_obs_source.reset();
	_handler_obs_scene.reset();
	_handler_obs_frontend.reset();
	_handler_system.reset();
}

std::string streamdeck::get_translated_text(const std::string& text)
{
	return obs_module_text(text.data());
}

void streamdeck::message(streamdeck::log_level level, const char* format, ...)
{
	static thread_local std::vector<char> buffer;

	va_list va;

	va_start(va, format);
	buffer.resize(vsnprintf(buffer.data(), 0, format, va) + 1);
	va_end(va);

	va_start(va, format);
	vsnprintf(buffer.data(), buffer.size(), format, va);
	va_end(va);

	blog(static_cast<int>(level), "<StreamDeck> %s", buffer.data());
}
