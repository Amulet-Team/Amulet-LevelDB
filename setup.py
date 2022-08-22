import sys
from setuptools import setup, Extension
from Cython.Build import cythonize
import versioneer

if sys.platform == "win32":
    define_macros = [
        ("WIN32", None),
        ("_WIN32_WINNT", "0x0601"),
        ("LEVELDB_PLATFORM_WINDOWS", None),
        ("DLLX", "__declspec(dllexport)")
    ]
elif sys.platform == "linux":
    define_macros = [
        ("LEVELDB_PLATFORM_POSIX", None),
        ("DLLX", "")
    ]
elif sys.platform == "darwin":
    define_macros = [
        ("LEVELDB_PLATFORM_POSIX", None),
        ("OS_MACOSX", None),
        ("DLLX", "")
    ]
else:
    raise Exception("Unsupported platform")


setup(
    version=versioneer.get_version(),
    cmdclass=versioneer.get_cmdclass(),
    ext_modules=cythonize(
        Extension(
            name="leveldb._leveldb",
            sources=[
                # cython
                "./leveldb/_leveldb.pyx",

                # leveldb
                "./leveldb_mcpe/db/builder.cc",
                "./leveldb_mcpe/db/c.cc",
                "./leveldb_mcpe/db/db_impl.cc",
                "./leveldb_mcpe/db/db_iter.cc",
                "./leveldb_mcpe/db/dbformat.cc",
                "./leveldb_mcpe/db/filename.cc",
                "./leveldb_mcpe/db/log_reader.cc",
                "./leveldb_mcpe/db/log_writer.cc",
                "./leveldb_mcpe/db/memtable.cc",
                "./leveldb_mcpe/db/repair.cc",
                "./leveldb_mcpe/db/table_cache.cc",
                "./leveldb_mcpe/db/version_edit.cc",
                "./leveldb_mcpe/db/version_set.cc",
                "./leveldb_mcpe/db/write_batch.cc",
                "./leveldb_mcpe/db/zlib_compressor.cc",
                "./leveldb_mcpe/db/zstd_compressor.cc",

                "./leveldb_mcpe/table/block.cc",
                "./leveldb_mcpe/table/block_builder.cc",
                "./leveldb_mcpe/table/filter_block.cc",
                "./leveldb_mcpe/table/format.cc",
                "./leveldb_mcpe/table/iterator.cc",
                "./leveldb_mcpe/table/merger.cc",
                "./leveldb_mcpe/table/table.cc",
                "./leveldb_mcpe/table/table_builder.cc",
                "./leveldb_mcpe/table/two_level_iterator.cc",

                "./leveldb_mcpe/util/arena.cc",
                "./leveldb_mcpe/util/bloom.cc",
                "./leveldb_mcpe/util/cache.cc",
                "./leveldb_mcpe/util/coding.cc",
                "./leveldb_mcpe/util/comparator.cc",
                "./leveldb_mcpe/util/crc32c.cc",
                "./leveldb_mcpe/util/env.cc",
                # "./leveldb_mcpe/util/env_boost.cc",
                "./leveldb_mcpe/util/env_posix.cc",
                "./leveldb_mcpe/util/env_win.cc",
                "./leveldb_mcpe/util/filter_policy.cc",
                "./leveldb_mcpe/util/hash.cc",
                "./leveldb_mcpe/util/histogram.cc",
                "./leveldb_mcpe/util/logging.cc",
                "./leveldb_mcpe/util/options.cc",
                "./leveldb_mcpe/util/status.cc",
                "./leveldb_mcpe/util/win_logger.cc",

                "./leveldb_mcpe/port/port_posix.cc",
                "./leveldb_mcpe/port/port_posix_sse.cc",
                "./leveldb_mcpe/port/port_win.cc",
            ],
            include_dirs=[
                "zlib",
                "leveldb_mcpe/include",
                "leveldb_mcpe",
            ],
            language="c++",
            # libraries=["stdc++"],
            define_macros=define_macros
        ),
        language_level=3,
    ),
    package_data={
        "leveldb": [
            "*.pyi",
            "py.typed",
        ]
    }
)
