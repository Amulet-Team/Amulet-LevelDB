#include <filesystem>
#include <optional>

#include <leveldb/cache.h>
#include <leveldb/db.h>
#include <leveldb/decompress_allocator.h>
#include <leveldb/env.h>
#include <leveldb/filter_policy.h>
#include <leveldb/write_batch.h>
#include <leveldb/zlib_compressor.h>

#include "leveldb.hpp"
#include <pybind11/pybind11.h>

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
            std::make_unique<leveldb::DB>(_db),
            options);
    case leveldb::Status::kCorruption:
        leveldb::RepairDB(path.string(), options->options);
        {
            auto status2 = leveldb::DB::Open(options->options, path.string(), &_db);
            if (status2.ok()) {
                return std::make_unique<Amulet::LevelDB>(
                    std::make_unique<leveldb::DB>(_db),
                    options);
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
}

PYBIND11_MODULE(__init__, m) { init_leveldb(m); }
PYBIND11_MODULE(leveldb, m) { init_leveldb(m); }
