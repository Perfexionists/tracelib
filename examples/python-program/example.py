import tracelibpy
from pprint import pprint


def diffing():
    print("---------- CCT Diffing ----------")
    builder = tracelibpy.Builder_CCTree_PerfFoldedNodeData()

    parser = tracelibpy.PerfFoldedParser("./input/perf-folded-diffing-example-1.txt")
    tree = tracelibpy.CCTree_PerfFoldedNodeData()
    builder.build(tree, parser)

    print(tree.pid, tree.tid, tree.process_name)
    print(tree)
    print(", ".join(f"{node}" for node in tree))

    parser.set_trace_file("./input/perf-folded-diffing-example-2.txt")
    other_tree = tracelibpy.CCTree_PerfFoldedNodeData()
    builder.build(other_tree, parser)

    print(other_tree.pid, other_tree.tid, other_tree.process_name)
    print(other_tree)
    print(", ".join(f"{node}" for node in other_tree))

    x = tree.edit_distance(other_tree)
    print(f"Minimal edit distance: {x[0]}")
    operations = ""
    for operation in x[1]:
        operations += f"    {operation}\n"
    print(f"Operations corresponding to the edit distance: \n{operations}")


def pruning():
    print("---------- CCT Pruning ----------")
    builder = tracelibpy.Builder_CCTree_PerfFoldedNodeData()

    parser = tracelibpy.PerfFoldedParser("./input/perf-folded-pruning-example.txt")
    tree = tracelibpy.CCTree_PerfFoldedNodeData()
    builder.build(tree, parser)

    print(tree.pid, tree.tid, tree.process_name)
    print(tree)
    print(", ".join(f"{node}" for node in tree))

    tree.prune(25)

    print(tree.pid, tree.tid, tree.process_name)
    print(tree)
    print(", ".join(f"{node}" for node in tree))


def example_ccf():
    print("---------- Perun Pin Example CCF ----------")
    pin_forest_builder = tracelibpy.Builder_CCForest_PerunPinNodeData()
    pin_parser = tracelibpy.PerunPinParser(
        "./input/pin-rtn-example.txt",
        "./input/pin-rtn-example-metadata.json"
    )
    pin_forest = tracelibpy.CCForest_PerunPinNodeData()
    pin_forest_builder.build(pin_forest, pin_parser)
    print("Metadata:")
    for function_id, function in pin_parser.metadata.functions.items():
        print(f"{function_id=}")
        print(f"{function.name=}")
        print(f"{function.argument_indices=}")
        print(f"{function.location.file_id=}")
        print(f"{function.location.lines=}")

    print("Forest iteration:")
    for (pid, tid), tree in pin_forest:
        print(pid, tid)
        print(tree)
        print("Tree iterations:")
        print("pre-order: " + ", ".join(f"{node}" for node in tree.pre_order()))
        print("post-order: " + ", ".join(f"{node}" for node in tree.post_order()))
        print("level-order: " + ", ".join(f"{node}" for node in tree.level_order()))
        print("pre-order with path-to-root iterator:")
        for node in tree:
            print(", ".join(f"{n}" for n in tree.path_to_root(node)))
        print("Node data:")
        for node in tree:
            print(f"{node.function_name=}")
            print(f"{node.data.file_path=}")
            print(f"{node.data.lines=}")
            print(f"{node.data.durations=}")

    serialized_file = "test-serialization.txt"
    pin_forest_builder.serialize(pin_forest, serialized_file, tracelibpy.BOOST_BINARY, 3)

    print("deserialized:")
    pin_forest_deserialized = tracelibpy.CCForest_PerunPinNodeData()
    pin_forest_builder.deserialize(pin_forest_deserialized, serialized_file + ".zst", tracelibpy.BOOST_BINARY, True)

    for (pid, tid), tree in pin_forest_deserialized:
        print(pid, tid)
        print(tree)
        print(", ".join(f"{node}" for node in tree))
        for node in tree:
            print(f"{node.function_name=}")
            print(f"{node.data.file_path=}")
            print(f"{node.data.lines=}")
            print(f"{node.data.durations=}")


def example_dcg():
    print("---------- Perun Pin Example DCG ----------")
    pin_dcg_builder = tracelibpy.Builder_DCGraph_PerunPinNodeData()
    pin_parser = tracelibpy.PerunPinParser(
        "./input/pin-rtn-example.txt",
        "./input/pin-rtn-example-metadata.json"
    )
    pin_dcg = tracelibpy.DCGraph_PerunPinNodeData()
    pin_dcg_builder.build(pin_dcg, pin_parser)
    print("Metadata:")
    for function_id, function in pin_parser.metadata.functions.items():
        print(f"{function_id=}")
        print(f"{function.name=}")
        print(f"{function.argument_indices=}")
        print(f"{function.location.file_id=}")
        print(f"{function.location.lines=}")

    print("DCG iteration:")
    for (pid, tid), graph in pin_dcg:
        print(pid, tid)
        print(graph)
        print("CCG iterations:")
        print("bfs: " + ", ".join(f"{node}" for node in graph.bfs()))
        print("dfs: " + ", ".join(f"{node}" for node in graph.dfs()))
        print("DFS from each node of the graph:")
        for node in graph:
            print(f"{node.function_name=}")
            print(", ".join(f"{n}" for n in graph.bfs(node)))
        print("Node data:")
        for node in graph:
            print(f"{node.function_name=}")
            print(f"{node.data.file_path=}")
            print(f"{node.data.lines=}")
            print(f"{node.data.durations=}")

    serialized_file = "test-serialization.txt"
    pin_dcg_builder.serialize(pin_dcg, serialized_file, tracelibpy.BOOST_BINARY, 3)

    print("deserialized:")
    pin_dcg_deserialized = tracelibpy.DCGraph_PerunPinNodeData()
    pin_dcg_builder.deserialize(pin_dcg_deserialized, serialized_file + ".zst", tracelibpy.BOOST_BINARY, True)

    for (pid, tid), graph in pin_dcg_deserialized:
        print("Graph:")
        print(graph)
        print("Nodes:")
        print(", ".join(f"{node}" for node in graph))
        print("Node data:")
        for node in graph:
            print(f"{node.function_name=}")
            print(f"{node.data.file_path=}")
            print(f"{node.data.lines=}")
            print(f"{node.data.durations=}")


def main():
    pprint(tracelibpy.__dict__)

    diffing()
    pruning()
    example_ccf()
    example_dcg()


if __name__ == "__main__":
    main()
