import unittest
from leveldb import LevelDB, LevelDBException
import struct
from tempfile import TemporaryDirectory
from uuid import uuid4
import glob
import os

num_keys = [struct.pack("<Q", i) for i in range(10_000)]
num_db = dict(zip(num_keys, num_keys))
incr_db = {f"key{i}".encode("utf-8"): f"val{i}".encode("utf-8") for i in range(10_000)}
full_db = {**incr_db, **num_db}


def get_directory_size(path: str):
    return sum(
        (os.path.getsize(item.path) for item in os.scandir(path) if item.is_file())
    )


class LevelDBTestCase(unittest.TestCase):
    def test_create_ldb(self) -> None:
        with TemporaryDirectory() as path:
            db = LevelDB(path, True)
            db.close()

    def test_create_fail(self) -> None:
        with self.assertRaises(LevelDBException):
            LevelDB("path")

    def test_read_write(self) -> None:
        with TemporaryDirectory() as path:
            db = LevelDB(path, True)

            with self.assertRaises(KeyError):
                db.get(b"random_key")

            key1 = b"key"
            value1 = b"value"
            db.put(key1, value1)
            self.assertEqual(db.get(key1), value1)

            key2 = key1 * 1000
            value2 = value1 * 1000
            db.put(key2, value2)
            self.assertEqual(db.get(key2), value2)

            db.close()

    def test_put(self) -> None:
        with TemporaryDirectory() as path:
            db = LevelDB(path, True)

            db.putBatch(incr_db)

            for k, v in num_db.items():
                db.put(k, v)

            self.assertEqual(dict(db.iterate()), full_db)
            self.assertEqual(dict(db.items()), full_db)
            self.assertEqual(set(db.keys()), full_db.keys())
            self.assertEqual(set(db), full_db.keys())

            db.close()

    def test_get_set_item(self) -> None:
        with TemporaryDirectory() as path:
            db = LevelDB(path, True)

            for k, v in num_db.items():
                db[k] = v

            for k, v in num_db.items():
                self.assertEqual(v, db[k])

            db.close()

    def test_contains(self) -> None:
        with TemporaryDirectory() as path:
            db = LevelDB(path, True)

            self.assertFalse(b"test_key2" in db)

            db.put(b"test_key2", b"test")

            self.assertTrue(b"test_key2" in db)

            db.close()

    def test_delete(self) -> None:
        with TemporaryDirectory() as path:
            db = LevelDB(path, True)

            self.assertFalse(b"test_key3" in db)

            db.put(b"test_key3", b"test")

            self.assertTrue(b"test_key3" in db)

            db.delete(b"test_key3")

            self.assertFalse(b"test_key3" in db)

            db[b"test_key3"] = b"test"

            self.assertTrue(b"test_key3" in db)

            del db[b"test_key3"]

            self.assertFalse(b"test_key3" in db)

            db.close()

    def test_exception(self) -> None:
        with TemporaryDirectory() as path:
            db = LevelDB(path, True)
            db.close()

            # if the db is closed all of these functions should error
            # they should not cause segmentation faults
            db.close()  # This should do nothing.
            with self.assertRaises(LevelDBException):
                db.get(b"key")
            with self.assertRaises(LevelDBException):
                db.put(b"key", b"value")
            with self.assertRaises(LevelDBException):
                db.putBatch({b"key": b"value"})
            with self.assertRaises(LevelDBException):
                db.delete(b"key")
            with self.assertRaises(LevelDBException):
                list(db.iterate(b"\x00", b"\xFF"))
            with self.assertRaises(LevelDBException):
                list(db.keys())
            with self.assertRaises(LevelDBException):
                list(db.items())
            with self.assertRaises(LevelDBException):
                b"key" in db
            with self.assertRaises(LevelDBException):
                list(db)

    def test_iterate_twice(self) -> None:
        with TemporaryDirectory() as path:
            db = LevelDB(path, True)
            db.putBatch(
                {
                    b"a": b"1",
                    b"b": b"2",
                    b"c": b"3",
                    b"d": b"4",
                    b"e": b"5",
                    b"f": b"6",
                }
            )

            it1 = db.iterate()
            self.assertEqual((b"a", b"1"), next(it1))
            self.assertEqual((b"b", b"2"), next(it1))
            self.assertEqual((b"c", b"3"), next(it1))
            it2 = db.iterate()
            self.assertEqual((b"a", b"1"), next(it2))
            self.assertEqual((b"d", b"4"), next(it1))
            self.assertEqual((b"b", b"2"), next(it2))

            db.close()

    def test_keys_twice(self) -> None:
        with TemporaryDirectory() as path:
            db = LevelDB(path, True)
            db.putBatch(
                {
                    b"a": b"1",
                    b"b": b"2",
                    b"c": b"3",
                    b"d": b"4",
                    b"e": b"5",
                    b"f": b"6",
                }
            )

            it1 = db.keys()
            self.assertEqual(b"a", next(it1))
            self.assertEqual(b"b", next(it1))
            self.assertEqual(b"c", next(it1))
            it2 = db.keys()
            self.assertEqual(b"a", next(it2))
            self.assertEqual(b"d", next(it1))
            self.assertEqual(b"b", next(it2))

            db.close()

    def test_iter_mutate(self) -> None:
        with TemporaryDirectory() as path:
            db = LevelDB(path, True)
            db.putBatch(
                {
                    b"a": b"1",
                    b"b": b"2",
                    b"c": b"3",
                    b"d": b"4",
                    b"e": b"5",
                    b"f": b"6",
                }
            )

            it = db.iterate()
            self.assertEqual((b"a", b"1"), next(it))
            self.assertEqual((b"b", b"2"), next(it))
            self.assertEqual((b"c", b"3"), next(it))
            db.put(b"d", b"10")
            self.assertEqual(b"10", db.get(b"d"))
            self.assertEqual((b"d", b"4"), next(it))
            self.assertEqual((b"e", b"5"), next(it))
            self.assertEqual((b"f", b"6"), next(it))
            with self.assertRaises(StopIteration):
                next(it)

            self.assertEqual(
                {
                    b"a": b"1",
                    b"b": b"2",
                    b"c": b"3",
                    b"d": b"10",
                    b"e": b"5",
                    b"f": b"6",
                },
                dict(db),
            )

            db.close()

    def test_lock(self) -> None:
        with TemporaryDirectory() as path:
            db = LevelDB(path, True)
            db.close()

            manifest_paths = glob.glob(os.path.join(path, "MANIFEST-*"))
            self.assertEqual(1, len(manifest_paths))
            manifest_path = manifest_paths[0]

            db = LevelDB(path, True)
            try:
                manifest_paths = glob.glob(os.path.join(path, "MANIFEST-*"))
                self.assertEqual(1, len(manifest_paths))
                self.assertNotEqual(manifest_path, manifest_paths[0])
            finally:
                db.close()

    def test_compact(self) -> None:
        with TemporaryDirectory() as path:
            db = LevelDB(path, True)
            try:
                for _ in range(100_000):
                    key = str(uuid4()).encode()
                    db.put(key, key)
            finally:
                db.close()

            self.assertGreater(get_directory_size(path), 1_000_000)

            db = LevelDB(path)
            try:
                for key in db.keys():
                    db.delete(key)
                db.compact()
            finally:
                db.close()

            self.assertLess(get_directory_size(path), 10_000)

    def test_corrupt(self) -> None:
        """Test how the library handles a corrupt db."""
        with TemporaryDirectory() as path:
            db = LevelDB(path, True)
            try:
                for _ in range(100_000):
                    key = str(uuid4()).encode()
                    db.put(key, key)
            finally:
                db.close()

            # delete one of the ldb files
            os.remove(next(glob.iglob(os.path.join(glob.escape(path), "*.ldb"))))

            db = LevelDB(path, True)
            try:
                self.assertTrue(40_000 <= len(list(db.keys())) < 100_000)
            finally:
                db.close()


if __name__ == "__main__":
    unittest.main()
