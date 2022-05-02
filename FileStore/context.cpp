#include <assistant_utility.h>
#include <config.h>
#include <context.h>
#include <context_methods.h>
Context::Context()
{
	auto idle = [](shared_ptr<WOPE> wope) {};
	// set by default, change in load
	m_WriteFile								  = ::stdio_WriteFile;
	m_ReadFile								  = ::stdio_ReadFile;
	m_GetReferedBlockStoragePath			  = ::GetReferedBlockStoragePath_deep2;
	replay_default_callback_when_log_done	  = idle;
	replay_default_callback_when_journal_done = idle;
	replay_default_callback_when_flush_done	  = idle;
}
bool Context::load(string name)
{
	json config;
// the default context is up to platform
#ifdef _WIN32
	config = GetConfig(name, "config", "windows.context", true);
#else
	config = GetConfig(name, "config", "linux.context", true);
#endif
	try {
		fspath		= filesystem::absolute(config["fsPath"].get<string>()).string();
		journalpath = filesystem::absolute(config["journalPath"].get<string>()).string();
		kvpath		= filesystem::absolute(config["kvPath"].get<string>()).string();
		rbpath		= filesystem::absolute(config["rbPath"].get<string>()).string();

		journal_callback_worker_count = config["journal_callback_worker_count"].get<int>();
		journal_write_worker_count	  = config["journal_write_worker_count"].get<int>();
		default_block_size			  = config["default_block_size"].get<int>();
		// traits
		{
			auto traits = config["traits"];
#if defined(_WIN64) || defined(_WIN32)
			if (traits["using_iocp"].get<bool>() == true)
#else
			if (traits["using_epoll"].get<bool>() == true)
#endif
			{
				m_WriteFile = ::stdio_WriteFile;
				m_ReadFile	= ::stdio_ReadFile;
			}
		}
		return true;
	} catch (std::exception& e) {
		LOG_ERROR(
			"context", format("context load from [{}] failed,catch error[{}]", name, e.what()));
		return false;
	}
}
