#pragma once
#include <string>
#include <vector>

struct map_info {
    uint64_t start;
    uint64_t end;
    std::string perms;
    uint64_t offset;
    std::string dev;
    std::string inode;
    std::string pathname;
};

std::vector<map_info> load_maps(const std::string& file_path);