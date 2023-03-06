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

#include "handler-system.hpp"
#include <memory>
#include <mutex>
#include "module.hpp"
#include "server.hpp"
#include "version.hpp"

/* clang-format off */
#define DLOG(LEVEL, ...) streamdeck::message(streamdeck::log_level:: LEVEL, "[Handler::System] " __VA_ARGS__)
/* clang-format on */

std::shared_ptr<streamdeck::handlers::system> streamdeck::handlers::system::instance()
{
	static std::weak_ptr<streamdeck::handlers::system> _instance;
	static std::mutex                                  _lock;

	std::unique_lock<std::mutex> lock(_lock);
	if (_instance.expired()) {
		auto instance = std::make_shared<streamdeck::handlers::system>();
		_instance     = instance;
		return instance;
	}

	return _instance.lock();
}

streamdeck::handlers::system::system()
{
	auto server = streamdeck::server::instance();
	server->handle("ping", std::bind(&streamdeck::handlers::system::_ping, this, std::placeholders::_1));
	server->handle_sync("version", std::bind(&streamdeck::handlers::system::_version, this, std::placeholders::_1,
											 std::placeholders::_2));
}

std::shared_ptr<streamdeck::jsonrpc::response>
	streamdeck::handlers::system::_ping(std::shared_ptr<streamdeck::jsonrpc::request>)
{
	return nullptr;
}

void streamdeck::handlers::system::_version(std::shared_ptr<streamdeck::jsonrpc::request>  req,
											std::shared_ptr<streamdeck::jsonrpc::response> res)
{
	nlohmann::json params;
	if (!req->get_params(params)) {
		throw jsonrpc::invalid_request_error("Method requires parameters.");
	}
	if (params.is_object() && params["version"].is_string()) {
		auto client = req->get_client();
		if (client) {
			client->set_version(params["version"]);
		}
	}

	nlohmann::json result = nlohmann::json::object();
	result["version"]     = STREAMDECK_VERSION;
	result["semver"]      = {STREAMDECK_VERSION_MAJOR, STREAMDECK_VERSION_MINOR, STREAMDECK_VERSION_PATCH, STREAMDECK_VERSION_BUILD};
	
	res->set_result(result);

}
