set(CLANG_PATH "" CACHE PATH "Path to Clang Toolset (if not in environment)")

function(string_escape)
	cmake_parse_arguments(
		PARSE_ARGV 0
		_ARGS
		""
		"OUTPUT;INPUT"
		""
	)

	set(_el "${_ARGS_INPUT}")
	string(REPLACE "\\" "\\\\" _el "${_el}")
	string(REPLACE "\"" "\\\"" _el "${_el}")
	set(${_ARGS_OUTPUT} "${_el}" PARENT_SCOPE)
endfunction()

function(string_append_escaped)
	cmake_parse_arguments(
		PARSE_ARGV 0
		_ARGS
		""
		"OUTPUT;INPUT"
		""
	)

	set(_el "${_ARGS_INPUT}")
	if(_el)
		string_escape(OUTPUT _el INPUT "${_el}")
		set(${_ARGS_OUTPUT} "${${_ARGS_OUTPUT}}${_el}" PARENT_SCOPE)
	endif()
endfunction()

function(string_append_target_includes)
	cmake_parse_arguments(
		PARSE_ARGV 0
		_ARGS
		"LINKED"
		"OUTPUT;PREFIX;TARGET"
		""
	)

	set(_out "")
	if(TARGET ${_ARGS_TARGET})
		get_target_property(target_type ${_ARGS_TARGET} TYPE)
		if(
			(${target_type} STREQUAL "MODULE_LIBRARY") OR
			(${target_type} STREQUAL "STATIC_LIBRARY") OR
			(${target_type} STREQUAL "SHARED_LIBRARY") OR
			(${target_type} STREQUAL "EXECUTABLE")
		)
			# Include Directories
			set(prop "$<TARGET_PROPERTY:${_ARGS_TARGET},INCLUDE_DIRECTORIES>")
			set(test "$<$<BOOL:${prop}>:${_ARGS_PREFIX}$<JOIN:${prop}, ${_ARGS_PREFIX}>>")
			set(prop2 "$<TARGET_PROPERTY:${_ARGS_TARGET},INTERFACE_INCLUDE_DIRECTORIES>")
			set(test2 "$<$<BOOL:${prop2}>:${_ARGS_PREFIX}$<JOIN:${prop2}, ${_ARGS_PREFIX}>>")
			set(prop3 "$<TARGET_PROPERTY:${_ARGS_TARGET},INTERFACE_SYSTEM_INCLUDE_DIRECTORIES>")
			set(test3 "$<$<BOOL:${prop3}>:${_ARGS_PREFIX}$<JOIN:${prop3}, ${_ARGS_PREFIX}>>")
			set(_out "${_out}${test} ${test2} ${test3}")

			# Try and scan any linked libraries for include directories.
			if(NOT _ARGS_LINKED)
				get_target_property(_els ${_target} LINK_LIBRARIES)
				foreach(_lib ${_els})
					set(_out2 "")
					string_append_target_includes(TARGET "${_lib}" PREFIX "${_ARGS_PREFIX}" OUTPUT _out2 LINKED)
					set(_out "${_out}${_out2}")
				endforeach()
			endif()
		elseif((${target_type} STREQUAL "INTERFACE_LIBRARY"))
			# Public Include Directories
			set(prop "$<TARGET_PROPERTY:${_ARGS_TARGET},INTERFACE_INCLUDE_DIRECTORIES>")
			set(test "$<$<BOOL:${prop}>:${_ARGS_PREFIX}$<JOIN:${prop}, ${_ARGS_PREFIX}>>")
			set(_out "${_out}${test} ")
		else()
			message("Clang for CMake: Unsupported Target type '${target_type}', please open an issue for this.")
		endif()
	endif()

	set(${_ARGS_OUTPUT} "${${_ARGS_OUTPUT}}${_out}" PARENT_SCOPE)
endfunction()

function(generate_compile_commands_json)
	cmake_parse_arguments(
		PARSE_ARGV 0
		_ARGS
		""
		"REGEX"
		"TARGETS"
	)

	set(COMPILER_TAG "")
	if((CMAKE_C_COMPILER_ID STREQUAL "MSVC") AND (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"))
		set(COMPILER_TAG "MSVC")
		set(COMPILER_INCLUDE_PREFIX "/I")
		set(COMPILER_DEFINE_PREFIX "/D")
	elseif((CMAKE_C_COMPILER_ID STREQUAL "GNU") AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
		set(COMPILER_TAG "GNU")
		set(COMPILER_INCLUDE_PREFIX "-I")
		set(COMPILER_DEFINE_PREFIX "-D")
	elseif((CMAKE_C_COMPILER_ID STREQUAL "Clang") AND (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
		set(COMPILER_TAG "CLANG") # Compatible with GNU
		set(COMPILER_INCLUDE_PREFIX "-I")
		set(COMPILER_DEFINE_PREFIX "-D")
	elseif((CMAKE_C_COMPILER_ID STREQUAL "AppleClang") AND (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang"))
		set(COMPILER_TAG "CLANG") # Compatible with Clang
		set(COMPILER_INCLUDE_PREFIX "-I")
		set(COMPILER_DEFINE_PREFIX "-D")
	else()
		message("Clang for CMake: C ID '${CMAKE_C_COMPILER_ID}'")
		message("Clang for CMake: C Version '${CMAKE_C_COMPILER_VERSION}'")
		message("Clang for CMake: C++ ID '${CMAKE_CXX_COMPILER_ID}'")
		message("Clang for CMake: C++ Version '${CMAKE_CXX_COMPILER_VERSION}'")
		message(FATAL_ERROR "Clang for CMake: Current Compiler is not yet supported, please open an issue for it.")
	endif()

	# Default Filter
	if(NOT _ARGS_REGEX)
		set(_ARGS_REGEX "\.(h|hpp|c|cpp)$")
	endif()

	foreach(_target ${_ARGS_TARGETS})
		set(COMPILE_COMMAND_JSON "")

		# Source Directory
		get_target_property(target_source_dir_rel ${_target} SOURCE_DIR)
		get_filename_component(target_source_dir ${target_source_dir_rel} ABSOLUTE)
		unset(target_source_dir_rel)

		# Binary Directory
		get_target_property(target_binary_dir_rel ${_target} BINARY_DIR)
		get_filename_component(target_binary_dir ${target_binary_dir_rel} ABSOLUTE)
		unset(target_binary_dir_rel)

		# Sources
		get_target_property(_els ${_target} SOURCES)
		set(target_sources "")
		foreach(_el ${_els})
			get_filename_component(_el "${_el}" ABSOLUTE)
			list(APPEND target_sources "${_el}")
		endforeach()
		list(FILTER target_sources INCLUDE REGEX "${_ARGS_REGEX}")

		# Combine Compiler String
		set(COMPILER_OPTIONS "")
		## Compiler Options
		get_target_property(_els ${_target} COMPILE_OPTIONS)
		foreach(_el ${_els})
			string_append_escaped(OUTPUT COMPILER_OPTIONS INPUT "${_el} ")
		endforeach()
		string(APPEND COMPILER_OPTIONS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} ")
		## C++ Standard
		get_target_property(_el ${_target} CXX_STANDARD)
		if(COMPILER_TAG STREQUAL "MSVC")			
			if(${MSVC_VERSION} GREATER_EQUAL "1920")
				if((_el EQUAL 98) OR (_el EQUAL 11))
					# Nothing to do, this is the default.
				elseif(_el EQUAL 14)
					string(APPEND COMPILER_OPTIONS "/std:c++14 ")
				elseif(_el EQUAL 17)
					string(APPEND COMPILER_OPTIONS "/std:c++17 ")
				elseif(_el EQUAL 20)
					string(APPEND COMPILER_OPTIONS "/std:c++latest ")
				endif()
			elseif(${MSVC_VERSION} GREATER_EQUAL "1910")
				if((_el EQUAL 98) OR (_el EQUAL 11))
					# Nothing to do, this is the default.
				elseif(_el EQUAL 14)
					string(APPEND COMPILER_OPTIONS "/std:c++14 ")
				elseif(_el EQUAL 17)
					string(APPEND COMPILER_OPTIONS "/std:c++17 ")
				elseif(_el EQUAL 20)
					string(APPEND COMPILER_OPTIONS "/std:c++latest ")
				endif()
			else()
				message("Clang for CMake: C ID '${CMAKE_C_COMPILER_ID}'")
				message("Clang for CMake: C Version '${CMAKE_C_COMPILER_VERSION}'")
				message("Clang for CMake: C++ ID '${CMAKE_CXX_COMPILER_ID}'")
				message("Clang for CMake: C++ Version '${CMAKE_CXX_COMPILER_VERSION}'")
				message(FATAL_ERROR "Clang for CMake: Current Compiler is not yet supported, please open an issue for it.")
			endif()
		elseif((COMPILER_TAG STREQUAL "CLANG") OR (COMPILER_TAG STREQUAL "GNU"))
			if(_el EQUAL 98)
				string(APPEND COMPILER_OPTIONS "-std=c++98 ")
			elseif(_el EQUAL 11)
				string(APPEND COMPILER_OPTIONS "-std=c++11 ")
			elseif(_el EQUAL 14)
				string(APPEND COMPILER_OPTIONS "-std=c++14 ")
			elseif(_el EQUAL 17)
				string(APPEND COMPILER_OPTIONS "-std=c++17 ")
			elseif(_el EQUAL 20)
				if((COMPILER_TAG STREQUAL "CLANG") OR (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 9))
					string(APPEND COMPILER_OPTIONS "-std=c++20 ")
				else()
					string(APPEND COMPILER_OPTIONS "-std=c++2a ")
				endif()
			endif()
		endif()
		## Definitions
		get_target_property(_els ${_target} COMPILE_DEFINITIONS)
		foreach(_el ${_els})
			string_append_escaped(OUTPUT COMPILER_OPTIONS INPUT "${COMPILER_DEFINE_PREFIX}${_el} ")
		endforeach()
		## Includes
		string_append_target_includes(TARGET ${_target} PREFIX ${COMPILER_INCLUDE_PREFIX} OUTPUT COMPILER_OPTIONS)

		# Create Compilation Database
		set(target_compile_db  "${target_binary_dir}/compile_commands.json")
		string(APPEND COMPILE_COMMAND_JSON "[\n")
		foreach(_el ${target_sources})
			file(TO_NATIVE_PATH "${_el}" _el)
			string(REPLACE "\\" "\\\\" _el "${_el}")
			string(REPLACE "\"" "\\\"" _el "${_el}")

			string(APPEND COMPILE_COMMAND_JSON "\t{\n")
			string(APPEND COMPILE_COMMAND_JSON "\t\t\"directory\": \"${target_binary_dir}\",\n")
			string(APPEND COMPILE_COMMAND_JSON "\t\t\"file\": \"${_el}\",\n")
			if(MSVC)
				string(APPEND COMPILE_COMMAND_JSON "\t\t\"command\": \"cl ")
			else()
				string(APPEND COMPILE_COMMAND_JSON "\t\t\"command\": \"cc ")
			endif()

			string(APPEND COMPILE_COMMAND_JSON "${COMPILER_OPTIONS}")

			if(MSVC)
				string(APPEND COMPILE_COMMAND_JSON "${_el}")
			else()
				string(APPEND COMPILE_COMMAND_JSON " -c ${_el}")
			endif()
			string(APPEND COMPILE_COMMAND_JSON "\",\n\t},\n")
		endforeach()
		string(APPEND COMPILE_COMMAND_JSON "]")

		file(GENERATE OUTPUT "$<TARGET_PROPERTY:${_target},BINARY_DIR>/$<CONFIG>/compile_commands.json" CONTENT "${COMPILE_COMMAND_JSON}")
	endforeach()
endfunction()

function(clang_format)
	cmake_parse_arguments(
		PARSE_ARGV 0
		_ARGS
		"DEPENDENCY;GLOBAL"
		"REGEX;VERSION"
		"TARGETS"
	)

	find_program(CLANG_FORMAT_BIN
		NAMES
			"clang-format"
			"clang-format-10"
			"clang-format-9"
			"clang-format-8"
			"clang-format-7"
			"clang-format-6"
			"clang-format-6.0"
			"clang-format-5.0"
			"clang-format-4.0"
			"clang-format-3.9"
			"clang-format-3.8"
			"clang-format-3.7"
			"clang-format-3.6"
			"clang-format-3.5"
		HINTS
			"${CLANG_PATH}"
		PATHS
			/bin
			/sbin
			/usr/bin
			/usr/local/bin
		PATH_SUFFIXES
			bin
			bin64
			bin32
		DOC "Path (or name) of the clang-format binary"
	)
	if(NOT CLANG_FORMAT_BIN)
		message(WARNING "Clang for CMake: Could not find clang-format at path '${CLANG_FORMAT_BIN}', disabling clang-format...")
		return()
	endif()

	# Validate Version
	if (_ARGS_VERSION)
		set(_VERSION_RESULT "")
		set(_VERSION_OUTPUT "")
		execute_process(
			COMMAND "${CLANG_FORMAT_BIN}" --version
			WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
			RESULT_VARIABLE _VERSION_RESULT
			OUTPUT_VARIABLE _VERSION_OUTPUT
			OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_STRIP_TRAILING_WHITESPACE
			ERROR_QUIET
		)
		if(NOT _VERSION_RESULT EQUAL 0)
			message(WARNING "Clang for CMake: Could not discover version, disabling clang-format...")
			return()
		endif()
		string(REGEX MATCH "([0-9]+\.[0-9]+\.[0-9]+)" _VERSION_MATCH ${_VERSION_OUTPUT})
		if(NOT ${_VERSION_MATCH} VERSION_GREATER_EQUAL ${_ARGS_VERSION})
			message(WARNING "Clang for CMake: Old version discovered, disabling clang-format...")
			return()
		endif()
	endif()

	# Default Filter
	if(NOT _ARGS_REGEX)
		set(_ARGS_REGEX "\.(h|hpp|c|cpp)$")
	endif()

	# Go through each target
	foreach(_target ${_ARGS_TARGETS})
		get_target_property(target_sources_rel ${_target} SOURCES)
		set(target_sources "")
		foreach(_el ${target_sources_rel})
			get_filename_component(_el "${_el}" ABSOLUTE)
			file(TO_NATIVE_PATH "${_el}" _el)
			list(APPEND target_sources "${_el}")
		endforeach()
		list(FILTER target_sources INCLUDE REGEX "${_ARGS_REGEX}")
		unset(target_sources_rel)
		
		get_target_property(target_source_dir_rel ${_target} SOURCE_DIR)
		get_filename_component(target_source_dir ${target_source_dir_rel} ABSOLUTE)
		unset(target_source_dir_rel)

		add_custom_target(${_target}_CLANG-FORMAT
			COMMAND "${CLANG_FORMAT_BIN}" -style=file -i ${target_sources}
			COMMENT "clang-format: Formatting ${_target}..."
			WORKING_DIRECTORY "${target_source_dir}"
		)

		if(_ARGS_DEPENDENCY)
			add_dependencies(${_target} ${_target}_CLANG-FORMAT)
		endif()

		if(_ARGS_GLOBAL)
			if(TARGET CLANG-FORMAT)
				add_dependencies(CLANG-FORMAT ${_target}_CLANG-FORMAT)
			else()
				add_custom_target(CLANG-FORMAT
					DEPENDS
						${_target}_CLANG-FORMAT
					COMMENT
						"clang-format: Formatting..."
				)
			endif()
		endif()
	endforeach()
endfunction()

function(clang_tidy)
	cmake_parse_arguments(
		PARSE_ARGV 0
		_ARGS
		"DEPENDENCY;GLOBAL"
		"REGEX;VERSION"
		"TARGETS"
	)

	find_program(CLANG_TIDY_BIN
		NAMES
			"clang-tidy"
			"clang-tidy-10"
			"clang-tidy-9"
			"clang-tidy-8"
			"clang-tidy-7"
			"clang-tidy-6"
			"clang-tidy-6.0"
			"clang-tidy-5.0"
			"clang-tidy-4.0"
			"clang-tidy-3.9"
			"clang-tidy-3.8"
			"clang-tidy-3.7"
			"clang-tidy-3.6"
		HINTS
			"${CLANG_PATH}"
		PATHS
			/bin
			/sbin
			/usr/bin
			/usr/local/bin
		PATH_SUFFIXES
			bin
			bin64
			bin32
		DOC "Path (or name) of the clang-tidy binary"
	)
	if(NOT CLANG_TIDY_BIN)
		message(WARNING "Clang for CMake: Could not find clang-tidy at path '${CLANG_TIDY_BIN}', disabling clang-tidy...")
		return()
	endif()

	# Validate Version
	if (_ARGS_VERSION)
		set(_VERSION_RESULT "")
		set(_VERSION_OUTPUT "")
		execute_process(
			COMMAND "${CLANG_TIDY_BIN}" --version
			WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
			RESULT_VARIABLE _VERSION_RESULT
			OUTPUT_VARIABLE _VERSION_OUTPUT
			OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_STRIP_TRAILING_WHITESPACE
			ERROR_QUIET
		)
		if(NOT _VERSION_RESULT EQUAL 0)
			message(WARNING "Clang for CMake: Could not discover version, disabling clang-tidy...")
			return()
		endif()
		string(REGEX MATCH "([0-9]+\.[0-9]+\.[0-9]+)" _VERSION_MATCH ${_VERSION_OUTPUT})
		if(NOT ${_VERSION_MATCH} VERSION_GREATER_EQUAL ${_ARGS_VERSION})
			message(WARNING "Clang for CMake: Old version discovered, disabling clang-tidy...")
			return()
		endif()
	endif()

	# Default Filter
	if(NOT _ARGS_REGEX)
		set(_ARGS_REGEX "\.(h|hpp|c|cpp)$")
	endif()

	# Go through each target
	foreach(_target ${_ARGS_TARGETS})
		# Source Directory
		get_target_property(_els ${_target} SOURCE_DIR)
		get_filename_component(target_source_dir ${_els} ABSOLUTE)
		file(TO_NATIVE_PATH "${target_source_dir}" target_source_dir_nat)
		unset(_els)

		# Binary Directory
		get_target_property(_els ${_target} BINARY_DIR)
		get_filename_component(target_binary_dir ${_els} ABSOLUTE)
		file(TO_NATIVE_PATH "${target_binary_dir}" target_binary_dir_nat)
		unset(_els)

		# Sources
		get_target_property(_els ${_target} SOURCES)
		set(target_sources "")
		foreach(_el ${_els})
			get_filename_component(_el ${_el} ABSOLUTE)
			file(TO_NATIVE_PATH "${_el}" _el)
			list(APPEND target_sources "${_el}")
		endforeach()
		list(FILTER target_sources INCLUDE REGEX "${_ARGS_REGEX}")
		unset(_els)

		add_custom_target(${_target}_CLANG-TIDY
			COMMENT "clang-tiy: Tidying ${_target}..."
			WORKING_DIRECTORY "${target_binary_dir}"
			VERBATIM
		)
		foreach(_el ${target_sources})
			add_custom_command(
				TARGET ${_target}_CLANG-TIDY
				POST_BUILD
				COMMAND "${CLANG_TIDY_BIN}"
				ARGS --quiet -p="$<TARGET_PROPERTY:${_target},BINARY_DIR>/$<CONFIG>" "${_el}"
				WORKING_DIRECTORY "${target_binary_dir}"
				COMMAND_EXPAND_LISTS
			)
		endforeach()

		if(_ARGS_DEPENDENCY)
			add_dependencies(${_target} ${_target}_CLANG-TIDY)
		endif()

		if(_ARGS_GLOBAL)
			if(TARGET CLANG-TIDY)
				add_dependencies(CLANG-TIDY ${_target}_CLANG-FORTIDYMAT)
			else()
				add_custom_target(CLANG-TIDY
					DEPENDS
						${_target}_CLANG-TIDY
					COMMENT
						"clang-tiy: Tidying..."
				)
			endif()
		endif()
	endforeach()
endfunction()
