#include<assistant_utility.h>
#include<filesystem>
#include<Mutex.h>
#include<object.h>
#pragma wanring(disable:4996)
nlohmann::json GetSetting(const string& settingFile) {
	static nlohmann::json default_setting = R"({
	"setting_name":"default",
	"bool":{
		"true_value":true,
		"false_value":false
	},
	"string": "str_string"
})"_json;
	ifstream in(settingFile);
	nlohmann::json setting;
	if (in.good())
		in >> setting;
	else {
		GetLogger("json setting")
			->error("read setting file failed.{}[{}:{}]", settingFile, __FILE__,
				__LINE__);
		setting = default_setting;
	};
	return setting;
};

void Error_Exit(const string &msg) {
	spdlog::default_logger()->error("msg:[{}]get error exit,check log for more info",msg);
	exit(0);
}
std::string getTimeStr(std::string_view fmt) {
	std::time_t now =
		std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	char s[40] = {};
	std::strftime(&s[0], 40, fmt.data(), std::localtime(&now));
	return s;
}


string GetReferedBlockStoragePath_deep4(const ReferedBlock& rb, string root_path) {
	auto s = rb.serial;
	//(s = boost::hash<decltype(rb.serial)>()(rb.serial));
	s = std::hash<int64_t>()(rb.serial);
	auto p1 = (s & 0xffff000000000000ll) >> 48;
	auto p2 = (s & 0x0000ffff00000000ll) >> 32;
	auto p3 = (s & 0x00000000ffff0000ll) >> 16;
	auto p4 = (s & 0x000000000000ffffll) >> 0;
	return fmt::format("{}/{}/{}/{}/{}.txt", root_path, p1, p2, p3, p4);
}
 string GetReferedBlockStoragePath_deep2(const ReferedBlock& rb, string root_path) {
	auto s = rb.serial;
	//(s = boost::hash<decltype(rb.serial)>()(rb.serial));
	s = std::hash<int64_t>()(rb.serial);
	auto p1 = (s & 0xffffffff00000000ll) >> 32;
	auto p2 = (s & 0x00000000ffffffffll) >> 0;
	return fmt::format("{}/{}/{}.txt", root_path, p1, p2);
}

Slice:: Slice(const string& str) :data(make_shared<buffer>(str)), start(0), end(str.size()) {
}

void Slice::Read(buffer& buf,const Slice* sli) {
	new(&const_cast<Slice*>(sli)->data)shared_ptr<buffer>(make_shared<buffer>());
	::Read(buf, &sli->data->data);
}

void Slice::Write(buffer& buf,const Slice* s) {
	if (s->data != nullptr)
		::Write(buf, &s->data->data);
	else {
		buffer b;
		::Write(buf, &b);
	}
	using tif = tuple<int*, float*>;
	tif a;
	auto i=std::get<0>(a);
}


 string GetParentDir(string& path) {
	//DebugArea(auto abpath = filesystem::absolute(path); LOG_EXPECT_EQ("IO", abpath.string(), path));
	for (int i = path.size() - 1; i >= 0; --i)
		if (path[i] != '/' && path[i] != '\\')
			path.pop_back();
		else break;
	return path;
}