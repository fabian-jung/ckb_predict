#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <thread>
#include <map>
#include <vector>
#include <numeric>
#include <csignal>
#include <mutex>

#include "color.hpp"

using key_id_t = unsigned int;

class signal_handler_t {
public:
	signal_handler_t(std::function<void(int)> handler)
	{
		static size_t id = 0;
		m_id = ++id;
		s_handler.insert(
			std::make_pair(
				m_id,
				handler
			)
		);
		static bool init = false;
		if(!init) {
			init = true;
			std::signal(SIGABRT, generic_signal_handler);
			std::signal(SIGFPE, generic_signal_handler);
			std::signal(SIGILL, generic_signal_handler);
			std::signal(SIGINT, generic_signal_handler);
			std::signal(SIGSEGV, generic_signal_handler);
			std::signal(SIGTERM, generic_signal_handler);
		}
	}

	~signal_handler_t() {
		s_handler.erase(m_id);
	}
private:
	size_t m_id;
	inline static std::map<
		size_t,
		std::function<void(int signum)>
	> s_handler;

	static void generic_signal_handler(int signum) {
		std::cout << "Catched Signal " << signum << std::endl;
		for(auto& [id, handler] : s_handler) {
			handler(signum);
		}
	}
};

int main(int argc, char **argv) {
	std::this_thread::sleep_for(std::chrono::seconds(2));

	const std::string home = getenv("HOME");
	const std::string preload_path = home + "/.ckb-predict-data";

	std::vector<color_t> cpu_heat_map {
		make_color(0,0xff,0),
		make_color(0xff,0xff,0),
		make_color(0xff,0,0)
	};

	const auto background_color = make_color(0x20, 0x20, 0xff);
	const auto active_color = make_color( 0xff, 0, 0 );

	std::vector<std::string> id_to_key_map {
		"nokey",
		"g1", "esc", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f10", "f11", "f12", "ptrscn", "scroll", "pause", "stop", "prev", "play", "next",
		"g2", "grave", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "minus", "equal", "bspace", "ins", "home", "pgup", "numloc", "numslash", "numstar", "numminus",
		"g3", "tab", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "lbrace", "rbrace", "enter", "del", "end", "pgdn", "num7", "num8", "num9", "numplus",
		"g4", "caps", "a", "s", "d", "f", "g", "h", "j", "k", "l", "colon", "quote", "hash", "num4", "num5", "num6",
		"g5", "lshift", "bslash_iso", "z", "x", "c", "v", "b", "n", "m", "comma", "dot", "slash", "rshift", "up", "num1", "num2", "num3", "numenter",
		"g6", "lctrl", "lwin", "lalt", "space", "ralt", "rwin", "rmenu", "rctrl", "left", "down", "right", "num0", "numdot"
	};
	std::string topbar;
	for(auto i = 1; i <= 19; ++i) {
		topbar += "topbar";
		topbar += std::to_string(i);
		if(i < 19) {
			topbar += ",";
		}
	}
	std::map<std::string, key_id_t> key_to_id_map;
	key_id_t i = 0;
	for(const auto& s : id_to_key_map) {
		key_to_id_map.insert(std::make_pair(s, i++));
	}
	const size_t total_keys = id_to_key_map.size();

	std::vector<size_t> press_map(total_keys*total_keys, 0);
	std::ifstream in(preload_path, std::ios::binary);
	if(in.is_open()) {
		std::cout << "Load data from " << preload_path << std::endl;
		in.read(
			reinterpret_cast<char*>(press_map.data()),
			press_map.size()*sizeof(decltype(press_map)::value_type)
		);
	}


	namespace fs = std::filesystem;
	constexpr auto device_path = "/dev/input/ckb1/";
	std::ofstream cmd(device_path+std::string("cmd"));
	std::mutex cmd_mutex;
	cmd << "active" << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(200));

	std::string notify_name;

	for(unsigned int i = 1; i < 9; ++i) {
		if(
			!fs::exists(device_path+std::string("notify")+std::to_string(i))
		) {
			notify_name = "notify"+std::to_string(i);
			cmd << "notifyon " << i << std::endl;
			cmd << "@" << i << " notify all" << std::endl;
			break;
		}
	}

	signal_handler_t cmd_handle(
		[&](int){
			const auto notify_id = std::stoi(notify_name.substr(notify_name.size()-1));
			std::cout	<< "notifyoff " << notify_id << std::endl;
			cmd			<< "notifyoff " << notify_id << std::endl;
		}
	);

	double cpu_usage = 0;
	std::thread cpu_freq_thread(
		[&](){
			size_t last_used = 0;
			size_t last_idle = 0;
			color_t old;
			while(cmd.is_open()) {
				std::ifstream proc("/proc/stat");
				proc.seekg(5);
				size_t user, nice, system, idle;
				proc >> user >> nice >> system >> idle;
				const auto used = user+nice+system;
				const auto delta_used = used-last_used;
				const auto delta_idle = idle-last_idle;
				cpu_usage = static_cast<double>(delta_used)/static_cast<double>(delta_used+delta_idle);
				cpu_usage = 2*cpu_usage; //make hyperthreads not count
				last_used = used;
				last_idle = idle;
				old = fade(old, multi_fade(cpu_heat_map, cpu_usage), 0.1);

				std::this_thread::sleep_for(std::chrono::milliseconds(1000/10)); // "fps"
				std::unique_lock lock(cmd_mutex);
				cmd << "rgb " << topbar << ":" << old << std::endl;

			}
		}
	);
	if(notify_name.empty()) {
		std::cerr << "Could not create a notify session on ckb-device. All ranks are in use." << std::endl;
		return 1;
	}
	std::cout << "Listening on " << notify_name << std::endl;

	std::cout << "Open " << device_path+notify_name << std::endl;
	cmd << std::hex << std::setfill('0') << std::right;
	std::cout << std::right << std::hex << std::setfill('0');
	std::ifstream notify;
	do {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		notify.open(device_path+notify_name);
	} while(!notify.is_open());
	std::cout << notify_name << " opened for reading." << std::endl;
	cmd << "rgb all:0000ff" << std::endl;
	std::string line;
	key_id_t last_key=0;
	std::vector<std::pair<double, key_id_t>> frequency(total_keys, {0.0, 0});
	while(std::getline(notify, line)) {
// 		std::cout << line << std::endl;
		if(line[4]=='+') {
			const auto key = line.substr(5);
			const auto id = key_to_id_map[key];
			++press_map[last_key*total_keys + id];
			last_key = id;
			auto total_presses =
				std::accumulate(
					press_map.begin()+last_key*total_keys,
					press_map.begin()+((last_key+1)*total_keys-1),
					0.0
				);

			for(key_id_t i = 0; i < total_keys; ++i) {
				frequency[i].first = static_cast<double>(press_map[last_key*total_keys+i]) / static_cast<double>(std::max(total_presses, 1.0));
				frequency[i].second = i;
			}

			std::sort(frequency.begin(), frequency.end(), std::greater<typename decltype(frequency)::value_type>());

			std::unique_lock lock(cmd_mutex);

			cmd << "rgb all:" << background_color << " " << topbar << ":" << multi_fade(cpu_heat_map, cpu_usage);
// 			std::cout << "rgb";

			double total_percentage = 0;
			for(key_id_t i = 0; i < total_keys; ++i) {
				total_percentage += frequency[i].first;
				cmd 		<< " " << id_to_key_map[frequency[i].second] << ":" << active_color;
				if(total_percentage >0.90) {
					break;
				}
			}
			cmd << std::endl;
// 			std::cout << std::endl;
		}
	}
	std::cout << "Stop listening on " << notify_name << std::endl;

	std::cout << "Saving state to " << preload_path << std::endl;
	std::ofstream out(preload_path, std::ios::binary | std::ios::trunc);
	out.write(
		reinterpret_cast<const char*>(press_map.data()),
		press_map.size()*sizeof(decltype(press_map)::value_type)
	);
	cpu_freq_thread.join();
	return 0;
}
