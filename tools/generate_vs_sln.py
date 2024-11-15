"""This generates a Visual Studio solution file and projects for each module."""

from __future__ import annotations
import os
import pybind11
import pybind11_extensions
from shared.generate_vs_sln import ProjectData, CompileMode, get_files, PythonIncludeDir, PythonLibraryDir, write

RootDir = os.path.dirname(os.path.dirname(__file__))
SrcDir = os.path.join(RootDir, "src")


def main() -> None:
    zlib_path = os.path.join(RootDir, "submodules", "zlib")
    leveldb_mcpe_path = os.path.join(RootDir, "submodules", "leveldb-mcpe")
    leveldb_lib = ProjectData(
        name="leveldb-mcpe",
        compile_mode=CompileMode.StaticLibrary,
        include_dirs=[
            zlib_path,
            leveldb_mcpe_path,
            os.path.join(leveldb_mcpe_path, "include"),
        ],
        preprocessor_definitions=[
            "WIN32",
            "_WIN32_WINNT=0x0601",
            "LEVELDB_PLATFORM_WINDOWS",
            "DLLX=__declspec(dllexport)",
        ],
        include_files=get_files(
            root_dir=leveldb_mcpe_path, root_dir_suffix="include", ext="h"
        ),
        source_files=[
            (leveldb_mcpe_path, "db", "builder.cc"),
            (leveldb_mcpe_path, "db", "c.cc"),
            (leveldb_mcpe_path, "db", "db_impl.cc"),
            (leveldb_mcpe_path, "db", "db_iter.cc"),
            (leveldb_mcpe_path, "db", "dbformat.cc"),
            (leveldb_mcpe_path, "db", "filename.cc"),
            (leveldb_mcpe_path, "db", "log_reader.cc"),
            (leveldb_mcpe_path, "db", "log_writer.cc"),
            (leveldb_mcpe_path, "db", "memtable.cc"),
            (leveldb_mcpe_path, "db", "repair.cc"),
            (leveldb_mcpe_path, "db", "table_cache.cc"),
            (leveldb_mcpe_path, "db", "version_edit.cc"),
            (leveldb_mcpe_path, "db", "version_set.cc"),
            (leveldb_mcpe_path, "db", "write_batch.cc"),
            (leveldb_mcpe_path, "table", "block.cc"),
            (leveldb_mcpe_path, "table", "block_builder.cc"),
            (leveldb_mcpe_path, "table", "filter_block.cc"),
            (leveldb_mcpe_path, "table", "format.cc"),
            (leveldb_mcpe_path, "table", "iterator.cc"),
            (leveldb_mcpe_path, "table", "merger.cc"),
            (leveldb_mcpe_path, "table", "table.cc"),
            (leveldb_mcpe_path, "table", "table_builder.cc"),
            (leveldb_mcpe_path, "table", "two_level_iterator.cc"),
            (leveldb_mcpe_path, "util", "arena.cc"),
            (leveldb_mcpe_path, "util", "bloom.cc"),
            (leveldb_mcpe_path, "util", "cache.cc"),
            (leveldb_mcpe_path, "util", "coding.cc"),
            (leveldb_mcpe_path, "util", "comparator.cc"),
            (leveldb_mcpe_path, "util", "crc32c.cc"),
            (leveldb_mcpe_path, "util", "env.cc"),
            (leveldb_mcpe_path, "util", "filter_policy.cc"),
            (leveldb_mcpe_path, "util", "hash.cc"),
            (leveldb_mcpe_path, "util", "histogram.cc"),
            (leveldb_mcpe_path, "util", "logging.cc"),
            (leveldb_mcpe_path, "util", "options.cc"),
            (leveldb_mcpe_path, "util", "status.cc"),
            (leveldb_mcpe_path, "db", "zlib_compressor.cc"),
            (leveldb_mcpe_path, "db", "zstd_compressor.cc"),
            (leveldb_mcpe_path, "port", "port_posix_sse.cc"),
            (leveldb_mcpe_path, "port", "port_win.cc"),
            (leveldb_mcpe_path, "util", "env_win.cc"),
            (leveldb_mcpe_path, "util", "win_logger.cc"),
        ]
    )
    leveldb_path = os.path.join(SrcDir, "leveldb")
    leveldb_py = ProjectData(
        name="__init__",
        compile_mode=CompileMode.PythonExtension,
        source_files=get_files(
            root_dir=leveldb_path,
            ext="cpp",
        ),
        include_files=get_files(
            root_dir=leveldb_path, ext="hpp"
        ),
        include_dirs=[
            PythonIncludeDir,
            pybind11.get_include(),
            pybind11_extensions.get_include(),
            leveldb_mcpe_path,
            os.path.join(leveldb_mcpe_path, "include"),
            SrcDir,
        ],
        preprocessor_definitions=[
            "WIN32",
            "_WIN32_WINNT=0x0601",
            "LEVELDB_PLATFORM_WINDOWS",
            "DLLX=__declspec(dllexport)",
        ],
        library_dirs=[
            PythonLibraryDir,
            os.path.join(RootDir, "bin", "zlib", "win64")
        ],
        dependencies=[
            leveldb_lib,
            "zlibstatic",
        ],
        py_package="leveldb",
        package_dir=os.path.dirname(leveldb_path),
    )
    projects = [
        leveldb_lib,
        leveldb_py,
    ]

    write(
        SrcDir,
        os.path.join(SrcDir, "sln"),
        "leveldb",
        projects,
    )


if __name__ == "__main__":
    main()
