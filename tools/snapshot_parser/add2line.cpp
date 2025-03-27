#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <memory>
#include <cstdint>
#include "add2line.h"

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
	if (it != is_so_cache.end()) {
		return it->second;
	}

	std::ifstream file(file_path, std::ios::binary);
	if (!file) {
		Logger::error("打开文件失败: " + file_path);
		return false;
	}

	file.seekg(16); // ei_ident
	uint16_t ei_type;
	file.read(reinterpret_cast<char*>(&ei_type), sizeof(ei_type));

	bool is_so = (ei_type == 3);
	is_so_cache[file_path] = is_so;
	return is_so;
}

void addr2line::popen_exec(const std::vector<uint64_t>& addrset, uint64_t fbase, const std::string& fname)
{
	bool is_so;
	try {
		is_so = is_shared_object(fname);
	}
	catch (const std::exception& e) {
		Logger::error("检查共享对象失败 " + fname + ": " + e.what());
		return;
	}

	std::vector<uint64_t> addrlist = addrset;
	if (is_so) {
		for (auto& addr : addrlist) {
			addr = addr - fbase;
		}
	}

	const size_t max_addr_line = 3000;
	for (size_t i = 0; i < addrlist.size(); i += max_addr_line) {
		size_t batch_size = std::min(max_addr_line, addrlist.size() - i);
		std::vector<uint64_t> curlist(addrlist.begin() + i, addrlist.begin() + i + batch_size);

		std::stringstream cmd;
		cmd << "addr2line -Cfse " << fname;
		for (const auto& addr : curlist) {
			cmd << " 0x" << std::hex << addr;
		}

		Logger::info("执行命令: " + cmd.str());

		FILE* pipe = popen(cmd.str().c_str(), "r");
		if (!pipe) {
			Logger::error("执行命令失败: " + cmd.str());
			continue;
		}

		char buffer[1024];
		for (size_t j = 0; j < curlist.size(); ++j) {
			uint64_t addr_raw = is_so ? curlist[j] + fbase : curlist[j];

			// 读取函数名
			if (!fgets(buffer, sizeof(buffer), pipe)) {
				Logger::error("读取函数名失败");
				break;
			}
			std::string function(buffer);
			function.erase(function.find_last_not_of("\r\n") + 1);

			// 读取文件和行号
			if (!fgets(buffer, sizeof(buffer), pipe)) {
				Logger::error("读取文件和行号失败");
				break;
			}
			std::string file(buffer);
			file.erase(file.find_last_not_of("\r\n") + 1);

			std::string line = function + "\t" + file + "\t" + fname;

			// 保存到映射
			std::lock_guard<std::mutex> lock(addr_line_mutex);
			address_to_line[addr_raw] = line;
		}

		pclose(pipe);
	}
}

void addr2line::add(uint64_t address, uint64_t fbase, const std::string& fname)
{
	fbase_to_fname[fbase] = fname;
	fbase_to_addrset[fbase].insert(address);
}

void addr2line::execute()
{
	std::vector<std::thread> threads;

	for (const auto& pair : fbase_to_addrset) {
		uint64_t fbase = pair.first;
		const auto& addrset = pair.second;
		const std::string& fname = fbase_to_fname[fbase];

		std::vector<uint64_t> addresses(addrset.begin(), addrset.end());

		threads.emplace_back([this, addresses, fbase, fname]() {
			popen_exec(addresses, fbase, fname);
			});
	}

	for (auto& thread : threads) {
		thread.join();
	}
}

std::string addr2line::get_from_cache(uint64_t address)
{
	std::lock_guard<std::mutex> lock(addr_line_mutex);
	auto it = address_to_line.find(address);
	return (it != address_to_line.end()) ? it->second : "";
}
