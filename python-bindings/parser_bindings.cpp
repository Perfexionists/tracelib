#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "tracelib/parser.hpp"

namespace py = pybind11;

void parserBindings(py::module& m) {
    // Note: needs to be defined before all the derived classes (PerfFoldedParser, PerunPinParser, PerunSystemTapParser)
    py::class_<Parser>(m, "Parser")
        .def("set_trace_file", &Parser::setTraceFile, py::arg("trace_file_path"))
        .def("get_trace_file", &Parser::getTraceFile, py::return_value_policy::copy)
        .def("set_metadata_file", &Parser::setMetadataFile, py::arg("trace_file_path"))
        .def("get_metadata_file", &Parser::getMetadataFile, py::return_value_policy::copy)
    ;
}
