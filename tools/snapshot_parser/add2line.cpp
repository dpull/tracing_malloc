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


// 辅助函数：从DIE中获取函数名
static std::string get_function_from_die(Dwarf_Debug dbg, Dwarf_Die cu_die, Dwarf_Addr address, Dwarf_Error& error)
{
    Dwarf_Die child_die = nullptr;
    std::string function_name = "??";
    
    // 获取编译单元的第一个子DIE
    if (dwarf_child(cu_die, &child_die, &error) != DW_DLV_OK) {
        return function_name;
    }
    
    // 遍历所有DIE查找包含该地址的函数
    Dwarf_Die sibling_die = nullptr;
    while (true) {
        Dwarf_Half tag;
        if (dwarf_tag(child_die, &tag, &error) != DW_DLV_OK) {
            break;
        }
        
        // 检查是否是子程序DIE
        if (tag == DW_TAG_subprogram) {
            Dwarf_Addr low_pc, high_pc;
            Dwarf_Bool has_low_pc = false;
            enum Dwarf_Form_Class has_high_pc = DW_FORM_CLASS_UNKNOWN;
            Dwarf_Half form;
            
            // 获取起始地址
            if (dwarf_lowpc(child_die, &low_pc, &error) == DW_DLV_OK) {
                has_low_pc = true;
            }
            
            // 获取结束地址
            Dwarf_Attribute high_pc_attr;
            if (dwarf_highpc_b(child_die, &high_pc, &form, &has_high_pc, &error) != DW_DLV_OK) {
                has_high_pc = DW_FORM_CLASS_UNKNOWN;
            }
            
            // 如果地址在函数范围内
            if (has_low_pc && has_high_pc != DW_FORM_CLASS_UNKNOWN && address >= low_pc && address < high_pc) {
                // 获取函数名
                char* name = nullptr;
                if (dwarf_diename(child_die, &name, &error) == DW_DLV_OK) {
                    function_name = name;
                    dwarf_dealloc(dbg, name, DW_DLA_STRING);
                }
                break;
            }
            
            // 检查是否有范围列表
            Dwarf_Ranges* ranges = nullptr;
            Dwarf_Signed ranges_count = 0;
            Dwarf_Off offset = 0;
            
            if (dwarf_get_ranges_a(dbg, 0, child_die, &ranges, &ranges_count, nullptr, &error) == DW_DLV_OK) {
                bool found = false;
                for (Dwarf_Signed i = 0; i < ranges_count; i++) {
                    if (ranges[i].dwr_type == DW_RANGES_END) {
                        continue;
                    }
                    
                    low_pc = ranges[i].dwr_addr1;
                    high_pc = ranges[i].dwr_addr2;
                    
                    if (address >= low_pc && address < high_pc) {
                        // 获取函数名
                        char* name = nullptr;
                        if (dwarf_diename(child_die, &name, &error) == DW_DLV_OK) {
                            function_name = name;
                            dwarf_dealloc(dbg, name, DW_DLA_STRING);
                        }
                        found = true;
                        break;
                    }
                }
                dwarf_ranges_dealloc(dbg, ranges, ranges_count);
                if (found) {
                    break;
                }
            }
        }
        
        // 检查子节点
        Dwarf_Die child_of_child;
        if (dwarf_child(child_die, &child_of_child, &error) == DW_DLV_OK) {
            std::string sub_function = get_function_from_die(dbg, child_die, address, error);
            if (sub_function != "??") {
                function_name = sub_function;
                break;
            }
        }
        
        // 获取下一个兄弟节点
        res = dwarf_siblingof_b(dbg, child_die, true, &sibling_die, &error);
        if (res != DW_DLV_OK) {
            break;
        }
        
        // 释放当前DIE并继续处理下一个
        dwarf_dealloc_die(child_die);
        child_die = sibling_die;
    }
    
    // 释放最后一个DIE
    if (child_die) {
        dwarf_dealloc_die(child_die);
    }
    
    return function_name;
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
    Dwarf_Error error = nullptr;
    Dwarf_Die cu_die = nullptr;
    Dwarf_Line* lines = nullptr;
    Dwarf_Signed line_count = 0;
    std::string result;

    // 遍历所有编译单元
    Dwarf_Unsigned next_cu_header = 0;
    int res = DW_DLV_OK;

    while ((res = dwarf_next_cu_header_d(dbg, true, nullptr, nullptr, 
                                        nullptr, nullptr, nullptr, 
                                        nullptr, nullptr, nullptr, 
                                        &next_cu_header, nullptr, &error)) == DW_DLV_OK) 
    {
        // 获取当前编译单元的DIE
        if (dwarf_siblingof_b(dbg, nullptr, true, &cu_die, &error) != DW_DLV_OK) {
            continue;
        }

        // 获取该编译单元的行信息
        if (dwarf_srclines(cu_die, &lines, &line_count, &error) != DW_DLV_OK) {
            dwarf_dealloc_die(cu_die);
            continue;
        }

        // 遍历行信息，查找匹配的地址
        for (Dwarf_Signed i = 0; i < line_count; i++) {
            Dwarf_Addr line_addr;
            if (dwarf_lineaddr(lines[i], &line_addr, &error) != DW_DLV_OK) {
                continue;
            }

            // 如果找到匹配的地址
            if (line_addr == address) {
                // 获取文件名
                char* file_name = nullptr;
                if (dwarf_linesrc(lines[i], &file_name, &error) == DW_DLV_OK) {
                    // 获取行号
                    Dwarf_Unsigned line_number;
                    if (dwarf_lineno(lines[i], &line_number, &error) == DW_DLV_OK) {
                        // 尝试获取函数名
                        std::string function_name = get_function_from_die(dbg, cu_die, address, error);
                        
                        // 组合结果
                        std::ostringstream oss;
                        oss << function_name << "\t" << file_name << ":" << line_number;
                        result = oss.str();
                        
                        // 释放文件名内存
                        dwarf_dealloc(dbg, file_name, DW_DLA_STRING);
                        break;
                    } else {
                        dwarf_dealloc(dbg, file_name, DW_DLA_STRING);
                    }
                }
            }
        }

        // 释放行信息和DIE
        dwarf_srclines_dealloc(dbg, lines, line_count);
        dwarf_dealloc_die(cu_die);
        
        // 如果已经找到结果，退出循环
        if (!result.empty()) {
            break;
        }
    }

    // 如果没有找到匹配的信息，返回地址的十六进制表示
    if (result.empty()) {
        std::ostringstream oss;
        oss << "0x" << std::hex << address << std::dec;
        return oss.str();
    }

    return result;
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
