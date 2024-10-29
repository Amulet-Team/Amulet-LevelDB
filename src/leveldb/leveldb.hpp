#pragma once
#include <functional>
#include <memory>
#include <set>
#include <shared_mutex>
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
    std::shared_mutex mutex;
    std::unique_ptr<leveldb::Iterator> iterator;
    std::vector<std::function<void()>> on_destroy;

    friend class LevelDB;
    void register_on_destroy(std::function<void()> f)
    {
        on_destroy.push_back(f);
    }
    void destroy()
    {
        std::unique_lock<std::shared_mutex> lock(mutex);
        if (iterator) {
            // Notify all callbacks that the iterator is about to be destroyed.
            for (const auto& f : on_destroy) {
                f();
            }
            // Destroy the iterator.
            iterator.reset();
        }
    }

public:
    LevelDBIterator(
        std::unique_ptr<leveldb::Iterator>&& iterator)
        : iterator(std::move(iterator))
    {
    }

    ~LevelDBIterator()
    {
        destroy();
    }

    // Lock the iterator in unique mode.
    // The returned lock must be destroyed when it is no longer needed.
    std::unique_lock<std::shared_mutex> lock_unique()
    {
        return std::unique_lock<std::shared_mutex>(mutex);
    }

    // Lock the iterator in shared mode.
    // The returned lock must be destroyed when it is no longer needed.
    std::shared_lock<std::shared_mutex> lock_shared()
    {
        return std::shared_lock<std::shared_mutex>(mutex);
    }

    // Check if the iterator is still alive.
    // If false other calls will error.
    // The mutex must be locked to stop the iterator being destroyed while it is being used.
    operator bool()
    {
        return static_cast<bool>(iterator);
    }

    // Get the raw iterator.
    // The caller must first lock the mutex to stop the iterator being destroyed
    // then check if the iterator is still alive before calling this.
    leveldb::Iterator* operator->()
    {
        return iterator.get();
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
    // Mutex to ensure nothing is processing when we close the database.
    std::shared_mutex mutex;

    // Leveldb objects
    std::unique_ptr<leveldb::DB> db;

    // Options
    std::unique_ptr<LevelDBOptions> options;

    // The iterators created by the leveldb object.
    // We need to allow deletion of the iterators while the db is open.
    // We need to destroy all iterators before closing the database.
    // We can't store them in a shared_ptr because this would keep them alive.
    // We can't store them in a weak_ptr because that does not support comparison for lookup.
    // The only way I can find is raw pointers.
    // During destruction of the iterator a callback will remove the pointer.
    std::set<LevelDBIterator*> iterators;

    LevelDB(const LevelDB&) { }
    void operator=(const LevelDB&) { }

public:
    LevelDB(
        std::unique_ptr<leveldb::DB>&& db,
        std::unique_ptr<LevelDBOptions>&& options)
        : db(std::move(db))
        , options(std::move(options))
    {
    }

    void close(bool compact = false)
    {
        std::unique_lock<std::shared_mutex> lock(mutex);
        if (db) {
            if (compact) {
                db->CompactRange(nullptr, nullptr);
            }
            while (!iterators.empty()) {
                (*iterators.begin())->destroy();
            }
            db.reset();
        }
    }

    ~LevelDB()
    {
        close();
    }

    // Lock the database in unique mode.
    // The returned lock must be destroyed when it is no longer needed.
    std::unique_lock<std::shared_mutex> lock_unique()
    {
        return std::unique_lock<std::shared_mutex>(mutex);
    }

    // Lock the database in shared mode.
    // The returned lock must be destroyed when it is no longer needed.
    std::shared_lock<std::shared_mutex> lock_shared()
    {
        return std::shared_lock<std::shared_mutex>(mutex);
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

    // Create an iterator that is automatically destroyed when the database is closed.
    // You may use raw iterators but you must ensure the database outlives the iterator.
    std::unique_ptr<LevelDBIterator> create_iterator()
    {
        std::shared_lock<std::shared_mutex> lock(mutex);
        if (!db) {
            throw std::runtime_error("The LevelDB database has been closed.");
        }

        // Create the iterator
        auto iterator = std::make_unique<LevelDBIterator>(
            std::unique_ptr<leveldb::Iterator>(db->NewIterator(options->read_options)));

        // Get a raw poiner to the iterator
        LevelDBIterator* ptr = iterator.get();

        // Add the iterator pointer to the set
        iterators.insert(ptr);

        // Create on destroy callback
        auto on_destroy = [this, ptr]() {
            // Remove the pointer from the set
            iterators.erase(ptr);
        };

        // Register on destory callback
        iterator->register_on_destroy(on_destroy);

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
