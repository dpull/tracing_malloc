#include "map_info.h"

bool extract_maps(const std::string& line, map_info& info)
{
    // Parse start and end addresses
    size_t pos = line.find('-');
    if (pos == std::string::npos) {
        printf("Failed to parse maps line: %s\n", line.c_str());
        return false; // Return an empty map_info
    }
    info.start = std::stoull(line.substr(0, pos), nullptr, 16);

    size_t end_pos = line.find(' ', pos + 1);
    if (end_pos == std::string::npos) {
        printf("Failed to parse maps line: %s\n", line.c_str());
        return false; // Return an empty map_info
    }
    info.end = std::stoull(line.substr(pos + 1, end_pos - pos - 1), nullptr, 16);

    // Parse permissions
    size_t perms_start = end_pos + 1;
    size_t perms_end = line.find(' ', perms_start);
    if (perms_end == std::string::npos) {
        printf("Failed to parse maps line: %s\n", line.c_str());
        return false; // Return an empty map_info
    }
    info.perms = line.substr(perms_start, perms_end - perms_start);

    // Parse offset
    size_t offset_start = perms_end + 1;
    size_t offset_end = line.find(' ', offset_start);
    if (offset_end == std::string::npos) {
        printf("Failed to parse maps line: %s\n", line.c_str());
        return false; // Return an empty map_info
    }
    info.offset = std::stoull(line.substr(offset_start, offset_end - offset_start), nullptr, 16);

    // Parse device
    size_t dev_start = offset_end + 1;
    size_t dev_end = line.find(' ', dev_start);
    if (dev_end == std::string::npos) {
        printf("Failed to parse maps line: %s\n", line.c_str());
        return false; // Return an empty map_info
    }
    info.dev = line.substr(dev_start, dev_end - dev_start);

    // Parse inode
    size_t inode_start = dev_end + 1;
    size_t inode_end = line.find(' ', inode_start);
    if (inode_end == std::string::npos) {
        printf("Failed to parse maps line: %s\n", line.c_str());
        return false; // Return an empty map_info
    }
    info.inode = line.substr(inode_start, inode_end - inode_start);

    // Parse pathname
    size_t pathname_start = inode_end + 1;
    info.pathname = line.substr(pathname_start);

    return true;
}

bool extract_maps(const std::string& line, map_info& info);

std::vector<map_info> load_maps(const std::string& file_path)
{
    std::vector<map_info> maps;
    maps.reserve(16);

    FILE* file = fopen(file_path.c_str(), "r");
    if (!file) {
        return maps;
    }

    char line[2048];
    while (fgets(line, sizeof(line), file)) {
        map_info info;
        if (!extract_maps(line, info))
            continue;

        if (info.perms == "r-xp") {
            maps.push_back(info);
        }
    }
    fclose(file);
    return maps;
}
