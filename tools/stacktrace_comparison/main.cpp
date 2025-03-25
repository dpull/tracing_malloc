#include "stacktrace_misc.h"
#include <stdio.h>
#include <stdlib.h>

stacktrace_type type;
int disable_output_flag = 0;
long long collect_cost = 0;
long long analysis_cost = 0;

int compare_func(const void* element1, const void* element2)
{
    auto impl = create(type);
    if (impl) {
        collect_cost += cost_time_us([impl] { impl->collect(); });
        analysis_cost += cost_time_us([impl] { impl->analysis(); });

        if (!disable_output_flag) {
            impl->output(stdout);
        }
        delete impl;
    }
    return (*(int*)element1) - (*(int*)element2);
}

void myfunc3(void)
{
    int array[] = { 1, 5, 4, 2, 3 };
    qsort(array, sizeof(array) / sizeof(array[0]), sizeof(array[0]), compare_func);
}

static void myfunc2(void) /* "static" means don't export the symbol... */ { myfunc3(); }

void myfunc(int ncalls)
{
    if (ncalls > 1)
        myfunc(ncalls - 1);
    else
        myfunc2();
}

int main(int argc, char* argv[])
{
    if (argc != 3) {
        fprintf(stderr, "%s st_type num_calls\n", argv[0]);
        return 1;
    }

    type = (stacktrace_type)atoi(argv[1]);
    auto num_calls = atoi(argv[2]);
    disable_output_flag = num_calls > 1;

    for (auto ncalls = 0; ncalls < num_calls; ++ncalls)
        myfunc(ncalls);

    fprintf(stdout, "%d\tcollect_cost:[%lld]µs\tanalysis_cost:[%lld]µs\n", (int)type, collect_cost, analysis_cost);
    return 0;
}