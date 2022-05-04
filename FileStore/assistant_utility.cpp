#include <Mutex.h>
#include <assistant_utility.h>
#include <filesystem>
#include <object.h>
#pragma warning(disable : 4996)
/**
 * get nlohmann::json from specified file settingFile.
 * @note recommend to using GetConfigFromFile,so \deprecated
 */
nlohmann::json GetSetting(const string& settingFile)
{
	static nlohmann::json default_setting = R"({
	"setting_name":"default",
	"bool":{
		"true_value":true,
		"false_value":false
	},
	"string": "str_string"
})"_json;
	ifstream			  in(settingFile);
	nlohmann::json		  setting;
	if (in.good())
		in >> setting;
	else {
		GetLogger("json setting")
			->error("read setting file failed.{}[{}:{}]", settingFile, __FILE__, __LINE__);
		setting = default_setting;
	};
	return setting;
};
/**
 * log down error into default_logger before call ::exit().
 */
void Error_Exit(const string& msg)
{
	spdlog::default_logger()->error("msg:[{}]get error exit,check log for more info", msg);
	exit(0);
}
/**
 * get formatted string of time.
 * @param fmt fmt format form ,which using by std::strftime
 */
std::string getTimeStr(std::string_view fmt)
{
	std::time_t now	  = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	char		s[40] = {};
	std::strftime(&s[0], 40, fmt.data(), std::localtime(&now));
	return s;
}

Slice::Slice(const string& str)
	: data(make_shared<buffer>(str))
	, start(0)
	, end(str.size())
{
}

void Slice::Read(buffer& buf, const Slice* sli)
{
	new (&const_cast<Slice*>(sli)->data) shared_ptr<buffer>(make_shared<buffer>());
	::Read(buf, &sli->data->data);
}

void Slice::Write(buffer& buf, const Slice* s)
{
	if (s->data != nullptr)
		::Write(buf, &s->data->data);
	else {
		buffer b;
		::Write(buf, &b);
	}
	using tif = tuple<int*, float*>;
	tif	 a;
	auto i = std::get<0>(a);
}

string GetParentDir(string& path)
{
	// DebugArea(auto abpath = filesystem::absolute(path); LOG_EXPECT_EQ("IO", abpath.string(),
	// path));
	for (int i = path.size() - 1; i >= 0; --i)
		if (path[i] != '/' && path[i] != '\\')
			path.pop_back();
		else
			break;
	return path;
}

unsigned int Get_Thread_Id()
{
	auto id = this_thread::get_id();
	auto p	= (unsigned int*)&id;
	return *p;
}
