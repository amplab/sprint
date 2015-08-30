#ifndef DSL_BENCH_BENCHMARK_H_
#define DSL_BENCH_BENCHMARK_H_

#include <sys/time.h>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <vector>

#if defined(__i386__)

static __inline__ unsigned long long rdtsc(void) {
    unsigned long long int x;
    __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
    return x;
}
#elif defined(__x86_64__)

static __inline__ unsigned long long rdtsc(void) {
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

#elif defined(__powerpc__)

static __inline__ unsigned long long rdtsc(void) {
    unsigned long long int result=0;
    unsigned long int upper, lower,tmp;
    __asm__ volatile(
                "0:                  \n"
                "\tmftbu   %0           \n"
                "\tmftb    %1           \n"
                "\tmftbu   %2           \n"
                "\tcmpw    %2,%0        \n"
                "\tbne     0b         \n"
                : "=r"(upper),"=r"(lower),"=r"(tmp)
                );
    result = upper;
    result = result<<32;
    result = result|lower;

    return(result);
}

#else

#error "No tick counter is available!"

#endif

namespace dsl_bench {
    class Benchmark {
    public:
        typedef unsigned long long int time_t;

        Benchmark() {}

        static time_t get_timestamp() {
            struct timeval now;
            gettimeofday (&now, NULL);
            return  now.tv_usec + (time_t)now.tv_sec * 1000000;
        }

        std::vector<std::string> readQueryFile(const std::string& query_file) const {
            std::vector<std::string> queries;
                std::ifstream query_stream(query_file);
                if(!query_stream.is_open()) {
                    fprintf(stderr, "Error: Query file [%s] may be missing.\n",
                            query_file.c_str());
                    exit(0);
                }

                std::string line, bin, query;
                while (getline(query_stream, line)) {
                    // Extract key and value
                    int split_index = line.find_first_of('\t');
                    bin = line.substr(0, split_index);
                    query = line.substr(split_index + 1);
                    queries.push_back(query);
                }
                query_stream.close();
                return queries;
        }
    };
}



#endif // DSL_BENCH_BENCHMARK_H_
