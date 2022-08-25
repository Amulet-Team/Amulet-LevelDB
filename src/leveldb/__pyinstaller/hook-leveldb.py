import sys
import os

from src import leveldb

LEVELDB_PATH = leveldb.__path__[0]


if sys.platform == "linux":
    dll = "libleveldb.so"
elif sys.platform == "win32":
    dll = "leveldb.dll"
elif sys.platform == "darwin":
    dll = "libleveldb.dylib"
else:
    raise Exception(f"Unsupported platform {sys.platform}")

binaries = [
    (
        os.path.join(
            LEVELDB_PATH, dll
        ),
        "leveldb",
    ),
]
