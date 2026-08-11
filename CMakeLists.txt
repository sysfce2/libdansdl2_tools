cmake_minimum_required(VERSION 3.5.1)
project(tools)

option(BUILD_EXAMPLES "Build example code" ON)
option(BUILD_DEBUG "Build a debug release" OFF)
option(BUILD_SHARED "Build a shared lib" ON)
option(BUILD_STATIC "Build a static lib" OFF)

#library version
set(MAJOR_VERSION 1)
set(MINOR_VERSION 1)
set(PATCH_VERSION 10)

if(${BUILD_DEBUG})

	set(RELEASE_VERSION "debug")

else()

	set(RELEASE_VERSION "prod")
endif()

if(${CMAKE_VERSION} VERSION_LESS "3.22.0")

	add_definitions(-DMAJOR_VERSION=${MAJOR_VERSION})
	add_definitions(-DMINOR_VERSION=${MINOR_VERSION})
	add_definitions(-DPATCH_VERSION=${PATCH_VERSION})
	add_definitions(-DRELEASE_VERSION=\"${RELEASE_VERSION}\")
else()

	add_compile_definitions(MAJOR_VERSION=${MAJOR_VERSION})
	add_compile_definitions(MINOR_VERSION=${MINOR_VERSION})
	add_compile_definitions(PATCH_VERSION=${PATCH_VERSION})
	add_compile_definitions(RELEASE_VERSION=\"${RELEASE_VERSION}\")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-Wall -Wextra -Wundef -Wcast-align -Wwrite-strings -Wlogical-op -Wmissing-declarations -Wredundant-decls -Wshadow -Woverloaded-virtual -Wno-deprecated -pedantic -fno-rtti)

#include and source...
include_directories("${PROJECT_SOURCE_DIR}/include")
set(SOURCE "")
add_subdirectory("${PROJECT_SOURCE_DIR}/lib")
#library type and filenames.

if(${BUILD_DEBUG})

	set(CMAKE_BUILD_TYPE Debug)
	set(LIB_FILENAME "tools_debug")
else()

	set(CMAKE_BUILD_TYPE Release)
	set(LIB_FILENAME "tools")
endif()

if(${BUILD_STATIC})

	add_library(tools_static STATIC ${SOURCE})
	set_target_properties(tools_static PROPERTIES OUTPUT_NAME ${LIB_FILENAME})
	target_compile_definitions(tools_static PUBLIC "-DLIB_VERSION=\"static\"")
	install(TARGETS tools_static DESTINATION lib)

	if(${BUILD_DEBUG})

		target_compile_definitions(tools_static PUBLIC "-DWITH_DEBUG_CODE=1")
	endif()

	message("will build ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}-${RELEASE_VERSION}-static")
endif()

if(${BUILD_SHARED})

	add_library(tools_shared SHARED ${SOURCE})
	set_target_properties(tools_shared PROPERTIES OUTPUT_NAME ${LIB_FILENAME})
	target_compile_definitions(tools_shared PUBLIC "-DLIB_VERSION=\"shared\"")
	install(TARGETS tools_shared DESTINATION lib)

	if(${BUILD_DEBUG})

		target_compile_definitions(tools_shared PUBLIC "-DWITH_DEBUG_CODE=1")
	endif()

	message("will build ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}-${RELEASE_VERSION}-shared")
endif()

install(DIRECTORY include/ DESTINATION include)

IF(WIN32)

	if(${BUILD_SHARED})
		target_compile_definitions(tools_shared PUBLIC -DWINBUILD)
	endif()

	if(${BUILD_STATIC})
		target_compile_definitions(tools_static PUBLIC -DWINBUILD)
	endif()
ENDIF()

if(${BUILD_EXAMPLES})

	if(!${BUILD_SHARED})

		message(FATAL_ERROR "examples must be built against the shared library!")
	endif()

	add_executable(i8n examples/i8n/main.cpp)
	target_link_libraries(i8n tools_shared stdc++fs)

	add_executable(system examples/system/main.cpp)
	target_link_libraries(system tools_shared stdc++fs)

	add_executable(json_config_file examples/json_config_file/main.cpp)
	target_link_libraries(json_config_file tools_shared stdc++fs)

	add_executable(file_utils examples/file_utils/main.cpp)
	target_link_libraries(file_utils tools_shared stdc++fs)
	add_custom_command(TARGET file_utils POST_BUILD COMMAND cp -r ../examples/file_utils/*.txt ./)

	add_executable(menu examples/menu/main.cpp)
	target_link_libraries(menu tools_shared stdc++fs)
	add_custom_command(TARGET menu POST_BUILD COMMAND cp -r ../examples/menu/menu.json ./)

	add_executable(chrono examples/chrono/main.cpp)
	target_link_libraries(chrono tools_shared stdc++fs)

	add_executable(version examples/lib_version.cpp)
	target_link_libraries(version tools_shared stdc++fs)
endif()
