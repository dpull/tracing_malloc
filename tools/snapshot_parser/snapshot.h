#pragma once
#include <vector>

struct snapshot {
    int64_t ptr;
    int64_t time;
    int64_t size;
    std::vector<uint64_t> stack;
    std::vector<std::string> stack_line;
};
std::vector<snapshot> load_snapshot(const std::string& file_path);
void save_snapshot(const std::string& file_path, const std::vector<snapshot>& data);