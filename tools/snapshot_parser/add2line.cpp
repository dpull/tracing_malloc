#include "add2line.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <dwarf.h>
#include <libdwarf.h>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

std::string addr2line::exec_command(const std::string& cmd)
{
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

bool addr2line::is_shared_object(const std::string& file_path)
{
    auto it = is_so_cache.find(file_path);
    if (it != is_so_cache.end())
        return it->second;

    FILE* file = fopen(file_path.c_str(), "rb");
    if (!file)
        return false;

    fseek(file, 16, SEEK_SET); // ei_ident
    uint16_t ei_type;
    fread(&ei_type, sizeof(ei_type), 1, file);
    fclose(file);

    bool is_so = (ei_type == 3); // 3 表示 ET_DYN
    is_so_cache[file_path] = is_so;
    return is_so;
}

static std::string get_function_info(Dwarf_Debug dbg, Dwarf_Addr address) 
{
    return "";
}

void addr2line::popen_exec(const std::string& fname, uint64_t fbase, std::vector<uint64_t>& addrset)
{
    bool is_so = is_shared_object(fname);
    if (is_so) {
        for (auto& addr : addrset) {
            addr = addr - fbase;
        }
    }

    char real_path[FILENAME_MAX];
    Dwarf_Debug dbg = 0;
    Dwarf_Error err = 0;

    if (dwarf_init_path(fname.c_str(), real_path, sizeof(real_path), DW_GROUPNUMBER_ANY, 0, 0, &dbg, &err) != DW_DLV_OK) {
        fprintf(stderr, "dwarf_init_path %s filed, %s\n", fname.c_str(), dwarf_errmsg(err));
        dwarf_dealloc_error(dbg, err);
        dwarf_finish(dbg);
        return;
    }

    const size_t num_threads = 16;
    std::vector<std::thread> threads;
    auto chunk_size = addrset.size() / num_threads;

    for (size_t i = 0; i < num_threads; ++i) {
        size_t start = i * chunk_size;
        size_t end = (i == num_threads - 1) ? addrset.size() : start + chunk_size;
        threads.emplace_back(
            [this, is_so, fbase, &fname, &dbg, &addrset](size_t start, size_t end) {
                for (size_t i = start; i < end; ++i) {
                    int64_t addr_raw = is_so ? addrset[i] + fbase : addrset[i];
                    auto line = get_function_info(dbg, addrset[i]);
                    if (line.empty())
                        continue;
                    line.append("\t");
                    line.append(fname);
                    std::lock_guard<std::mutex> lock(this->addr_line_mutex);
                    address_to_line[addr_raw] = line;
                }
            },
            start, end);
    }

    for (auto& t : threads) {
        t.join();
    }
    dwarf_finish(dbg);
}

void addr2line::add(uint64_t address, uint64_t fbase, const std::string& fname)
{
    fbase_to_fname[fbase] = fname;
    fbase_to_addrset[fbase].insert(address);
}

void addr2line::execute()
{
    for (const auto& pair : fbase_to_addrset) {
        uint64_t fbase = pair.first;
        const auto& addrset = pair.second;
        const std::string& fname = fbase_to_fname[fbase];

        std::vector<uint64_t> addresses(addrset.begin(), addrset.end());
        popen_exec(fname, fbase, addresses);
    }
}

std::string addr2line::get_from_cache(uint64_t address)
{
    std::lock_guard<std::mutex> lock(addr_line_mutex);
    auto it = address_to_line.find(address);
    return (it != address_to_line.end()) ? it->second : "";
}
