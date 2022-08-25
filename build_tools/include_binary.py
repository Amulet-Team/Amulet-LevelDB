import os
import sys
import shutil
from typing import Dict, Type

from setuptools import Command
from setuptools.command.build import build as build_

# Template for how to do this from here https://github.com/abravalheri/experiment-setuptools-plugin


def get_bin_dir() -> str:
    if sys.platform == "win32":
        if sys.maxsize > 2 ** 32:  # 64 bit python
            return "win_amd64"
        else:  # 32 bit python
            return "win_32"
    elif sys.platform == "linux":
        return "linux_x86_64"
    elif sys.platform == "darwin":
        return "macosx_10_9_x86_64"
    else:
        raise Exception("Unsupported platform")


def get_bin_name() -> str:
    return {
        "win32": "leveldb.dll",
        "linux_x86_64": "libleveldb.so",
        "macosx_10_9_x86_64": "libleveldb.dylib",
    }[sys.platform]


def register(cmdclass: Dict[str, Type[Command]]):
    # register a new command class
    cmdclass["include_binary"] = IncludeBinary
    # get the build command class
    build = cmdclass.setdefault("build", build_)
    # register our command class as a subcommand of the build command class
    build.sub_commands.append(("include_binary", None))


class IncludeBinary(Command):
    def initialize_options(self):
        self.build_lib = None

    def finalize_options(self):
        self.set_undefined_options("build_py", ("build_lib", "build_lib"))

    def run(self):
        shutil.copyfile(
            os.path.join(self.build_lib, "bin", get_bin_dir(), get_bin_name()),
            os.path.join(self.build_lib, "leveldb", get_bin_name())
        )
        shutil.rmtree(os.path.join(self.build_lib, "bin"))
