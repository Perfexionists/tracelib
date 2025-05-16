#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "tracelib/perfFolded.hpp"
#include "tracelib/parser.hpp"

namespace py = pybind11;

void perfFoldedBindings(py::module& m) {
    py::class_<PerfFoldedParser, Parser>(m, "PerfFoldedParser")
        .def(py::init<const std::string&, const std::string&>(),
            py::arg("traceFilePath"), py::arg("metadataFIlePath") = "")
    ;

    py::class_<PerfFoldedNodeData>(m, "PerfFoldedNodeData")
        .def(py::init<>())
        .def_readonly("sample_cnt", &PerfFoldedNodeData::samplesCnt)
    ;
}
