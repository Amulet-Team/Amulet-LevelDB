import sys
import subprocess
import os
import shutil

import pybind11
import pybind11_extensions


def fix_path(path: str) -> str:
    return path.replace(os.sep, "/")


RootDir = fix_path(os.path.dirname(os.path.dirname(__file__)))


def main():
    platform_args = []
    if sys.platform == "win32":
        platform_args.extend(["-G", "Visual Studio 17 2022"])
        if sys.maxsize > 2**32:
            platform_args.extend(["-A", "x64"])
        else:
            platform_args.extend(["-A", "Win32"])
        platform_args.extend(["-T", "v143"])

    os.chdir(RootDir)
    shutil.rmtree("build/CMakeFiles", ignore_errors=True)

    if subprocess.run(
        [
            "cmake",
            *platform_args,
            f"-DPYTHON_EXECUTABLE={sys.executable}",
            f"-Dpybind11_DIR={pybind11.get_cmake_dir().replace(os.sep, '/')}",
            f"-Dpybind11_extensions_DIR={pybind11_extensions.__path__[0].replace(os.sep, '/')}",
            f"-DCMAKE_INSTALL_PREFIX=install",
            f"-DSRC_INSTALL_DIR=src",
            "-B",
            "build",
        ]
    ).returncode:
        raise RuntimeError("Error configuring amulet_core")


if __name__ == "__main__":
    main()
