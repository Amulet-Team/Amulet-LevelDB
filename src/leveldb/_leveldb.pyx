# A cython (and python) wrapper for leveldb.

import os
from typing import Dict, Iterator as IteratorT, Optional
from libcpp.string cimport string
from libcpp cimport bool
from libcpp.memory cimport shared_ptr, make_shared
from weakref import WeakSet

from leveldb_mcpe cimport (
    DB,
    Status,
    Options,
    WriteOptions,
    ReadOptions,
    Slice,
    NewBloomFilterPolicy,
    NewLRUCache,
    ZlibCompressor,
    ZlibCompressorRaw,
    DecompressAllocator,
    Logger,
    WriteBatch,
    Iterator as CIterator,
    RepairDB,
)


cdef extern from "<shared_mutex>" namespace "std" nogil:
    cdef cppclass shared_mutex:
        pass
    cdef cppclass shared_lock[T]:
        shared_lock(shared_mutex)

cdef extern from "<mutex>" namespace "std" nogil:
    cdef cppclass unique_lock[T]:
        unique_lock(shared_mutex)


class LevelDBException(Exception):
    """
    The exception thrown for all leveldb related errors.
    """
    pass


class LevelDBEncrypted(LevelDBException):
    pass


class LevelDBIteratorException(LevelDBException):
    """
    The exception thrown for issues related to the iterator.
    """


cdef extern from *:
    """
    class NullLogger : public leveldb::Logger {
    	public:
    		void Logv(const char*, va_list) override {}
    };
    """
    cdef cppclass NullLogger(Logger):
        void Logv(const char *, va_list) nogil except +


cdef inline bint _check_iterator(CIterator *iterator) nogil except -1:
    if iterator is NULL:
        with gil:
            raise LevelDBIteratorException("The iterator has been deleted.")


cdef class Iterator:
    cdef CIterator *iterator
    cdef object __weakref__

    def __init__(self):
        raise RuntimeError("Iterator cannot be created from python")

    @staticmethod
    cdef Iterator wrap(CIterator *iterator):
        cdef Iterator cy_iterator = Iterator.__new__(Iterator)
        cy_iterator.iterator = iterator
        return cy_iterator

    def __del__(self):
        self.destroy()

    cdef void destroy(self) nogil:
        """Destroy the C iterator object."""
        if self.iterator is not NULL:
            del self.iterator
            self.iterator = NULL

    cdef CIterator* get_c_iterator(self) nogil except NULL:
        """Get the C iterator object."""
        _check_iterator(self.iterator)
        return self.iterator

    cpdef bool valid(self):
        """
        Is the iterator at a valid entry.
        If False, calls to other methods may error.
        """
        _check_iterator(self.iterator)
        return self.iterator.Valid()

    cpdef void seek_to_first(self):
        """Seek to the first entry in the database."""
        _check_iterator(self.iterator)
        self.iterator.SeekToFirst()

    cpdef void seek_to_last(self):
        """Seek to the last entry in the database."""
        _check_iterator(self.iterator)
        self.iterator.SeekToLast()

    cpdef void seek(self, string target):
        """
        Seek to the given entry in the database.
        If the entry does not exist it will seek to the location after.
        """
        _check_iterator(self.iterator)
        self.iterator.Seek(Slice(target))

    cpdef void next(self):
        """Seek to the next entry in the database."""
        _check_iterator(self.iterator)
        self.iterator.Next()

    cpdef void prev(self):
        """Seek to the previous entry in the database."""
        _check_iterator(self.iterator)
        self.iterator.Prev()

    cpdef string key(self):
        """
        Get the key of the current entry in the database.
        If valid returns False this will error.
        """
        _check_iterator(self.iterator)
        if not self.iterator.Valid():
            raise LevelDBIteratorException("Iterator is not valid")
        return self.iterator.key().ToString()

    cpdef string value(self):
        """
        Get the value of the current entry in the database.
        If valid returns False this will error.
        """
        _check_iterator(self.iterator)
        if not self.iterator.Valid():
            raise LevelDBIteratorException("Iterator is not valid")
        return self.iterator.value().ToString()


cdef inline bint _check_db(DB *db) nogil except -1:
    if db is NULL:
        with gil:
            raise LevelDBException("The database has been closed.")


cdef class LevelDB:
    cdef DB *db
    cdef ReadOptions read_options
    cdef WriteOptions write_options
    cdef shared_mutex _mutex
    cdef object iterators
    cdef object __weakref__

    def __init__(self, str path, bool create_if_missing = False):
        """
        Construct a new :class:`LevelDB` instance from the database at the given path.

        A leveldb database is like a dictionary that only contains bytes as the keys and values and exists entirely on the disk.

        :param path: The path to the database directory.
        :param create_if_missing: If True and there is no database at the given path a new database will be created.
        :raises: LevelDBException if create_if_missing is False and the db does not exist.
        """
        self.iterators = WeakSet()

        if not os.path.isdir(path):
            if create_if_missing:
                os.makedirs(path)
            else:
                raise LevelDBException(f"No database exists to open at {path}")

        cdef Options options
        options.create_if_missing = create_if_missing
        options.filter_policy = NewBloomFilterPolicy(10)
        options.block_cache = NewLRUCache(40 * 1024 * 1024)
        options.write_buffer_size = 4 * 1024 * 1024
        options.info_log = new NullLogger()
        options.compressors[0] = new ZlibCompressorRaw()
        options.compressors[1] = new ZlibCompressor()
        options.block_size = 163840

        self.read_options.decompress_allocator = new DecompressAllocator()

        cdef string s_path = path.encode()
        cdef Status status
        status = self.db.Open(options, s_path, &self.db)
        if not status.ok():
            msg = status.ToString()
            if status.IsCorruption():
                RepairDB(s_path, options)
                status = self.db.Open(options, s_path, &self.db)
                if not status.ok():
                    raise LevelDBException(f"Could not recover corrupted database. {msg}")
            else:
                if status.IsNotSupportedError() and msg.endswith("Marketplace worlds are not supported."):
                    raise LevelDBEncrypted
                raise LevelDBException(msg)

    cpdef void close(self, unsigned char compact=False) except *:
        """
        Close the leveldb database.

        :param compact: If True will compact the database making it take less memory.
        """
        cdef shared_ptr[unique_lock[shared_mutex]] lock = make_shared[unique_lock[shared_mutex]](self._mutex)
        cdef Iterator iterator
        if self.db is not NULL:
            if compact:
                self.compact()
            for iterator in self.iterators:
                iterator.destroy()
            del self.db
            self.db = NULL

    def __del__(self):
        self.close()

    cpdef void compact(self) except *:
        cdef shared_ptr[shared_lock[shared_mutex]] lock = make_shared[shared_lock[shared_mutex]](self._mutex)
        _check_db(self.db)
        self.db.CompactRange(NULL, NULL)

    cdef void Put(self, string key, string value) nogil except *:
        cdef shared_ptr[shared_lock[shared_mutex]] lock = make_shared[shared_lock[shared_mutex]](self._mutex)
        _check_db(self.db)
        cdef Status status = self.db.Put(self.write_options, Slice(key), Slice(value))
        if not status.ok():
            with gil:
                raise LevelDBException(status.ToString())

    def put(self, string key, string value):
        self.Put(key, value)

    cdef string Get(self, string key) nogil except *:
        """
        Get a key from the database.

        :param key: The key to get from the database.
        :return: The data stored behind the given key.
        :raises: KeyError if the requested key is not present.
        """
        cdef shared_ptr[shared_lock[shared_mutex]] lock = make_shared[shared_lock[shared_mutex]](self._mutex)
        _check_db(self.db)
        cdef string value
        cdef Status status = self.db.Get(self.read_options, Slice(key), &value)
        if not status.ok():
            if status.IsNotFound():
                with gil:
                    raise KeyError(key)
            else:
                with gil:
                    raise LevelDBException(status.ToString())
        return value

    def get(self, string key) -> string:
        return self.Get(key)

    cpdef void putBatch(self, dict data: Dict[bytes, Optional[bytes]]) except *:
        """
        Put one or more key and value pair into the database. Works the same as dict.update

        :param data: A dictionary of keys and values to add to the database
        """
        cdef shared_ptr[shared_lock[shared_mutex]] lock = make_shared[shared_lock[shared_mutex]](self._mutex)
        _check_db(self.db)
        cdef string k, s
        cdef object v
        cdef WriteBatch batch
        for k, v in data.items():
            if v is None:
                batch.Delete(Slice(k))
            elif isinstance(v, bytes):
                s = v
                batch.Put(Slice(k), Slice(s))
            else:
                raise TypeError(f"Expected bytes or None. Got {v.__class__}")
        cdef Status status = self.db.Write(self.write_options, &batch)
        if not status.ok():
            raise LevelDBException(status.ToString())

    cdef void Delete(self, string key) nogil except *:
        """
        Delete a key from the database.

        :param key: The key to delete from the database.
        """
        cdef shared_ptr[shared_lock[shared_mutex]] lock = make_shared[shared_lock[shared_mutex]](self._mutex)
        _check_db(self.db)
        cdef Status status = self.db.Delete(self.write_options, Slice(key))
        if not status.ok():
            raise LevelDBException(status.ToString())

    def delete(self, string key):
        """
        Delete a key from the database.

        :param key: The key to delete from the database.
        """
        self.Delete(key)

    cpdef Iterator new_iterator(self):
        cdef shared_ptr[shared_lock[shared_mutex]] lock = make_shared[shared_lock[shared_mutex]](self._mutex)
        _check_db(self.db)
        cdef CIterator *c_iterator = self.db.NewIterator(self.read_options)
        cdef Iterator iterator = Iterator.wrap(c_iterator)
        self.iterators.add(iterator)
        return iterator

    def iterate(
        self, bytes start = None, bytes end = None
    ):  # -> IteratorT[Tuple[bytes, bytes]]:
        """
        Iterate through all keys and data that exist between the given keys.

        :param start: The key to start at. Leave as None to start at the beginning.
        :param end: The key to end at. Leave as None to finish at the end.
        """
        cdef string key, value, s_start, s_end
        cdef Iterator iterator = self.new_iterator()
        if start is None:
            iterator.seek_to_first()
        else:
            s_start = start
            iterator.seek(s_start)

        if end is None:
            while iterator.valid():
                yield iterator.key(), iterator.value()
                iterator.next()
        else:
            s_end = end
            while iterator.valid():
                key = iterator.key()
                if key >= s_end:
                    break
                value = iterator.value()
                yield key, value
                iterator.next()

    def keys(self):  # -> IteratorT[bytes]:
        """An iterable of all the keys in the database."""
        cdef Iterator iterator = self.new_iterator()
        iterator.seek_to_first()
        while iterator.valid():
            yield iterator.key()
            iterator.next()

    def items(self):  # -> IteratorT[Tuple[bytes, bytes]]:
        cdef Iterator iterator = self.new_iterator()
        iterator.seek_to_first()
        while iterator.valid():
            yield iterator.key(), iterator.value()
            iterator.next()

    def __contains__(self, string key):
        try:
            self.Get(key)
        except KeyError:
            return False
        else:
            return True

    def __getitem__(self, string key):
        return self.Get(key)

    def __setitem__(self, string key, string value):
        self.Put(key, value)

    def __delitem__(self, string key):
        self.Delete(key)

    def __iter__(self) -> IteratorT[bytes]:
        return self.keys()
