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

#include "json-rpc.hpp"

#define PROTOCOL_ID "2.0"

#define VALIDATION_JSONRPC_MISSING "'jsonrpc' is missing"
#define VALIDATION_JSONRPC_TYPE "'jsonrpc' has wrong type"
#define VALIDATION_JSONRPC_VALUE "'jsonrpc' has the wrong value, must be '" PROTOCOL_ID "'"
#define VALIDATION_ID_MISSING "'id' is missing"
#define VALIDATION_ID_TYPE "'id' has wrong type"
#define VALIDATION_REQUEST_METHOD_MISSING "'method' is missing"
#define VALIDATION_REQUEST_METHOD_TYPE "'method' has wrong type"
#define VALIDATION_REQUEST_PARAMS_TYPE "'params' has wrong type"
#define VALIDATION_RESPONSE_MISSING "'result' and 'error' are missing"
#define VALIDATION_RESPONSE_MALFORMED "'result' and 'error' can't coexist"
#define VALIDATION_RESPONSE_ERROR_TYPE "'error' has wrong type"
#define VALIDATION_RESPONSE_ERROR_CODE_MISSING "'error.code' is missing"
#define VALIDATION_RESPONSE_ERROR_CODE_TYPE "'error.code' has wrong type"
#define VALIDATION_RESPONSE_ERROR_MESSAGE_MISSING "'error.message' is missing"
#define VALIDATION_RESPONSE_ERROR_MESSAGE_TYPE "'error.message' has wrong type"

void rpc_validate(nlohmann::json& json)
{
	// 'jsonrpc' must exist, be a string, and have the correct value.
	auto jsonrpc_obj = json.find("jsonrpc");
	if (jsonrpc_obj != json.end()) {
		if (!jsonrpc_obj->is_string())
			throw streamdeck::jsonrpc::parse_error(VALIDATION_JSONRPC_TYPE);
		if (jsonrpc_obj->get<std::string>() != PROTOCOL_ID)
			throw streamdeck::jsonrpc::parse_error(VALIDATION_JSONRPC_VALUE);
	} else {
		throw streamdeck::jsonrpc::parse_error(VALIDATION_JSONRPC_MISSING);
	}

	// 'id' must exist, and be a string or an integer.
	auto id_obj = json.find("id");
	if (id_obj != json.end()) {
		if (!(id_obj->is_null() || id_obj->is_string() || id_obj->is_number_integer()))
			throw streamdeck::jsonrpc::parse_error(VALIDATION_ID_TYPE);
	}
}

streamdeck::jsonrpc::jsonrpc::~jsonrpc() {}

streamdeck::jsonrpc::jsonrpc::jsonrpc() : _json(nlohmann::json::object())
{
	_json["jsonrpc"] = PROTOCOL_ID;
}

streamdeck::jsonrpc::jsonrpc& streamdeck::jsonrpc::jsonrpc::clear_id()
{
	_json.erase("id");
	return *this;
}

streamdeck::jsonrpc::jsonrpc& streamdeck::jsonrpc::jsonrpc::set_id()
{
	_json["id"] = nlohmann::json();
	return *this;
}

streamdeck::jsonrpc::jsonrpc& streamdeck::jsonrpc::jsonrpc::set_id(int64_t value)
{
	_json["id"] = value;
	return *this;
}

streamdeck::jsonrpc::jsonrpc& streamdeck::jsonrpc::jsonrpc::set_id(const std::string& value)
{
	_json["id"] = value;
	return *this;
}

bool streamdeck::jsonrpc::jsonrpc::get_id(int64_t& value)
{
	auto obj_id = _json.find("id");
	if ((obj_id != _json.end()) && obj_id->is_number_integer()) {
		obj_id->get_to(value);
		return true;
	}
	return false;
}

bool streamdeck::jsonrpc::jsonrpc::get_id(std::string& value)
{
	auto obj_id = _json.find("id");
	if ((obj_id != _json.end()) && obj_id->is_string()) {
		obj_id->get_to(value);
		return true;
	}
	return false;
}

bool streamdeck::jsonrpc::jsonrpc::has_id()
{
	auto obj_id = _json.find("id");
	return (obj_id != _json.end());
}

nlohmann::json streamdeck::jsonrpc::jsonrpc::compile()
{
	validate();
	return _json;
}

streamdeck::jsonrpc::jsonrpc& streamdeck::jsonrpc::jsonrpc::copy_id(streamdeck::jsonrpc::jsonrpc& other)
{
	nlohmann::json id;
	auto           id_obj = other._json.find("id");
	if (id_obj != other._json.end()) {
		id_obj->get_to(id);
		_json["id"] = id;
	}
	return *this;
}

streamdeck::jsonrpc::client* streamdeck::jsonrpc::jsonrpc::get_client() {
	return _client;
}

streamdeck::jsonrpc::request::~request() {}

streamdeck::jsonrpc::request::request()
{
	_json["method"] = "";
}

streamdeck::jsonrpc::request::request(nlohmann::json& json, client* c = nullptr)
{
	_json = json;
	_client = c;
	validate();
}

streamdeck::jsonrpc::request& streamdeck::jsonrpc::request::set_method(const std::string& value)
{
	_json["method"] = value;
	return *this;
}

bool streamdeck::jsonrpc::request::get_method(std::string& value)
{
	auto method_obj = _json.find("method");
	if ((method_obj != _json.end()) && method_obj->is_string()) {
		method_obj->get_to(value);
		return true;
	}
	return false;
}

std::string streamdeck::jsonrpc::request::get_method()
{
	std::string value;
	auto        method_obj = _json.find("method");
	if ((method_obj != _json.end()) && method_obj->is_string()) {
		method_obj->get_to(value);
		return value;
	}
	return value;
}

streamdeck::jsonrpc::request& streamdeck::jsonrpc::request::clear_params()
{
	_json.erase("params");
	return *this;
}

streamdeck::jsonrpc::request& streamdeck::jsonrpc::request::set_params(nlohmann::json value)
{
	_json["params"] = value;
	return *this;
}

bool streamdeck::jsonrpc::request::get_params(nlohmann::json& value)
{
	auto params_obj = _json.find("params");
	if ((params_obj != _json.end())) {
		params_obj->get_to(value);
		return true;
	}
	return false;
}

void streamdeck::jsonrpc::request::validate()
{
	rpc_validate(_json);

	auto method_obj = _json.find("method");
	if (method_obj != _json.end()) {
		if (!method_obj->is_string())
			throw streamdeck::jsonrpc::parse_error(VALIDATION_REQUEST_METHOD_TYPE);
	} else {
		throw streamdeck::jsonrpc::parse_error(VALIDATION_REQUEST_METHOD_MISSING);
	}

	auto params_obj = _json.find("params");
	if (params_obj != _json.end()) {
		if (!params_obj->is_structured())
			throw streamdeck::jsonrpc::parse_error(VALIDATION_REQUEST_PARAMS_TYPE);
	}
}

streamdeck::jsonrpc::response::~response() {}

streamdeck::jsonrpc::response::response()
{
	_json["result"] = 0;
}

streamdeck::jsonrpc::response::response(nlohmann::json& json, client* c)
{
	_json = json;
	_client = c;
	validate();
}

streamdeck::jsonrpc::response& streamdeck::jsonrpc::response::set_result(nlohmann::json value)
{
	_json.erase("error");
	_json["result"] = value;
	return *this;
}

bool streamdeck::jsonrpc::response::get_result(nlohmann::json& value)
{
	auto result_obj = _json.find("result");
	if ((result_obj != _json.end())) {
		result_obj->get_to(value);
		return true;
	}
	return false;
}

streamdeck::jsonrpc::response& streamdeck::jsonrpc::response::set_error(int64_t code, const std::string& message)
{
	_json.erase("result");

	nlohmann::json error_obj = nlohmann::json::object();
	error_obj["code"]        = code;
	error_obj["message"]     = message;
	_json["error"]           = error_obj;

	return *this;
}

streamdeck::jsonrpc::response& streamdeck::jsonrpc::response::set_error(int64_t code, const std::string& message,
																		nlohmann::json data)
{
	_json.erase("result");

	nlohmann::json error_obj = nlohmann::json::object();
	error_obj["code"]        = code;
	error_obj["message"]     = message;
	error_obj["data"]        = data;
	_json["error"]           = error_obj;

	return *this;
}

bool streamdeck::jsonrpc::response::get_error_code(int64_t& value)
{
	auto error_obj = _json.find("error");
	if ((error_obj != _json.end())) {
		auto code_obj = error_obj->find("code");
		if ((code_obj != error_obj->end())) {
			code_obj->get_to(value);
			return true;
		}
	}
	return false;
}

bool streamdeck::jsonrpc::response::get_error_message(std::string& value)
{
	auto error_obj = _json.find("error");
	if ((error_obj != _json.end())) {
		auto message_obj = error_obj->find("message");
		if ((message_obj != error_obj->end())) {
			message_obj->get_to(value);
			return true;
		}
	}
	return false;
}

bool streamdeck::jsonrpc::response::get_error_data(nlohmann::json& value)
{
	auto error_obj = _json.find("error");
	if ((error_obj != _json.end())) {
		auto data_obj = error_obj->find("data");
		if ((data_obj != error_obj->end())) {
			data_obj->get_to(value);
			return true;
		}
	}
	return false;
}

void streamdeck::jsonrpc::response::validate()
{
	rpc_validate(_json);

	auto id_obj = _json.find("id");
	if (id_obj == _json.end()) {
		throw streamdeck::jsonrpc::parse_error(VALIDATION_ID_MISSING);
	}

	auto result_obj = _json.find("result");
	auto error_obj  = _json.find("error");
	if (result_obj != _json.end()) {    // Response was a success.
		if (error_obj != _json.end()) { // Response can't contain both error and result.
			throw streamdeck::jsonrpc::parse_error(VALIDATION_RESPONSE_MALFORMED);
		}
	} else if (error_obj != _json.end()) { // Response was an error.
		// Don't have result, but have error.
		if (!error_obj->is_object()) {
			throw streamdeck::jsonrpc::parse_error(VALIDATION_RESPONSE_ERROR_TYPE);
		}

		auto code_obj = error_obj->find("code");
		if (code_obj != error_obj->end()) {
			if (!code_obj->is_number_integer())
				throw streamdeck::jsonrpc::parse_error(VALIDATION_RESPONSE_ERROR_CODE_TYPE);
		} else {
			throw streamdeck::jsonrpc::parse_error(VALIDATION_RESPONSE_ERROR_CODE_MISSING);
		}

		auto message_obj = error_obj->find("message");
		if (message_obj != error_obj->end()) {
			if (!message_obj->is_string())
				throw streamdeck::jsonrpc::parse_error(VALIDATION_RESPONSE_ERROR_MESSAGE_TYPE);
		} else {
			throw streamdeck::jsonrpc::parse_error(VALIDATION_RESPONSE_ERROR_MESSAGE_MISSING);
		}
	} else {
		// Have neither.
		throw streamdeck::jsonrpc::parse_error(VALIDATION_RESPONSE_MISSING);
	}
}

streamdeck::jsonrpc::client::client() {
	_remote_version = REMOTE_NOT_CONNECTED;
}

void streamdeck::jsonrpc::client::set_version(const std::string& version)
{
	_remote_version = version;
}

std::string streamdeck::jsonrpc::client::get_version() const
{
	return _remote_version;
}
