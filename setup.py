import sys
import os

from setuptools import setup, Extension
from Cython.Build import cythonize
import versioneer

sys.path.append(os.path.join(os.path.dirname(__file__), "build_tools"))

import include_binary

cmdclass = versioneer.get_cmdclass()
include_binary.register(cmdclass)

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

library_dirs = [os.path.join("bin", include_binary.get_bin_dir())]


setup(
    version=versioneer.get_version(),
    cmdclass=cmdclass,
    ext_modules=cythonize(
        Extension(
            name="leveldb._leveldb",
            sources=["./leveldb/_leveldb.pyx"],
            include_dirs=[
                "leveldb_mcpe",
                "leveldb_mcpe/include",
            ],
            language="c++",
            library_dirs=library_dirs,
            libraries=["leveldb"],
            define_macros=define_macros,
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
