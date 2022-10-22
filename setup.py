import sys
import os

from setuptools import setup, Extension
from Cython.Build import cythonize

sys.path.append(os.path.dirname(__file__))
import versioneer
sys.path.remove(os.path.dirname(__file__))

define_macros = []
extra_compile_args = []
extra_link_args = []
libraries = []
extra_objects = []

if sys.platform == "win32":
    define_macros = [
        ("WIN32", None),
        ("_WIN32_WINNT", "0x0601"),
        ("LEVELDB_PLATFORM_WINDOWS", None),
        ("DLLX", "__declspec(dllexport)")
    ]
    if sys.maxsize > 2 ** 32:  # 64 bit python
        extra_objects = ["bin/zlib/win64/zlibstatic.lib"]
    else:  # 32 bit python
        extra_objects = ["bin/zlib/win32/zlibstatic.lib"]
elif sys.platform == "linux":
    define_macros = [
        ("LEVELDB_PLATFORM_POSIX", None),
        ("DLLX", "")
    ]
    libraries = ["z"]
elif sys.platform == "darwin":
    define_macros = [
        ("LEVELDB_PLATFORM_POSIX", None),
        ("OS_MACOSX", None),
        ("DLLX", "")
    ]
    libraries = ["z"]
    # shared_mutex needs MacOS 10.12+
    extra_compile_args = ["-mmacosx-version-min=10.12", "-Werror=partial-availability"]
    extra_link_args = ["-Wl,-no_weak_imports"]
else:
    raise Exception("Unsupported platform")


setup(
    version=versioneer.get_version(),
    cmdclass=versioneer.get_cmdclass(),
    ext_modules=cythonize(
        Extension(
            name="leveldb._leveldb",
            sources=[
                "./src/leveldb/_leveldb.pyx",
                "./leveldb-mcpe/db/builder.cc",
                "./leveldb-mcpe/db/c.cc",
                "./leveldb-mcpe/db/db_impl.cc",
                "./leveldb-mcpe/db/db_iter.cc",
                "./leveldb-mcpe/db/dbformat.cc",
                "./leveldb-mcpe/db/filename.cc",
                "./leveldb-mcpe/db/log_reader.cc",
                "./leveldb-mcpe/db/log_writer.cc",
                "./leveldb-mcpe/db/memtable.cc",
                "./leveldb-mcpe/db/repair.cc",
                "./leveldb-mcpe/db/table_cache.cc",
                "./leveldb-mcpe/db/version_edit.cc",
                "./leveldb-mcpe/db/version_set.cc",
                "./leveldb-mcpe/db/write_batch.cc",
                "./leveldb-mcpe/db/zlib_compressor.cc",
                "./leveldb-mcpe/db/zstd_compressor.cc",

                "./leveldb-mcpe/table/block.cc",
                "./leveldb-mcpe/table/block_builder.cc",
                "./leveldb-mcpe/table/filter_block.cc",
                "./leveldb-mcpe/table/format.cc",
                "./leveldb-mcpe/table/iterator.cc",
                "./leveldb-mcpe/table/merger.cc",
                "./leveldb-mcpe/table/table.cc",
                "./leveldb-mcpe/table/table_builder.cc",
                "./leveldb-mcpe/table/two_level_iterator.cc",

                "./leveldb-mcpe/util/arena.cc",
                "./leveldb-mcpe/util/bloom.cc",
                "./leveldb-mcpe/util/cache.cc",
                "./leveldb-mcpe/util/coding.cc",
                "./leveldb-mcpe/util/comparator.cc",
                "./leveldb-mcpe/util/crc32c.cc",
                "./leveldb-mcpe/util/env.cc",
                "./leveldb-mcpe/util/env_posix.cc",
                "./leveldb-mcpe/util/env_win.cc",
                "./leveldb-mcpe/util/filter_policy.cc",
                "./leveldb-mcpe/util/hash.cc",
                "./leveldb-mcpe/util/histogram.cc",
                "./leveldb-mcpe/util/logging.cc",
                "./leveldb-mcpe/util/options.cc",
                "./leveldb-mcpe/util/status.cc",
                "./leveldb-mcpe/util/win_logger.cc",

                "./leveldb-mcpe/port/port_posix.cc",
                "./leveldb-mcpe/port/port_posix_sse.cc",
                "./leveldb-mcpe/port/port_win.cc",
            ],
            include_dirs=[
                "zlib",
                "leveldb-mcpe",
                "leveldb-mcpe/include",
            ],
            language="c++",
            extra_compile_args=["-std=c++17", *extra_compile_args],
            extra_link_args=extra_link_args,
            extra_objects=extra_objects,
            libraries=libraries,
            define_macros=define_macros,
        ),
        language_level=3,
    )
)
