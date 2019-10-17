#include <stdio.h>
#include <stdlib.h>
#include "stacktrace_comparison.h"

int disable_output_flag = 0;

int compare_func(const void* element1, const void* element2)
{
    auto comparison = new stacktrace_comparison();
    comparison->collect();
    comparison->analysis();
    if (!disable_output_flag) {
        comparison->output(stdout);
    }
    return (*(int*)element1) - (*(int*)element2);
}

void myfunc3(void)
{
    int array[] = {1, 5, 4, 2, 3};
    qsort(array, sizeof(array) / sizeof(array[0]), sizeof(array[0]), compare_func);
}

static void myfunc2(void) /* "static" means don't export the symbol... */
{
    myfunc3();
}

void myfunc(int ncalls)
{
    if (ncalls > 1)
        myfunc(ncalls - 1);
    else
        myfunc2();
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "%s st_type num_calls\n", argv[0]);
        return 1;
    }

    auto stacktrace_type = atoi(argv[1]);
    auto num_calls = atoi(argv[2]);
    disable_output_flag = num_calls > 1;

    set_comparison_type((comparison_type)stacktrace_type);
    for (auto ncalls = 0; ncalls < num_calls; ++ncalls) 
        myfunc(ncalls);

    stacktrace_comparison::output_comparison(stderr);
    return 0;
}