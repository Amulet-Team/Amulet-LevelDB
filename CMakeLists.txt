cmake_minimum_required(VERSION 3.13)

project(amulet_leveldb LANGUAGES CXX)
set(SRC_INSTALL_DIR "." CACHE PATH "The path to the src directory to install into.")

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

# Add pybind11
find_package(pybind11 CONFIG REQUIRED)
find_package(pybind11_extensions CONFIG REQUIRED)

# Add implementation
FetchContent_Declare(
  leveldb_mcpe
  GIT_REPOSITORY https://github.com/Amulet-Team/leveldb-mcpe.git
  GIT_TAG        4846fc72c7eda860b1bcf6efc58920a9273da928
)
FetchContent_MakeAvailable(leveldb_mcpe)

# Add python extension
pybind11_add_module(_leveldb src/leveldb/__init__leveldb.py.cpp)
target_compile_definitions(_leveldb PRIVATE PYBIND11_DETAILED_ERROR_MESSAGES)
target_compile_definitions(_leveldb PRIVATE PYBIND11_VERSION="${pybind11_VERSION}")
target_compile_definitions(_leveldb PRIVATE COMPILER_ID="${CMAKE_CXX_COMPILER_ID}")
target_compile_definitions(_leveldb PRIVATE COMPILER_VERSION="${CMAKE_CXX_COMPILER_VERSION}")
if (WIN32)
    target_compile_definitions(_leveldb PRIVATE "DLLX=__declspec(dllimport)")
else()
    target_compile_definitions(_leveldb PRIVATE "DLLX=")
endif()
target_link_libraries( _leveldb PRIVATE leveldb_mcpe )
target_link_libraries( _leveldb PRIVATE pybind11_extensions )
target_include_directories(_leveldb PRIVATE src/leveldb/include)

# Install
install(TARGETS leveldb_mcpe DESTINATION "${SRC_INSTALL_DIR}/leveldb")
install(TARGETS _leveldb DESTINATION "${SRC_INSTALL_DIR}/leveldb")
INSTALL (
    DIRECTORY ${leveldb_mcpe_SOURCE_DIR}/include/
    DESTINATION ${SRC_INSTALL_DIR}/leveldb/include
)
