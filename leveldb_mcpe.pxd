from libcpp.string cimport string
from libcpp cimport bool

cdef extern from "leveldb/db.h" namespace "leveldb":
    cdef cppclass DB:
        DB() nogil except +
        # ~DB() except +
        Status Open(
            const Options &options,
            const string &name,
            DB** dbptr,
        ) nogil
        Status Put(
            const WriteOptions &options,
            const Slice &key,
            const Slice &value
        ) nogil
        Status Delete(
            const WriteOptions& options,
            const Slice& key
        ) nogil
        Status Write(
            const WriteOptions& options,
            WriteBatch* updates
        ) nogil
        Status Get(
            const ReadOptions & options,
            const Slice & key,
            string *value
        ) nogil
        Iterator * NewIterator(
            const ReadOptions& options
        ) nogil
        const Snapshot * GetSnapshot() nogil
        void ReleaseSnapshot(
            const Snapshot * snapshot
        ) nogil
        bool GetProperty(
            const Slice& property, string* value
        ) nogil
        void GetApproximateSizes(
            const Range* range, int n, unsigned long long* sizes
        ) nogil
        void CompactRange(
            const Slice * begin, const Slice * end
        ) nogil
        void SuspendCompaction() nogil
        void ResumeCompaction() nogil

    Status DestroyDB(const string& name, const Options& options) nogil
    Status RepairDB(const string& dbname, const Options& options) nogil

    cdef cppclass Snapshot:
        pass

    cdef cppclass Range:
        Slice start
        Slice limit
        Range() nogil
        Range(const Slice& s, const Slice& l) nogil

cdef extern from "leveldb/status.h" namespace "leveldb":
    cdef cppclass Status:
        enum Code:
            kOk = 0,
            kNotFound = 1,
            kCorruption = 2,
            kNotSupported = 3,
            kInvalidArgument = 4,
            kIOError = 5
        Status() nogil except +
        # ~Status() except +
        bool ok() nogil
        bool IsNotFound() nogil
        bool IsCorruption() nogil
        bool IsIOError() nogil
        bool IsNotSupportedError() nogil
        bool IsInvalidArgument() nogil
        string ToString() nogil

cdef extern from "leveldb/options.h" namespace "leveldb":
    cdef cppclass Options:
        Options() nogil except +
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

        ReadOptions() nogil except +

    cdef cppclass WriteOptions:
        bool sync

        WriteOptions() nogil except +

cdef extern from "leveldb/write_batch.h" namespace "leveldb":
    cdef cppclass WriteBatch:
        WriteBatch() nogil
        void Put(const Slice& key, const Slice& value) nogil
        void Delete(const Slice& key) nogil
        void Clear() nogil
        size_t ApproximateSize() nogil

cdef extern from "leveldb/iterator.h" namespace "leveldb":
    cdef cppclass Iterator:
        Iterator() nogil
        bool Valid() nogil
        void SeekToFirst() nogil
        void SeekToLast() nogil
        void Seek(const Slice& target) nogil
        void Next() nogil
        void Prev() nogil
        Slice key() nogil
        Slice value() nogil
        Status status() nogil

cdef extern from "leveldb/decompress_allocator.h" namespace "leveldb":
    cdef cppclass DecompressAllocator:
        pass

cdef extern from "leveldb/comparator.h" namespace "leveldb":
    cdef cppclass Comparator:
        pass
        # ~Comparator() except +

cdef extern from "leveldb/env.h" namespace "leveldb":
    ctypedef struct va_list

    cdef cppclass Env:
        pass

    cdef cppclass Logger:
        Logger() nogil
        # ~Logger()
        void Logv(const char * format, va_list ap) nogil

cdef extern from "leveldb/cache.h" namespace "leveldb":
    cdef cppclass Cache:
        Cache() nogil except +

    Cache * NewLRUCache(size_t capacity) nogil

cdef extern from "leveldb/slice.h" namespace "leveldb":
    cdef cppclass Slice:
        Slice() nogil except +
        Slice(const char * d, size_t n) nogil except +
        Slice(const string &s) nogil except +
        Slice(const char * s) nogil except +
        const char * data()
        size_t size()
        bool empty()
        void clear()
        string ToString()

cdef extern from "leveldb/compressor.h" namespace "leveldb":
    cdef cppclass Compressor:
        pass

cdef extern from "leveldb/filter_policy.h" namespace "leveldb":
    cdef cppclass FilterPolicy:
        const char* Name() nogil except +
        void CreateFilter(const Slice* keys, int n, string* dst) nogil
        bool KeyMayMatch(const Slice& key, const Slice& filter) nogil
    const FilterPolicy * NewBloomFilterPolicy(int bits_per_key) nogil

cdef extern from "leveldb/compressor.h" namespace "leveldb":
    cdef cppclass Compressor:
        pass

cdef extern from "leveldb/zlib_compressor.h" namespace "leveldb":
    cdef cppclass ZlibCompressor(Compressor):
        ZlibCompressor(int compressionLevel = -1) nogil except +

    cdef cppclass ZlibCompressorRaw(Compressor):
        ZlibCompressorRaw(int compressionLevel = -1) nogil except +

cdef extern from "leveldb/decompress_allocator.h" namespace "leveldb":
    cdef cppclass DecompressAllocator:
        pass
