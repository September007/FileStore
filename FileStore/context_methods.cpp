#include <context_methods.h>
#pragma warning(disable : 4996)
string stdio_ReadFile(const string& path)
{
	fstream in(path);
	if (!in.good()) {
		LOG_INFO("IO", fmt::format("stdio_ReadFile[{}] failed.", path));
		return "";
	}
	string ret;
#ifdef __linux__
	in.seekg(0, ios_base::end);
	size_t length = in.tellg();
	in.seekg(ios_base::beg);
	ret.resize(length);
	in.read(const_cast<char*>(ret.c_str()), length);
	// if (ret.back() == '\0')ret.pop_back();
#else
	// windows file storage would append {how much '\n' in file} '\0' chars at end
	// of file so read as above would read these '\0'
#ifdef _WIN32
	stringstream ss;
	ss << in.rdbuf();
	ret = ss.str();
#endif
#endif
	return ret;
}
bool stdio_WriteFile(
	const string& path, const string& content, const bool create_parent_dir_if_missing)
try {
	// auto& m = GetMutex(__func__);
	// unique_lock lg(m);
	auto&	 objDir = path;
	ofstream out(objDir);
	if (!out.good()) {
		auto parentDir = filesystem::path(objDir).parent_path();
		// maybe missing pg directory
		if (create_parent_dir_if_missing && !filesystem::is_directory(parentDir.c_str())) {
			// GetLogger("IO")->warn(
			//"write file[{}] failed because parent dir missed,now creating.{}:{}",
			//	path, __FILE__, __LINE__);
			filesystem::create_directories(parentDir);
			out.open(objDir);
			LOG_ASSERT_TRUE("IO", out.good(), "write file because create parent dir failed ");
		} else {
			GetLogger("IO")->error("open file[{}] failed .{}:{}", path, __FILE__, __LINE__);
			return false;
		}
	}
	out << content << flush;
	out.close();
	return out.good();
} catch (std::exception& ex) {
	GetLogger("IO")->error("catch error at{}:{}\n{}", __FILE__, __LINE__, ex.what());
	throw ex;
	return false;
}

string keep_ReadFile(const string& path, int sz)
{
	string ret(sz, '\0');
	auto   p = ret.data(), end = ret.data() + sz;
	auto   fd = fopen(path.c_str(), "r");
	while (p < end) {
		int read_len = fread(p, 1, sz, fd);
		if (read_len > 0) {
			p += read_len;
			sz -= read_len;
		} else if (read_len == 0) {
			// EOF
			break;
		} else {
			// bad,but warn not error
			LOG_WARN("io", format("read file {} of size {} failed", path, sz));
			break;
		}
	}
	ret.resize(p - ret.data());
	fclose(fd);
	return move(ret);
}
bool keep_WriteFile(
	const string& path, const string& content, const bool create_parent_dir_if_missing)
{
	char* p	  = const_cast<char*>(content.data());
	int	  sz  = content.size();
	char* end = p + sz;
	auto  fd  = fopen(path.c_str(), "w");
	if (fd == 0 && create_parent_dir_if_missing) {
		auto noncvp = path;
		auto ppath	= GetParentDir(noncvp);
		if (!filesystem::is_directory(ppath))
			// if (mkdir(ppath.c_str()));
			// else
			filesystem::create_directories(ppath);
		fd = fopen(path.c_str(), "w");
	}
	LOG_EXPECT_TRUE("IO", fd != 0);
	while (p < end) {
		auto r = fwrite(p, 1, sz, fd);
		if (r > 0) {
			sz -= r;
			p += r;
		} else if (r == 0) {
			// EOF
			break;
		} else {
			// something bad
			LOG_ERROR("io", format("Write file {} failed", path));
			return false;
		}
	}
	if (fd)
		fclose(fd);
	return true;
}