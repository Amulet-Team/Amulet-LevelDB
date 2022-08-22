from libcpp.string cimport string

cdef extern from "leveldb/status.h" namespace "leveldb":
    cdef cppclass Status:
        Status() except +

cdef extern from "leveldb/options.h" namespace "leveldb":
    cdef cppclass Options:
        Options() except +

cdef extern from "leveldb/slice.h" namespace "leveldb":
    cdef cppclass Slice:
        Slice() except +

cdef extern from "leveldb/options.h" namespace "leveldb":
    cdef cppclass WriteOptions:
        WriteOptions() except +

cdef extern from "leveldb_mcpe/db/db_impl.cc" namespace "leveldb":
    cdef cppclass DBImpl:
        DBImpl(const Options& options, const string & dbname) except +

# cdef extern from "leveldb/db.h" namespace "leveldb":
    # cdef cppclass DB:
    #     DB() except +
        # ~DB() except +
        # Status Open(
        #     const Options& options,
        #     const string& name,
        #     DB** dbptr,
        # )
        # Status Put(
        #     const WriteOptions& options,
        #     const Slice& key,
        #     const Slice& value
        # )
