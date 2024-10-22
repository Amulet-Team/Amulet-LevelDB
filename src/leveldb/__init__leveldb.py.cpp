#include <pybind11/pybind11.h>
#include "leveldb.hpp"

namespace py = pybind11;


static bool init_run = false;

void init_leveldb(py::module m){
    if (init_run){ return; }
    init_run = true;
}

PYBIND11_MODULE(__init__, m) { init_leveldb(m); }
PYBIND11_MODULE(leveldb, m) { init_leveldb(m); }
