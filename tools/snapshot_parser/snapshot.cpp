#include "snapshot.h"
#include <algorithm>
#include <stdio.h>
#include <string>
#include <vector>

std::vector<snapshot> load_snapshot(const std::string& file_path)
{
    std::vector<snapshot> data;
    data.reserve(65536);

    FILE* file = fopen(file_path.c_str(), "rb");
    if (!file) {
        return data;
    }

    constexpr size_t buf_size = 256; // 8*4+8*28 = 256 bytes
    char buffer[buf_size];
    snapshot item;

    while (fread(buffer, 1, buf_size, file) == buf_size) {
        item.ptr = *reinterpret_cast<int64_t*>(buffer);
        item.time = *reinterpret_cast<int64_t*>(buffer + 8);
        item.size = *reinterpret_cast<int64_t*>(buffer + 16);

        const int stack_size = 28;
        item.stack.resize(stack_size);
        std::memcpy(item.stack.data(), buffer + 32, stack_size * sizeof(uint64_t));

        if (item.ptr < 0) {
            continue;
        }

        auto last_non_zero = stack_size - 1;
        while (last_non_zero >= 0 && item.stack[last_non_zero] == 0) {
            last_non_zero--;
        }

        if (last_non_zero >= 0) {
            item.stack.resize(last_non_zero + 1);
            data.push_back(item);
        }
    }
    fclose(file);

    std::sort(data.begin(), data.end(), [](const snapshot& a, const snapshot& b) { return a.time < b.time; });
    return data;
}

int save_snapshot(const std::string& file_path, const std::vector<snapshot>& data)
{
    FILE* file = fopen(file_path.c_str(), "w");
    if (!file) {
        return -1;
    }

    for (const auto& item : data) {
        fprintf(file, "time:%lld\tsize:%lld\tptr:0x%llx\n", item.time, item.size, item.ptr);

        for (const auto& frame : item.stack_line) {
            fprintf(file, "%llu\n", frame);
        }

        fprintf(file, "========\n\n");
    }

    fclose(file);
    return 0;
}
