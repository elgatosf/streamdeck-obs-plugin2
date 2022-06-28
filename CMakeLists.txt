# Copyright (C) 2017 - 2021 Michael Fabian 'Xaymar' Dirks <info@xaymar.com>
# Copyright (C) 2020 - 2021 Corsair GmbH
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

# CMake Setup
cmake_minimum_required(VERSION 3.8...4.0)

################################################################################
# Configure Type
################################################################################

# Detect if we are building by ourselves or as part of something else.
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_LIST_DIR}")
	set(GROUPED OFF)
	set(PREFIX "")
else()
	set(GROUPED ON)
	set(PREFIX "StreamDeck_")
endif()
set(LOGPREFIX "StreamDeck:")

################################################################################
# Versioning
################################################################################

set(VERSION_MAJOR 5)
set(VERSION_MINOR 0)
set(VERSION_PATCH 5)
set(VERSION_TWEAK 0)
set(VERSION_SUFFIX "")
set(VERSION_COMMIT "00000000")

# Check if we are in a git repository.
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/.git")
	# Try and figure out where git is.
	find_program(GIT git
		PATHS
			/bin
			/sbin
			/usr/bin
			/usr/local/bin
	)

	if(GIT)
		set(GIT_RESULT)
		set(GIT_OUTPUT)
		set(GIT_ERROR)

		execute_process(
			COMMAND "${GIT}" describe --tags --long --match "[0-9]*.[0-9]*.[0-9]*" --abbrev=8 HEAD
			WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
			RESULT_VARIABLE GIT_RESULT
			OUTPUT_VARIABLE GIT_OUTPUT
			ERROR_VARIABLE GIT_ERROR
			OUTPUT_STRIP_TRAILING_WHITESPACE
			ERROR_STRIP_TRAILING_WHITESPACE
			ERROR_QUIET
		)

		if(GIT_RESULT EQUAL 0)
			string(REPLACE "-" "." GIT_OUTPUT "${GIT_OUTPUT}")
			string(REPLACE "." ";" GIT_OUTPUT "${GIT_OUTPUT}")

			# Parse Version
			list(GET GIT_OUTPUT 0 VERSION_MAJOR)
			list(GET GIT_OUTPUT 1 VERSION_MINOR)
			list(GET GIT_OUTPUT 2 VERSION_PATCH)
			list(GET GIT_OUTPUT 3 VERSION_TWEAK)
			list(GET GIT_OUTPUT 4 VERSION_COMMIT)

			# Patch needs additional parsing.
			# This may be a [0-9]*[a-z]*[0-9]+ string.
			string(REGEX MATCHALL "^([0-9]+)([a-z]+[0-9]+)?" T_MATCHES "${VERSION_PATCH}")
			set(VERSION_PATCH "${CMAKE_MATCH_1}")
			if(CMAKE_MATCH_2)
				set(VERSION_SUFFIX "${CMAKE_MATCH_2}")
			else()
				set(VERSION_SUFFIX "")
			endif()
		else()
			message(WARNING "${LOGPREFIX} Failed to detect version, using default instead.")
		endif()
	endif()
else()
	message(STATUS "${LOGPREFIX} Not a git repository, automatic version detection disabled.")
endif()

# Allow manual overrides of the detected version.
set(${PREFIX}VERSION "" CACHE STRING "Override StreamDeck version with this string. Format: Major.Minor.Patch[Suffix][-Tweak[-Commit8c]]")
if(NOT (${PREFIX}VERSION STREQUAL ""))
	string(REPLACE "-" "." T_VERSION "${${PREFIX}VERSION}")
	string(REPLACE "." ";" T_VERSION "${${PREFIX}VERSION}")

	list(LENGTH T_VERSION T_VERSIONLEN)
	list(GET T_VERSION 0 VERSION_MAJOR)
	list(GET T_VERSION 1 VERSION_MINOR)
	list(GET T_VERSION 2 VERSION_PATCH)
	if (T_VERSIONLEN GREATER_EQUAL 3)
		list(GET T_VERSION 3 VERSION_TWEAK)
	else()
		set(VERSION_BUILD 0)
	endif()
	if (T_VERSIONLEN GREATER_EQUAL 4)
		list(GET T_VERSION 4 VERSION_COMMIT)
	else()
		set(VERSION_COMMIT "")
	endif()

	# Patch needs additional parsing.
	# This may be a [0-9]*[a-z]*[0-9]+ string.
	string(REGEX MATCHALL "^([0-9]+)([a-z]+[0-9]+)?" T_MATCHES "${VERSION_PATCH}")
	set(VERSION_PATCH "${CMAKE_MATCH_1}")
	if(CMAKE_MATCH_2)
		set(VERSION_SUFFIX "${CMAKE_MATCH_2}")
	else()
		set(VERSION_SUFFIX "")
	endif()
endif()

# Generate Version String
if(NOT (VERSION_COMMIT STREQUAL ""))
	set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}${VERSION_SUFFIX}-${VERSION_COMMIT}")
else()
	set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}${VERSION_SUFFIX}")
endif()

# Log the detected version.
message(STATUS "${LOGPREFIX} Version ${VERSION_STRING}")

################################################################################
# Project
################################################################################
project(
	StreamDeckPlugin
	VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}
	DESCRIPTION "Stream Deck integration for OBS Studio."
	HOMEPAGE_URL "https://elgato.com/"
)

# Full Project Name
set(PROJECT_FULL_NAME "Stream Deck (for OBS Studio)")

# Description
set(PROJECT_DESCRIPTION "Integration for Stream Deck into OBS Studio.")

# Authors (TODO: Generate this from AUTHORS)
set(PROJECT_AUTHORS "Michael Fabian 'Xaymar' Dirks <info@xaymar.com>")

# Copyright Years (TODO: Update every year)
set(PROJECT_COPYRIGHT_YEARS "2021")

# Versioning
set(PROJECT_VERSION_STRING ${VERSION_STRING})

################################################################################
# Modules
################################################################################

# Search Paths
set(CMAKE_MODULE_PATH
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules"
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake"
)

# Include
include("Architecture")					# Architecture Detection
include("util")							# CacheClear, CacheSet
include("DownloadProject")				# DownloadProject

################################################################################
# Platform Setup
################################################################################

# Operating System
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set(D_PLATFORM_OS "windows")
	set(D_PLATFORM_WINDOWS 1)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(D_PLATFORM_OS "linux")
	set(D_PLATFORM_LINUX 1)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	set(D_PLATFORM_OS "macos")
	set(D_PLATFORM_MAC 1)
else()
	set(D_PLATFORM_OS "unknown")
	set(D_PLATFORM_UNKNOWN 1)
	message(WARNING "${LOGPREFIX} The operating system '${CMAKE_SYSTEM_NAME}' is unknown to to this script, continue at your own risk.")
endif()

# Architecture
set(D_PLATFORM_INSTR ${ARCH_INST})
if(ARCH_INST STREQUAL "x86")
	set(D_PLATFORM_INSTR_X86 ON)
	set(D_PLATFORM_ARCH_X86 ON)
elseif(ARCH_INST STREQUAL "ARM")
	set(D_PLATFORM_INSTR_ARM ON)
	set(D_PLATFORM_ARCH_ARM ON)
elseif(ARCH_INST STREQUAL "IA64")
	set(D_PLATFORM_INSTR_ITANIUM ON)
	set(D_PLATFORM_ARCH_ITANIUM ON)
endif()
set(D_PLATFORM_ARCH ${ARCH_INST})

# Bitness
set(D_PLATFORM_BITS ${ARCH_BITS})
set(D_PLATFORM_BITS_PTR ${ARCH_BITS_POINTER})

################################################################################
# C/C++ Compiler Adjustments
################################################################################
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR (WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
	message(STATUS "Applying custom flags for MSVC style build.")

	# MSVC/ClangCL
	# - Dynamically link Microsoft C/C++ Redistributable.
	# - Enable /W3 and disable useless warnings.
	# - Enable C++ exceptions with SEH exceptions.
	# - Enable multi-processor compiling.

	# Build with dynamic MSVC linkage.
	add_compile_options(
		$<$<CONFIG:>:/MD>
		$<$<CONFIG:Debug>:/MDd>
		$<$<CONFIG:Release>:/MD>
		$<$<CONFIG:RelWithDebInfo>:/MD>
		$<$<CONFIG:MinSizeRel>:/MD>
	)

	# Enable most useful warnings.
	set(DISABLED_WARNINGS
		"/wd4061" "/wd4100" "/wd4180" "/wd4201" "/wd4464" "/wd4505" "/wd4514"
		"/wd4571" "/wd4623" "/wd4625" "/wd4626" "/wd4668" "/wd4710" "/wd4774"
		"/wd4820" "/wd5026" "/wd5027" "/wd5039" "/wd5045" "/wd26812"
	)
	add_compile_options("/W3")
	foreach(WARN ${DISABLED_WARNINGS})
		add_compile_options("${WARN}")
	endforeach()

	# C++ Exceptions & SEH
	add_compile_options("/EHa")

	# Multiprocessor compiling
	add_compile_options("/MP")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	message(STATUS "Applying custom flags for GNU style build.")

	# Clang/AppleClang/GNU
	# - Don't export by default. (Temporarily disabled)
	# - Enable all and extra warnings.

	add_compile_options("-Wall")
	add_compile_options("-Wextra")
	# add_compile_options("-fvisibility=hidden")
endif()

################################################################################
# Detect if we are building with OBS Studio (different from Grouped builds)
################################################################################

set(STANDALONE ON)
if(GROUPED AND (TARGET libobs))
	set(STANDALONE OFF)
endif()
if(STANDALONE)
	message(STATUS "${LOGPREFIX} This is a standalone build.")
	set(${PREFIX}OBS_NATIVE OFF)
else()
	message(STATUS "${LOGPREFIX} This is a combined build.")
	set(${PREFIX}OBS_NATIVE ON)
endif()

################################################################################
# Options
################################################################################

## Code Related
set(${PREFIX}ENABLE_CLANG ON CACHE BOOL "Enable Clang integration for supported compilers.")
set(${PREFIX}ENABLE_CODESIGN OFF CACHE BOOL "Enable Code Signing integration for supported environments.")

# Installation / Packaging
if(STANDALONE)
	set(STRUCTURE_UNIFIED CACHE BOOL "Install for use in a Plugin Manager")
	if(D_PLATFORM_LINUX)
		set(STRUCTURE_PACKAGEMANAGER CACHE BOOL "Install for use in a Package Manager (system-wide installation)")
	endif()

	set(PACKAGE_PREFIX "${CMAKE_BINARY_DIR}" CACHE PATH "Where to place the packages?")
	set(PACKAGE_NAME "${PROJECT_NAME}" CACHE STRING "What should the package be called?")
	set(PACKAGE_SUFFIX "" CACHE STRING "Any suffix for the package name? (Defaults to the current version string)")
endif()

################################################################################
# Clang
################################################################################

if(${PREFIX}ENABLE_CLANG AND (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/clang/Clang.cmake"))
	include("cmake/clang/Clang.cmake")
	set(HAVE_CLANG ON)
endif()

################################################################################
# Codesign
################################################################################

if(${PREFIX}ENABLE_CODESIGN AND (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/codesign/CodeSign.cmake"))
	include("cmake/codesign/CodeSign.cmake")
	set(HAVE_CODESIGN ON)

	set(${PREFIX}CODESIGN_FILE "" CACHE FILEPATH "Path to Code-Signing certificate.")
	if(WIN32)
		set(${PREFIX}CODESIGN_NAME "" CACHE STRING "Name of Code-Signing certificate in Windows's certificate storage.")
	endif()
	set(${PREFIX}CODESIGN_PASS "" CACHE STRING "Password for Code-Signing certificate.")
	set(${PREFIX}CODESIGN_TIMESTAMPS ON CACHE BOOL "Should the resulting binary be timestamped?")

	set(_CODESIGN_FILE "$ENV{${PREFIX}CODESIGN_FILE}")
	if(${PREFIX}CODESIGN_FILE)
		set(_CODESIGN_FILE "${${PREFIX}CODESIGN_FILE}")
	endif()
	set(_CODESIGN_NAME "$ENV{${PREFIX}CODESIGN_NAME}")
	if(${PREFIX}CODESIGN_NAME)
		set(_CODESIGN_NAME "${${PREFIX}CODESIGN_NAME}")
	endif()
	set(_CODESIGN_PASS "$ENV{${PREFIX}CODESIGN_PASS}")
	if(${PREFIX}CODESIGN_PASS)
		set(_CODESIGN_PASS "${${PREFIX}CODESIGN_PASS}")
	endif()
endif()


################################################################################
# Standalone Build: OBS Studio
################################################################################

if(NOT ${PREFIX}OBS_NATIVE)
	# Options
	set(${PREFIX}DOWNLOAD_OBS_URL "" CACHE STRING "(Optional) URL of prebuilt libOBS archive to download.")
	set(${PREFIX}DOWNLOAD_OBS_HASH "" CACHE STRING "(Optional) The hash for the libOBS archive.")
	mark_as_advanced(
		${PREFIX}DOWNLOAD_OBS_URL
		${PREFIX}DOWNLOAD_OBS_HASH
	)

	# Allow overriding what version we build against.
	if(${PREFIX}DOWNLOAD_OBS_URL)
		set(_DOWNLOAD_OBS_URL "${${PREFIX}DOWNLOAD_OBS_URL}")
		set(_DOWNLOAD_OBS_HASH "${${PREFIX}DOWNLOAD_OBS_HASH}")
	else()
		set(_DOWNLOAD_OBS_VERSION "27.0.0-ci")
		if (D_PLATFORM_WINDOWS)
			if (D_PLATFORM_ARCH_X86)
				set(_DOWNLOAD_OBS_URL "https://github.com/Xaymar/obs-studio/releases/download/${_DOWNLOAD_OBS_VERSION}/obs-studio-x64-0.0.0.0-windows-${D_PLATFORM_ARCH}-${D_PLATFORM_BITS}.7z")
				if (D_PLATFORM_BITS EQUAL 64)
					set(_DOWNLOAD_OBS_HASH "SHA256=EBF9853C8A553E16ECBCA22523F401E6CF1EB2E8DA93F1493FEF41D65BD06633")
				else()
					message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
					return()
				endif()
			else()
				message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
				return()
			endif()
		elseif(D_PLATFORM_LINUX)
			if (D_PLATFORM_ARCH_X86)
				set(_DOWNLOAD_OBS_URL "https://github.com/Xaymar/obs-studio/releases/download/${_DOWNLOAD_OBS_VERSION}/obs-studio-x64-0.0.0.0-ubuntu-${D_PLATFORM_ARCH}-${D_PLATFORM_BITS}.7z")
				if (D_PLATFORM_BITS EQUAL 64)
					set(_DOWNLOAD_OBS_HASH "SHA256=0AF6C7262C37D80C24CB18523A851FD765C04E766D8EB0F4AC0F6E75D13A035F")
				else()
					message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
					return()
				endif()
			else()
				message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
				return()
			endif()
		elseif(D_PLATFORM_MAC)
			if (D_PLATFORM_ARCH_X86)
				set(_DOWNLOAD_OBS_URL "https://github.com/Xaymar/obs-studio/releases/download/${_DOWNLOAD_OBS_VERSION}/obs-studio-x64-0.0.0.0-macos-${D_PLATFORM_ARCH}-${D_PLATFORM_BITS}.7z")
				if (D_PLATFORM_BITS EQUAL 64)
					set(_DOWNLOAD_OBS_HASH "SHA256=F15BC4CA8EB3F581A94372759CFE554E30D202B604B541445A5756B878E4E799")
				else()
					message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
					return()
				endif()
			else()
				message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
				return()
			endif()
		else()
			message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
			return()
		endif()
	endif()

	# Download libOBS
	download_project(
		PROJ libobs
		URL "${_DOWNLOAD_OBS_URL}"
		URL_HASH "${_DOWNLOAD_OBS_HASH}"
		DOWNLOAD_NO_PROGRESS OFF
		UPDATE_DISCONNECTED OFF
	)

	include("${libobs_SOURCE_DIR}/cmake/LibObs/LibObsConfig.cmake")
endif()

if(${PREFIX}OBS_NATIVE)
	if(TARGET obs-frontend-api)
		set(HAVE_OBSFE ON)
	endif()
else()
	if (EXISTS "${libobs_SOURCE_DIR}/cmake/obs-frontend-api/obs-frontend-apiConfig.cmake")
		include("${libobs_SOURCE_DIR}/cmake/obs-frontend-api/obs-frontend-apiConfig.cmake")
		set(HAVE_OBSFE ON)
	endif()
endif()

################################################################################
# Standalone Build: OBS Studio Dependencies
################################################################################

if(STANDALONE AND NOT D_PLATFORM_LINUX)
	# Options
	set(${PREFIX}DOWNLOAD_OBSDEPS_URL "" CACHE STRING "(Optional) URL of prebuilt libOBS archive to download.")
	set(${PREFIX}DOWNLOAD_OBSDEPS_HASH "" CACHE STRING "(Optional) The hash for the libOBS archive.")
	mark_as_advanced(
		${PREFIX}DOWNLOAD_OBSDEPS_URL
		${PREFIX}DOWNLOAD_OBSDEPS_HASH
	)

	# Allow overriding what version we build against.
	if(${PREFIX}DOWNLOAD_OBSDEPS_URL)
		set(_DOWNLOAD_OBSDEPS_URL "${${PREFIX}DOWNLOAD_OBSDEPS_URL}")
		set(_DOWNLOAD_OBSDEPS_HASH "${${PREFIX}DOWNLOAD_OBSDEPS_HASH}")
	else()
		if (D_PLATFORM_WINDOWS)
			if (D_PLATFORM_ARCH_X86)
				set(_DOWNLOAD_OBSDEPS_URL "https://github.com/Xaymar/obs-studio/releases/download/27.0.0/deps-windows-x86.7z")
				if (D_PLATFORM_BITS EQUAL 64)
					set(_DOWNLOAD_OBSDEPS_HASH "SHA256=B4AED165016F0B64A7E8B256CCC12EAF8AF087F61B0B239B9D3D00277485B5B5")
				elseif (D_PLATFORM_BITS EQUAL 32)
					set(_DOWNLOAD_OBSDEPS_HASH "SHA256=B4AED165016F0B64A7E8B256CCC12EAF8AF087F61B0B239B9D3D00277485B5B5")
				else()
					message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
					return()
				endif()
			else()
				message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
				return()
			endif()
		elseif(D_PLATFORM_MAC)
			if (D_PLATFORM_ARCH_X86)
				if (D_PLATFORM_BITS EQUAL 64)
					set(_DOWNLOAD_OBSDEPS_URL "https://github.com/Xaymar/obs-studio/releases/download/27.0.0/deps-macos-x86_64-2021-03-25.tar.gz")
					set(_DOWNLOAD_OBSDEPS_HASH "SHA256=1C409374BCAB9D5CEEAFC121AA327E13AB222096718AF62F2648302DF62898D6")
				else()
					message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
					return()
				endif()
			elseif(D_PLATFORM_ARCH_ARM)
				if (D_PLATFORM_BITS EQUAL 64)
					set(_DOWNLOAD_OBSDEPS_URL "https://github.com/Xaymar/obs-studio/releases/download/27.0.0/deps-macos-arm64-2021-03-25.tar.gz")
					set(_DOWNLOAD_OBSDEPS_HASH "SHA256=C0EC57D360AF190E372D6BB883134FA26B1A7E49840DD146B172B48D548B55BC")
				else()
					message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
					return()
				endif()
			else()
				message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
				return()
			endif()
		else()
			message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
			return()
		endif()
	endif()

	# Download libOBS
	download_project(
		PROJ obsdeps
		URL "${_DOWNLOAD_OBSDEPS_URL}"
		URL_HASH "${_DOWNLOAD_OBSDEPS_HASH}"
		DOWNLOAD_NO_PROGRESS OFF
		UPDATE_DISCONNECTED OFF
	)

	if (D_PLATFORM_WINDOWS)
		set(_OBSDEPS_PATH "${obsdeps_SOURCE_DIR}/win${D_PLATFORM_BITS}")
	elseif(D_PLATFORM_MAC)
		set(_OBSDEPS_PATH "${obsdeps_SOURCE_DIR}/obsdeps")
	endif()
endif()

################################################################################
# Standalone Build: Qt v5.x
################################################################################

if(STANDALONE AND NOT D_PLATFORM_LINUX)
	set(${PREFIX}DOWNLOAD_QT OFF CACHE BOOL "Download Qt?")

	if(${PREFIX}DOWNLOAD_QT)
		set(${PREFIX}DOWNLOAD_QT_URL "" CACHE STRING "")
		set(${PREFIX}DOWNLOAD_QT_HASH "" CACHE STRING "")
		mark_as_advanced(
			${PREFIX}DOWNLOAD_QT_URL
			${PREFIX}DOWNLOAD_QT_HASH
		)

		# Allow overriding what version we build against.
		if(${PREFIX}DOWNLOAD_QT_URL)
			set(_DOWNLOAD_QT_URL "${${PREFIX}DOWNLOAD_QT_URL}")
			set(_DOWNLOAD_QT_HASH "${${PREFIX}DOWNLOAD_QT_HASH}")
		else()
			if (D_PLATFORM_WINDOWS)
				if (D_PLATFORM_ARCH_X86)
					if (D_PLATFORM_BITS EQUAL 64)
						set(_DOWNLOAD_QT_URL "https://github.com/Xaymar/obs-studio/releases/download/27.0.0/qt-5.15.2-windows-x86-64.7z")
						set(_DOWNLOAD_QT_HASH "SHA256=109B9C21EF165B0C46DFAA9AD23124F2070ED4D74207C4AFB308183CB8D43BDD")
					elseif (D_PLATFORM_BITS EQUAL 32)
						set(_DOWNLOAD_QT_URL "https://github.com/Xaymar/obs-studio/releases/download/27.0.0/qt-5.15.2-windows-x86-32.7z")
						set(_DOWNLOAD_QT_HASH "SHA256=372E4FBF2A15DD4FDA955A07334D8B8AC6802990148C9CB4E766C90205F8F570")
					else()
						message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
						return()
					endif()
				else()
					message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
					return()
				endif()
			elseif(D_PLATFORM_MAC)
				if (D_PLATFORM_ARCH_X86)
					if (D_PLATFORM_BITS EQUAL 64)
						set(_DOWNLOAD_QT_URL "https://github.com/Xaymar/obs-studio/releases/download/27.0.0/qt-5.15.2-macos-x86_64-2021-03-25.tar.gz")
						set(_DOWNLOAD_QT_HASH "SHA256=FFABB54624B931EA3FCC06BED244895F50CEFC95DE09D792D280C46D4F91D4C5")
					else()
						message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
						return()
					endif()
				elseif(D_PLATFORM_ARCH_ARM)
					if (D_PLATFORM_BITS EQUAL 64)
						set(_DOWNLOAD_QT_URL "https://github.com/Xaymar/obs-studio/releases/download/27.0.0/qt-5.15.2-macos-arm64-2021-03-25.tar.gz")
						set(_DOWNLOAD_QT_HASH "SHA256=366BA8AC0FA0CAC440AFB9ED1C2EF5932E50091DC43BDE8B5C4B490082B6F19F")
					else()
						message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
						return()
					endif()
				else()
					message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
					return()
				endif()
			else()
				message(FATAL_ERROR "${LOGPREFIX} Platform '${D_PLATFORM_OS}' with architecture '${D_PLATFORM_ARCH}' and bitness '${D_PLATFORM_BITS}' is not supported.")
				return()
			endif()
		endif()

		download_project(
			PROJ qt
			URL "${_DOWNLOAD_QT_URL}"
			URL_HASH "${_DOWNLOAD_QT_HASH}"
			DOWNLOAD_NO_PROGRESS OFF
			UPDATE_DISCONNECTED OFF
		)

		set(Qt5_DIR "${qt_SOURCE_DIR}" CACHE STRING "Path to Qt5")
		if (D_PLATFORM_WINDOWS)
			if (D_PLATFORM_ARCH_X86)
				if (D_PLATFORM_BITS EQUAL 64)
					CacheSet(Qt5_DIR "${qt_SOURCE_DIR}/lib/cmake/Qt5")
				elseif (D_PLATFORM_BITS EQUAL 32)
					CacheSet(Qt5_DIR "${qt_SOURCE_DIR}/lib/cmake/Qt5")
				endif()
			endif()
		elseif(D_PLATFORM_MAC)
			CacheSet(Qt5_DIR "${qt_SOURCE_DIR}/lib/cmake/Qt5")
		endif()
	endif()
endif()

if(NOT Qt5_DIR)
	if(ENV{Qt5_DIR})
		set(Qt5_DIR "$ENV{Qt5_DIR}")
	endif()
endif()
find_package(Qt5
	REQUIRED COMPONENTS Core Widgets
	CONFIG
	HINTS
		"${Qt5_DIR}"
		"${Qt5_DIR}/lib"
		"${Qt5_DIR}/lib/cmake"
		"${Qt5_DIR}/lib/cmake/Qt5"
)
set(HAVE_QT ${Qt5_FOUND})

################################################################################
# Code
################################################################################
set(PROJECT_DATA_LOCALE )
set(PROJECT_DATA_EFFECTS )
set(PROJECT_DATA_SHADERS )
set(PROJECT_LIBRARIES )
set(PROJECT_LIBRARIES_DELAYED )
set(PROJECT_INCLUDE_DIRS )
set(PROJECT_TEMPLATES )
set(PROJECT_PRIVATE_GENERATED )
set(PROJECT_PRIVATE_SOURCE )
set(PROJECT_UI )
set(PROJECT_UI_SOURCE )
set(PROJECT_DEFINITIONS )

# Configure Files
configure_file(
	"templates/config.hpp.in"
	"generated/config.hpp"
)
LIST(APPEND PROJECT_TEMPLATES "templates/config.hpp.in")
LIST(APPEND PROJECT_PRIVATE_GENERATED "${PROJECT_BINARY_DIR}/generated/config.hpp")

configure_file(
	"templates/version.hpp.in"
	"generated/version.hpp"
)
LIST(APPEND PROJECT_TEMPLATES "templates/version.hpp.in")
LIST(APPEND PROJECT_PRIVATE_GENERATED "${PROJECT_BINARY_DIR}/generated/version.hpp")

configure_file(
	"templates/module.cpp.in"
	"generated/module.cpp"
)
LIST(APPEND PROJECT_TEMPLATES "templates/module.cpp.in")
LIST(APPEND PROJECT_PRIVATE_GENERATED "${PROJECT_BINARY_DIR}/generated/module.cpp")

if(D_PLATFORM_WINDOWS) # Windows Support
	set(PROJECT_PRODUCT_NAME "${PROJECT_FULL_NAME}")
	set(PROJECT_COMPANY_NAME "${PROJECT_AUTHORS}")
	set(PROJECT_COPYRIGHT "${PROJECT_AUTHORS} © ${PROJECT_COPYRIGHT_YEARS}")
	set(PROJECT_LEGAL_TRADEMARKS_1 "")
	set(PROJECT_LEGAL_TRADEMARKS_2 "")

	configure_file(
		"templates/version.rc.in"
		"generated/version.rc"
		@ONLY
	)
	LIST(APPEND PROJECT_TEMPLATES "templates/version.rc.in")
	LIST(APPEND PROJECT_PRIVATE_GENERATED "${PROJECT_BINARY_DIR}/generated/version.rc")
endif()

# Minimum Dependencies
list(APPEND PROJECT_LIBRARIES
	libobs
	obs-frontend-api
)

# Qt5
list(APPEND PROJECT_LIBRARIES
	Qt5::Core
	Qt5::Widgets
)

# Project itself
list(APPEND PROJECT_PRIVATE_SOURCE
	"source/module.hpp"
	"source/module.cpp"
	"source/json-rpc.hpp"
	"source/json-rpc.cpp"
	"source/server.hpp"
	"source/server.cpp"
	"source/handlers/handler-system.hpp"
	"source/handlers/handler-system.cpp"
	"source/handlers/handler-obs-frontend.hpp"
	"source/handlers/handler-obs-frontend.cpp"
	"source/handlers/handler-obs-source.hpp"
	"source/handlers/handler-obs-source.cpp"
	"source/handlers/handler-obs-scene.hpp"
	"source/handlers/handler-obs-scene.cpp"
)
list(APPEND PROJECT_INCLUDE_DIRS
	"${PROJECT_BINARY_DIR}/generated"
	"${PROJECT_SOURCE_DIR}/source"
	"third-party/nlohmann-json/single_include/"
	"third-party/websocketpp/"
	"third-party/asio/asio/include"
)
list(APPEND PROJECT_UI
	"ui/popup.ui"
	"ui/popup.qrc"
)
list(APPEND PROJECT_UI_SOURCE
	"source/details-popup.cpp"
	"source/details-popup.hpp"
)
list(APPEND PROJECT_DEFINITIONS
	-DASIO_STANDALONE
	-D_WEBSOCKETPP_CPP11_STL_
	-DVERSION_STR=\"${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}\"
)

# Windows
if(D_PLATFORM_WINDOWS)
	list(APPEND PROJECT_PRIVATE_SOURCE
		"source/windll.cpp"
	)
	list(APPEND PROJECT_LIBRARIES
		Delayimp.lib
	)
	# Disable/Enable a ton of things.
	list(APPEND PROJECT_DEFINITIONS
		# Microsoft Visual C++
		_CRT_SECURE_NO_WARNINGS
		_ENABLE_EXTENDED_ALIGNED_STORAGE
		# windows.h
		WIN32_LEAN_AND_MEAN
		NOGPICAPMASKS
		NOVIRTUALKEYCODES
		#NOWINMESSAGES
		NOWINSTYLES
		NOSYSMETRICS
		NOMENUS
		NOICONS
		NOKEYSTATES
		NOSYSCOMMANDS
		NORASTEROPS
		NOSHOWWINDOW
		NOATOM
		NOCLIPBOARD
		NOCOLOR
		#NOCTLMGR
		NODRAWTEXT
		#NOGDI
		NOKERNEL
		#NOUSER
		#NONLS
		NOMB
		NOMEMMGR
		NOMETAFILE
		NOMINMAX
		#NOMSG
		NOOPENFILE
		NOSCROLL
		NOSERVICE
		NOSOUND
		#NOTEXTMETRIC
		NOWH
		NOWINOFFSETS
		NOCOMM
		NOKANJI
		#NOHELP
		NOPROFILER
		NODEFERWINDOWPOS
		NOMCX
		NOIME
		NOMDI
		NOINOUT
	)
endif()

# GCC before 9.0, Clang before 9.0
if((CMAKE_C_COMPILER_ID STREQUAL "GNU")
	OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	OR (CMAKE_C_COMPILER_ID STREQUAL "Clang")
	OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
		list(APPEND PROJECT_LIBRARIES
			"stdc++fs"
		)
	endif()
endif()

################################################################################
# Register Library
################################################################################

# Combine all variables that matter.
set(PROJECT_FILES
	# Always exist
	${PROJECT_DATA}
	${PROJECT_PRIVATE_GENERATED}
	${PROJECT_PRIVATE_SOURCE}
	${PROJECT_TEMPLATES}
	# UI-only (empty if not enabled)
	${PROJECT_UI}
	${PROJECT_UI_SOURCE}
)

# Set source groups for IDE generators.
source_group(TREE "${PROJECT_SOURCE_DIR}/data" PREFIX "Data" FILES ${PROJECT_DATA})
source_group(TREE "${PROJECT_SOURCE_DIR}/source" PREFIX "Source" FILES ${PROJECT_PRIVATE_SOURCE} ${PROJECT_UI_SOURCE})
source_group(TREE "${PROJECT_BINARY_DIR}/generated" PREFIX "Source" FILES ${PROJECT_PRIVATE_GENERATED})
source_group(TREE "${PROJECT_SOURCE_DIR}/templates" PREFIX "Templates" FILES ${PROJECT_TEMPLATES})
source_group(TREE "${PROJECT_SOURCE_DIR}/ui" PREFIX "User Interface" FILES ${PROJECT_UI})

# Prevent unwanted files from being built as source.
set_source_files_properties(${PROJECT_DATA} ${PROJECT_TEMPLATES} ${PROJECT_UI} PROPERTIES
	HEADER_FILE_ONLY ON
)

# Prevent non-UI files from being Qt'd
if(HAVE_QT)
	set_source_files_properties(${PROJECT_DATA} ${PROJECT_TEMPLATES} ${PROJECT_PRIVATE_GENERATED} ${PROJECT_PRIVATE_SOURCE} PROPERTIES
		SKIP_AUTOGEN ON
		SKIP_AUTOMOC ON
		SKIP_AUTORCC ON
		SKIP_AUTOUIC ON
	)
endif()

# Register the library
add_library(${PROJECT_NAME} MODULE ${PROJECT_FILES}) # We are a module for libOBS.
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_INCLUDE_DIRS})
target_compile_definitions(${PROJECT_NAME} PRIVATE ${PROJECT_DEFINITIONS})
target_link_libraries(${PROJECT_NAME} ${PROJECT_LIBRARIES})

# Set C++ Standard and Extensions
set_target_properties(${PROJECT_NAME} PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	CXX_EXTENSIONS OFF
)

# Remove prefix on other platforms.
set_target_properties(${PROJECT_NAME} PROPERTIES
	PREFIX ""
	IMPORT_PREFIX ""
)

# Set file version (on anything but MacOS)
if(NOT D_PLATFORM_MAC)
	set_target_properties(${PROJECT_NAME} PROPERTIES
		VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK}
		SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK}
	)
endif()

# Delay-Loading on Microsoft Visual C++
if(D_PLATFORM_WINDOWS)
	foreach(DELAYLOAD ${PROJECT_LIBRARIES_DELAYED})
		get_target_property(_lf ${PROJECT_NAME} LINK_FLAGS)
		if (NOT _lf)
			set(_lf "")
		endif()
		set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "${_lf} /DELAYLOAD:${DELAYLOAD}")
		add_link_options("/DELAYLOAD:${DELAYLOAD}")
	endforeach()
endif()

# Enable Qt if needed
if(HAVE_QT)
	set_target_properties(${PROJECT_NAME} PROPERTIES
		AUTOUIC ON
		AUTOUIC_SEARCH_PATHS "${PROJECT_SOURCE_DIR};${PROJECT_SOURCE_DIR}/ui"
		AUTOMOC ON
		AUTORCC ON
		AUTOGEN_BUILD_DIR "${PROJECT_BINARY_DIR}/generated"
	)
endif()

# MacOS: Disable automatic Code Signing in Xcode
if(D_PLATFORM_MAC)
	set_target_properties(${PROJECT_NAME} PROPERTIES
		XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
		XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
	)
endif()


################################################################################
# Extra Tools
################################################################################

# Clang
if(HAVE_CLANG)
	generate_compile_commands_json(
		TARGETS ${PROJECT_NAME}
	)
	clang_tidy(
		TARGETS ${PROJECT_NAME}
		VERSION 9.0.0
	)
	clang_format(
		TARGETS ${PROJECT_NAME}
		DEPENDENCY
		VERSION 9.0.0
	)
endif()

# Apple otool
if(D_PLATFORM_MAC)
	# OBS
	mac_get_linker_id(TARGET libobs OUTPUT T_OBS_LINK)
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND install_name_tool ARGS -change "${T_OBS_LINK}" "@executable_path/../Frameworks/libobs.0.dylib" $<TARGET_FILE:${PROJECT_NAME}>
	)
	message(STATUS "${LOGPREFIX} Added post-build step for adjusting libobs linking path.")

	# OBS Front-End API
	if (TRUE)
		mac_get_linker_id(TARGET obs-frontend-api OUTPUT T_OBSFE_LINK)
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND install_name_tool ARGS -change "${T_OBSFE_LINK}" "@executable_path/../Frameworks/libobs-frontend-api.dylib" $<TARGET_FILE:${PROJECT_NAME}>
		)
		message(STATUS "${LOGPREFIX} Added post-build step for adjusting libobs-frontend-api linking path.")
	endif()

	# Qt5
	if (TRUE)
		# Figure out the linker location for Qt5::Core
		mac_get_linker_id(TARGET Qt5::Core OUTPUT T_QT5CORE_LINK)

		# Figure out the linker location for Qt5::Gui
		mac_get_linker_id(TARGET Qt5::Gui OUTPUT T_QT5GUI_LINK)

		# Figure out the linker location for Qt5::Widsgets
		mac_get_linker_id(TARGET Qt5::Widgets OUTPUT T_QT5WIDGETS_LINK)

		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		# - QtCore
			COMMAND install_name_tool ARGS -change "${T_QT5CORE_LINK}" "@executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore" $<TARGET_FILE:${PROJECT_NAME}>
		# - QtGui
			COMMAND install_name_tool ARGS -change "${T_QT5GUI_LINK}" "@executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui" $<TARGET_FILE:${PROJECT_NAME}>
		# - QtWidgets
			COMMAND install_name_tool ARGS -change "${T_QT5WIDGETS_LINK}" "@executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets" $<TARGET_FILE:${PROJECT_NAME}>
		)
		message(STATUS "${LOGPREFIX} Added post-build step for adjusting Qt5::Core linking path (Found: ${Qt5_DIR} resolved to ${T_QT5CORE_LINK}).")
		message(STATUS "${LOGPREFIX} Added post-build step for adjusting Qt5::Gui linking path (Found: ${Qt5_DIR} resolved to ${T_QT5GUI_LINK}).")
		message(STATUS "${LOGPREFIX} Added post-build step for adjusting Qt5::Widgets linking path (Found: ${Qt5_DIR} resolved to ${T_QT5WIDGETS_LINK}).")
	endif()
endif()

# Code Sign
if(HAVE_CODESIGN)
	set(_CODESIGN_TIMESTAMP "")
	if(${PREFIX}CODESIGN_TIMESTAMPS)
		set(_CODESIGN_TIMESTAMP "TIMESTAMPS")
	endif()
	codesign(
		TARGETS ${PROJECT_NAME}
		CERTIFICATE_FILE "${_CODESIGN_FILE}"
		CERTIFICATE_NAME "${_CODESIGN_NAME}"
		CERTIFICATE_PASS "${_CODESIGN_PASS}"
		${_CODESIGN_TIMESTAMP}
	)
endif()

################################################################################
# Installation
################################################################################

if(${PREFIX}OBS_NATIVE)
	# Grouped builds don't offer standalone services.
	install_obs_plugin_with_data(${PROJECT_NAME} data)
else()
	if(STRUCTURE_UNIFIED)
		install(
			DIRECTORY "data/"
			DESTINATION "data/"
			FILE_PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
			DIRECTORY_PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
		)

		if(D_PLATFORM_WINDOWS)
			install(
				TARGETS ${PROJECT_NAME}
				RUNTIME DESTINATION "bin/windows-${D_PLATFORM_INSTR}-${D_PLATFORM_BITS}/" COMPONENT StreamDeck
				LIBRARY DESTINATION "bin/windows-${D_PLATFORM_INSTR}-${D_PLATFORM_BITS}/" COMPONENT StreamDeck
				PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
			)
			if(MSVC)
				install(
					FILES $<TARGET_PDB_FILE:${PROJECT_NAME}>
					DESTINATION "bin/windows-${D_PLATFORM_INSTR}-${D_PLATFORM_BITS}/"
					COMPONENT StreamDeck
					OPTIONAL
				)
			endif()
		elseif(D_PLATFORM_LINUX)
			install(
				TARGETS ${PROJECT_NAME}
				RUNTIME DESTINATION "bin/linux-${D_PLATFORM_INSTR}-${D_PLATFORM_BITS}/" COMPONENT StreamDeck
				LIBRARY DESTINATION "bin/linux-${D_PLATFORM_INSTR}-${D_PLATFORM_BITS}/" COMPONENT StreamDeck
				PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
			)
		elseif(D_PLATFORM_MAC)
			install(
				TARGETS ${PROJECT_NAME}
				RUNTIME DESTINATION "bin/mac-${D_PLATFORM_INSTR}-${D_PLATFORM_BITS}/" COMPONENT StreamDeck
				LIBRARY DESTINATION "bin/mac-${D_PLATFORM_INSTR}-${D_PLATFORM_BITS}/" COMPONENT StreamDeck
				PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
			)
		endif()

		install(
			FILES LICENSE
			DESTINATION "LICENSE"
			COMPONENT StreamDeck
		)
		install(
			FILES icon.png
			DESTINATION "icon.png"
			COMPONENT StreamDeck
		)
	elseif(D_PLATFORM_WINDOWS)
		install(
			TARGETS ${PROJECT_NAME}
			RUNTIME DESTINATION "obs-plugins/${D_PLATFORM_BITS}bit/" COMPONENT StreamDeck
			LIBRARY DESTINATION "obs-plugins/${D_PLATFORM_BITS}bit/" COMPONENT StreamDeck
		)
		install(
			DIRECTORY "data/"
			DESTINATION "data/obs-plugins/${PROJECT_NAME}/"
		)
		if(MSVC)
			install(
				FILES $<TARGET_PDB_FILE:${PROJECT_NAME}>
				DESTINATION "obs-plugins/${D_PLATFORM_BITS}bit/"
				OPTIONAL
			)
		endif()
	elseif(D_PLATFORM_LINUX)
		if(STRUCTURE_PACKAGEMANAGER)
			install(
				TARGETS ${PROJECT_NAME}
				RUNTIME DESTINATION "lib/obs-plugins/" COMPONENT StreamDeck
				LIBRARY DESTINATION "lib/obs-plugins/" COMPONENT StreamDeck
				PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
			)
			install(
				DIRECTORY "data/"
				DESTINATION "share/obs/obs-plugins/${PROJECT_NAME}"
				COMPONENT StreamDeck
				FILE_PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
				DIRECTORY_PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
			)
		else()
			install(
				TARGETS ${PROJECT_NAME}
				RUNTIME DESTINATION "plugins/${PROJECT_NAME}/bin/${D_PLATFORM_BITS}bit/" COMPONENT StreamDeck
				LIBRARY DESTINATION "plugins/${PROJECT_NAME}/bin/${D_PLATFORM_BITS}bit/" COMPONENT StreamDeck
				PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
			)
			install(
				DIRECTORY "data/"
				DESTINATION "plugins/${PROJECT_NAME}/data/"
				COMPONENT StreamDeck
				FILE_PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
				DIRECTORY_PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
			)
		endif()
	elseif(D_PLATFORM_MAC)
		install(
			TARGETS ${PROJECT_NAME}
			RUNTIME DESTINATION "${PROJECT_NAME}/bin/" COMPONENT StreamDeck
			LIBRARY DESTINATION "${PROJECT_NAME}/bin/" COMPONENT StreamDeck
			PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
		)
		install(
			DIRECTORY "data/"
			DESTINATION "${PROJECT_NAME}/data/"
			COMPONENT StreamDeck
			FILE_PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
			DIRECTORY_PERMISSIONS WORLD_EXECUTE;WORLD_READ;OWNER_EXECUTE;OWNER_READ;OWNER_WRITE;GROUP_EXECUTE;GROUP_READ;GROUP_WRITE
		)
	endif()
endif()

################################################################################
# Packaging
################################################################################

if(NOT ${PREFIX}OBS_NATIVE)
	# Packaging
	if(NOT PACKAGE_SUFFIX)
		set(_PACKAGE_SUFFIX_OVERRIDE "${VERSION_STRING}")
	else()
		set(_PACKAGE_SUFFIX_OVERRIDE "${PACKAGE_SUFFIX}")
	endif()
	set(_PACKAGE_FULL_NAME "${PACKAGE_PREFIX}/${PACKAGE_NAME}-${_PACKAGE_SUFFIX_OVERRIDE}")

	if(STRUCTURE_UNIFIED)
		add_custom_target(
			PACKAGE_ZIP
			${CMAKE_COMMAND} -E tar cfv "${_PACKAGE_FULL_NAME}.obs" --format=zip --
				"."
			WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}"
		)
	else()
		add_custom_target(
			PACKAGE_7Z
			${CMAKE_COMMAND} -E tar cfv "${_PACKAGE_FULL_NAME}.7z" --format=7zip --
				"."
			WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}"
		)
		add_custom_target(
			PACKAGE_ZIP
			${CMAKE_COMMAND} -E tar cfv "${_PACKAGE_FULL_NAME}.zip" --format=zip --
				"."
			WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}"
		)

		# Windows
		if(D_PLATFORM_WINDOWS)
			## Installer (InnoSetup)
			get_filename_component(ISS_FILES_DIR "${CMAKE_INSTALL_PREFIX}" ABSOLUTE)
			file(TO_NATIVE_PATH "${ISS_FILES_DIR}" ISS_FILES_DIR)

			get_filename_component(ISS_PACKAGE_DIR "${PACKAGE_PREFIX}" ABSOLUTE)
			file(TO_NATIVE_PATH "${ISS_PACKAGE_DIR}" ISS_PACKAGE_DIR)

			get_filename_component(ISS_SOURCE_DIR "${PROJECT_SOURCE_DIR}" ABSOLUTE)
			file(TO_NATIVE_PATH "${ISS_SOURCE_DIR}" ISS_SOURCE_DIR)

			get_filename_component(ISS_MSVCHELPER_PATH "${msvc-redist-helper_BUILD_DIR}" ABSOLUTE)
			file(TO_NATIVE_PATH "${ISS_MSVCHELPER_PATH}" ISS_MSVCHELPER_PATH)

			if(HAVE_CODESIGN)
				codesign_command_win32(
					SHA1
					RETURN_BIN ISS_CODESIGN_BIN_SHA1
					RETURN_ARGS ISS_CODESIGN_CMD_SHA1
					CERTIFICATE_FILE "${_CODESIGN_FILE}"
					CERTIFICATE_NAME "${_CODESIGN_NAME}"
					CERTIFICATE_PASS "${_CODESIGN_PASS}"
					${_CODESIGN_TIMESTAMP}
				)
				codesign_command_win32(
					SHA2 APPEND
					RETURN_BIN ISS_CODESIGN_BIN_SHA2
					RETURN_ARGS ISS_CODESIGN_CMD_SHA2
					CERTIFICATE_FILE "${_CODESIGN_FILE}"
					CERTIFICATE_NAME "${_CODESIGN_NAME}"
					CERTIFICATE_PASS "${_CODESIGN_PASS}"
					${_CODESIGN_TIMESTAMP}
				)
				list(JOIN ISS_CODESIGN_CMD_SHA1 " " ISS_CODESIGN_CMD_SHA1)
				list(JOIN ISS_CODESIGN_CMD_SHA2 " " ISS_CODESIGN_CMD_SHA2)

				configure_file(
					"templates/installer-signed.iss.in"
					"installer.iss"
				)
			else()
				configure_file(
					"templates/installer.iss.in"
					"installer.iss"
				)
			endif()
		endif()
	endif()
endif()

