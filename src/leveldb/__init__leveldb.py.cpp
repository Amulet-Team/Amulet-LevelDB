#include <filesystem>
#include <optional>
#include <string>
#include <variant>

#include <leveldb/cache.h>
#include <leveldb/db.h>
#include <leveldb/decompress_allocator.h>
#include <leveldb/env.h>
#include <leveldb/filter_policy.h>
#include <leveldb/write_batch.h>
#include <leveldb/zlib_compressor.h>

#include "leveldb.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/typing.h>

namespace py = pybind11;

namespace {

class LevelDBException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};
class LevelDBEncrypted : public LevelDBException {
    using LevelDBException::LevelDBException;
};

class NullLogger : public leveldb::Logger {
public:
    void Logv(const char*, va_list) override { }
};

class LevelDBOptions : public Amulet::LevelDBOptions {
public:
    NullLogger logger;
    leveldb::ZlibCompressorRaw zlib_compressor_raw;
    leveldb::ZlibCompressor zlib_compressor;
    leveldb::DecompressAllocator decompress_allocator;
};

std::unique_ptr<Amulet::LevelDB> open_leveldb(
    std::filesystem::path path,
    bool create_if_missing = false)
{
    // Expand dots and symbolic links
    path = std::filesystem::canonical(path);
    // If there is not a directory at the path
    if (!std::filesystem::is_directory(path)) {
        if (std::filesystem::exists(path)) {
            // If the path exists but is not a directory
            throw LevelDBException("A non-directory file exists at " + path.string());
        } else if (create_if_missing) {
            // Create if requested
            std::filesystem::create_directories(path);
        } else {
            throw LevelDBException("No database exists to open at " + path.string());
        }
    }

    auto options = std::make_unique<LevelDBOptions>();
    options->options.create_if_missing = create_if_missing;
    options->options.filter_policy = leveldb::NewBloomFilterPolicy(10);
    options->options.block_cache = leveldb::NewLRUCache(40 * 1024 * 1024);
    options->options.write_buffer_size = 4 * 1024 * 1024;
    options->options.info_log = &options->logger;
    options->options.compressors[0] = &options->zlib_compressor_raw;
    options->options.compressors[1] = &options->zlib_compressor;
    options->options.block_size = 163840;

    options->read_options.decompress_allocator = &options->decompress_allocator;

    leveldb::DB* _db = NULL;
    auto status = leveldb::DB::Open(options->options, path.string(), &_db);
    switch (status.code()) {
    case leveldb::Status::kOk:
        return std::make_unique<Amulet::LevelDB>(
            std::unique_ptr<leveldb::DB>(_db),
            std::move(options));
    case leveldb::Status::kCorruption:
        leveldb::RepairDB(path.string(), options->options);
        {
            auto status2 = leveldb::DB::Open(options->options, path.string(), &_db);
            if (status2.ok()) {
                return std::make_unique<Amulet::LevelDB>(
                    std::unique_ptr<leveldb::DB>(_db),
                    std::move(options));
            } else {
                throw LevelDBException("Could not recover corrupted database. " + status.ToString());
            }
        }
        break;
    case leveldb::Status::kNotSupported:
        if (status.ToString().ends_with("Marketplace worlds are not supported.")) {
            throw LevelDBEncrypted("Marketplace worlds are not supported.");
        }
    default:
        throw LevelDBException(status.ToString());
    }
}

class LevelDBKeysIterator {
private:
    std::unique_ptr<Amulet::LevelDBIterator> iterator_ptr;

public:
    LevelDBKeysIterator(
        std::unique_ptr<Amulet::LevelDBIterator> iterator_ptr)
        : iterator_ptr(std::move(iterator_ptr))
    {
    }

    py::bytes next()
    {
        auto lock = iterator_ptr->lock_shared();
        auto& iterator = *iterator_ptr;
        if (!iterator) {
            throw std::runtime_error("LevelDBIterator has been deleted.");
        }
        if (!iterator->Valid()) {
            throw py::stop_iteration();
        }
        // Get value.
        auto key = py::bytes(iterator->key().ToString());
        // Increment for next time.
        iterator->Next();
        // Return value
        return key;
    }
};

class LevelDBValuesIterator {
private:
    std::unique_ptr<Amulet::LevelDBIterator> iterator_ptr;

public:
    LevelDBValuesIterator(
        std::unique_ptr<Amulet::LevelDBIterator> iterator_ptr)
        : iterator_ptr(std::move(iterator_ptr))
    {
    }

    py::bytes next()
    {
        auto lock = iterator_ptr->lock_shared();
        auto& iterator = *iterator_ptr;
        if (!iterator) {
            throw std::runtime_error("LevelDBIterator has been deleted.");
        }
        if (!iterator->Valid()) {
            throw py::stop_iteration();
        }
        // Get value.
        auto value = py::bytes(iterator->value().ToString());
        // Increment for next time.
        iterator->Next();
        // Return value
        return value;
    }
};

class LevelDBItemsIterator {
private:
    std::unique_ptr<Amulet::LevelDBIterator> iterator_ptr;

public:
    LevelDBItemsIterator(
        std::unique_ptr<Amulet::LevelDBIterator> iterator_ptr)
        : iterator_ptr(std::move(iterator_ptr))
    {
    }

    py::typing::Tuple<py::bytes, py::bytes> next()
    {
        auto lock = iterator_ptr->lock_shared();
        auto& iterator = *iterator_ptr;
        if (!iterator) {
            throw std::runtime_error("LevelDBIterator has been deleted.");
        }
        if (!iterator->Valid()) {
            throw py::stop_iteration();
        }
        // Get value.
        auto item = py::make_tuple(
            py::bytes(iterator->key().ToString()),
            py::bytes(iterator->value().ToString()));
        // Increment for next time.
        iterator->Next();
        // Return value
        return item;
    }
};

class LevelDBItemsRangeIterator {
private:
    std::unique_ptr<Amulet::LevelDBIterator> iterator_ptr;
    std::optional<std::string> end;

public:
    LevelDBItemsRangeIterator(
        std::unique_ptr<Amulet::LevelDBIterator> iterator_ptr,
        std::string end)
        : iterator_ptr(std::move(iterator_ptr))
        , end(end)
    {
    }

    py::typing::Tuple<py::bytes, py::bytes> next()
    {
        auto lock = iterator_ptr->lock_shared();
        auto& iterator = *iterator_ptr;
        if (!iterator) {
            throw std::runtime_error("LevelDBIterator has been deleted.");
        }
        if (!iterator->Valid()) {
            throw py::stop_iteration();
        }
        // Get value.
        std::string key = iterator->key().ToString();
        if (end <= key) {
            throw py::stop_iteration();
        }
        auto item = py::make_tuple(
            py::bytes(key),
            py::bytes(iterator->value().ToString()));
        // Increment for next time.
        iterator->Next();
        // Return value
        return item;
    }
};

bool init_run = false;

} // namespace

void init_leveldb(py::module m)
{
    if (init_run) {
        return;
    }
    init_run = true;

    py::dict version_data = py::module::import("leveldb._version").attr("get_versions")();
    m.attr("__version__") = version_data["version"];

    py::register_local_exception<LevelDBException>(m, "LevelDBException");
    py::register_local_exception<LevelDBEncrypted>(m, "LevelDBEncrypted");

    py::class_<Amulet::LevelDBIterator> LevelDBIterator(m, "LevelDBIterator");
    LevelDBIterator.def(
        "valid",
        [](Amulet::LevelDBIterator& self) {
            auto lock = self.lock_shared();
            return self && self->Valid();
        },
        py::doc(
            "Is the iterator at a valid entry."
            "If False, calls to other methods may error."));
    LevelDBIterator.def(
        "seek_to_first",
        [](Amulet::LevelDBIterator& self) {
            auto lock = self.lock_shared();
            if (!self) {
                throw std::runtime_error("LevelDBIterator has been deleted.");
            }
            self->SeekToFirst();
        },
        py::doc("Seek to the first entry in the database."));
    LevelDBIterator.def(
        "seek_to_last",
        [](Amulet::LevelDBIterator& self) {
            auto lock = self.lock_shared();
            if (!self) {
                throw std::runtime_error("LevelDBIterator has been deleted.");
            }
            self->SeekToLast();
        },
        py::doc("Seek to the last entry in the database."));
    LevelDBIterator.def(
        "seek",
        [](Amulet::LevelDBIterator& self, std::string target) {
            auto lock = self.lock_shared();
            if (!self) {
                throw std::runtime_error("LevelDBIterator has been deleted.");
            }
            self->Seek(target);
        },
        py::arg("target"),
        py::doc(
            "Seek to the given entry in the database.\n"
            "If the entry does not exist it will seek to the location after."));
    LevelDBIterator.def(
        "next",
        [](Amulet::LevelDBIterator& self) {
            auto lock = self.lock_shared();
            if (!self) {
                throw std::runtime_error("LevelDBIterator has been deleted.");
            }
            self->Next();
        },
        py::doc(
            "Seek to the next entry in the database."));
    LevelDBIterator.def(
        "prev",
        [](Amulet::LevelDBIterator& self) {
            auto lock = self.lock_shared();
            if (!self) {
                throw std::runtime_error("LevelDBIterator has been deleted.");
            }
            self->Prev();
        },
        py::doc(
            "Seek to the previous entry in the database."));
    LevelDBIterator.def(
        "key",
        [](Amulet::LevelDBIterator& self) {
            auto lock = self.lock_shared();
            if (!self) {
                throw std::runtime_error("LevelDBIterator has been deleted.");
            }
            if (!self->Valid()) {
                throw std::runtime_error("LevelDBIterator does not point to a valid value.");
            }
            return py::bytes(self->key().ToString());
        },
        py::doc(
            "Get the key of the current entry in the database.\n"
            ":raises: runtime_error if iterator is not valid."));
    LevelDBIterator.def(
        "value",
        [](Amulet::LevelDBIterator& self) {
            auto lock = self.lock_shared();
            if (!self) {
                throw std::runtime_error("LevelDBIterator has been deleted.");
            }
            if (!self->Valid()) {
                throw std::runtime_error("LevelDBIterator does not point to a valid value.");
            }
            return py::bytes(self->value().ToString());
        },
        py::doc(
            "Get the value of the current entry in the database.\n"
            ":raises: runtime_error if iterator is not valid."));

    py::class_<LevelDBKeysIterator, std::shared_ptr<LevelDBKeysIterator>>(m, "LevelDBKeysIterator")
        .def("__iter__", [](py::object self) { return self; })
        .def("__next__", &LevelDBKeysIterator::next);

    py::class_<LevelDBValuesIterator, std::shared_ptr<LevelDBValuesIterator>>(m, "LevelDBValuesIterator")
        .def("__iter__", [](py::object self) { return self; })
        .def("__next__", &LevelDBValuesIterator::next);

    py::class_<LevelDBItemsIterator, std::shared_ptr<LevelDBItemsIterator>>(m, "LevelDBItemsIterator")
        .def("__iter__", [](py::object self) { return self; })
        .def("__next__", &LevelDBItemsIterator::next);

    py::class_<LevelDBItemsRangeIterator, std::shared_ptr<LevelDBItemsRangeIterator>>(m, "LevelDBItemsRangeIterator")
        .def("__iter__", [](py::object self) { return self; })
        .def("__next__", &LevelDBItemsRangeIterator::next);

    py::class_<Amulet::LevelDB> LevelDB(m, "LevelDB",
        "A LevelDB database");
    LevelDB.def(
        py::init(&open_leveldb),
        py::arg("path"),
        py::arg("create_if_missing"),
        py::doc(
            "Construct a new :class :`LevelDB` instance from the database at the given path.\n"
            "\n"
            "A leveldb database is like a dictionary that only contains bytes as the keys and values and exists entirely on the disk.\n"
            "\n"
            ":param path: The path to the database directory.\n"
            ":param create_if_missing: If True a new database will be created if one does not exist at the given path.\n"
            ":raises: LevelDBException if create_if_missing is False and the db does not exist."));

    LevelDB.def(
        "close",
        &Amulet::LevelDB::close,
        py::arg("compact") = false,
        py::doc(
            "Close the leveldb database.\n"
            "\n"
            ":param compact: If True will compact the database making it take less memory."));

    LevelDB.def(
        "compact",
        [](Amulet::LevelDB& self) {
            auto lock = self.lock_shared();
            if (!self) {
                throw LevelDBException("The LevelDB database has been closed.");
            }
            self->CompactRange(nullptr, nullptr);
        },
        py::doc("Remove deleted entries from the database to reduce its size."));

    auto put = [](Amulet::LevelDB& self, std::string key, std::string value) {
        auto lock = self.lock_shared();
        if (!self) {
            throw LevelDBException("The LevelDB database has been closed.");
        }
        auto status = self->Put(self.get_write_options(), key, value);
        if (!status.ok()) {
            throw LevelDBException(status.ToString());
        }
    };
    LevelDB.def("put", put, py::doc("Set a value in the database."));
    LevelDB.def("__setitem__", put);

    LevelDB.def(
        "put_batch",
        [](Amulet::LevelDB& self, const std::unordered_map<std::string, std::optional<std::string>> data) {
            auto lock = self.lock_shared();
            if (!self) {
                throw LevelDBException("The LevelDB database has been closed.");
            }
            leveldb::WriteBatch batch;
            for (const auto& [k, v] : data) {
                if (v) {
                    batch.Put(k, *v);
                } else {
                    batch.Delete(k);
                }
            }
            leveldb::Status status = self->Write(self.get_write_options(), &batch);
            if (!status.ok()) {
                throw LevelDBException(status.ToString());
            }
        },
        py::doc("Set a group of values in the database."));

    LevelDB.def(
        "__contains__",
        [](Amulet::LevelDB& self, std::string key) {
            auto lock = self.lock_shared();
            if (!self) {
                throw LevelDBException("The LevelDB database has been closed.");
            }
            std::string value;
            return self->Get(self.get_read_options(), key, &value).ok();
        });

    auto get = [](Amulet::LevelDB& self, std::string key) {
        auto lock = self.lock_shared();
        if (!self) {
            throw LevelDBException("The LevelDB database has been closed.");
        }
        std::string value;
        auto status = self->Get(self.get_read_options(), key, &value);
        switch (status.code()) {
        case leveldb::Status::kOk:
            return value;
        case leveldb::Status::kNotFound:
            throw py::key_error(key);
        default:
            throw LevelDBException(status.ToString());
        }
    };
    LevelDB.def(
        "get",
        get,
        py::doc(
            "Get a key from the database.\n"
            "\n"
            ":param key: The key to get from the database.\n"
            ":return: The data stored behind the given key.\n"
            ":raises: KeyError if the requested key is not present.\n"
            ":raises: LevelDBException on other error."));
    LevelDB.def("__get_item__", get);

    auto del = [](Amulet::LevelDB& self, std::string key) {
        auto lock = self.lock_shared();
        if (!self) {
            throw LevelDBException("The LevelDB database has been closed.");
        }
        auto status = self->Delete(self.get_write_options(), key);
        if (!status.ok()) {
            throw LevelDBException(status.ToString());
        }
    };
    LevelDB.def(
        "delete",
        del,
        py::doc(
            "Delete a key from the database.\n"
            "\n"
            ":param key: The key to delete from the database."));
    LevelDB.def("__delitem__", del);

    LevelDB.def(
        "create_iterator",
        &Amulet::LevelDB::create_iterator,
        py::doc("Create a new leveldb Iterator."));

    LevelDB.def(
        "iterate",
        [](
            Amulet::LevelDB& self,
            std::optional<std::string> start,
            std::optional<std::string> end) -> std::variant<std::shared_ptr<LevelDBItemsIterator>,
                                                std::shared_ptr<LevelDBItemsRangeIterator>> {
            auto lock = self.lock_shared();
            if (!self) {
                throw LevelDBException("The LevelDB database has been closed.");
            }
            auto iterator_ptr = self.create_iterator();
            auto& iterator = *iterator_ptr;
            if (start) {
                iterator->Seek(*start);
            } else {
                iterator->SeekToFirst();
            }

            if (end) {
                return std::make_shared<LevelDBItemsRangeIterator>(std::move(iterator_ptr), *end);
            } else {
                return std::make_shared<LevelDBItemsIterator>(std::move(iterator_ptr));
            }
        },
        py::arg("start") = py::none(),
        py::arg("end") = py::none(),
        py::doc(
            "Iterate through all keys and data that exist between the given keys.\n"
            "\n"
            ":param start: The key to start at. Leave as None to start at the beginning.\n"
            ":param end: The key to end at. Leave as None to finish at the end."));

    LevelDB.def(
        "__iter__",
        [](Amulet::LevelDB& self) {
            auto iterator_ptr = self.create_iterator();
            auto& iterator = *iterator_ptr;
            iterator->SeekToFirst();
            return std::make_shared<LevelDBKeysIterator>(std::move(iterator_ptr));
        });
    LevelDB.def(
        "keys",
        [](Amulet::LevelDB& self) {
            auto iterator_ptr = self.create_iterator();
            auto& iterator = *iterator_ptr;
            iterator->SeekToFirst();
            return std::make_shared<LevelDBKeysIterator>(std::move(iterator_ptr));
        },
        py::doc("An iterable of all keys in the database."));

    LevelDB.def(
        "values",
        [](Amulet::LevelDB& self) {
            auto iterator_ptr = self.create_iterator();
            auto& iterator = *iterator_ptr;
            iterator->SeekToFirst();
            return std::make_shared<LevelDBValuesIterator>(std::move(iterator_ptr));
        },
        py::doc("An iterable of all values in the database."));

    LevelDB.def(
        "items",
        [](Amulet::LevelDB& self) {
            auto iterator_ptr = self.create_iterator();
            auto& iterator = *iterator_ptr;
            iterator->SeekToFirst();
            return std::make_shared<LevelDBItemsIterator>(std::move(iterator_ptr));
        },
        py::doc("An iterable of all items in the database."));
}

PYBIND11_MODULE(__init__, m) { init_leveldb(m); }
PYBIND11_MODULE(leveldb, m) { init_leveldb(m); }
