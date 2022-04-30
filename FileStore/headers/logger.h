#pragma once 
#include<fmt/format.h>
#include<memory>
#include<spdlog/spdlog.h>
#include<spdlog/sinks/basic_file_sink.h>
#include<filesystem>
using std::shared_ptr;
using std::string;
using fmt::format;
inline shared_ptr<spdlog::logger> GetLogger(const string& name,
	const bool force_isolate = false, const string& fileClass = "integrated") {
	//@Todo: read log file root from json
	static auto logFileRoot = std::filesystem::absolute("./tmp/logs").string();
	// set all output into one file for debug
	//if (!force_isolate && name != "integrated")
	//	return GetLogger("integrated");
	auto ret = spdlog::get(name);
	// if missing,create
	if (ret == nullptr) {
		{
			//create is mutually exclusive
			static std::mutex m_;
			std::unique_lock lg(m_);
			//try get again
			if ((ret = spdlog::get(name)) != nullptr)
				return ret;
			auto logfilename = fmt::format("{}/{}.log", logFileRoot, force_isolate ? name : fileClass);
			bool create_new = false;
			ret // = spdlog::basic_logger_mt<spdlog::synchronous_factory>(name, logfilename, false);
				= spdlog::synchronous_factory::create<spdlog::sinks::basic_file_sink_st>(name, logfilename, false);
			if (ret != nullptr)
				create_new = true;
			// if create failed, return default
			if (!ret) {
				// try return selfdefined first
				if (name != "default")
					ret = GetLogger("default", true, "default");
			}
			if (!ret) {
				spdlog::warn("create log file[{}]failed {}:{}", logfilename, __FILE__,
					__LINE__);
				ret = spdlog::default_logger();
			}
			if (create_new) {
				ret->info("log:{} just created", name);
			}
			else {
				ret->info("creating log: {} just failed, this is default", name);
			}
		}
	}
	return ret;
}

template <typename T>
inline void LogExpectOrWarn(const string logName, T&& t, T expect) {
	if (t != expect) {
		GetLogger(logName)->warn("expect {} but get {}.{}:{}", expect, t, __FILE__,
			__LINE__);
	}
}
#pragma region macro
#define LOG_ASSERT_TRUE(logName,condi,msg) do { if((condi)!=true) GetLogger(logName)->error(msg" expect true but get false. [{}]at {}:{}"\
        ,#condi,__FILE__,__LINE__);}while(0)
#define LOG_EXPECT_TRUE(logName,condi) do { if((condi)!=true) GetLogger(logName)->error("expect true but get false. [{}]at {}:{}"\
        ,#condi,__FILE__,__LINE__);}while(0)
#define LOG_EXPECT_EQ(logName,l,r) do { if((l)!=(r)) GetLogger(logName)->error("expect equal but not. [{}:{}]!=[{}:{}]at {}:{}"\
        ,#l,l,#r,r,__FILE__,__LINE__);}while(0)

#define LOG_INFO(logName,msg,...) do { GetLogger(logName,##__VA_ARGS__ )->info("{}: {} at {}:{}",__func__,msg,__FILE__,__LINE__);}while(0)

#define LOG_WARN(logName,msg,...) do { GetLogger(logName,##__VA_ARGS__ )->warn("{}: msg {} at {}:{}",__func__,msg,__FILE__,__LINE__);}while(0)

#define LOG_ERROR(logName,msg,...) do { GetLogger(logName,##__VA_ARGS__ )->error("{}: {} at {}:{}",__func__,msg,__FILE__,__LINE__);}while(0)

#ifndef DebugArea
#define DebugArea(l,...)  l,##__VA_ARGS__ 
#define ReleaseArea(l,...)
#else
#define DebugArea(l,...)  
#define ReleaseArea(l,...) l,##__VA_ARGS__ 
#define DebugArea()
#endif
#pragma endregion