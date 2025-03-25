#include <chrono>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

int times = 0;
int loop = 0;
int leak = 0;
std::vector<char*> buffer_array;
std::vector<char*> leak_array;

void myfunc3()
{
    for (auto i = 0; i < loop; ++i) {
        void* p = NULL;
        if (posix_memalign(&p, 16, i))
            continue;

        buffer_array.push_back((char*)p);
        if (i % leak)
            free(p);
        else
            leak_array.push_back((char*)p);
    }
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
    if (argc != 4) {
        fprintf(stderr, "%s times loop leak\n", argv[0]);
        return 1;
    }

    times = atoi(argv[1]);
    loop = atoi(argv[2]);
    leak = atoi(argv[3]);

    auto start = std::chrono::system_clock::now();
    for (auto i = 0; i < times; ++i) {
        myfunc(i);
    }
    auto end = std::chrono::system_clock::now();
    auto cost_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    printf("cost_time:%f s\talloc count:%lld\tleak count:%lld\n", (float)cost_time.count() / 1000000, (long long)buffer_array.size(), (long long)leak_array.size());
    return 0;
}