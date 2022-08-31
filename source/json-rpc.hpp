// Copyright (C) 2022, Corsair Memory Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 3. Neither the name of the copyright holder nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

// Simple wrapper around nlohmann::json to implement the JSON RPC 2.0 specification:
// https://www.jsonrpc.org/specification

#pragma once // Replaces Preprocessor guards, works on most major compilers (MSVC, GCC, Clang, ...)

#include <cstddef>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

#define REMOTE_NOT_CONNECTED "[Not Connected]"

namespace streamdeck {
	namespace jsonrpc {
		enum error_codes : int64_t {
			PARSE_ERROR      = -32700,
			INTERNAL_ERROR   = -32603,
			INVALID_PARAMS   = -32602,
			METHOD_NOT_FOUND = -32601,
			INVALID_REQUEST  = -32600,
			SERVER_ERROR_MAX = -32099,
			SERVER_ERROR     = -32000,
			// Reserved error codes (-32099 - -32000)
		};

		class error : public std::runtime_error {
			public:
			error(const char* reason) : std::runtime_error(reason) {}

			virtual int64_t id() const
			{
				return 0;
			}
		};

		class parse_error : public error {
			public:
			parse_error(const char* reason) : error(reason) {}

			int64_t id() const
			{
				return static_cast<int64_t>(error_codes::PARSE_ERROR);
			}
		};

		class invalid_params_error : public error {
			public:
			invalid_params_error(const char* reason) : error(reason) {}

			int64_t id() const
			{
				return static_cast<int64_t>(error_codes::INVALID_PARAMS);
			}
		};

		class invalid_request_error : public error {
			public:
			invalid_request_error(const char* reason) : error(reason) {}

			int64_t id() const
			{
				return static_cast<int64_t>(error_codes::INVALID_REQUEST);
			}
		};

		class method_not_found_error : public error {
			public:
			method_not_found_error(const char* reason) : error(reason) {}

			int64_t id() const
			{
				return static_cast<int64_t>(error_codes::METHOD_NOT_FOUND);
			}
		};

		class internal_error : public error {
			public:
			internal_error(const char* reason) : error(reason) {}

			int64_t id() const
			{
				return static_cast<int64_t>(error_codes::INTERNAL_ERROR);
			}
		};

		class server_error : public error {
			int64_t _id;

			public:
			server_error(const char* reason, int64_t rid) : error(reason), _id(rid) {}

			int64_t id() const
			{
				return static_cast<int64_t>(error_codes::SERVER_ERROR) - _id;
			}
		};

		class custom_error : public error {
			int64_t _id;

			public:
			custom_error(const char* reason, int64_t rid) : error(reason), _id(rid) {}

			int64_t id() const
			{
				return _id;
			}
		};

		class client {
			std::string _remote_version;

			public:
			client();
			void set_version(const std::string& version);
			std::string get_version() const;
		};

		class jsonrpc {
			protected:
			virtual ~jsonrpc();
			jsonrpc();

			nlohmann::json _json;
			client*        _client;

			public:
			jsonrpc& clear_id();
			jsonrpc& set_id();
			jsonrpc& set_id(int64_t value);
			jsonrpc& set_id(const std::string& value);
			bool     get_id(int64_t& value);
			bool     get_id(std::string& value);
			bool     has_id();
			jsonrpc& copy_id(streamdeck::jsonrpc::jsonrpc& other);
			client*  get_client();

			nlohmann::json compile();

			public:
			virtual void validate() = 0;
		};

		class request : public jsonrpc {
			public:
			virtual ~request();
			request();
			request(nlohmann::json& json, client* c);

			request&    set_method(const std::string& value);
			bool        get_method(std::string& value);
			std::string get_method();

			request& clear_params();
			request& set_params(nlohmann::json value);
			bool     get_params(nlohmann::json& value);

			public:
			virtual void validate() override;
		};

		class response : public jsonrpc {
			public:
			virtual ~response();
			response();
			response(nlohmann::json& json, client* c);

			response& set_result(nlohmann::json value);
			bool      get_result(nlohmann::json& value);

			response& set_error(int64_t code, const std::string& message);
			response& set_error(int64_t code, const std::string& message, nlohmann::json data);
			bool      get_error_code(int64_t& value);
			bool      get_error_message(std::string& value);
			bool      get_error_data(nlohmann::json& value);

			public:
			virtual void validate() override;
		};
	} // namespace jsonrpc
} // namespace streamdeck
