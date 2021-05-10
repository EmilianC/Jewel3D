cmake_minimum_required(VERSION 3.16)

set(CMAKE_GENERATOR_PLATFORM Win32)
set(CMAKE_VS_PLATFORM_NAME Win32)
set(CMAKE_SKIP_INSTALL_RULES TRUE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

project(Gemcutter LANGUAGES C CXX)

find_package(Git REQUIRED)
find_package(OpenGL REQUIRED)

# Ensures that a git submodule is initialized and updated.
function(update_submodule submodule_dir)
	if (NOT EXISTS "${submodule_dir}/.git")
		execute_process(
			COMMAND "${GIT_EXECUTABLE}" submodule init "${submodule_dir}/"
			WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
			RESULT_VARIABLE error_code_init
		)
		execute_process(
			COMMAND "${GIT_EXECUTABLE}" submodule update --recursive --remote --merge "${submodule_dir}/"
			WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
			RESULT_VARIABLE error_code_update
		)

		if (error_code OR error_code_update)
			message(FATAL_ERROR "Failed to update submodule: ${submodule_dir}")
		else()
			message(STATUS "Submodule updated: ${submodule_dir}")
		endif()
	endif()
endfunction()

update_submodule("${CMAKE_CURRENT_LIST_DIR}/external/openal-soft")
update_submodule("${CMAKE_CURRENT_LIST_DIR}/external/scaffold")

# Load Scaffold CMake utilities.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/scaffold")
include(scaffold)
sf_is_project_top_level(IS_TOP_LEVEL)

add_subdirectory(external)
add_subdirectory(gemcutter)
add_subdirectory(tools)

option(ENABLE_TESTS "Build unit tests." ${IS_TOP_LEVEL})
if (ENABLE_TESTS)
	add_subdirectory(tests)
endif()

option(ENABLE_SAMPLES "Download and build sample projects." ${IS_TOP_LEVEL})
if (ENABLE_SAMPLES)
	update_submodule("${CMAKE_CURRENT_LIST_DIR}/samples")
	add_subdirectory(samples)
endif()
