#include <memory>
#include <mutex>
#include <set>

#include <leveldb/db.h>
#include <leveldb/iterator.h>
#include <leveldb/options.h>

#include <amulet/leveldb.hpp>

namespace Amulet {

LevelDBIterator::LevelDBIterator(leveldb::Iterator* it)
    : _it(it)
{
}

LevelDBIterator::~LevelDBIterator()
{
    destroy();
}

void LevelDBIterator::destroy()
{
    delete _it;
    _it = nullptr;
}

LevelDBIterator::operator bool()
{
    return _it != nullptr;
}

leveldb::Iterator* LevelDBIterator::operator->()
{
    return _it;
}

leveldb::Iterator& LevelDBIterator::operator*() {
    return *_it;
}

leveldb::Iterator& LevelDBIterator::get_iterator()
{
    return *_it;
}

class LevelDBImpl {
private:
    static void remove_iterator(LevelDBImpl* self, LevelDBIterator* it);

public:
    // Leveldb object
    std::unique_ptr<leveldb::DB> db;

    // Options
    std::unique_ptr<LevelDBOptions> options;

    // The iterators created by the leveldb object.
    // We need to allow deletion of the iterators while the db is open.
    // We need to destroy all iterators before closing the database.
    // During destruction of the iterator a callback will remove the pointer.
    std::set<LevelDBIterator*> iterators;
    std::recursive_mutex iterators_mutex;

    void close();

    std::unique_ptr<LevelDBIterator> create_iterator();
};

void LevelDBImpl::remove_iterator(LevelDBImpl* self, LevelDBIterator* it)
{
    std::lock_guard lock(self->iterators_mutex);
    self->iterators.erase(it);
}

LevelDB::LevelDB(
    std::unique_ptr<leveldb::DB> db,
    std::unique_ptr<LevelDBOptions> options)
    : _impl(new LevelDBImpl { std::move(db), std::move(options) })
{
}

void LevelDBImpl::close()
{
    std::lock_guard lock(iterators_mutex);
    while (!iterators.empty()) {
        // Destroy automatically removes the item from iterators.
        (*iterators.begin())->destroy();
    }
    db.reset();
}

void LevelDB::close()
{
    if (_impl) {
        _impl->close();
        _impl = nullptr;
    }
}

LevelDB::~LevelDB()
{
    close();
}

LevelDB::operator bool()
{
    return _impl != nullptr;
}

// Get the raw leveldb object.
leveldb::DB* LevelDB::operator->()
{
    return _impl->db.get();
}

leveldb::DB& LevelDB::operator*()
{
    return *_impl->db;
}

leveldb::DB& LevelDB::get_database() {
    return *_impl->db;
}

// Create an iterator that is automatically destroyed when the database is closed.
// You may use raw iterators but you must ensure the database outlives the iterator.
std::unique_ptr<LevelDBIterator> LevelDBImpl::create_iterator()
{
    std::lock_guard lock(iterators_mutex);

    // Create the iterator
    auto iterator = std::unique_ptr<LevelDBIterator>(
        new LevelDBIterator(
            db->NewIterator(options->read_options)));

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
    iterator->get_iterator().RegisterCleanup(
        reinterpret_cast<leveldb::Iterator::CleanupFunction>(remove_iterator),
        this, ptr);

    // Return newly created iterator
    return iterator;
}

std::unique_ptr<LevelDBIterator> LevelDB::create_iterator()
{
    return _impl->create_iterator();
}

const leveldb::ReadOptions& LevelDB::get_read_options()
{
    return _impl->options->read_options;
}

const leveldb::WriteOptions& LevelDB::get_write_options()
{
    return _impl->options->write_options;
}

} // namespace Amulet
