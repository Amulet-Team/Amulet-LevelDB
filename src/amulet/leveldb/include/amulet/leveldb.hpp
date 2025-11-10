#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <leveldb/db.h>
#include <leveldb/env.h>
#include <leveldb/iterator.h>
#include <leveldb/options.h>

namespace Amulet {

class LevelDBIterator {
private:
    std::unique_ptr<leveldb::Iterator> iterator;

    friend class LevelDB;
    void destroy()
    {
        // Destroy the iterator.
        iterator.reset();
    }

public:
    LevelDBIterator(
        std::unique_ptr<leveldb::Iterator> iterator)
        : iterator(std::move(iterator))
    {
    }

    ~LevelDBIterator()
    {
        destroy();
    }

    // Check if the iterator is still alive.
    // If false other calls will error.
    operator bool()
    {
        return static_cast<bool>(iterator);
    }

    // Get the raw iterator.
    leveldb::Iterator* operator->()
    {
        return iterator.get();
    }

    leveldb::Iterator& it()
    {
        return *iterator;
    }
};

class LevelDBOptions {
public:
    leveldb::Options options;
    leveldb::ReadOptions read_options;
    leveldb::WriteOptions write_options;

    virtual ~LevelDBOptions() = default;
};

class LevelDB {
private:
    // Leveldb objects
    std::unique_ptr<leveldb::DB> db;

    // Options
    std::unique_ptr<LevelDBOptions> options;

    // The iterators created by the leveldb object.
    // We need to allow deletion of the iterators while the db is open.
    // We need to destroy all iterators before closing the database.
    // During destruction of the iterator a callback will remove the pointer.
    std::set<LevelDBIterator*> iterators;
    std::recursive_mutex iterators_mutex;

    static void remove_iterator(LevelDB* self, LevelDBIterator* it)
    {
        std::lock_guard lock(self->iterators_mutex);
        self->iterators.erase(it);
    }

public:
    LevelDB(
        std::unique_ptr<leveldb::DB> db,
        std::unique_ptr<LevelDBOptions> options)
        : db(std::move(db))
        , options(std::move(options))
    {
    }

    LevelDB(const LevelDB&) = delete;
    LevelDB(LevelDB&&) = delete;

    void close()
    {
        if (db) {
            std::lock_guard lock(iterators_mutex);
            while (!iterators.empty()) {
                // Destroy automatically removes the item from iterators.
                (*iterators.begin())->destroy();
            }
            db.reset();
        }
    }

    ~LevelDB()
    {
        close();
    }

    operator bool()
    {
        return static_cast<bool>(db);
    }

    // Get the raw leveldb object.
    leveldb::DB* operator->()
    {
        return db.get();
    }

    leveldb::DB& operator*()
    {
        return *db;
    }

    // Create an iterator that is automatically destroyed when the database is closed.
    // You may use raw iterators but you must ensure the database outlives the iterator.
    std::unique_ptr<LevelDBIterator> create_iterator()
    {
        if (!db) {
            throw std::runtime_error("The LevelDB database has been closed.");
        }

        std::lock_guard lock(iterators_mutex);

        // Create the iterator
        auto iterator = std::make_unique<LevelDBIterator>(
            std::unique_ptr<leveldb::Iterator>(db->NewIterator(options->read_options)));

        // Get a raw pointer to the iterator
        LevelDBIterator* ptr = iterator.get();

        // Add the iterator pointer to the set
        iterators.insert(ptr);

        // Create on destroy callback
        auto on_destroy = [this, ptr]() {
            // Remove the pointer from the set
            iterators.erase(ptr);
        };

        // Register on destory callback
        iterator->it().RegisterCleanup(
            reinterpret_cast<leveldb::Iterator::CleanupFunction>(remove_iterator),
            this, ptr);

        // Return newly created iterator
        return iterator;
    }

    const leveldb::ReadOptions& get_read_options()
    {
        return options->read_options;
    }

    const leveldb::WriteOptions& get_write_options()
    {
        return options->write_options;
    }
};

} // namespace Amulet
