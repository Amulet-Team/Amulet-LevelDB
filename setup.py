import os
import subprocess
import sys
import typing
from pathlib import Path

from setuptools import setup, Extension, Command
from setuptools.command.build_ext import build_ext

import versioneer


def fix_path(path: str | os.PathLike[typing.AnyStr]) -> str:
    return os.path.realpath(path).replace(os.sep, "/")


dependencies = [
    "amulet-compiler-target==1.0",
]
setup_args = {}

try:
    import amulet_compiler_version
except ImportError:
    dependencies.append("amulet-compiler-version==1.3.0")
else:
    dependencies.append(
        f"amulet-compiler-version=={amulet_compiler_version.__version__}"
    )
    setup_args["options"] = {
        "bdist_wheel": {
            "build_number": f"1.{amulet_compiler_version.compiler_id}.{amulet_compiler_version.compiler_version}"
        }
    }


cmdclass: dict[str, type[Command]] = versioneer.get_cmdclass()


class CMakeBuild(cmdclass.get("build_ext", build_ext)):
    def build_extension(self, ext):
        import pybind11
        import amulet.pybind11_extensions

        ext_dir = (
            (Path.cwd() / self.get_ext_fullpath("")).parent.resolve()
            / "amulet"
            / "leveldb"
        )
        leveldb_src_dir = (
            Path.cwd() / "src" / "amulet" / "leveldb" if self.editable_mode else ext_dir
        )

        platform_args = []
        if sys.platform == "win32":
            platform_args.extend(["-G", "Visual Studio 17 2022"])
            if sys.maxsize > 2**32:
                platform_args.extend(["-A", "x64"])
            else:
                platform_args.extend(["-A", "Win32"])
            platform_args.extend(["-T", "v143"])

        if subprocess.run(["cmake", "--version"]).returncode:
            raise RuntimeError("Could not find cmake")
        if subprocess.run(
            [
                "cmake",
                *platform_args,
                f"-DPYTHON_EXECUTABLE={sys.executable}",
                f"-Dpybind11_DIR={pybind11.get_cmake_dir().replace(os.sep, '/')}",
                f"-Damulet_pybind11_extensions_DIR={fix_path(amulet.pybind11_extensions.__path__[0])}",
                f"-Damulet_leveldb_DIR={fix_path(leveldb_src_dir)}",
                f"-DAMULET_LEVELDB_EXT_DIR={fix_path(ext_dir)}",
                f"-DCMAKE_INSTALL_PREFIX=install",
                "-B",
                "build",
            ]
        ).returncode:
            raise RuntimeError("Error configuring amulet_leveldb")
        if subprocess.run(
            ["cmake", "--build", "build", "--config", "Release"]
        ).returncode:
            raise RuntimeError("Error building amulet_leveldb")
        if subprocess.run(
            ["cmake", "--install", "build", "--config", "Release"]
        ).returncode:
            raise RuntimeError("Error installing amulet_leveldb")


cmdclass["build_ext"] = CMakeBuild


setup(
    version=versioneer.get_version(),
    cmdclass=cmdclass,
    ext_modules=[Extension("amulet.leveldb._leveldb", [])],
    install_requires=dependencies,
    **setup_args,
)
