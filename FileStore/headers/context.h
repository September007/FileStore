#pragma once
#include<string>
#include<Config.h>
using std::string;


class Context {
public:
/**
* as name indicating
*/
	string fsPath;
	string journalPath;
	string kvPath;
	string rbPath;

	int journal_callback_worker_count;
	int journal_write_worker_count;

	bool load(string name) {
		auto config = GetConfig(name, "config", "Context", true);
		try {
			fsPath = filesystem::absolute(config["fsPath"].get<string>()).string();
			journalPath = filesystem::absolute(config["journalPath"].get<string>()).string();
			kvPath = filesystem::absolute(config["kvPath"].get<string>()).string();
			rbPath = filesystem::absolute(config["rbPath"].get<string>()).string();

			journal_callback_worker_count = config["journal_callback_worker_count"].get<int>();
			journal_write_worker_count = config["journal_write_worker_count"].get<int>();
			return true;
		}
		catch (std::exception& e) {
			LOG_ERROR("context", format("context load from [{}] failed", name));
			return false;
		}
	}
};