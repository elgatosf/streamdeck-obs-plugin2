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

#include "obs-module.h"
#include "util/platform.h"
#include <string>

void* dl_handle;
void (*ind_obs_module_set_pointer)(obs_module_t*);
bool (*ind_obs_module_load)(void);
void (*ind_obs_module_unload)(void);
void (*ind_obs_module_post_load)(void);
bool (*ind_obs_module_get_string)(const char*, const char**);
const char* (*ind_obs_module_author)(void);
const char* (*ind_obs_module_name)(void);
const char* (*ind_obs_module_description)(void);
void (*ind_obs_module_set_locale)(const char*);
void (*ind_obs_module_free_locale)(void);

std::string current_locale;

// OBS_DECLARE_MODULE()
static obs_module_t* obs_module_pointer;
MODULE_EXPORT void   obs_module_set_pointer(obs_module_t* module)
{
	obs_module_pointer = module;
	blog(static_cast<int>(300), "<StreamDeck> obs_module_set_pointer");
	if (ind_obs_module_set_pointer) {
		blog(static_cast<int>(300), "<StreamDeck> obs_module_set_pointer done");
		ind_obs_module_set_pointer(module);
	}
}
obs_module_t* obs_current_module(void)
{
	return obs_module_pointer;
}
MODULE_EXPORT uint32_t obs_module_ver(void)
{
	return LIBOBS_API_VER;
}

MODULE_EXPORT bool obs_module_load(void)
{
	int version_major = 0;
	// obs_get_version is reporting 27.2.4 for the OBS 28 betas. We need to parse
	// the string version which gives the correct version
	std::string    version_str   = obs_get_version_string();
	size_t major_break = version_str.find_first_of('.');
	if (major_break != std::string::npos) {
		version_major = std::stoi(version_str.substr(0, major_break));
	}

	std::string data_path = obs_get_module_data_path(obs_current_module());
	if (version_major < 28) {
		data_path += "/StreamDeckPluginQt5";
	} else {
		data_path += "/StreamDeckPluginQt6";
	}
	dl_handle = os_dlopen(data_path.c_str());
	if (!dl_handle) {
		return false;
	}
	ind_obs_module_load = (bool(*)(void))(os_dlsym(dl_handle, "obs_module_load"));
	ind_obs_module_unload = (void(*)(void))(os_dlsym(dl_handle, "obs_module_unload"));
	ind_obs_module_set_pointer = (void (*)(obs_module_t*))(os_dlsym(dl_handle, "obs_module_set_pointer"));
	ind_obs_module_set_locale  = (void (*)(const char*))(os_dlsym(dl_handle, "obs_module_set_locale"));
	ind_obs_module_free_locale = (void (*)(void))(os_dlsym(dl_handle, "obs_module_free_locale"));
	ind_obs_module_post_load   = (void (*)(void))(os_dlsym(dl_handle, "obs_module_post_load"));
	ind_obs_module_get_string  = (bool (*)(const char*, const char**))(os_dlsym(dl_handle, "obs_module_get_string"));
	ind_obs_module_author      = (const char* (*)(void))(os_dlsym(dl_handle, "obs_module_author"));
	ind_obs_module_name        = (const char* (*)(void))(os_dlsym(dl_handle, "obs_module_name"));
	ind_obs_module_description = (const char* (*)(void))(os_dlsym(dl_handle, "obs_module_description"));

	// Ensure the module pointer and locale are set if they were set before the load was called
	if (obs_module_pointer) {
		obs_module_set_pointer(obs_module_pointer);
	}
	if (current_locale.size() > 0) {
		obs_module_set_locale(current_locale.c_str());
	}

	if (ind_obs_module_load) {
		return ind_obs_module_load();
	}
	return false;
}

MODULE_EXPORT void obs_module_unload(void)
{
	if (ind_obs_module_unload) {
		ind_obs_module_unload();
	}
	if (dl_handle) {
		os_dlclose(dl_handle);
	}
}

MODULE_EXPORT void obs_module_post_load(void)
{
	if (ind_obs_module_post_load) {
		ind_obs_module_post_load();
	}
}

MODULE_EXPORT void obs_module_set_locale(const char* locale)
{
	current_locale = locale;
	if (ind_obs_module_set_locale) {
		ind_obs_module_set_locale(locale);
	}
}

MODULE_EXPORT void obs_module_free_locale(void)
{
	current_locale = "";
	if (ind_obs_module_free_locale) {
		ind_obs_module_free_locale();
	}
}

MODULE_EXPORT bool obs_module_get_string(const char* lookup_string, const char** translated_string)
{
	if (ind_obs_module_get_string) {
		return ind_obs_module_get_string(lookup_string, translated_string);
	}
	return false;
}

MODULE_EXPORT const char* obs_module_author(void)
{
	if (ind_obs_module_author) {
		return ind_obs_module_author();
	}
	return "";
}

MODULE_EXPORT const char* obs_module_name(void)
{
	if (ind_obs_module_name) {
		return ind_obs_module_name();
	}
	return "";
}

MODULE_EXPORT const char* obs_module_description(void)
{
	if (ind_obs_module_description) {
		return ind_obs_module_description();
	}
	return "";
}
