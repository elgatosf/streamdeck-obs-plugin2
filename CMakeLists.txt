
cmake_minimum_required(VERSION 3.8...4.0)

################################################################################
# Versioning
################################################################################

set(VERSION_MAJOR 5)
set(VERSION_MINOR 5)
set(VERSION_PATCH 3)
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
    set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}${VERSION_SUFFIX}")
else()
    set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}${VERSION_SUFFIX}")
endif()

# Log the detected version.
message(STATUS "${LOGPREFIX} Version ${VERSION_STRING}")

################################################################################
# Project
################################################################################

set(PROJECT_SUFFIX "" CACHE STRING "Optional suffix for project name")
set(ASIO_PATH "third-party/asio/asio" CACHE STRING "Which path should ASIO be loaded from")
set(BUILD_LOADER OFF CACHE BOOL "Build Loader library instead")

project(
    StreamDeckPlugin${PROJECT_SUFFIX}
    VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}.${VERSION_TWEAK}
    DESCRIPTION "Stream Deck integration for OBS Studio."
    HOMEPAGE_URL "https://elgato.com/"
)

# Full Project Name
set(PROJECT_FULL_NAME "Stream Deck (for OBS Studio)")

# Description
set(PROJECT_DESCRIPTION "Integration for Stream Deck into OBS Studio.")

# Authors
set(PROJECT_AUTHORS "Corsair Memory Inc. All Rights Reserved")

# Copyright Years
set(PROJECT_COPYRIGHT_YEARS "2022")

# Versioning
set(PROJECT_VERSION_STRING ${VERSION_STRING})


set(BUILD_OLD "" CACHE BOOL "Build with the old CMake file?")

if(WIN32)
  add_definitions(/wd4127 /wd4100 /wd4189)
endif()

################################################################################
# Codesign
################################################################################

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/submodules/maketools-for-windows/Signing/corsair_sign_regular.bat")
	set(HAVE_CODESIGN ON)
endif()

if ( ${BUILD_OLD} )
    # Minimum version = MMmmpp, major minor patch all squished together
    add_compile_definitions(OBS_MINIMUM_SUPPORT=260000)
    include("OldCMake.cmake")
else()
    add_compile_definitions(OBS_MINIMUM_SUPPORT=280000)
    include("OBSTemplate.cmake")
endif()


# Windows
if(D_PLATFORM_WINDOWS)
    if(HAVE_CODESIGN)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND call ${PROJECT_SOURCE_DIR}/submodules/maketools-for-windows/Signing/corsair_sign_regular.bat $(TargetDir)$(TargetFileName)
        )
    endif()
endif()









