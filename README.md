# Pull-Star

Implementation for Black-Box and Pull-Star algorithms for Regular Expressions.

## Supported Data Structures

We currently support the following data structures:

1. Suffix Tree (ST)
2. Suffix Array (SA)
3. Compressed Suffix Tree (CST)
4. k-gram Index (kGM)

We also support Pull-Star/Black Box Algorithms on CSA, but they are more closely
integrated with the data structures; the implementation can be found in the 
[Succinct](https://github.com/amplab/succinct-cpp) repository (see the 
[regex](https://github.com/amplab/succinct-cpp/tree/master/core/include/regex)
sub-module).

## Pre-requisites

* C++11 support from the c++ compiler
* CMake build system
* Thrift for the multicore implementation

## Building and Cleaning up

To build all binaries and benchmarks, run:

```
./build.sh
```

To clean-up build files, run:

```
./cleanup.sh
```

## Constructing datasets

Since pre-processing can be time consuming for most indexes, we provide a tool
to construct the indexes and serialize them to disk. Additionally, all our 
benchmarks require that the indexes be present in the serialized form. To 
construct and serialize an index, run:

```
./build/ds-lib/construct/bin/construct [-d data-structure] [file]
```

after the build step.

The `data-structure` parameter is an integer, and uses the following mapping:

```
0   ST
1   CST
2   Plain Suffix Array (no LCP)
3   SA (with LCP)
4   kGM
```

The `file` parameter is simply the path to the input data.

Example:
```
./build/ds-lib/construct/bin/construct -d 3 data/sample.data
```

## Running Single Core Benchmark

To benchmark the single core performance for black-box or pull-star approaches, 
run:

```
./build/bench/bin/rxbench [-m mode] [-q query_file] [-r res_file] [-d data-structure] [-e executor_type] [-b benchmark] [file]
```

`mode` specifies whether the data structure should be constructed on `file` (1) 
or should should be read from `file` (0).

`query\_file` specifies the path to the file containing RegEx queries (see 
[queries/](queries/) for sample queries).

`res\_file` specifies the path to the output file for the results.

`data-structure` specifies which index to use (same mapping as described above).

`executor\_type` specifies whether Black-Box (0) or Pull-Star (1) approach 
should be used.

`benchmark` specifies the benchmark-type, and should be set to "regex-latency".

`file` specifies the input, as before. Note that even if reading the index from
its serialized version, specify the name of the original input file.

Example:
```
./build/bench/bin/rxbench -m 0 -q queries/queries.regex -r regex-latency.txt -d 3 -e 1 -b "latency-regex" data/sample.data
```

## Running Multi-core Benchmark

To benchmark the multi-core performance for black-box or pull-star approaches,
we use thrift as the communication system between different processes which run
on different cores. To this end, the query engine must be started as a service
before running the benchmarks.

Before starting the service, modify parameters in [conf/pullstar-env.sh](conf/pullstar-env.sh).
Note that the `DATA\_PATH` must point to the directory containing the different
shards (each shard would correspond to a different core) encoded with the 
correct `DATA\_STRUCTURE`. Each shard must be named 
`$DATA\_PATH/data\_${i}.${suffix}`, where ${i} ranges from 0 to NUM\_SHARDS-1,
and ${suffix} is the suffix assigned to the serialized shard by the construct 
tool (e.g., `.st` for Suffix Trees, `.cst` for Compressed Suffix Trees, etc.).

To start the service, run:

```
./sbin/start-all.sh
```

Once the service is started, you can benchmark the multicore performance by 
running:

```
./build/bench/bin/rxbench -t [-q query_file] [-r res_file] [-b benchmark]
```

`-t` instructs the benchmark tool to run the benchmark in the multi-core mode.
Other parameters have the same meaning as the single-core benchmark.

If you have any doubts or would like to report bugs, please contact us at 
* anuragk [at] berkeley.edu
* ragarwal [at] berkeley.edu
