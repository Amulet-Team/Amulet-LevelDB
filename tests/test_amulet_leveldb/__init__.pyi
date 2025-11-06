from __future__ import annotations

import faulthandler as _faulthandler

from . import _test_amulet_leveldb

__all__: list[str] = ["compiler_config"]

def _init() -> None: ...

compiler_config: dict
