import logging as _logging

from . import _version

__version__ = _version.get_versions()["version"]

# init a default logger
_logging.basicConfig(level=_logging.INFO, format="%(levelname)s - %(message)s")


def _init() -> None:
    import os
    import sys
    import ctypes

    if os.environ.get("AMULET_SKIP_COMPILE", None):
        return

    if sys.platform == "win32":
        lib_path = os.path.join(os.path.dirname(__file__), "leveldb.dll")
    elif sys.platform == "darwin":
        lib_path = os.path.join(os.path.dirname(__file__), "libleveldb.dylib")
    elif sys.platform == "linux":
        lib_path = os.path.join(os.path.dirname(__file__), "libleveldb.so")
    else:
        raise RuntimeError(f"Unsupported platform {sys.platform}")

    # Load the shared library
    ctypes.cdll.LoadLibrary(lib_path)

    from ._leveldb import init

    init(sys.modules[__name__])


_init()
del _init
