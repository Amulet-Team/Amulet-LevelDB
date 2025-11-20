#include <pybind11/pybind11.h>

#include <amulet/pybind11_extensions/compatibility.hpp>

namespace py = pybind11;
namespace pyext = Amulet::pybind11_extensions;

void init_module(py::module m){
    pyext::init_compiler_config(m);
    pyext::check_compatibility(py::module::import("amulet.leveldb"), m);
}

PYBIND11_MODULE(_test_amulet_leveldb, m) {
    m.def("init", &init_module, py::arg("m"));
}
