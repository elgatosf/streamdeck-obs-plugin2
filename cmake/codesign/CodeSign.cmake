# Copyright (C) 2017 - 2021 Michael Fabian Dirks
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
# Options
################################################################################

set(CODESIGN_PATH "" CACHE PATH "Path to code signing tool (if not in environment).")
set(CODESIGN_ARGS "" CACHE STRING "Additional Arguments to pass to tool.")

################################################################################
# Functions
################################################################################
function(codesign_initialize_win32)
	# Windows/Cygwin/MSYS
	# - Figure out where the Windows 10 SDKs are.
	# - From here on out, figure out the best match for the SDK version (prefer newer versions!).
	# - Then find signtool in this.
	# Windows Code Signing is done with 'signtool.exe'.

	# ToDo:
	# - Consider CygWin osslsigncode
	# - Consider enabling Win8.1 SDK support

	if(NOT CODESIGN_BIN_SIGNTOOL)
		# Find the root path to installed Windows 10 Kits.
		set(WINDOWS10KITS_REG_KEY "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots")
		set(WINDOWS10KITS_REG_VAL "KitsRoot10")
		set(WINDOWS10KITS_ROOT_DIR)
		if(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
			# Note: must be a cache operation in order to read from the registry.
			get_filename_component(WINDOWS10KITS_ROOT_DIR "[${WINDOWS10KITS_REG_KEY};${WINDOWS10KITS_REG_VAL}]" ABSOLUTE)
		else()
			include(CygwinPaths)

			# On Cygwin, CMake's built-in registry query won't work. Use Cygwin utility "regtool" instead.
			execute_process(COMMAND regtool get "\\${WINDOWS10KITS_REG_KEY}\\${WINDOWS10KITS_REG_VAL}"
				OUTPUT_VARIABLE WINDOWS10KITS_ROOT_DIR
				ERROR_QUIET
				OUTPUT_STRIP_TRAILING_WHITESPACE
			)
			if(WINDOWS10KITS_ROOT_DIR)
				convert_windows_path(WINDOWS10KITS_ROOT_DIR)
			endif()
		endif()

		# If we have no path, show an error.
		if(NOT WINDOWS10KITS_ROOT_DIR)
			message(WARNING "CMake CodeSign: Could not find a working Windows 10 SDK, disabling code signing.")
			return()
		endif()

		# List up any found Windows 10 SDK versions.
		file(GLOB WINDOWS10KITS_VERSIONS "${WINDOWS10KITS_ROOT_DIR}/bin/10.*")
		if(CMAKE_VERSION VERSION_LESS "3.18.0")
			list(REVERSE WINDOWS10KITS_VERSIONS)
		else()
			list(SORT WINDOWS10KITS_VERSIONS COMPARE NATURAL CASE INSENSITIVE ORDER DESCENDING)
		endif()

		# Find a fitting 'signtool.exe'.
		foreach(VERSION ${WINDOWS10KITS_VERSIONS})
			message(STATUS "Windows 10 Kit: ${VERSION}")
			find_program(CODESIGN_BIN_SIGNTOOL
				NAMES
					"signtool"
				HINTS
					"${CODESIGN_PATH}"
					"${VERSION}"
				PATHS
					"${CODESIGN_PATH}"
					"${VERSION}"
				PATH_SUFFIXES
					"x64"
					"x86"
			)

			if(CODESIGN_BIN_SIGNTOOL)
				break()
			endif()
		endforeach()
	endif()
endfunction()

function(codesign_initialize)
	# Find the tools by platform.
	if(WIN32)
		codesign_initialize_win32()
	elseif(APPLE)
		codesign_initialize_apple()
	else()
		message(FATAL_ERROR "CMake CodeSign: No supported Platform found.")
	endif()
endfunction()

function(codesign_timestamp_server)
	cmake_parse_arguments(
		PARSE_ARGV 0
		_ARGS
		"RFC3161"
		"RETURN"
		""
	)

	set(_list "")
	if(_ARGS_RFC3161)
		list(APPEND _list
			"http://timestamp.digicert.com"
#			"http://aatl-timestamp.globalsign.com/tsa/aohfewat2389535fnasgnlg5m23"
#			"https://timestamp.sectigo.com"
			"http://timestamp.entrust.net/TSS/RFC3161sha2TS"
#			"http://tsa.swisssign.net"
			"http://kstamp.keynectis.com/KSign/"
#			"http://tsa.quovadisglobal.com/TSS/HttpTspServer"
#			"http://ts.cartaodecidadao.pt/tsa/server"
			"http://tss.accv.es:8318/tsa"
#			"http://tsa.izenpe.com"
			"http://time.certum.pl"
#			"http://zeitstempel.dfn.de"
			"http://psis.catcert.cat/psis/catcert/tsp"
			"http://sha256timestamp.ws.symantec.com/sha256/timestamp"
#			"http://rfc3161timestamp.globalsign.com/advanced"
#			"http://timestamp.globalsign.com/tsa/r6advanced1"
			"http://timestamp.apple.com/ts01"
#			"http://tsa.baltstamp.lt"
#			"https://freetsa.org/tsr"
#			"https://www.safestamper.com/tsa"
#			"http://tsa.mesign.com"
#			"https://tsa.wotrus.com"
#			"http://tsa.lex-persona.com/tsa"
		)
	else()
		list(APPEND _list
			# ToDo: Do these still exist?
		)
	endif()

	list(LENGTH _list _len)
	if(_len EQUAL 0)
		set(${_ARGS_RETURN} "" PARENT_SCOPE)
		return()
	endif()

	# Retrieve random entry from list.
	string(RANDOM LENGTH 4 ALPHABET 0123456789 number)
	math(EXPR number "(${number} + 0) % ${_len}")  # Remove extra leading 0s.
	list(GET _list ${number} ${_ARGS_RETURN})

	# Propagate to parent.
	set(${_ARGS_RETURN} "${${_ARGS_RETURN}}" PARENT_SCOPE)
endfunction()

function(codesign_command_win32)
	cmake_parse_arguments(
		PARSE_ARGV 0
		_ARGS
		"SHA1;APPEND;TIMESTAMPS"
		"RETURN_BIN;RETURN_ARGS;CERTIFICATE_FILE;CERTIFICATE_NAME;CERTIFICATE_PASS"
		""
	)

	set(CMD_SHA1 "")
	set(CMD_SHA2 "")

	codesign_timestamp_server(RETURN TIMESTAMP_SHA1)
	codesign_timestamp_server(RFC3161 RETURN TIMESTAMP_SHA1_RFC3161)
	codesign_timestamp_server(RFC3161 RETURN TIMESTAMP_SHA2_RFC3161)

	separate_arguments(CODESIGN_ARGS NATIVE_COMMAND ${CODESIGN_ARGS})

	if(CODESIGN_BIN_SIGNTOOL)
		# This is 'signtool.exe'
		SET(CMD_ARGS "")
		SET(CMD_ARGS_1 "")
		SET(CMD_ARGS_2 "")

		# Parameters: File/Name
		if(_ARGS_CERTIFICATE_FILE)
			file(TO_NATIVE_PATH "${_ARGS_CERTIFICATE_FILE}" CERT_PATH)
			list(APPEND CMD_ARGS /f "${CERT_PATH}")
		elseif(_ARGS_CERTIFICATE_NAME)
			list(APPEND CMD_ARGS /n "${_ARGS_CERTIFICATE_NAME}")
		else()
			message(FATAL_ERROR "CMake CodeSign: 'signtool' requires a certificate.")
		endif()

		# Parameters: Password
		if(_ARGS_CERTIFICATE_PASS)
			list(APPEND CMD_ARGS /p "${_ARGS_CERTIFICATE_PASS}")
		endif()

		# Parameters: Timestamping
		if(_ARGS_TIMESTAMPS)
			if(TIMESTAMP_SHA1)
				list(APPEND CMD_ARGS_1 /t "${TIMESTAMP_SHA1}")
			elseif(TIMESTAMP_SHA1_RFC3161)
				list(APPEND CMD_ARGS_1 /tr "${TIMESTAMP_SHA1_RFC3161}")
			endif()

			if(TIMESTAMP_SHA2_RFC3161)
				list(APPEND CMD_ARGS_2 /tr "${TIMESTAMP_SHA2_RFC3161}")
			endif()
		endif()

		# Sign SHA-1 (Windows 8 and earlier)
		list(APPEND CMD_SHA1 sign ${CMD_ARGS} ${CMD_ARGS_1} ${CODESIGN_ARGS} /fd sha1 /td sha1)

		# Sign SHA-256 (Windows 10 and newer)
		list(APPEND CMD_SHA2 sign ${CMD_ARGS} ${CMD_ARGS_2} ${CODESIGN_ARGS} /fd sha256 /td sha256)

		if(_ARGS_APPEND)
			list(APPEND CMD_SHA1 /as)
			list(APPEND CMD_SHA2 /as)
		endif()

		set(${_ARGS_RETURN_BIN} "${CODESIGN_BIN_SIGNTOOL}" PARENT_SCOPE)
	elseif(CODESIGN_BIN_OSSLSIGNCODE)
		# This is 'osslsigncode' from CygWin
		SET(CMD_ARGS "")
		SET(CMD_ARGS_1 "")
		SET(CMD_ARGS_2 "")

		# Parameters: File/Name
		if(_ARGS_CERTIFICATE_FILE)
			file(TO_NATIVE_PATH "${_ARGS_CERTIFICATE_FILE}" CERT_PATH)
			list(APPEND CMD_ARGS -pkcs12 "${CERT_PATH}")
		elseif(_ARGS_CERTIFICATE_NAME)
			message(FATAL_ERROR "CMake CodeSign: 'osslsigncode' is unable to use Windows's certificate store, define CERTIFICATE_FILE.")
		else()
			message(FATAL_ERROR "CMake CodeSign: 'osslsigncode' requires a certificate.")
		endif()

		# Parameters: Password
		if(_ARGS_CERTIFICATE_PASS)
			list(APPEND CMD_ARGS -pass "${_ARGS_CERTIFICATE_PASS}")
		endif()

		# Parameters: Timestamping
		if(_ARGS_TIMESTAMPS)
			if(TIMESTAMP_SHA1)
				list(APPEND CMD_ARGS_1 -t \"${TIMESTAMP_SHA1}\")
			elseif(TIMESTAMP_SHA1_RFC3161)
				list(APPEND CMD_ARGS_1 -ts \"${TIMESTAMP_SHA1_RFC3161}\")
			endif()

			if(TIMESTAMP_SHA2_RFC3161)
				list(APPEND CMD_ARGS_2 -ts \"${TIMESTAMP_SHA2_RFC3161}\")
			endif()
		endif()

		# Sign SHA-1 (Windows 8 and earlier)
		list(APPEND CMD_SHA1 ${CMD_ARGS} ${CMD_ARGS_1} ${CODESIGN_ARGS} -h sha1)

		# Sign SHA-256 (Windows 10 and newer)
		list(APPEND CMD_SHA2 ${CMD_ARGS} ${CMD_ARGS_2} ${CODESIGN_ARGS} -h sha256)

		if(_ARGS_APPEND)
			list(APPEND CMD_SHA1 -nest)
			list(APPEND CMD_SHA2 -nest)
		endif()

		set(${_ARGS_RETURN_BIN} "${CODESIGN_BIN_OSSLSIGNCODE}" PARENT_SCOPE)
	else()
		message(FATAL_ERROR "CMake CodeSign: No supported Tool found.")
	endif()

	if(_ARGS_SHA1)
		set(${_ARGS_RETURN_ARGS} ${CMD_SHA1} PARENT_SCOPE)
	else()
		set(${_ARGS_RETURN_ARGS} ${CMD_SHA2} PARENT_SCOPE)
	endif()
endfunction()

function(codesign_win32)
	cmake_parse_arguments(
		PARSE_ARGV 0
		_ARGS
		"TIMESTAMPS"
		"CERTIFICATE_FILE;CERTIFICATE_NAME;CERTIFICATE_PASS"
		"TARGETS"
	)

	if((NOT _ARGS_CERTIFICATE_FILE) AND (NOT _ARGS_CERTIFICATE_NAME))
		message(FATAL_ERROR "CMake CodeSign: One of CERTIFICATE_FILE or CERTIFICATE_NAME must be defined.")
	endif()

	# Generate Commands
	codesign_command_win32(
		SHA1
		RETURN_BIN BIN_SHA1
		RETURN_ARGS CMD_SHA1
		${ARGV}
	)
	codesign_command_win32(
		SHA2 APPEND
		RETURN_BIN BIN_SHA2
		RETURN_ARGS CMD_SHA2
		${ARGV}
	)
#		CERTIFICATE_FILE "${_ARGS_CERTIFICATE_FILE}"
#		CERTIFICATE_NAME "${_ARGS_CERTIFICATE_NAME}"
#		CERTIFICATE_PASS "${_ARGS_CERTIFICATE_PASS}"

	# Sign Binaries
	foreach(_target ${_ARGS_TARGETS})
		file(TO_NATIVE_PATH "${CODESIGN_CERT_FILE}" CERT_PATH)
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND ${BIN_SHA1} ARGS ${CMD_SHA1} $<SHELL_PATH:$<TARGET_FILE:${_target}>>
			COMMAND ${BIN_SHA2} ARGS ${CMD_SHA2} $<SHELL_PATH:$<TARGET_FILE:${_target}>>
			VERBATIM
		)
		message(STATUS "CMake CodeSign: Added post-build step to project '${_target}'.")
	endforeach()
endfunction()

function(codesign)
	# Initialize code sign code.
	codesign_initialize()

	if(WIN32)
		codesign_win32(${ARGV})
	elseif(APPLE)
		codesign_apple(${ARGV})
	else()
		codesign_unix(${ARGV})
	endif()
endfunction()
