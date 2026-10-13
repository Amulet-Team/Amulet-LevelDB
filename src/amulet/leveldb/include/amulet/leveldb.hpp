#pragma once

#include <memory>

#include <leveldb/db.h>
#include <leveldb/iterator.h>
#include <leveldb/options.h>

namespace Amulet {

class LevelDBImpl;
class LevelDBImplShared;

// A wrapper for leveldb's iterator.
// The iterator will be destroyed when the database is closed.
// The caller must ensure the iterator is not used during or after closing the database.
class LEVELDB_EXPORT LevelDBIterator {
private:
    std::shared_ptr<LevelDBImplShared> _shared;
    leveldb::Iterator* _it;

    friend class LevelDBImpl;

    // Constructor
    LevelDBIterator(std::shared_ptr<LevelDBImplShared>, leveldb::Iterator*);

    // Destroy the iterator while holding the lock.
    void destroy_locked();

    // Destroy the iterator
    void destroy();

public:
    // Copy
    LevelDBIterator(const LevelDBIterator&) = delete;
    LevelDBIterator& operator=(const LevelDBIterator&) = delete;

    // Move
    LevelDBIterator(LevelDBIterator&&) = delete;
    LevelDBIterator& operator=(LevelDBIterator&&) = delete;

    // Destructor
    ~LevelDBIterator();

    // Check if the iterator is still alive.
    // If false, calls to other functions are undefined behaviour.
    explicit operator bool() const;

    // Get the raw iterator.
    // The caller must ensure the iterator is not used during or after closing the database.
    leveldb::Iterator* operator->();

    // Get the raw iterator.
    // The caller must ensure the iterator is not used during or after closing the database.
    leveldb::Iterator& operator*();

    // Get the raw iterator.
    // The caller must ensure the iterator is not used during or after closing the database.
    leveldb::Iterator& get_iterator();
};

class LEVELDB_EXPORT LevelDBOptions {
public:
    leveldb::Options options;
    leveldb::ReadOptions read_options;
    leveldb::WriteOptions write_options;

    virtual ~LevelDBOptions() = default;
};

// A wrapper for Mojang's leveldb database.
// This class is mostly thread safe. LevelDB handles locking internally.
// The one exception is when closing the database.
// No threads may interact with the database or its iterators during or after closing the database.
class LEVELDB_EXPORT LevelDB {
private:
    LevelDBImpl* _impl;

public:
    // Constructor
    // Takes ownership of the db and options.
    // Options must delete any resources it allocated in its destructor.
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
    // The caller must ensure no code is interacting with this class, the db or its iterators when this is called.
    void close();

    // Is the database open?
    // If this returns false, all other calls will fail.
    bool is_open() const;

    // Alias for is_open()
    explicit operator bool() const;

    // Raw leveldb accessor.
    // Requires is_open()
    // close() must not be called while the db is in use.
    leveldb::DB* operator->();

    // Raw leveldb accessor.
    // Requires is_open()
    // close() must not be called while the db is in use.
    leveldb::DB& operator*();

    // Raw leveldb accessor.
    // Requires is_open()
    // close() must not be called while the db is in use.
    leveldb::DB& get_database();

    // Create an iterator that is automatically destroyed when the database is closed.
    // Requires is_open()
    // close() must not be called while the iterator is in use.
    std::unique_ptr<LevelDBIterator> create_iterator();

    // Get the read options for the database.
    const leveldb::ReadOptions& get_read_options() const;

    // Get the write options for the database.
    const leveldb::WriteOptions& get_write_options() const;
};

} // namespace Amulet
