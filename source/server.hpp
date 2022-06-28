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
#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <thread>

#include <nlohmann/json.hpp>
#include "json-rpc.hpp"
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4005 4244 4267)
#endif
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace streamdeck {
	class server {
		typedef websocketpp::server<websocketpp::config::asio>                                             ws_server_t;
		typedef std::map<websocketpp::connection_hdl, jsonrpc::client*, std::owner_less<websocketpp::connection_hdl>> ws_clients_t;

		typedef std::function<std::shared_ptr<streamdeck::jsonrpc::response>(
			std::shared_ptr<streamdeck::jsonrpc::request>)>
			handler_callback_t;
		typedef std::function<void(std::shared_ptr<streamdeck::jsonrpc::request>,
								   std::shared_ptr<streamdeck::jsonrpc::response>)>
			sync_handler_callback_t;
		typedef std::function<void(std::weak_ptr<void>, std::shared_ptr<streamdeck::jsonrpc::request>)>
			async_handler_callback_t;

		enum handler_type {
			DEFAULT,
			SYNCHRONOUS,
			ASYNCHRONOUS,
		};

		std::map<std::string, handler_type>             _methods;
		std::map<std::string, handler_callback_t>       _handler_default;
		std::map<std::string, sync_handler_callback_t>  _handler_sync;
		std::map<std::string, async_handler_callback_t> _handler_async;

		ws_server_t  _ws;
		ws_clients_t _ws_clients;

		std::thread       _worker;
		std::atomic<bool> _worker_alive;

		public:
		~server();
		server();

		void handle(std::string method, streamdeck::server::handler_callback_t callback);
		void handle_sync(std::string method, streamdeck::server::sync_handler_callback_t callback);
		void handle_async(std::string method, streamdeck::server::async_handler_callback_t callback);

		void notify(std::string method, nlohmann::json params = nlohmann::json());

		void reply(std::weak_ptr<void> handle, std::shared_ptr<streamdeck::jsonrpc::response> response);

		std::string remote_version_string() const;

		private:
		void run();

		nlohmann::json handle_call(websocketpp::connection_hdl handle, nlohmann::json& request);

		private /* WebSocket Callbacks */:
		bool ws_on_validate(websocketpp::connection_hdl);
		void ws_on_open(websocketpp::connection_hdl);
		void ws_on_message(websocketpp::connection_hdl, ws_server_t::message_ptr);
		void ws_on_close(websocketpp::connection_hdl);

		public /* Singleton */:
		static std::shared_ptr<streamdeck::server> instance();
	};
} // namespace streamdeck
