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

# Add pybind11
find_package(pybind11 CONFIG REQUIRED)
find_package(pybind11_extensions CONFIG REQUIRED)

# Add zlib fallback
set(ZLIB_BUILD_EXAMPLES OFF)
add_subdirectory(submodules/zlib)
set(ZLIB_ROOT submodules/zlib)
set(ZLIB_USE_STATIC_LIBS ON)
set(ZLIB_LIBRARY ${INSTALL_LIB_DIR})
find_package(ZLIB REQUIRED)

# Add implementation
add_subdirectory(submodules/leveldb-mcpe)

# Add python extension
pybind11_add_module(_leveldb src/leveldb/__init__leveldb.py.cpp)
target_compile_definitions(_leveldb PRIVATE PYBIND11_DETAILED_ERROR_MESSAGES)
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
