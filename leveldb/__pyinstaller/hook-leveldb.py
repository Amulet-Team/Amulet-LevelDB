import sys
import os

import leveldb


LEVELDB_PATH = leveldb.__path__[0]


if sys.platform == "linux":
    binaries = [
        (
            os.path.join(
                LEVELDB_PATH, "leveldb_mcpe_linux_x86_64.so"
            ),
            "leveldb",
        ),
    ]
elif sys.platform == "win32":
    if sys.maxsize > 2**32:  # 64 bit python
        binaries = [
            (
                os.path.join(
                    LEVELDB_PATH, "leveldb_mcpe_win_amd64.dll"
                ),
                "leveldb",
            ),
        ]
    else:
        binaries = [
            (
                os.path.join(LEVELDB_PATH, "leveldb_mcpe_win32.dll"),
                "leveldb",
            )
        ]

elif sys.platform == "darwin":
    binaries = [
        (
            os.path.join(
                LEVELDB_PATH, "leveldb_mcpe_macosx_10_9_x86_64.dylib"
            ),
            "leveldb",
        ),
    ]
else:
    raise Exception(f"Unsupported platform {sys.platform}")
