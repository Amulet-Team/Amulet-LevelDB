cmake_minimum_required(VERSION 3.13)

project(amulet_leveldb LANGUAGES CXX)

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
else()
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

if (MSVC)
    add_definitions("/MP")
endif()

include(FetchContent)

# Add zlib
set(ZLIB_BUILD_EXAMPLES OFF)
FetchContent_Declare(
  zlib
  GIT_REPOSITORY https://github.com/madler/zlib.git
  GIT_TAG        51b7f2abdade71cd9bb0e7a373ef2610ec6f9daf
)
FetchContent_MakeAvailable(zlib)
set(ZLIB_ROOT ${zlib_SOURCE_DIR})
set(ZLIB_USE_STATIC_LIBS ON)
set(ZLIB_LIBRARY ${INSTALL_LIB_DIR})
find_package(ZLIB REQUIRED)
target_compile_definitions(zlibstatic PUBLIC ZLIB_CONST)

# Find libraries
find_package(pybind11 CONFIG REQUIRED)
find_package(amulet_pybind11_extensions CONFIG REQUIRED)

# Add implementation
FetchContent_Declare(
  leveldb_mcpe
  GIT_REPOSITORY https://github.com/Amulet-Team/leveldb-mcpe.git
  GIT_TAG        4846fc72c7eda860b1bcf6efc58920a9273da928
)
FetchContent_MakeAvailable(leveldb_mcpe)

# Find C++ files
file(REAL_PATH src SOURCE_PATH)
file(GLOB_RECURSE EXTENSION_SOURCES LIST_DIRECTORIES false ${SOURCE_PATH}/amulet/*.py.cpp)
file(GLOB_RECURSE EXTENSION_HEADERS LIST_DIRECTORIES false ${SOURCE_PATH}/amulet/*.py.hpp)

# Add python extension
pybind11_add_module(_leveldb)
target_link_libraries(_leveldb PRIVATE amulet_pybind11_extensions)
target_link_libraries(_leveldb PRIVATE leveldb_mcpe)
target_compile_definitions(_leveldb PRIVATE PYBIND11_DETAILED_ERROR_MESSAGES)
target_compile_definitions(_leveldb PRIVATE PYBIND11_VERSION="${pybind11_VERSION}")
target_compile_definitions(_leveldb PRIVATE COMPILER_ID="${CMAKE_CXX_COMPILER_ID}")
target_compile_definitions(_leveldb PRIVATE COMPILER_VERSION="${CMAKE_CXX_COMPILER_VERSION}")
if (WIN32)
    target_compile_definitions(_leveldb PRIVATE "DLLX=__declspec(dllimport)")
else()
    target_compile_definitions(_leveldb PRIVATE "DLLX=")
endif()
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
install(TARGETS leveldb_mcpe DESTINATION ${amulet_leveldb_DIR})
install(TARGETS _leveldb DESTINATION ${AMULET_LEVELDB_EXT_DIR})
INSTALL (
    DIRECTORY ${leveldb_mcpe_SOURCE_DIR}/include/
    DESTINATION ${amulet_leveldb_DIR}/include
)

if (DEFINED BUILD_AMULET_LEVELDB_TESTS)
    add_subdirectory(tests)
endif()
