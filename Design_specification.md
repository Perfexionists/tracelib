# Tracelib Design Specification

## A) Input Format

  1. The library should accept *folded profiles* as inputs.
  2. The library should accept its own serialized format.
  3. The folded profiles may be either in plain text file, or they may be
     compressed using `gzip`.
     - For parsing compressed formats, we should use well-established and
       performant libraries.
  4. The parsing architecture should be designed such that it can be extended
     with new compression formats.
     - Efficiency is crucial; this will likely require highly-specialized code
       for individual compression formats, possibly with unique APIs.


## B) Inputs

  1. The library should accept multiple profiles on input, arbitrarily combining
     plain text and compressed formats.
  2. The library should distinguish between *baseline* and *target* profiles.
  3. The user may specify multiple baseline (target) profiles, and an
     aggregation function (e.g., min, max, sum, mean, median, ...).
  4. The set of baseline (target) profiles do not have to necessarily contain
     measurements of the same resource, e.g., `profile1.folded` may measure CPU
     cycles consumption and `profile2.folded` may contain cache miss counts.  
  5. Each set of baseline (target) profiles containing the same resource will be
     aggregated into a single baseline (target) profile.
     - For example, consider `baseline1.folded` (CPU cycles), `baseline2.folded`
       (cache misses), `baseline3.folded.gz` (CPU cycles), and
       `target1.folded.gz` (CPU cycles). Only the `baseline1.folded` and
       `baseline3.folded.gz` should be aggregated.
  6. Aggregations should be implemented as efficiently as possible.
     - Some aggregations, e.g., median, may require parsing all profiles first
       and only then aggregating them.
     - Other, e.g., sum, can and should be implemented more efficiently.
  7. The library should support parallelization both per-profile (i.e., multiple
     threads parsing the same profile) and across profiles (i.e., 10 profiles in
     total, 1 thread parsing one profile).
  8. Plain text profiles should support parallelized parsing.
  9. Compressed profiles should support some form of parallel parsing if possible.
    - This will heavily depend on the concrete compression formats. Some are
      better suited for this than others.
    - Each format may require a different parallelization scheme.
  

## C) Representation

  1. The library should store call-context-aware consumption, i.e., consumption
     w.r.t. the call stack. For example, the function `g` from the profile
     ```
     main;f;g 50
     main;g 30
     ```
     should associate 50 and 30 consumed resources with the `main;f` and `main`
     call stack, respectively.
     - This representation will be used for generating flame graphs and should
       be implemented using the *Calling Context Tree (CCT)* structure.
  2. The library should also store call-context-aggregated consumption. Using
     the example from C.1, function `g` should be associated with a total
     resource consumption of 80.
     - This representation will be used for generating tree maps.
     - We will call this representation as *Symbol Map (SM)*.
  3. The library should also store both *inclusive (cumulative)* and *exclusive
     (self)* consumption for each resource, and for call-context-aware and
     call-context-aggregated representations.
  4. Each stack trace record (e.g., function or process name) must be able to
     carry additional information such as source location (i.e., source file and
     line), type flag (e.g., types associated with the _[k], _[w], ... suffixes
     from the folded profiles).
  5. Both CCT and SM should store additional statistics, e.g., total number of
     nodes, the maximum stack trace depth, etc.
  6. The library should implement efficient DiffCCT and DiffSM variants for
     comparing baseline and target profiles. 


## D) Postprocessing

  1. The library should use *artificial root* node in CCT since not all stack
     traces need to have the same initial node. 
  2. The library should support *cropping*: removing CCT branches or SM records
     that have a lower **inclusive** consumption than some specified threshold.
     - The API should support specification of baseline/target profile, CCT/SM,
       and resource(s) to apply the cropping to.
     - Such cropping should produce a result with details about the number of
       removed nodes / records and their total consumption.
  3. The library should support *sorting*:
     - Each CCT node should support sorting its children according to some sort
       function.
     - The entire SM should support sorting according to some sort function.
     - We may predefine sorting functions, e.g., by name (dictionary,
       alphanumeric, natural); or by consumption.
  4. The library should support postprocessing of function or process names,
     e.g., replacing C++ template instantiations with `<*>`.
  5. The library should support topology-preserving *squashing* of recursive
     calls in CCT: recursive calls with no exclusive consumption should be
     merged into a single node and somehow indicate the number of calls that
     were merged. For example, the following folded profile:
     ```
     main 100
     main;f;f;f;f;g 60
     main;f;f;f;f 50
     main;f;f;g 40
     main;f;f 20
     main;a 30
     ```
     should result in the following tree structure (illustrated as a flame
     graph):
     ```
                       +-----+
                       |  g  |
           +-----+-----+-----+
           |  g  |   f{x2}   |
     +-----+---------+-------+
     |  a  |      f{x2}      |
     +-----+-----------------+
     |          main         |
     +-----------------------+
     ```
     - Although the `f{x2}` name is used, its consumption should still be
       attributed to the function `f` and not the artificial function `f{x2}`.


## E) Traversal

  1. CCT should support pre-order, in-order, and post-order iterators.
     - These iterators should efficiently report the depth of the current node
       in O(1).
  2. Both CCT and SM should support random-access iterator.
  3. Each iterator should support retrieval of any resource consumption, i.e,
     the same iterator may be used to obtain inclusive/exclusive consumption of
     arbitrary resource stored within the iterated structure. 
  4. All iterators should support generic filters. For example, skipping nodes
     or records with consumption lower than some threshold, names not matching a
     regex, etc.


## F) Outputs

  1. The library should support generating output folded profiles.
  2. The library should support serialization of its internal representations.
     - We can either create our own (de)serialization format, or use an existing
       library. 
  3. The API should support specification of:
     - The baseline or target profile.
     - A specific metric.
     - CCT or SM.
     - Inclusive or exclusive consumption.
     - Sorted or unsorted output.


## G) Language Bindings

  1. The library should support Python bindings.
  2. The library may support directly Python-C++ bindings, or expose a C
     interface and use Python-C bindings.


## H) Distribution

  1. The library should use an established packaging and installation systems
     such as `cmake`, `meson`, `conan`, `vcpkg`, or others that can manage
     external libraries and dependencies. 


## I) Testing

  1. The library should have a sufficient functional test suite that may be used
     in CI.
  2. The library should also implement benchmarks to evaluate the performance of
     implemented optimizations.
     - Both time and memory consumption should be measured.
     - The benchmarks should focus on, among others, parsing, structures
       construction, postprocessing, traversals, generating output,
       (de)serialization.


## J) Miscellaneous

  1. Performance is crucial. The library should focus on optimizing time, but
     should not unnecessarily waste memory.
  2. The library may implement custom low-level containers, complex efficient
     data structures, may use bit manipulation techniques (etc.) to achieve
     reasonable performance. 
  3. The library may use third-party dependencies with compatible licenses.
     However, each such dependency should be justified and the number of
     dependencies should stay reasonable.


# Design and Performance Questions

  ## I) Design

  1. How does the `stackcollapse.pl` script handle perf records with mixed
     metrics, e.g., CPU cycles and cache misses records in a single report?
     - If it handles them, should the library support it as well?
  2. Does the folded format carry suffixes such as _[k], _[w]?
  3. Can the folded format carry source information such as source file + line
     number?
     - If it can, our CCT should optionally distinguish different call sites of
       a function even within the same call stack, i.e., generate two different
       children nodes.
  4. How to best integrate Tracelib with Python projects using the python-meson
     build system?


  ## II) Performance

  1. Should we use array of structures or a structure of arrays?
  2. Is parsing faster if we remember the last processed stack trace?
  3. Is it faster to compute inclusive consumption in postprocessing or during
     parsing?
  4. Is it faster to construct SM during CCT construction, or to construct it
     separately after the CCT is created?
  5. Is the memory/time trade-off worth storing the exclusive consumption in
     some sparse array?
  6. Can we design efficient lock-free implementations of the CCT and/or SM
     structures?


# Minimal Requirements:

  - A.1, A.3
  - B.1, B.2, B.3, B.5
  - C.1, C.2, C.3, C.5
  - D.1, D.2, D.3, D.5
  - E.1, E.2, E.3
  - F.1, F.3
  - G.1, G.2
  - H.1


# Tracelib Architecture

TODO