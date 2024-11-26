if (NOT TARGET leveldb_mcpe)
    message(STATUS "Finding leveldb_mcpe")

    set(leveldb_mcpe_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")
    find_library(leveldb_mcpe_LIBRARY NAMES leveldb_mcpe PATHS "${CMAKE_CURRENT_LIST_DIR}")
    message(STATUS "leveldb_mcpe_LIBRARY: ${leveldb_mcpe_LIBRARY}")

    add_library(leveldb_mcpe SHARED IMPORTED)
    set_target_properties(leveldb_mcpe PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${leveldb_mcpe_INCLUDE_DIR}"
        IMPORTED_IMPLIB "${leveldb_mcpe_LIBRARY}"
    )
endif()
