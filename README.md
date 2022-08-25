# Amulet LevelDB

A Cython wrapper for Mojang's modified LevelDB library.


## Install
`pip install amulet-leveldb`

## Use
```py
from leveldb import LevelDB

create_if_missing = True  # optional input. Default False.
db = LevelDB("path/to/db", create_if_missing)
db.put(b"key", b"value")
print(db.get(b"key"))
# b"value"
```

See the [source code](src/leveldb/_leveldb.pyx) for full documentation.
