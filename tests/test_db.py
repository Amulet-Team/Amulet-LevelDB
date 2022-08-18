import unittest
from leveldb import LevelDB, LevelDBException
import os
import shutil
import struct

DB_PATH = "./worlds_temp/leveldb"

num_keys = [struct.pack("<Q", i) for i in range(10_000)]
num_db = dict(zip(num_keys, num_keys))
incr_db = {f"key{i}".encode("utf-8"): f"val{i}".encode("utf-8") for i in range(10_000)}
full_db = {**incr_db, **num_db}


class LevelDBTestCase(unittest.TestCase):
    def _clear_db(self):
        if os.path.isdir(DB_PATH):
            shutil.rmtree(DB_PATH)

    def test_create_ldb(self):
        self._clear_db()
        db = LevelDB(DB_PATH, True)
        db.close()

    def test_create_fail(self):
        self._clear_db()
        with self.assertRaises(LevelDBException):
            db = LevelDB(DB_PATH)

    def test_read_write(self):
        self._clear_db()
        db = LevelDB(DB_PATH, True)

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

    def test_put(self):
        self._clear_db()
        db = LevelDB(DB_PATH, True)

        db.putBatch(incr_db)

        for k, v in num_db.items():
            db.put(k, v)

        self.assertEqual(dict(db.iterate()), full_db)
        self.assertEqual(dict(db.items()), full_db)
        self.assertEqual(set(db.keys()), full_db.keys())
        self.assertEqual(set(db), full_db.keys())

        db.close()

    def test_contains(self):
        self._clear_db()
        db = LevelDB(DB_PATH, True)

        self.assertFalse(b"test_key2" in db)

        db.put(b"test_key2", b"test")

        self.assertTrue(b"test_key2" in db)

        db.close()

    def test_delete(self):
        self._clear_db()
        db = LevelDB(DB_PATH, True)

        self.assertFalse(b"test_key3" in db)

        db.put(b"test_key3", b"test")

        self.assertTrue(b"test_key3" in db)

        db.delete(b"test_key3")

        self.assertFalse(b"test_key3" in db)

        db.close()

    def test_exception(self):
        self._clear_db()
        db = LevelDB(DB_PATH, True)
        db.close()

        # if the db is closed all of these functions should error
        # they should not cause segmentation faults
        with self.assertRaises(LevelDBException):
            db.close()
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


if __name__ == "__main__":
    unittest.main()
