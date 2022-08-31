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

#include "server.hpp"
#include <cinttypes>
#include <functional>
#include <memory>
#include <mutex>
#include "json-rpc.hpp"
#include "module.hpp"
#include "obs-frontend-api.h"


const unsigned short WEBSOCKET_PORTS[] = {28186, 39726, 34247, 42206, 38535, 40829, 40624, 0};
#define WEBSOCKET_PROTOCOL "streamdeck-obs"

/* clang-format off */
#define DLOG(LEVEL, ...) streamdeck::message(streamdeck::log_level:: LEVEL, "[Server] " __VA_ARGS__)
/* clang-format on */

streamdeck::server::~server()
{
	DLOG(LOG_DEBUG, "Destroying WebSocket handler...");

	if (_ws.is_listening()) {
		// WebSocket: Stop listening.
		_ws.stop_listening();

		// WebSocket: Disconnect any clients.
		for (auto handle : _ws_clients) { // !TODO!
			auto con = _ws.get_con_from_hdl(handle.first);
			_ws.close(con, websocketpp::close::status::going_away, "Shutting down.");
		}
	}

	// Worker: Signal to stop and join.
	_worker_alive = false;
	if (_worker.joinable())
		_worker.join();
}

streamdeck::server::server()
	: _ws(),

	  _worker(), _worker_alive(true)
{
	DLOG(LOG_DEBUG, "Creating WebSocket handler...");

	// WebSocket: Initialize.
	_ws.set_access_channels(websocketpp::log::alevel::all ^ websocketpp::log::alevel::frame_payload);
	_ws.set_error_channels(websocketpp::log::elevel::all);
	_ws.set_listen_backlog(1);
	_ws.init_asio();

	// WebSocket: Set callbacks.
	_ws.set_validate_handler(std::bind(&streamdeck::server::ws_on_validate, this, std::placeholders::_1));
	_ws.set_open_handler(std::bind(&streamdeck::server::ws_on_open, this, std::placeholders::_1));
	_ws.set_message_handler(
		std::bind(&streamdeck::server::ws_on_message, this, std::placeholders::_1, std::placeholders::_2));
	_ws.set_close_handler(std::bind(&streamdeck::server::ws_on_close, this, std::placeholders::_1));

	// Worker: Launch and set active.
	_worker_alive = true;
	_worker       = std::thread(std::bind(&streamdeck::server::run, this));
}

void streamdeck::server::handle(std::string method, streamdeck::server::handler_callback_t callback)
{
	_methods.emplace(method, handler_type::DEFAULT);
	_handler_default.emplace(method, callback);
}

void streamdeck::server::handle_sync(std::string method, sync_handler_callback_t callback)
{
	_methods.emplace(method, handler_type::SYNCHRONOUS);
	_handler_sync.emplace(method, callback);
}

void streamdeck::server::handle_async(std::string method, streamdeck::server::async_handler_callback_t callback)
{
	_methods.emplace(method, handler_type::ASYNCHRONOUS);
	_handler_async.emplace(method, callback);
}

void streamdeck::server::notify(std::string method, nlohmann::json params)
{
	streamdeck::jsonrpc::request rq;
	rq.set_method(method);
	rq.set_params(params);
	rq.clear_id();

	auto str = rq.compile().dump();
	for (auto handle : _ws_clients) { // !TODO!
		auto con = _ws.get_con_from_hdl(handle.first);
#ifdef _DEBUG
		DLOG(LOG_DEBUG, "<%s> Notify \"%s\"", con->get_remote_endpoint().c_str(), str.c_str());
#endif
		con->send(str, websocketpp::frame::opcode::text);
	}
}

void streamdeck::server::reply(std::weak_ptr<void> handle, std::shared_ptr<streamdeck::jsonrpc::response> response)
{
	auto str = response->compile().dump();
	auto con = _ws.get_con_from_hdl(handle);
#ifdef _DEBUG
	DLOG(LOG_DEBUG, "<%s> Async Reply \"%s\"", con->get_remote_endpoint().c_str(), str.c_str());
#endif
	con->send(str, websocketpp::frame::opcode::text);
}

void streamdeck::server::run()
{
	// Build the appropriate endpoint.
	asio::ip::tcp::endpoint ep;
	unsigned short          portIdx    = 0;
	unsigned short          listenPort = WEBSOCKET_PORTS[portIdx];

	ep.address(asio::ip::address_v4::loopback());
	ep.port(listenPort);
	std::string address = ep.address().to_string();

	// Try to listen for new connections for 30 seconds.
	uint64_t                                       attempts = 0;
	std::chrono::high_resolution_clock::time_point start    = std::chrono::high_resolution_clock::now();
	while (((std::chrono::high_resolution_clock::now() - start) < std::chrono::seconds(30)) && _worker_alive) {
		try {
			_ws.listen(ep);
			break;
		} catch (std::exception const& ex) {
			attempts++;
			if (attempts % 100 == 0) {
				DLOG(LOG_WARNING, "Attempt %" PRIu64 " at listening on '%s:%" PRIu16 "' failed with exception: %s",
					 attempts, address.c_str(), uint16_t(ep.port()), ex.what());
			}
			listenPort = WEBSOCKET_PORTS[portIdx++];
			if (listenPort == 0) {
				portIdx    = 0;
				listenPort = WEBSOCKET_PORTS[portIdx];
			}
			ep.port(listenPort);
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}
	if (!_ws.is_listening()) {
		DLOG(LOG_ERROR, "Failed to listen on '%s:%" PRIu16 "' after 30 seconds.", address.c_str(), uint16_t(ep.port()));
		return;
	} else {
		DLOG(LOG_INFO, "Listening on '%s:%" PRIu16 "'.", address.c_str(), uint16_t(ep.port()));
	}

	// Start accepting new connections.
	_ws.start_accept();

	// Perform work
	while (_worker_alive) {
		while (_ws.poll_one() != 0) {
			_ws.run_one();
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

nlohmann::json streamdeck::server::handle_call(websocketpp::connection_hdl handle, nlohmann::json& request)
{
	std::shared_ptr<streamdeck::jsonrpc::request>  req;
	std::shared_ptr<streamdeck::jsonrpc::response> res_allocated = std::make_shared<streamdeck::jsonrpc::response>();
	std::shared_ptr<streamdeck::jsonrpc::response> res           = res_allocated;

	try {
		auto clientIter = _ws_clients.find(handle);
		jsonrpc::client* client = nullptr;
		if (clientIter != _ws_clients.end()) {
			client = clientIter->second;
		}
		req = std::make_shared<streamdeck::jsonrpc::request>(request, client);

		// Figure out the type of handler we have.
		std::string method = req->get_method();
		auto        mf     = _methods.find(method);
		if (mf != _methods.end()) {
			if (mf->second == handler_type::ASYNCHRONOUS) {
				auto itr = _handler_async.find(method);
				if (itr != _handler_async.end()) {
					itr->second(handle, req);
					// Skip all other processing, as asynchronous calls have a delayed response.
					return nlohmann::json();
				} else {
					throw streamdeck::jsonrpc::internal_error("Failed to find asynchronous handler.");
				}
			} else if (mf->second == handler_type::SYNCHRONOUS) {
				auto itr = _handler_sync.find(method);
				if (itr != _handler_sync.end()) {
					res_allocated->copy_id(*req);
					itr->second(req, res_allocated);
				} else {
					throw streamdeck::jsonrpc::internal_error("Failed to find synchronous handler.");
				}
			} else if (mf->second == handler_type::DEFAULT) {
				auto itr = _handler_default.find(method);
				if (itr != _handler_default.end()) {
					res = itr->second(req);
					if (!res) {
						res = res_allocated;
						res->copy_id(*(req.get()));
						res->set_result(nlohmann::json());
					}
				} else {
					throw streamdeck::jsonrpc::internal_error("Failed to find default handler.");
				}
			} else {
				throw streamdeck::jsonrpc::internal_error("Failed to resolve method handler.");
			}
		} else {
			throw streamdeck::jsonrpc::method_not_found_error("Method is unknown to us.");
		}
	} catch (streamdeck::jsonrpc::error const& ex) {
		res->set_error(ex.id(), ex.what() ? ex.what() : "Unknown error.");
	} catch (nlohmann::json::parse_error const& ex) {
		res = std::make_shared<streamdeck::jsonrpc::response>();
		res->set_error(streamdeck::jsonrpc::error_codes::INVALID_REQUEST, ex.what() ? ex.what() : "Unknown error.");
	} catch (std::exception const& ex) {
		res->set_error(streamdeck::jsonrpc::error_codes::INTERNAL_ERROR, ex.what() ? ex.what() : "Unknown error.");
	}
	try {
		res->validate();
	} catch (streamdeck::jsonrpc::error const& ex) {
		res->set_error(streamdeck::jsonrpc::error_codes::INTERNAL_ERROR, ex.what() ? ex.what() : "Unknown error.");
	} catch (nlohmann::json::parse_error const& ex) {
		res->set_error(streamdeck::jsonrpc::error_codes::INTERNAL_ERROR, ex.what() ? ex.what() : "Unknown error.");
	}
	return res->compile();
}

bool streamdeck::server::ws_on_validate(websocketpp::connection_hdl handle)
{
	auto con = _ws.get_con_from_hdl(handle);

	{ // Validate SubProtocols
		auto sps     = con->get_requested_subprotocols();
		bool have_sp = false;
		for (auto sp : sps) {
			if (sp == WEBSOCKET_PROTOCOL) {
				con->select_subprotocol(sp);
				have_sp = true;
				break;
			}
		}
		if (!have_sp)
			return false;
	}

	return true;
}

void streamdeck::server::ws_on_open(websocketpp::connection_hdl handle)
{ // Client connected to us, do things with it.
	// Track the new client.
	_ws_clients.emplace(handle, new jsonrpc::client());

	auto con = _ws.get_con_from_hdl(handle);

	auto host = con->get_remote_endpoint();
#ifdef _DEBUG
	DLOG(LOG_DEBUG, "New Client %s", host.c_str());
#endif
}

void streamdeck::server::ws_on_close(websocketpp::connection_hdl handle)
{ // Server disconnected from us,
	// Untrack the client.
	auto iter = _ws_clients.find(handle);
	if (iter != _ws_clients.end()) {
		delete iter->second;
	}

	_ws_clients.erase(handle);

	auto con = _ws.get_con_from_hdl(handle);

	auto host = con->get_remote_endpoint();
#ifdef _DEBUG
	DLOG(LOG_DEBUG, "Lost Client %s", host.c_str());
#endif
}

void streamdeck::server::ws_on_message(websocketpp::connection_hdl handle, ws_server_t::message_ptr msg)
{
	auto con = _ws.get_con_from_hdl(handle);
	try {
		nlohmann::json input = nlohmann::json::parse(msg->get_payload());
		std::string    output;
		if (input.is_array()) {
			// Group Call
			nlohmann::json responses = nlohmann::json::array();
			for (auto idx = 0; idx < input.size(); idx++) {
				nlohmann::json entry = input.at(idx);
				auto           obj   = handle_call(handle, entry);
				if (obj.is_object()) {
					responses.emplace_back(obj);
				}
			}
			output = responses.dump();
		} else {
			// Solo Call
			auto obj = handle_call(handle, input);
			if (obj.is_object()) {
				output = obj.dump();
			}
		}
		if (output.length() > 0) {
			std::error_code ec = con->send(output);
#ifdef _DEBUG
			DLOG(LOG_DEBUG, "<%s> Query \"%s\" Reply \"%s\"", con->get_remote_endpoint().c_str(),
				 msg->get_payload().c_str(), output.c_str());
		} else {
			DLOG(LOG_DEBUG, "<%s> Async Query \"%s\"", con->get_remote_endpoint().c_str(), msg->get_payload().c_str());
#endif
		}
	} catch (std::exception const& ex) {
		DLOG(LOG_WARNING, "<%s> Exception: %s", con->get_remote_endpoint().c_str(), ex.what());
	}
}

std::shared_ptr<streamdeck::server> streamdeck::server::instance()
{
	static std::weak_ptr<streamdeck::server> _instance;
	static std::mutex                        _lock;

	std::unique_lock<std::mutex> lock(_lock);
	if (_instance.expired()) {
		auto instance = std::make_shared<streamdeck::server>();
		_instance     = instance;
		return instance;
	}

	return _instance.lock();
}

std::string streamdeck::server::remote_version_string() const
{
	std::map<std::string, int> versions;
	for (auto handle : _ws_clients) {
		versions[handle.second->get_version()]++;
	}
	auto iter = versions.end();
	std::string result = REMOTE_NOT_CONNECTED;
	bool        found_result = false;
	int         other_result_count = 0;
	int         loops              = 0;
	while (iter != versions.begin()) {
		iter--;
		if (iter->first != REMOTE_NOT_CONNECTED) {
			if (!found_result) {
				result       = iter->first;
				found_result = true;
			} else {
				other_result_count++;
			}
			loops++;
		}
	}
	if (result == REMOTE_NOT_CONNECTED) {
		result = get_translated_text("SDNotConnected");
	}
	if (other_result_count > 0) {
		result += " (+" + std::to_string(other_result_count) + ")";
	}
	return result;
}
