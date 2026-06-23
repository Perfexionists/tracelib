# High level documentation

TraceLib is a library for processing program traces. It supports multiple formats and data structures, however I have only worked with Perf Folded format and the Calling Context Tree data structure. My additions to the project were generic optimization and buildParCCT, a function which builds a CCTree in parallel. The following is an overview of how this function works and lessons learned.

buildParCCT first mmaps the input file, and splits the work evenly among the threads, with each thread processing a different section of the sample data. Each thread then regularly builds a CCTree from its data. Finally, all threads merge their trees together to form the final result.
This was just the simplest approach, the merging at the end gets costly with increasing threads. Could consider removing the need for merging by having the threads all work on the same data structure. This would, however, require synchronization overhead, don't know if it's worth it.

The single threaded building of a tree calls Builder::build, which basically just loops for all lines in the file, parses each line into an Event using PerfFoldedParser::getNextEvent, and finally processes each event using EventProcessor::processEvent. These two functions are very important to optimize, because they get called for every line on the input.

getNextEvent processes a line of delimited function names into a vector of string_views. Having string_views instead of strings is a big performance boost; one should seek to avoid as many string copies as possible.
Each getNextEvent call, a new Event is allocated, and a stack sample vector gets reallocated multiple times. This is a result of the generic design of the library and removing this repeated construction and destruction will probably result in a performance improvement.

processEvent basically just calls callEventHandler, which for Perf Folded format always results calling handleStackSampleEvent. Again, this is because the library supports multiple types of events, but creating a specialized function for CCTrees (or somehow making the switch case into a compile-time check) should increase performance.

handleStackSampleEvent iterates over the Event's stack sample vector, traverses the CCTree, and adds new nodes when necessary.
Before working with a function name, it is first mapped to an index. This makes further comparisons between such keys faster, but adds overhead of lookups into a map. It is up to debate if this mapping is beneficial, one could try removing it. In a CCTree with around 1 million nodes, there were around 5 thousand unique function names. If not removing the map completely, one could try using a map optimized for fast lookups and slower inserts, since inserts occur relatively rarely.
Additionally, handleStackSampleEvent includes an optimization of storing the last processed line. This is very helpful, since adjacent lines tend to be similar, and allow for skipping of the first few map lookups (traversals in the tree).

# Future work

## Maintenance

### (high   importance, medium urgency) - Update Python bindings
PyBind11 bindings were disregarded and not tested starting development. Since then, the API of some functions was updated and new functions were added.

### (high   importance, high   urgency) - Benchmarking suite

### (medium importance, medium urgency) - Test suite

## Possible optimizations

### (high   potential, medium difficulty) - Remove fNameToIdMap
Function names are mapped to IDs. Avoids duplication and compares are faster (comparing ints vs strings), but at the cost of mapping the strings. Might just be faster if we don't map at all.

### (medium potential, high   difficulty) - Build CCTree truly parallel
Current parCCT implementation uses an MAP-REDUCE pattern. Threads work on disjoint parts of the input, each build their own CCTree, and they merge at the end. The merge has linear complexity. It might be better for the threads to work on (and add to) the same thread-safe CCTree data structure. Consider using lock-free data structures.
Relevant project: https://hpctoolkit.org/

### (medium potential, low    difficulty) - Check different base sizes of small_vector
The base size of small_vector defaults to 4. Reasoning: In the one perf folded sample I tested on, around 90% of nodes has less than 6 children. There are some nodes with up to 300 children though. Try changing the default size, it could be faster.

### (medium potential, high   difficulty) - Bypass parser
The library supports other formats and structures than Perf Folded format and the CCTree. It does so through a generalized interface, and is losing some performance because of the abstractions. Bypassing them, and reimplementing a function that just build a CCTree, with none of the 'dependencies', could be faster.

### (high   potential, low    difficulty) - Try different string hash
Strings (or string_views) are used as keys in maps in multiple places. Maps work by first hashing the string. The default string hash may output the same hash for two different string too often, thus complicating map operations. Try different string hashes.

### (medium potential, medium difficulty) - Try different map implementations
CCTNode::children uses boost::flat_map, and CCTree::functionNameToIdMap uses std::unordered_map.
In theory, the maps should be ranked in terms of speed: abseil::flat_hash_map > boost::flat_map > std::unordered_map.
However, benchmarks on my old machine with potentially limited vector operations showed the currently used implementations to be the fastest. Run benchmarks on a newer machine, see if they're faster there.

## CCTree Features

### (high   importance, medium urgency) - Inclusive values
The values stored in CCTNodes are exclusive. Calculate also the inclusive value, which is the sum of values of the whole subtree.

### (medium importance, low    urgency) - Squashing
Squash sequences of the same function down into a single node. Example:
unknown;unknown;unknown
turns to
[unknown]{3x}

### (low    importance, low    urgency) - Compressed input
Allow taking compressed samples (.tar.gz) as input, maybe doing it smarter than just decompressing it first.

### (medium importance, low    urgency) - Generate flamegraph natively
Add an option to generate flamegraph from CCTree. Currently this is done by a Perl script. Another related project: https://docs.rs/inferno/latest/inferno/

### (medium importance, medium urgency) - Fix invalid (de)serialization
A new feature mmaps the input file and all references to the strings are just string_views (pointers). This invalidates serialization and deserialization, currently it is mostly ignored.

### (medium importance, low    urgency) - Fix arbitrary reserving
Currently CCTree reserves containers of arbitrary size, because reallocation is expensive. This is an improper solution and wastes memory. Could use chunked vector, segmented vector, hashed array tree, boost::stable_vector...

### (low    importance, low    urgency) - Change Event::strings to string_views
The attributes 'name' and 'processName' unnecessarily copy a string, when it can just be a string_view.

## Data structures

### (high   importance, medium urgency) - Diff structures
For example representing a difference of the CCTrees.

### (medium importance, low    urgency) - Tree edit distance
(Someone is working on this I believe). Quantifying the difference between two CCTrees.

### (low    importance, low    urgency) - Top N CCTree
Trim CCTree to only store the nodes with top N values.
