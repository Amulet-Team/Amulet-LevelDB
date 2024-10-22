#pragma once
#include <string>
#include <unordered_map>
#include <optional>
#include <shared_mutex>
#include <memory>
#include <set>
#include <vector>
#include <filesystem>

#include <leveldb/db.h>
#include <leveldb/status.h>
#include <leveldb/options.h>
#include <leveldb/write_batch.h>
#include <leveldb/iterator.h>
#include <leveldb/decompress_allocator.h>
#include <leveldb/comparator.h>
#include <leveldb/env.h>
#include <leveldb/cache.h>
#include <leveldb/slice.h>
#include <leveldb/compressor.h>
#include <leveldb/zlib_compressor.h>
#include <leveldb/decompress_allocator.h>
#include <leveldb/filter_policy.h>


namespace Amulet {
	class NullLogger : public leveldb::Logger {
	public:
		void Logv(const char*, va_list) override {}
	};

	class LevelDBException : public std::runtime_error {
		using std::runtime_error::runtime_error;
	};
	class LevelDBEncrypted : public LevelDBException {
		using LevelDBException::LevelDBException;
	};
	class LevelDBIteratorException : public LevelDBException {
		using LevelDBException::LevelDBException;
	};

	class LevelDBIterator {
	private:
		std::shared_mutex mutex;
		std::unique_ptr<leveldb::Iterator> iterator;
		std::vector<std::function<void()>> on_destroy;

		friend class LevelDB;
		void register_on_destroy(std::function<void()> f) {
			on_destroy.push_back(f);
		}

	public:
		LevelDBIterator(
			std::unique_ptr<leveldb::Iterator>&& iterator,
			std::function<void()> on_destroy
		) : iterator(std::move(iterator)) {}
		void destroy() {
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
		~LevelDBIterator() {
			destroy();
		}
		bool is_destroyed() {
			std::shared_lock<std::shared_mutex> lock(mutex);
			return static_cast<bool>(iterator);
		}
		bool valid() {
			std::shared_lock<std::shared_mutex> lock(mutex);
			return iterator && iterator->Valid();
		}
	};

	class LevelDB {
	private:
		// Mutex to ensure nothing is processing when we close the database.
		std::shared_mutex mutex;

		// Leveldb objects
		std::unique_ptr<leveldb::DB> db;
		
		// Options
		NullLogger logger;
		leveldb::ZlibCompressorRaw zlib_compressor_raw;
		leveldb::ZlibCompressor zlib_compressor;
		leveldb::Options options;
		leveldb::DecompressAllocator decompress_allocator;
		leveldb::ReadOptions read_options;
		leveldb::WriteOptions write_options;

		// The iterators created by the leveldb object.
		// We need to allow deletion of the iterators while the db is open.
		// We need to destroy all iterators before closing the database.
		// We can't store them in a shared_ptr because this would keep them alive.
		// We can't store them in a weak_ptr because that does not support comparison for lookup.
		// The only way I can find is raw pointers.
		// During destruction of the iterator a callback will remove the pointer.
		std::set<LevelDBIterator*> iterators;

		LevelDB(const LevelDB&) {};
		void operator=(const LevelDB&) {};

		void check_db() {
			if (!db) {
				throw LevelDBException("The database has been closed.");
			}
		}
		std::unique_ptr<LevelDBIterator> create_iterator() {
			std::shared_lock<std::shared_mutex> lock(mutex);
			check_db();

			// Create the iterator
			auto iterator = std::make_unique<LevelDBIterator>(db->NewIterator(read_options));

			// Get a raw poiner to the iterator
			LevelDBIterator* ptr = iterator.get();

			// Add the iterator pointer to the set
			iterators.insert(ptr);

			// Create on destroy callback
			auto on_destroy = [this, &ptr]() {
				// Remove the pointer from the set
				iterators.erase(ptr);
				};

			// Register on destory callback
			iterator->register_on_destroy(on_destroy);

			// Return newly created iterator
			return iterator;
		}

	public:
		LevelDB(std::filesystem::path path, bool create_if_missing = false) :
			zlib_compressor_raw(-1),
			zlib_compressor(-1)
		{
			// Expand dots and symbolic links
			path = std::filesystem::canonical(path);
			// If there is not a directory at the path
			if (!std::filesystem::is_directory(path)) {
				if (std::filesystem::exists(path)) {
					// If the path exists but is not a directory
					throw LevelDBException("A non-directory file exists at " + path.string());
				}
				else if (create_if_missing) {
					// Create if requested
					std::filesystem::create_directories(path);
				}
				else {
					throw LevelDBException("No database exists to open at " + path.string());
				}
			}

			options.create_if_missing = create_if_missing;
			options.filter_policy = leveldb::NewBloomFilterPolicy(10);
			options.block_cache = leveldb::NewLRUCache(40 * 1024 * 1024);
			options.write_buffer_size = 4 * 1024 * 1024;
			options.info_log = &logger;
			options.compressors[0] = &zlib_compressor_raw;
			options.compressors[1] = &zlib_compressor;
			options.block_size = 163840;
			
			read_options.decompress_allocator = &decompress_allocator;

			leveldb::DB* _db = NULL;
			auto status = leveldb::DB::Open(options, path.string(), &_db);
			switch (status.code()) {
			case leveldb::Status::kOk:
				db = std::make_unique<leveldb::DB>(_db);
				break;
			case leveldb::Status::kCorruption:
				leveldb::RepairDB(path.string(), options);
				{
					auto status2 = leveldb::DB::Open(options, path.string(), &_db);
					if (status2.ok()) {
						db = std::make_unique<leveldb::DB>(_db);
					}
					else {
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
		};

		void close(bool compact = false) {
			while (!iterators.empty()) {
				(*iterators.begin())->destroy();
			}
		};
		~LevelDB() {
			close();
		};
		void compact();
		void Put(std::string key, std::string value);
		std::string Get(std::string key);
		void putBatch(std::unordered_map<std::string, std::optional<std::string>> batch);
		void Delete(std::string key);
		void iterate(
			std::optional<std::string> start = std::nullopt,
			std::optional<std::string> end = std::nullopt
		);
		void keys();
		void items();
	};

} // namespace Amulet
