#pragma once
#include <vector>
#include <string>
#include <cstdint>

struct snapshot {
    int64_t ptr;
    int64_t time;
    int64_t size;
    std::vector<uint64_t> stack;
    std::vector<std::string> stack_line;
};
std::vector<snapshot> load_snapshot(const std::string& file_path);
int save_snapshot(const std::string& file_path, const std::vector<snapshot>& data);