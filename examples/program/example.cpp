#include <iostream>
#include <tracelib.hpp>


int main() {
  auto parser = PerunPinParser(
    "input/pin-rtn-example.txt",
    "input/pin-rtn-example-metadata.json"
  );
  auto cct = CCTree<PerunPinNodeData>();
  auto builder = Builder<CCTree<PerunPinNodeData>>();

  builder.build(&cct, &parser);
  std::cout << cct.toString() << std::endl;

  std::cout << "Number of events: " << parser.numberOfEvents << std::endl;
  std::cout << "Number of function calls: " << parser.numberOfFunctionCalls << std::endl;
  std::cout << "Number of nodes: " << cct.getNumberOfNodes() << std::endl;
  std::cout << "Number of functions: " << cct.getNumberOfFunctions() << std::endl;

  std::cout << "Serializing without compression..." << std::endl;
  builder.serialize(&cct, "serialized-1.bin", SerializationFormat::BOOST_BINARY);
  std::cout << "Serializing with compression..." << std::endl;
  builder.serialize(&cct, "serialized-2.bin", SerializationFormat::BOOST_BINARY, 3);

  auto cct_deserialized_1 = CCTree<PerunPinNodeData>();
  auto cct_deserialized_2 = CCTree<PerunPinNodeData>();

  std::cout << "Deserializing without compression..." << std::endl;
  builder.deserialize(&cct_deserialized_1, "serialized-1.bin", SerializationFormat::BOOST_BINARY);
  std::cout << "Number of nodes: " << cct_deserialized_1.getNumberOfNodes() << std::endl;
  std::cout << "Number of functions: " << cct_deserialized_1.getNumberOfFunctions() << std::endl;
  bool areEqual = cct == cct_deserialized_1;
  std::cout << "Deserialized (without compression) equal to original: " << (areEqual ? "true" : "false") << std::endl;

  std::cout << "Deserializing with compression..." << std::endl;
  builder.deserialize(&cct_deserialized_2, "serialized-2.bin.zst", SerializationFormat::BOOST_BINARY, true);
  std::cout << "Number of nodes: " << cct_deserialized_2.getNumberOfNodes() << std::endl;
  std::cout << "Number of functions: " << cct_deserialized_2.getNumberOfFunctions() << std::endl;
  areEqual = cct == cct_deserialized_2;
  std::cout << "Deserialized (with compression) equal to original: " << (areEqual ? "true" : "false") << std::endl;

  return 0;
}
