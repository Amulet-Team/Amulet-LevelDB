#include <memory>
#include <mutex>
#include <set>
#include <stdexcept>
#include <utility>

#include <amulet/utils/threading/mutex.hpp>
#include <amulet/utils/threading/thread_safety.hpp>

#include <leveldb/db.h>
#include <leveldb/iterator.h>
#include <leveldb/options.h>

#include <amulet/leveldb.hpp>

namespace Amulet {

class LevelDBImplShared {
public:
    astd::mutex m;

    // The iterators created by the leveldb object.
    // This is needed to know which iterators to destroy when the database is closed.
    std::set<LevelDBIterator*> iterators ASTD_GUARDED_BY(m);
};

LevelDBIterator::LevelDBIterator(std::shared_ptr<LevelDBImplShared> shared, leveldb::Iterator* it)
    : _shared(std::move(shared))
    , _it(it)
{
}

LevelDBIterator::~LevelDBIterator()
{
    destroy();
}

void LevelDBIterator::destroy_locked(LevelDBImplShared& shared)
{
    if (_it) {
        shared.iterators.erase(this);
        delete _it;
        _it = nullptr;
    }
}

void LevelDBIterator::destroy()
{
    astd::lock_guard lock(_shared->m);
    destroy_locked(*_shared);
}

LevelDBIterator::operator bool() const
{
    return _it != nullptr;
}

leveldb::Iterator* LevelDBIterator::operator->()
{
    if (!_it) {
        throw std::runtime_error("leveldb iterator has been destroyed.");
    }
    return _it;
}

leveldb::Iterator& LevelDBIterator::operator*()
{
    if (!_it) {
        throw std::runtime_error("leveldb iterator has been destroyed.");
    }
    return *_it;
}

leveldb::Iterator& LevelDBIterator::get_iterator()
{
    if (!_it) {
        throw std::runtime_error("leveldb iterator has been destroyed.");
    }
    return *_it;
}

class LevelDBImpl {
public:
    std::shared_ptr<LevelDBImplShared> _shared;

    // Options
    std::unique_ptr<LevelDBOptions> _options;

    // Leveldb object
    std::unique_ptr<leveldb::DB> _db;

    // Constructor
    LevelDBImpl(
        std::unique_ptr<leveldb::DB> db,
        std::unique_ptr<LevelDBOptions> options);

    // Destructor
    ~LevelDBImpl();

    // Close the database and delete all iterators.
    // The caller must ensure no code is interacting with this class, the db or its iterators when this is called.
    void close();

    // Is the database open?
    // If this returns false, all other calls will fail.
    bool is_open() const;

    // Raw leveldb accessor.
    // Requires is_open()
    // close() must not be called while the db is in use.
    leveldb::DB* get_db_ptr();

    // Get the read options for the database.
    const leveldb::ReadOptions& get_read_options() const;

    // Get the write options for the database.
    const leveldb::WriteOptions& get_write_options() const;

    // Create an iterator that is automatically destroyed when the database is closed.
    // Requires is_open()
    // close() must not be called while the iterator is in use.
    std::unique_ptr<LevelDBIterator> create_iterator();
};

LevelDBImpl::LevelDBImpl(
    std::unique_ptr<leveldb::DB> db,
    std::unique_ptr<LevelDBOptions> options)
    : _shared(std::make_shared<LevelDBImplShared>())
    , _options(std::move(options))
    , _db(std::move(db))
{
    if (!_db || !_options) {
        throw std::invalid_argument("db and options must not be null");
    }
}

LevelDBImpl::~LevelDBImpl()
{
    close();
}

void LevelDBImpl::close()
{
    astd::lock_guard iterators_lock(_shared->m);

    // Extract the db to stop new iterators from being created.
    auto db = std::move(_db);

    // Destroy all iterators
    std::set<LevelDBIterator*> iterators;
    iterators.swap(_shared->iterators);
    for (auto* it : iterators) {
        it->destroy_locked(*_shared);
    }

    // Destroy db
    db.reset();
}

bool LevelDBImpl::is_open() const
{
    return _db.operator bool();
}

leveldb::DB* LevelDBImpl::get_db_ptr()
{
    if (!_db) {
        throw std::runtime_error("leveldb database has been closed.");
    }
    return _db.get();
}

const leveldb::ReadOptions& LevelDBImpl::get_read_options() const
{
    return _options->read_options;
}

const leveldb::WriteOptions& LevelDBImpl::get_write_options() const
{
    return _options->write_options;
}

std::unique_ptr<LevelDBIterator> LevelDBImpl::create_iterator()
{
    // Declared before the lock so it is destroyed after the lock is released.
    std::unique_ptr<LevelDBIterator> iterator;

    // Acquire the lock
    astd::lock_guard lock(_shared->m);
    if (!_db) {
        throw std::runtime_error("leveldb database has been closed.");
    }

    // Create the iterator
    std::unique_ptr<leveldb::Iterator> raw_iterator(_db->NewIterator(_options->read_options));
    iterator.reset(new LevelDBIterator(_shared, raw_iterator.get()));
    raw_iterator.release();

    // Add the iterator pointer to the set
    _shared->iterators.insert(iterator.get());

    // Return newly created iterator
    return iterator;
}

LevelDB::LevelDB(
    std::unique_ptr<leveldb::DB> db,
    std::unique_ptr<LevelDBOptions> options)
    : _impl(new LevelDBImpl(std::move(db), std::move(options)))
{
}

LevelDB::~LevelDB()
{
    delete _impl;
}

void LevelDB::close()
{
    _impl->close();
}

bool LevelDB::is_open() const
{
    return _impl->is_open();
}

LevelDB::operator bool() const
{
    return _impl->is_open();
}

leveldb::DB* LevelDB::operator->()
{
    return _impl->get_db_ptr();
}

leveldb::DB& LevelDB::operator*()
{
    return *_impl->get_db_ptr();
}

leveldb::DB& LevelDB::get_database()
{
    return *_impl->get_db_ptr();
}

std::unique_ptr<LevelDBIterator> LevelDB::create_iterator()
{
    return _impl->create_iterator();
}

const leveldb::ReadOptions& LevelDB::get_read_options() const
{
    return _impl->get_read_options();
}

const leveldb::WriteOptions& LevelDB::get_write_options() const
{
    return _impl->get_write_options();
}

} // namespace Amulet
