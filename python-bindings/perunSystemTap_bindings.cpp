#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "tracelib/perunSystemTap.hpp"

namespace py = pybind11;

void perunSystemTapBindings(py::module& m) {
    py::class_<PerunSystemTapParser::Metadata>(m, "PerunSystemTapMetadata")
        .def_readonly("functions", &PerunSystemTapParser::Metadata::functions)
    ;

    py::class_<PerunSystemTapParser, Parser>(m, "PerunSystemTapParser")
        .def(py::init<const std::string&, const std::string&>(),
            py::arg("traceFilePath"), py::arg("metadataFIlePath") = "")
        .def_readonly("metadata", &PerunSystemTapParser::metadata)
    ;

    py::class_<PerunSystemTapNodeData>(m, "PerunSystemTapNodeData")
        .def(py::init<>())
        .def_readonly("durations", &PerunSystemTapNodeData::durations)
    ;
}
