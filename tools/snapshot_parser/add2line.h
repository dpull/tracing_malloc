#pragma once
#include <unordered_map>
#include <string>
#include <unordered_set>
#include <mutex>

class addr2line {
    private:
        std::unordered_map<uint64_t, std::string> fbase_to_fname;
        std::unordered_map<uint64_t, std::unordered_set<uint64_t>> fbase_to_addrset;
        std::unordered_map<std::string, bool> is_so_cache;
        std::unordered_map<uint64_t, std::string> address_to_line;
        std::mutex addr_line_mutex;
    
        std::string exec_command(const std::string& cmd);
        bool is_shared_object(const std::string& file_path);
        void popen_exec(const std::vector<uint64_t>& addrset, uint64_t fbase, const std::string& fname);
    
    public:
        void add(uint64_t address, uint64_t fbase, const std::string& fname);
        void execute();
        std::string get_from_cache(uint64_t address);
    };
    