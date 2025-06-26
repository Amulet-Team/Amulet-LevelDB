if (NOT TARGET leveldb_mcpe)
    message(STATUS "Finding leveldb_mcpe")

    set(leveldb_mcpe_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")
    find_library(leveldb_mcpe_LIBRARY NAMES leveldb_mcpe PATHS "${CMAKE_CURRENT_LIST_DIR}")
    message(STATUS "leveldb_mcpe_LIBRARY: ${leveldb_mcpe_LIBRARY}")

    add_library(leveldb_mcpe_bin SHARED IMPORTED)
    set_target_properties(leveldb_mcpe_bin PROPERTIES
        IMPORTED_IMPLIB "${leveldb_mcpe_LIBRARY}"
    )

    add_library(leveldb_mcpe INTERFACE)
    target_link_libraries(leveldb_mcpe INTERFACE leveldb_mcpe_bin)
    target_include_directories(leveldb_mcpe INTERFACE ${leveldb_mcpe_INCLUDE_DIR})
    if (WIN32)
        target_compile_definitions(leveldb_mcpe INTERFACE "DLLX=__declspec(dllimport)")
    else()
        target_compile_definitions(leveldb_mcpe INTERFACE "DLLX=")
    endif()
endif()
