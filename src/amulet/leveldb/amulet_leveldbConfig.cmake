if (NOT TARGET leveldb)
    message(STATUS "Finding leveldb")

    set(leveldb_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")
    find_library(leveldb_LIBRARY NAMES leveldb PATHS "${CMAKE_CURRENT_LIST_DIR}")
    message(STATUS "leveldb_LIBRARY: ${leveldb_LIBRARY}")

    add_library(leveldb_bin SHARED IMPORTED)
    set_target_properties(leveldb_bin PROPERTIES
        IMPORTED_IMPLIB "${leveldb_LIBRARY}"
    )

    add_library(leveldb INTERFACE)
    target_link_libraries(leveldb INTERFACE leveldb_bin)
    target_include_directories(leveldb INTERFACE ${leveldb_INCLUDE_DIR})
endif()
