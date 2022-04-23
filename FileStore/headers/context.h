#pragma once
#include<string>
#include<config.h>
#include<assistant_utility.h>
#include<object.h>
#include<context_methods.h>
using std::string;

bool dd(const string&, const string&, const bool = 1);
bool dd(const string&, const string&, const bool);

class Context {
public:
	/**
	* as name indicating
	*/
	string fsPath;
	string journalPath;
	string kvPath;
	string rbPath;

	int default_block_size;
	int journal_callback_worker_count;
	int journal_write_worker_count;
	Context();
	bool load(string name) {
		auto config = GetConfig(name, "config", "context", true);
		try {
			fsPath = filesystem::absolute(config["fsPath"].get<string>()).string();
			journalPath = filesystem::absolute(config["journalPath"].get<string>()).string();
			kvPath = filesystem::absolute(config["kvPath"].get<string>()).string();
			rbPath = filesystem::absolute(config["rbPath"].get<string>()).string();

			journal_callback_worker_count = config["journal_callback_worker_count"].get<int>();
			journal_write_worker_count = config["journal_write_worker_count"].get<int>();
			default_block_size = config["default_block_size"].get<int>();
			//traits
			{
				auto traits = config["traits"];
#if defined(_WIN64)||defined(_WIN32)
				if (traits["using_iocp"].get<bool>() == true)
#else
				if (traits["using_epoll"].get<bool>() == true)
#endif				
				{
					m_WriteFile = ::stdio_WriteFile;
					m_ReadFile = ::stdio_ReadFile;
				}
		}
			return true;
	}
		catch (std::exception& e) {
			LOG_ERROR("context", format("context load from [{}] failed,catch error[{}]", name,e.what()));
			return false;
		}
}

	// them often set by load
	std::function<bool(const string&, const string&, const bool)> m_WriteFile;
	std::function<string(const string&)> m_ReadFile;
	std::function<string(const ReferedBlock& rb, string root_pat)> m_GetReferedBlockStoragePath;

	// io accelerate
	//bool using_iocp = false;
	//bool using_epoll = false;
	bool using_io_acc = false;
};