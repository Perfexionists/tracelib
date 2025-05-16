#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "tracelib/serialization.hpp"

namespace py = pybind11;

void serializationBindings(py::module& m) {
     py::enum_<SerializationFormat>(m, "SerializationFormat")
        .value("BOOST_BINARY", SerializationFormat::BOOST_BINARY)
        .value("BOOST_TEXT", SerializationFormat::BOOST_TEXT)
        .value("PERF_FOLDED", SerializationFormat::PERF_FOLDED)
        .export_values();
}
