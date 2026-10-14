cmake_minimum_required(VERSION 4.1)

project(amulet_leveldb LANGUAGES CXX)

set(amulet_leveldb_DIR ${CMAKE_CURRENT_LIST_DIR}/src/amulet/leveldb CACHE PATH "")
set(BUILD_AMULET_LEVELDB_TESTS OFF CACHE BOOL "Should tests be built?")
option(THREAD_SAFETY_ANALYSIS "Run clang thread safety analysis over the sources before building" OFF)

if(THREAD_SAFETY_ANALYSIS AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "THREAD_SAFETY_ANALYSIS requires Clang")
endif()

# Set C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set platform variables
if (WIN32)
	# set windows 7 as the minimum version
	add_definitions(-D_WIN32_WINNT=0x0601)
elseif(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15")
elseif(UNIX)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
else()
    message( FATAL_ERROR "Unsupported platform. Please submit a pull request to support this platform." )
endif()

if (MSVC)
    add_definitions("/MP")
endif()

find_package(Python3 COMPONENTS Interpreter Development.Module REQUIRED)

# Find libraries
if (NOT TARGET pybind11::module)
    find_package(pybind11 CONFIG REQUIRED)
endif()
if (NOT TARGET amulet_pybind11_extensions)
    find_package(amulet_pybind11_extensions CONFIG REQUIRED)
endif()
if (NOT TARGET amulet_utils)
    find_package(amulet_utils CONFIG REQUIRED)
endif()

# Add implementation
include(FetchContent)
set(LEVELDB_BUILD_UTILS OFF CACHE BOOL "" FORCE)
set(LEVELDB_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LEVELDB_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(LEVELDB_INSTALL OFF CACHE BOOL "" FORCE)
set(LEVELDB_DISABLE_EXCEPTIONS OFF CACHE BOOL "" FORCE)
set(LEVELDB_DISABLE_RTTI OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
FetchContent_Declare(
  leveldb
  GIT_REPOSITORY https://github.com/Amulet-Team/leveldb.git
  GIT_TAG        1352243bb27d287b27be94f7591218dddb3ef900
)
FetchContent_MakeAvailable(leveldb)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set_target_properties(crc32c PROPERTIES FOLDER "ThirdParty")
if (TARGET crc32c_arm64)
    set_target_properties(crc32c_arm64 PROPERTIES FOLDER "ThirdParty")
endif()
if (TARGET crc32c_sse42)
    set_target_properties(crc32c_sse42 PROPERTIES FOLDER "ThirdParty")
endif()
set_target_properties(snappy PROPERTIES FOLDER "ThirdParty")
set_target_properties(libzstd_static PROPERTIES FOLDER "ThirdParty")
set_target_properties(zlibstatic PROPERTIES FOLDER "ThirdParty")


# Find C++ files
file(REAL_PATH src SOURCE_PATH)
file(GLOB_RECURSE EXTENSION_SOURCES LIST_DIRECTORIES false ${SOURCE_PATH}/amulet/leveldb/*.py.cpp)
file(GLOB_RECURSE EXTENSION_HEADERS LIST_DIRECTORIES false ${SOURCE_PATH}/amulet/leveldb/*.py.hpp)
file(GLOB_RECURSE SOURCES LIST_DIRECTORIES false ${SOURCE_PATH}/amulet/leveldb/*.cpp)
file(GLOB_RECURSE HEADERS LIST_DIRECTORIES false ${SOURCE_PATH}/amulet/leveldb/*.hpp)
list(REMOVE_ITEM SOURCES ${EXTENSION_SOURCES})
list(REMOVE_ITEM HEADERS ${EXTENSION_HEADERS})

# Extend the leveldb binary
target_sources(leveldb PRIVATE ${SOURCES} ${HEADERS})
set_target_properties(leveldb PROPERTIES CXX_VISIBILITY_PRESET hidden)
set_target_properties(leveldb PROPERTIES FOLDER "CPP")
target_link_libraries(leveldb amulet_utils)
target_include_directories(leveldb PUBLIC "${leveldb_SOURCE_DIR}/include")
target_include_directories(leveldb PUBLIC "${amulet_leveldb_DIR}/include")
foreach(FILE ${SOURCES} ${HEADERS})
    file(RELATIVE_PATH REL_PATH ${SOURCE_PATH} ${FILE})
    get_filename_component(GROUP ${REL_PATH} DIRECTORY)
    string(REPLACE "/" "\\" GROUP ${GROUP})
    source_group(${GROUP} FILES ${FILE})
endforeach()

if(THREAD_SAFETY_ANALYSIS)
    add_library(leveldb_thread_analysis OBJECT ${SOURCES} ${HEADERS})
    target_compile_definitions(leveldb_thread_analysis PRIVATE AMULET_THREAD_SAFETY_ANALYSIS)
    set_target_properties(leveldb_thread_analysis PROPERTIES CXX_VISIBILITY_PRESET hidden)
	set_target_properties(leveldb_thread_analysis PROPERTIES FOLDER "CPP")
	target_link_libraries(leveldb_thread_analysis amulet_utils)
	target_include_directories(leveldb_thread_analysis PUBLIC "${leveldb_SOURCE_DIR}/include")
	target_include_directories(leveldb_thread_analysis PUBLIC "${amulet_leveldb_DIR}/include")
    target_compile_options(leveldb_thread_analysis PRIVATE -fsyntax-only -Wthread-safety -Werror=thread-safety)
    add_dependencies(leveldb leveldb_thread_analysis)
endif()

# Add python extension
pybind11_add_module(_leveldb ${EXTENSION_SOURCES} ${EXTENSION_HEADERS})
if(APPLE)
    set_target_properties(_leveldb PROPERTIES INSTALL_RPATH "@loader_path")
elseif(UNIX)
    set_target_properties(_leveldb PROPERTIES INSTALL_RPATH "$ORIGIN")
endif()
set_target_properties(_leveldb PROPERTIES CXX_VISIBILITY_PRESET hidden)
set_target_properties(_leveldb PROPERTIES FOLDER "Python")
target_link_libraries(_leveldb PRIVATE amulet_pybind11_extensions)
target_link_libraries(_leveldb PRIVATE leveldb)
target_compile_definitions(_leveldb PRIVATE PYBIND11_DETAILED_ERROR_MESSAGES)
target_compile_definitions(_leveldb PRIVATE PYBIND11_VERSION="${pybind11_VERSION}")
target_compile_definitions(_leveldb PRIVATE COMPILER_ID="${CMAKE_CXX_COMPILER_ID}")
target_compile_definitions(_leveldb PRIVATE COMPILER_VERSION="${CMAKE_CXX_COMPILER_VERSION}")
foreach(FILE ${EXTENSION_SOURCES} ${EXTENSION_HEADERS})
    file(RELATIVE_PATH REL_PATH ${SOURCE_PATH} ${FILE})
    get_filename_component(GROUP ${REL_PATH} DIRECTORY)
    string(REPLACE "/" "\\" GROUP ${GROUP})
    source_group(${GROUP} FILES ${FILE})
endforeach()

if(THREAD_SAFETY_ANALYSIS)
    add_library(_leveldb_thread_analysis OBJECT ${EXTENSION_SOURCES} ${EXTENSION_HEADERS})
    target_link_libraries(_leveldb_thread_analysis PRIVATE pybind11::module)
    target_compile_definitions(_leveldb_thread_analysis PRIVATE AMULET_THREAD_SAFETY_ANALYSIS)
    set_target_properties(_leveldb_thread_analysis PROPERTIES CXX_VISIBILITY_PRESET hidden)
	set_target_properties(_leveldb_thread_analysis PROPERTIES FOLDER "Python")
	target_link_libraries(_leveldb_thread_analysis PRIVATE amulet_pybind11_extensions)
	target_link_libraries(_leveldb_thread_analysis PRIVATE leveldb)
	target_compile_definitions(_leveldb_thread_analysis PRIVATE PYBIND11_DETAILED_ERROR_MESSAGES)
	target_compile_definitions(_leveldb_thread_analysis PRIVATE PYBIND11_VERSION="${pybind11_VERSION}")
	target_compile_definitions(_leveldb_thread_analysis PRIVATE COMPILER_ID="${CMAKE_CXX_COMPILER_ID}")
	target_compile_definitions(_leveldb_thread_analysis PRIVATE COMPILER_VERSION="${CMAKE_CXX_COMPILER_VERSION}")
    target_compile_options(_leveldb_thread_analysis PRIVATE -fsyntax-only -Wthread-safety -Werror=thread-safety)
    add_dependencies(_leveldb _leveldb_thread_analysis)
endif()

if(NOT DEFINED AMULET_LEVELDB_EXT_DIR)
    set(AMULET_LEVELDB_EXT_DIR ${amulet_leveldb_DIR})
endif()

# Install
install(TARGETS leveldb DESTINATION ${amulet_leveldb_DIR})
install(TARGETS _leveldb DESTINATION ${AMULET_LEVELDB_EXT_DIR})
INSTALL (
    DIRECTORY ${leveldb_SOURCE_DIR}/include/
    DESTINATION ${amulet_leveldb_DIR}/include
)

if (BUILD_AMULET_LEVELDB_TESTS)
    add_subdirectory(tests)
endif()
