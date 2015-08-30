## Data Structure Library

A C++ library of selected data-structures that I've implemented (or ported)
during the course of my research.

## Compiling and installing

This library employs CMake as its build tool for building across different
platforms. The minimum required versison of CMake is 2.8.

To compile the library and all included benchmarks run the following:

```
mkdir build
cd build
cmake ../
make
```

This will create the executables and libraries within the build directory (under
the bin/ and lib/ directories respectively). In order to install the library and
its benchmarks, additionally run:

```
make install
```

## Usage

A sample use-case of the suffix tree is shown below:

```c++
#include <suffix-tree.h>
#include <iostream>

int main(int argc, char **argv) {
    
    // Dummy input
    std::string input = "mississippi";

    // Create the suffix tree
    dsl::SuffixTree suffix_tree(input);

    // Search for substrings
    std::vector<int64_t> results;
    suffix_tree.search(results, "ssi");

    // Display the results
    for(auto offset: results) {
        std::cout << "Found at offset " << offset << std::endl;
    }
    
    return 0;
}

```
