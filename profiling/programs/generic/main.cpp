#include <iostream>
#include <tracelib.hpp>

int main(int argc, const char *argv[]) {
    if (argc != 4) {
        std::cerr << "Invalid number of argumenrs." << std::endl;
        return EXIT_FAILURE;
    }

    // FIXME Ignoring the test name, argv[2], for now
    auto cct = buildParCCT(argv[3], std::stoi(argv[1]));

    std::cout << "Number of nodes: " << cct.getNumberOfNodes() << std::endl;
    std::cout << "Number of functions: " << cct.getNumberOfFunctions() << std::endl;

    return EXIT_SUCCESS;
}
