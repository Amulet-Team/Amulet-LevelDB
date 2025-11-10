#pragma once

#include <leveldb/db.h>
#include <leveldb/iterator.h>
#include <leveldb/options.h>

namespace Amulet {

class LevelDBImpl;

class LEVELDB_EXPORT LevelDBIterator {
private:
    leveldb::Iterator* _it;

    friend class LevelDBImpl;
    void destroy();

    // Constructor
    LevelDBIterator(leveldb::Iterator*);

public:
    // Copy
    LevelDBIterator(const LevelDBIterator&) = delete;
    LevelDBIterator& operator=(const LevelDBIterator&) = delete;

    // Move
    LevelDBIterator(LevelDBIterator&&) = delete;
    LevelDBIterator& operator=(LevelDBIterator&&) = delete;

    // Delete
    ~LevelDBIterator();

    // Check if the iterator is still alive.
    // If false other calls will error.
    operator bool();

    // Get the raw iterator.
    leveldb::Iterator* operator->();
    leveldb::Iterator& operator*();
    leveldb::Iterator& get_iterator();
};

class LEVELDB_EXPORT LevelDBOptions {
public:
    leveldb::Options options;
    leveldb::ReadOptions read_options;
    leveldb::WriteOptions write_options;

    virtual ~LevelDBOptions() = default;
};

class LEVELDB_EXPORT LevelDB {
private:
    LevelDBImpl* _impl;

public:
    // Constructor
    LevelDB(
        std::unique_ptr<leveldb::DB> db,
        std::unique_ptr<LevelDBOptions> options);

    // Copy
    LevelDB(const LevelDB&) = delete;
    LevelDB& operator=(const LevelDB&) = delete;

    // Move
    LevelDB(LevelDB&&) = delete;
    LevelDB& operator=(LevelDB&&) = delete;

    // Destructor
    ~LevelDB();

    // Close the database and delete all iterators.
    // This must only be called by the owner.
    void close();

    // Is the database valid?
    // If this returns false, all other calls will fail.
    operator bool();

    // Get the raw leveldb object.
    leveldb::DB* operator->();
    leveldb::DB& operator*();
    leveldb::DB& get_database();

    // Create an iterator that is automatically destroyed when the database is closed.
    // You may use raw iterators but you must ensure the database outlives the iterator.
    std::unique_ptr<LevelDBIterator> create_iterator();

    // Get the read options for the database.
    const leveldb::ReadOptions& get_read_options();

    // Get the write options for the database.
    const leveldb::WriteOptions& get_write_options();
};

} // namespace Amulet
