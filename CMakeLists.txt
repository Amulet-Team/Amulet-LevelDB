cmake_minimum_required(VERSION 3.13)

project(amulet_leveldb LANGUAGES CXX)

set(amulet_leveldb_DIR ${CMAKE_CURRENT_LIST_DIR}/src/amulet/leveldb CACHE PATH "")
set(BUILD_AMULET_LEVELDB_TESTS OFF CACHE BOOL "Should tests be built?")

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

# Find libraries
if (NOT TARGET pybind11::module)
    find_package(pybind11 CONFIG REQUIRED)
endif()
if (NOT TARGET amulet_pybind11_extensions)
    find_package(amulet_pybind11_extensions CONFIG REQUIRED)
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
target_include_directories(leveldb PUBLIC "${amulet_leveldb_DIR}/include")


# Find C++ files
file(REAL_PATH src SOURCE_PATH)
file(GLOB_RECURSE EXTENSION_SOURCES LIST_DIRECTORIES false ${SOURCE_PATH}/amulet/*.py.cpp)
file(GLOB_RECURSE EXTENSION_HEADERS LIST_DIRECTORIES false ${SOURCE_PATH}/amulet/*.py.hpp)

# Extend the leveldb binary
target_sources(leveldb PRIVATE ${SOURCE_PATH}/amulet/leveldb/leveldb.cpp)

# Add python extension
pybind11_add_module(_leveldb)
target_link_libraries(_leveldb PRIVATE amulet_pybind11_extensions)
target_link_libraries(_leveldb PRIVATE leveldb)
target_compile_definitions(_leveldb PRIVATE PYBIND11_DETAILED_ERROR_MESSAGES)
target_compile_definitions(_leveldb PRIVATE PYBIND11_VERSION="${pybind11_VERSION}")
target_compile_definitions(_leveldb PRIVATE COMPILER_ID="${CMAKE_CXX_COMPILER_ID}")
target_compile_definitions(_leveldb PRIVATE COMPILER_VERSION="${CMAKE_CXX_COMPILER_VERSION}")
target_include_directories(_leveldb PRIVATE "${amulet_leveldb_DIR}/include")
target_sources(_leveldb PRIVATE ${EXTENSION_SOURCES} ${EXTENSION_HEADERS})
foreach(FILE ${EXTENSION_SOURCES} ${EXTENSION_HEADERS})
    file(RELATIVE_PATH REL_PATH ${SOURCE_PATH} ${FILE})
    get_filename_component(GROUP ${REL_PATH} DIRECTORY)
    string(REPLACE "/" "\\" GROUP ${GROUP})
    source_group(${GROUP} FILES ${FILE})
endforeach()

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
