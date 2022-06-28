# Clang Toolset integration for all CMake Generators
Adds clang-tidy and clang-format support for all current CMake generators, such as the Visual Studio and ninja ones. 

# Supported Compilers
* Visual Studio 2017
* Visual Studio 2019
* GCC 8
* GCC 9
* Clang

# Usage Reference
### `generate_compile_commands_json([REGEX <filter>] TARGETS <target>[;<target>[;...]])`
Generates a compile_commands.json for each supported build configuration which is used for various Clang tools. These files use generator expressions if possible, and should contain everything necessary to run other tools.

* `REGEX <filter>`: Specify a regular expression by which to filter all sources of the given targets. Defaults to `\.(h|hpp|c|cpp)$`.
* `TARGETS <target>[;<target>[;...]]`: Declare for which targets the compile commands database should be created.

### `clang_format([DEPENDENCY] [GLOBAL] [REGEX <filter>] [VERSION <version] TARGETS <target>[<target>[;...]])`
Add a `TARGETNAME_CLANG-FORMAT` target to the project which runs clang-format on the project, using whichever `.clang-format` file it finds next to the source. Automatically applies all formatting and can be used as a build step using the DEPENDENCY flag, in order to prevent formatting from not being applied. 

* `DEPENDENCY`: Add the `TARGETNAME_CLANG-FORMAT` as a build dependency to the target.
* `GLOBAL`: Add a `CLANG-FORMAT` target that contains all other clang-format targets that also specified GLOBAL.
* `REGEX <filter>`: Specify a regular expression by which to filter all sources of the given targets. Defaults to `\.(h|hpp|c|cpp)$`.
* `VERSION <version>`: Require a minimum Clang Toolset version to run clang-format.
* `TARGETS <target>[;<target>[;...]]`: Specify on which targets clang-format is supposed to be run.

### `clang_tidy([DEPENDENCY] [GLOBAL] [REGEX <filter>] [VERSION <version] TARGETS <target>[<target>[;...]])`
Add a `TARGETNAME_CLANG-TIDY` target to the project which runs clang-tidy on the project, using whichever `.clang-tidy` file it finds next to the source. Does not apply suggested fixes automatically, and requires the creation of compile command databases for each target.

* `DEPENDENCY`: Add the `TARGETNAME_CLANG-TIDY` as a build dependency to the target.
* `GLOBAL`: Add a `CLANG-TIDY` target that contains all other clang-tidy targets that also specified GLOBAL.
* `REGEX <filter>`: Specify a regular expression by which to filter all sources of the given targets. Defaults to `\.(h|hpp|c|cpp)$`.
* `VERSION <version>`: Require a minimum Clang Toolset version to run clang-tidy.
* `TARGETS <target>[;<target>[;...]]`: Specify on which targets clang-tidy is supposed to be run.

# Example Usage
## Adding clang-format as a dependency and clang-tidy as a normal target
```
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Clang/Clang.cmake")
	include("Clang")
  option(ENABLE_CLANG ON)
  if(ENABLE_CLANG)
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
endif()
```
