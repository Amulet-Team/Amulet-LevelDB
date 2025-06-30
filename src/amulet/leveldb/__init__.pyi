from __future__ import annotations

import collections as collections
import typing

from . import _leveldb, _version

__all__: list = ["LevelDB", "LevelDBEncrypted", "LevelDBException", "LevelDBIterator"]

class CompressionType:
    """
    Members:

      NoCompression : No compression.

      ZstdCompression : Zstd compression.

      ZlibRawCompression : Zlib raw compression.
    """

    NoCompression: typing.ClassVar[
        CompressionType
    ]  # value = amulet.leveldb.CompressionType.NoCompression
    ZlibRawCompression: typing.ClassVar[
        CompressionType
    ]  # value = amulet.leveldb.CompressionType.ZlibRawCompression
    ZstdCompression: typing.ClassVar[
        CompressionType
    ]  # value = amulet.leveldb.CompressionType.ZstdCompression
    __members__: typing.ClassVar[
        dict[str, CompressionType]
    ]  # value = {'NoCompression': amulet.leveldb.CompressionType.NoCompression, 'ZstdCompression': amulet.leveldb.CompressionType.ZstdCompression, 'ZlibRawCompression': amulet.leveldb.CompressionType.ZlibRawCompression}
    def __eq__(self, other: typing.Any) -> bool: ...
    def __hash__(self) -> int: ...
    def __index__(self) -> int: ...
    def __init__(self, value: int) -> None: ...
    def __int__(self) -> int: ...
    def __ne__(self, other: typing.Any) -> bool: ...
    def __repr__(self) -> str: ...
    def __str__(self) -> str: ...
    @property
    def name(self) -> str: ...
    @property
    def value(self) -> int: ...

class LevelDB:
    """
    A LevelDB database
    """

    def __contains__(self, key: bytes) -> bool: ...
    def __delitem__(self, key: bytes) -> None: ...
    def __getitem__(self, key: bytes) -> bytes: ...
    def __init__(
        self,
        path: str,
        create_if_missing: bool = False,
        compression_type: CompressionType = ...,
    ) -> None:
        """
        Construct a new :class :`LevelDB` instance from the database at the given path.

        A leveldb database is like a dictionary that only contains bytes as the keys and values and exists entirely on the disk.

        :param path: The path to the database directory.
        :param create_if_missing: If True a new database will be created if one does not exist at the given path.
        :param compression_type: The compression type to use when writing data to the database. Defaults to zlib raw.
        :raises: LevelDBException if create_if_missing is False and the db does not exist.
        """

    def __iter__(self) -> collections.abc.Iterator[bytes]: ...
    def __setitem__(self, key: bytes, value: bytes) -> None: ...
    def close(self) -> None:
        """
        Close the leveldb database.
        Only the owner of the database may close it.
        If needed, an external lock must be used to ensure that no other threads are accessing the database.
        """

    def compact(self) -> None:
        """
        Remove deleted entries from the database to reduce its size.
        """

    def create_iterator(self) -> LevelDBIterator:
        """
        Create a new leveldb Iterator.
        """

    def delete(self, key: bytes) -> None:
        """
        Delete a key from the database.

        :param key: The key to delete from the database.
        """

    def get(self, key: bytes) -> bytes:
        """
        Get a key from the database.

        :param key: The key to get from the database.
        :return: The data stored behind the given key.
        :raises: KeyError if the requested key is not present.
        :raises: LevelDBException on other error.
        """

    def items(self) -> collections.abc.Iterator[tuple[bytes, bytes]]:
        """
        An iterable of all items in the database.
        """

    def iterate(
        self, start: bytes | None = None, end: bytes | None = None
    ) -> collections.abc.Iterator[tuple[bytes, bytes]]:
        """
        Iterate through all keys and data that exist between the given keys.

        :param start: The key to start at. Leave as None to start at the beginning.
        :param end: The key to end at. Leave as None to finish at the end.
        """

    def keys(self) -> collections.abc.Iterator[bytes]:
        """
        An iterable of all keys in the database.
        """

    def put(self, key: bytes, value: bytes) -> None:
        """
        Set a value in the database.
        """

    def put_batch(self, batch: collections.abc.Mapping[bytes, bytes]) -> None:
        """
        Set a group of values in the database.
        """

    def values(self) -> collections.abc.Iterator[bytes]:
        """
        An iterable of all values in the database.
        """

class LevelDBEncrypted(Exception):
    pass

class LevelDBException(Exception):
    pass

class LevelDBIterator:
    def key(self) -> bytes:
        """
        Get the key of the current entry in the database.
        :raises: runtime_error if iterator is not valid.
        """

    def next(self) -> None:
        """
        Seek to the next entry in the database.
        """

    def prev(self) -> None:
        """
        Seek to the previous entry in the database.
        """

    def seek(self, target: bytes) -> None:
        """
        Seek to the given entry in the database.
        If the entry does not exist it will seek to the location after.
        """

    def seek_to_first(self) -> None:
        """
        Seek to the first entry in the database.
        """

    def seek_to_last(self) -> None:
        """
        Seek to the last entry in the database.
        """

    def valid(self) -> bool:
        """
        Is the iterator at a valid entry.
        If False, calls to other methods may error.
        """

    def value(self) -> bytes:
        """
        Get the value of the current entry in the database.
        :raises: runtime_error if iterator is not valid.
        """

def _init() -> None: ...

__version__: str
compiler_config: dict
