from libcpp.string cimport string
from libcpp cimport bool

cdef extern from "leveldb/db.h" namespace "leveldb" nogil:
    cdef cppclass DB:
        DB() except +
        # ~DB() except +
        Status Open(
            const Options &options,
            const string &name,
            DB** dbptr,
        )
        Status Put(
            const WriteOptions &options,
            const Slice &key,
            const Slice &value
        )
        Status Delete(
            const WriteOptions& options,
            const Slice& key
        )
        Status Write(
            const WriteOptions& options,
            WriteBatch* updates
        )
        Status Get(
            const ReadOptions & options,
            const Slice & key,
            string *value
        )
        Iterator * NewIterator(
            const ReadOptions& options
        )
        const Snapshot * GetSnapshot()
        void ReleaseSnapshot(
            const Snapshot * snapshot
        )
        bool GetProperty(
            const Slice& property, string* value
        )
        void GetApproximateSizes(
            const Range* range, int n, unsigned long long* sizes
        )
        void CompactRange(
            const Slice * begin, const Slice * end
        )
        void SuspendCompaction()
        void ResumeCompaction()

    Status DestroyDB(const string& name, const Options& options)
    Status RepairDB(const string& dbname, const Options& options)

    cdef cppclass Snapshot:
        pass

    cdef cppclass Range:
        Slice start
        Slice limit
        Range()
        Range(const Slice& s, const Slice& l)

cdef extern from "leveldb/status.h" namespace "leveldb" nogil:
    cdef cppclass Status:
        enum Code:
            kOk = 0,
            kNotFound = 1,
            kCorruption = 2,
            kNotSupported = 3,
            kInvalidArgument = 4,
            kIOError = 5
        Status() except +
        # ~Status() except +
        bool ok()
        bool IsNotFound()
        bool IsCorruption()
        bool IsIOError()
        bool IsNotSupportedError()
        bool IsInvalidArgument()
        string ToString()

cdef extern from "leveldb/options.h" namespace "leveldb" nogil:
    cdef cppclass Options:
        Options() except +
        const Comparator * comparator
        bool create_if_missing
        bool error_if_exists
        bool paranoid_checks
        Env * env
        Logger * info_log
        size_t write_buffer_size
        int max_open_files
        Cache * block_cache
        size_t block_size
        int block_restart_interval
        size_t max_file_size
        Compressor * compressors[256]
        bool reuse_logs
        const FilterPolicy * filter_policy

    cdef cppclass ReadOptions:
        bool verify_checksums
        bool fill_cache
        const Snapshot* snapshot
        DecompressAllocator* decompress_allocator

        ReadOptions() except +

    cdef cppclass WriteOptions:
        bool sync

        WriteOptions() except +

cdef extern from "leveldb/write_batch.h" namespace "leveldb" nogil:
    cdef cppclass WriteBatch:
        WriteBatch()
        void Put(const Slice& key, const Slice& value)
        void Delete(const Slice& key)
        void Clear()
        size_t ApproximateSize()

cdef extern from "leveldb/iterator.h" namespace "leveldb" nogil:
    cdef cppclass Iterator:
        Iterator()
        bool Valid()
        void SeekToFirst()
        void SeekToLast()
        void Seek(const Slice& target)
        void Next()
        void Prev()
        Slice key()
        Slice value()
        Status status()

cdef extern from "leveldb/decompress_allocator.h" namespace "leveldb" nogil:
    cdef cppclass DecompressAllocator:
        pass

cdef extern from "leveldb/comparator.h" namespace "leveldb" nogil:
    cdef cppclass Comparator:
        pass
        # ~Comparator() except +

cdef extern from "leveldb/env.h" namespace "leveldb" nogil:
    ctypedef struct va_list

    cdef cppclass Env:
        pass

    cdef cppclass Logger:
        Logger()
        # ~Logger()
        void Logv(const char * format, va_list ap)

cdef extern from "leveldb/cache.h" namespace "leveldb" nogil:
    cdef cppclass Cache:
        Cache() except +

    Cache * NewLRUCache(size_t capacity)

cdef extern from "leveldb/slice.h" namespace "leveldb" nogil:
    cdef cppclass Slice:
        Slice() except +
        Slice(const char * d, size_t n) except +
        Slice(const string &s) except +
        Slice(const char * s) except +
        const char * data()
        size_t size()
        bool empty()
        void clear()
        string ToString()

cdef extern from "leveldb/compressor.h" namespace "leveldb" nogil:
    cdef cppclass Compressor:
        pass

cdef extern from "leveldb/filter_policy.h" namespace "leveldb" nogil:
    cdef cppclass FilterPolicy:
        const char* Name() except +
        void CreateFilter(const Slice* keys, int n, string* dst)
        bool KeyMayMatch(const Slice& key, const Slice& filter)
    const FilterPolicy * NewBloomFilterPolicy(int bits_per_key)

cdef extern from "leveldb/compressor.h" namespace "leveldb" nogil:
    cdef cppclass Compressor:
        pass

cdef extern from "leveldb/zlib_compressor.h" namespace "leveldb" nogil:
    cdef cppclass ZlibCompressor(Compressor):
        ZlibCompressor(int compressionLevel) except +

    cdef cppclass ZlibCompressorRaw(Compressor):
        ZlibCompressorRaw(int compressionLevel) except +

cdef extern from "leveldb/decompress_allocator.h" namespace "leveldb" nogil:
    cdef cppclass DecompressAllocator:
        pass
