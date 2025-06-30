#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/typing.h>

#include <filesystem>
#include <optional>
#include <string>
#include <variant>

#include <leveldb/cache.h>
#include <leveldb/db.h>
#include <leveldb/decompress_allocator.h>
#include <leveldb/env.h>
#include <leveldb/filter_policy.h>
#include <leveldb/options.h>
#include <leveldb/write_batch.h>

#include <amulet/pybind11_extensions/compatibility.hpp>
#include <amulet/pybind11_extensions/iterator.hpp>

#include <amulet/leveldb.hpp>

namespace py = pybind11;
namespace pyext = Amulet::pybind11_extensions;

namespace PYBIND11_NAMESPACE {
namespace detail {
    template <>
    struct type_caster<leveldb::Slice> {
    public:
        PYBIND11_TYPE_CASTER(leveldb::Slice, const_name("bytes"));

        bool load(handle src, bool)
        {
            PyObject* source = src.ptr();
            if (!PyBytes_Check(source)) {
                return false;
            }
            Py_ssize_t size = PyBytes_Size(src.ptr());
            const char* buffer = PyBytes_AsString(src.ptr());
            if (!buffer) {
                return false;
            }
            value = leveldb::Slice(buffer, size);
            return true;
        }

        // This causes a crash that I don't understand
        // static handle cast(const leveldb::Slice& src, return_value_policy /* policy */, handle /* parent */)
        //{
        //    return py::bytes(src.data(), src.size());
        //}
    };

    template <>
    struct type_caster<leveldb::WriteBatch> {
    public:
        PYBIND11_TYPE_CASTER(leveldb::WriteBatch, const_name("collections.abc.Mapping[bytes, bytes]"));

        bool load(handle src, bool)
        {
            auto getitem = src.attr("__getitem__");
            for (auto& key : src) {
                if (!PyBytes_Check(key.ptr())) {
                    return false;
                }
                Py_ssize_t key_size = PyBytes_Size(key.ptr());
                const char* key_buffer = PyBytes_AsString(key.ptr());
                if (!key_buffer) {
                    return false;
                }

                auto val = getitem(key);
                if (val.is_none()) {
                    value.Delete(leveldb::Slice(key_buffer, key_size));
                } else {
                    Py_ssize_t val_size = PyBytes_Size(val.ptr());
                    const char* val_buffer = PyBytes_AsString(val.ptr());
                    if (!val_buffer) {
                        return false;
                    }
                    value.Put(leveldb::Slice(key_buffer, key_size), leveldb::Slice(val_buffer, val_size));
                }
            }
            return true;
        }
    };
}
} // namespace PYBIND11_NAMESPACE::detail

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
    leveldb::DecompressAllocator decompress_allocator;
};

std::unique_ptr<Amulet::LevelDB> open_leveldb(
    std::string path_str,
    bool create_if_missing = false,
    leveldb::CompressionType compression_type = leveldb::kZlibRawCompression)
{
    // Expand dots and symbolic links
    auto path = std::filesystem::absolute(path_str);
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
    options->options.compression = compression_type;
    options->options.block_size = 163840;

    options->read_options.decompress_allocator = &options->decompress_allocator;

    leveldb::DB* _db = NULL;
    auto status = leveldb::DB::Open(options->options, path.string(), &_db);
    if (status.ok()) {
        return std::make_unique<Amulet::LevelDB>(
            std::unique_ptr<leveldb::DB>(_db),
            std::move(options));
    } else if (status.IsCorruption()) {
        leveldb::RepairDB(path.string(), options->options);
        {
            auto status2 = leveldb::DB::Open(options->options, path.string(), &_db);
            if (status2.ok()) {
                return std::make_unique<Amulet::LevelDB>(
                    std::unique_ptr<leveldb::DB>(_db),
                    std::move(options));
            }
        }
        throw LevelDBException("Could not recover corrupted database. " + status.ToString());
    } else if (status.IsNotSupportedError()) {
        if (status.ToString().ends_with("Marketplace worlds are not supported.")) {
            throw LevelDBEncrypted("Marketplace worlds are not supported.");
        }
    }
    throw LevelDBException(status.ToString());
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

} // namespace

void init_module(py::module m)
{
    pyext::init_compiler_config(m);
    std::string module_name = m.attr("__name__").cast<std::string>();

    py::register_local_exception<LevelDBException>(m, "LevelDBException");
    py::register_local_exception<LevelDBEncrypted>(m, "LevelDBEncrypted");

    py::class_<Amulet::LevelDBIterator> LevelDBIterator(m, "LevelDBIterator");
    LevelDBIterator.def(
        "valid",
        [](Amulet::LevelDBIterator& self) {
            return self && self->Valid();
        },
        py::doc(
            "Is the iterator at a valid entry.\n"
            "If False, calls to other methods may error."));
    LevelDBIterator.def(
        "seek_to_first",
        [](Amulet::LevelDBIterator& self) {
            if (!self) {
                throw std::runtime_error("LevelDBIterator has been deleted.");
            }
            self->SeekToFirst();
        },
        py::doc("Seek to the first entry in the database."));
    LevelDBIterator.def(
        "seek_to_last",
        [](Amulet::LevelDBIterator& self) {
            if (!self) {
                throw std::runtime_error("LevelDBIterator has been deleted.");
            }
            self->SeekToLast();
        },
        py::doc("Seek to the last entry in the database."));
    LevelDBIterator.def(
        "seek",
        [](Amulet::LevelDBIterator& self, leveldb::Slice target) {
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
            if (!self) {
                throw std::runtime_error("LevelDBIterator has been deleted.");
            }
            if (!self->Valid()) {
                throw std::runtime_error("LevelDBIterator does not point to a valid value.");
            }
            return py::bytes(self->key().data(), self->key().size());
        },
        py::doc(
            "Get the key of the current entry in the database.\n"
            ":raises: runtime_error if iterator is not valid."));
    LevelDBIterator.def(
        "value",
        [](Amulet::LevelDBIterator& self) {
            if (!self) {
                throw std::runtime_error("LevelDBIterator has been deleted.");
            }
            if (!self->Valid()) {
                throw std::runtime_error("LevelDBIterator does not point to a valid value.");
            }
            return py::bytes(self->value().data(), self->value().size());
        },
        py::doc(
            "Get the value of the current entry in the database.\n"
            ":raises: runtime_error if iterator is not valid."));

    py::enum_<leveldb::CompressionType> CompressionType(m, "CompressionType");
    CompressionType.value(
        "NoCompression",
        leveldb::CompressionType::kNoCompression,
        "No compression.");
    CompressionType.value(
        "SnappyCompression",
        leveldb::CompressionType::kSnappyCompression,
        "Snappy compression.");
    CompressionType.value(
        "ZstdCompression",
        leveldb::CompressionType::kZstdCompression,
        "Zstd compression.");
    CompressionType.value(
        "ZlibRawCompression",
        leveldb::CompressionType::kZlibRawCompression,
        "Zlib raw compression.");
    CompressionType.attr("__repr__") = py::cpp_function(
        [module_name, CompressionType](const py::object& arg) -> py::str {
            return py::str("{}.{}").format(module_name, CompressionType.attr("__str__")(arg));
        },
        py::name("__repr__"),
        py::is_method(CompressionType));

    py::class_<Amulet::LevelDB> LevelDB(m, "LevelDB",
        "A LevelDB database");
    LevelDB.def(
        py::init(&open_leveldb),
        py::arg("path"),
        py::arg("create_if_missing") = false,
        py::arg("compression_type") = leveldb::kZlibRawCompression,
        py::doc(
            "Construct a new :class :`LevelDB` instance from the database at the given path.\n"
            "\n"
            "A leveldb database is like a dictionary that only contains bytes as the keys and values and exists entirely on the disk.\n"
            "\n"
            ":param path: The path to the database directory.\n"
            ":param create_if_missing: If True a new database will be created if one does not exist at the given path.\n"
            ":param compression_type: The compression type to use when writing data to the database. Defaults to zlib raw.\n"
            ":raises: LevelDBException if create_if_missing is False and the db does not exist."));

    LevelDB.def(
        "close",
        &Amulet::LevelDB::close,
        py::doc(
            "Close the leveldb database.\n"
            "Only the owner of the database may close it.\n"
            "If needed, an external lock must be used to ensure that no other threads are accessing the database."),
        py::call_guard<py::gil_scoped_release>());

    LevelDB.def(
        "compact",
        [](Amulet::LevelDB& self) {
            py::gil_scoped_release gil;
            if (!self) {
                throw std::runtime_error("The LevelDB database has been closed.");
            }
            self->CompactRange(nullptr, nullptr);
        },
        py::doc("Remove deleted entries from the database to reduce its size."));

    auto put = [](Amulet::LevelDB& self, leveldb::Slice key, leveldb::Slice value) {
        py::gil_scoped_release gil;
        if (!self) {
            throw std::runtime_error("The LevelDB database has been closed.");
        }
        auto status = self->Put(self.get_write_options(), key, value);
        if (!status.ok()) {
            throw LevelDBException(status.ToString());
        }
    };
    LevelDB.def("put", put, py::arg("key"), py::arg("value"), py::doc("Set a value in the database."));
    LevelDB.def("__setitem__", put, py::arg("key"), py::arg("value"));

    LevelDB.def(
        "put_batch",
        [](Amulet::LevelDB& self, leveldb::WriteBatch batch) {
            py::gil_scoped_release gil;
            if (!self) {
                throw std::runtime_error("The LevelDB database has been closed.");
            }
            leveldb::Status status = self->Write(self.get_write_options(), &batch);
            if (!status.ok()) {
                throw LevelDBException(status.ToString());
            }
        },
        py::arg("batch"),
        py::doc("Set a group of values in the database."));

    LevelDB.def(
        "__contains__",
        [](Amulet::LevelDB& self, leveldb::Slice key) {
            py::gil_scoped_release gil;
            if (!self) {
                throw std::runtime_error("The LevelDB database has been closed.");
            }
            std::string value;
            return self->Get(self.get_read_options(), key, &value).ok();
        },
        py::arg("key"));

    auto get = [](Amulet::LevelDB& self, leveldb::Slice key) {
        std::string value;
        leveldb::Status status;
        {
            py::gil_scoped_release gil;
            if (!self) {
                throw std::runtime_error("The LevelDB database has been closed.");
            }
            status = self->Get(self.get_read_options(), key, &value);
        }
        if (status.ok()) {
            return py::bytes(value);
        } else if (status.IsNotFound()) {
            throw py::key_error(key.ToString());
        } else {
            throw LevelDBException(status.ToString());
        }
    };
    LevelDB.def(
        "get",
        get,
        py::arg("key"),
        py::doc(
            "Get a key from the database.\n"
            "\n"
            ":param key: The key to get from the database.\n"
            ":return: The data stored behind the given key.\n"
            ":raises: KeyError if the requested key is not present.\n"
            ":raises: LevelDBException on other error."));
    LevelDB.def("__getitem__", get, py::arg("key"));

    auto del = [](Amulet::LevelDB& self, leveldb::Slice key) {
        py::gil_scoped_release gil;
        if (!self) {
            throw std::runtime_error("The LevelDB database has been closed.");
        }
        auto status = self->Delete(self.get_write_options(), key);
        if (!status.ok()) {
            throw LevelDBException(status.ToString());
        }
    };
    LevelDB.def(
        "delete",
        del,
        py::arg("key"),
        py::doc(
            "Delete a key from the database.\n"
            "\n"
            ":param key: The key to delete from the database."));
    LevelDB.def("__delitem__", del, py::arg("key"));

    LevelDB.def(
        "create_iterator",
        &Amulet::LevelDB::create_iterator,
        py::doc("Create a new leveldb Iterator."));

    LevelDB.def(
        "iterate",
        [](
            Amulet::LevelDB& self,
            std::optional<py::bytes> start,
            std::optional<py::bytes> end) {
            if (!self) {
                throw std::runtime_error("The LevelDB database has been closed.");
            }
            auto iterator_ptr = self.create_iterator();
            auto& iterator = *iterator_ptr;
            if (start) {
                iterator->Seek(start->cast<std::string>());
            } else {
                iterator->SeekToFirst();
            }

            if (end) {
                return pyext::make_iterator(
                    LevelDBItemsRangeIterator(std::move(iterator_ptr), end->cast<std::string>()));
            } else {
                return pyext::make_iterator(
                    LevelDBItemsIterator(std::move(iterator_ptr)));
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
            return pyext::make_iterator(
                LevelDBKeysIterator(std::move(iterator_ptr)));
        });
    LevelDB.def(
        "keys",
        [](Amulet::LevelDB& self) {
            auto iterator_ptr = self.create_iterator();
            auto& iterator = *iterator_ptr;
            iterator->SeekToFirst();
            return pyext::make_iterator(
                LevelDBKeysIterator(std::move(iterator_ptr)));
        },
        py::doc("An iterable of all keys in the database."));

    LevelDB.def(
        "values",
        [](Amulet::LevelDB& self) {
            auto iterator_ptr = self.create_iterator();
            auto& iterator = *iterator_ptr;
            iterator->SeekToFirst();
            return pyext::make_iterator(
                LevelDBValuesIterator(std::move(iterator_ptr)));
        },
        py::doc("An iterable of all values in the database."));

    LevelDB.def(
        "items",
        [](Amulet::LevelDB& self) {
            auto iterator_ptr = self.create_iterator();
            auto& iterator = *iterator_ptr;
            iterator->SeekToFirst();
            return pyext::make_iterator(
                LevelDBItemsIterator(std::move(iterator_ptr)));
        },
        py::doc("An iterable of all items in the database."));
}

PYBIND11_MODULE(_leveldb, m)
{
    py::options options;
    options.disable_function_signatures();
    m.def("init", &init_module, py::doc("init(arg0: types.ModuleType) -> None"));
    options.enable_function_signatures();
}
