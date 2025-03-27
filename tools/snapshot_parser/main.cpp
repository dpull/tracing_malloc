#include "map_info.h"
#include "snapshot.h"
#include "add2line.h"

int analyze(const std::string& maps_path, const std::string& snapshot_path, const std::string& output_path) {
	auto maps = load_maps(maps_path);
	if (maps.size() == 0) {
		printf("load maps failed, %s", maps_path.c_str());
		return -1;
	}

	auto data = load_snapshot(snapshot_path);
	if (maps.size() == 0) {
		printf("load snapshot failed, %s", maps_path.c_str());
		return -1;
	}



	addr2line a2l;
	for (const auto& item : data) {
		for (const auto& address : item.stack) {
			for (const auto& module : maps) {
				if (address >= module.start && address < module.end) {
					a2l.add(address, module.start, module.pathname);
					break;
				}
			}
		}
	}

	// 执行地址解析
	Logger::info("执行地址解析");
	a2l.execute();

	// 填充解析结果
	Logger::info("填充解析结果");
	for (auto& item : data) {
		for (const auto& address : item.stack) {
			std::string line = a2l.get_from_cache(address);
			if (line.empty()) {
				std::stringstream ss;
				ss << "0x" << std::hex << address;
				line = ss.str();
			}
			item.stack_line.push_back(line);
		}
	}

	// 保存结果
	Logger::info("保存结果到: " + output_path);
	save_snapshot(output_path, data);
}


int main(int argc, char* argv[]) {
	if (argc < 3) {
		printf("usage: %s tracing.malloc.[pid].maps tracing.malloc.[pid]", argv[0]);
		return -1;
	}
	std::string output_path(argv[2]);
	output_path.append(".addr2line");
	return analyze(argv[1], argv[2], output_path);
}
