/*****************************************************************/
/**
 * \file   Config.h
 * \brief  imple some methods to load config in form of json which storing in file
 */
/*****************************************************************/
#pragma once
#ifndef GD_CONFIG_HEAD
#define GD_CONFIG_HEAD
#include <assistant_utility.h>
#include <context_methods.h>
#include <map>
#include <nlohmann/json.hpp>
using fmt::format;
using nlohmann::json;
/**
 * get json object which read from file filename.
 * @param rootpaths point out possiable root path of some file with name ${filename},and the first
 * will be use
 * @exception if read candiate file failed,will log error into `IO` instead of throw error
 * @return if find one legal file ,will return it,otherwise return empty json object
 */
inline json GetConfigFromFile(const string& filename, const vector<string>& rootpaths)
{
	json ret;
	try {
		for (auto& root : rootpaths) {
			auto path = format("{}/{}", root, filename);
			if (filesystem::is_regular_file(path)) {
				auto f = stdio_ReadFile(path);
				ret	   = json::parse(f);
				break;
			}
		}
	} catch (std::exception& e) {
		LOG_INFO("IO",
			fmt::format(
				"read config file [{}]catch exception[{}]", as_string(rootpaths), e.what()));
	}
	return ret;
}
/**
 * need check empty before use.
 * @param name		using to find specified file
 * @param key		using to find the subodinated(child) json object of ones just find by ${name}
 * @param default_class	if above find operation fialed,will log down failure and tried to find
 * default setting,which is specified by this param
 * @param reload			in case of config file content changed, this option point out if reload
 * config is needed
 * @return			if above tring success, return the json content, otherwise log down infomation
 * and return empty json obejcvt
 * @detail search sequence
 * integrated.default.json.{name}		first
 * integrated.json.{name}				overload
 * {name}.default.json.{name}			overload
 * {name}.json.{name}					overload
 * but this overwrite is over simple ,on in the surface of dirct key of  name
 */
inline json GetConfig(
	const string& name, const string& key, const string& default_class = "", bool reload = false)
{
	//@FATAL: change root
	static vector<string> configRoots
		= { filesystem::absolute("../../../FileStore/config").generic_string(),
			  filesystem::absolute("../../../../FileStore/config").generic_string(),
			  filesystem::absolute("../../../../../FileStore/config").generic_string(),
			  filesystem::absolute("./config").generic_string() };
	static auto sid = GetConfigFromFile(fmt::format("integrated.default.json"), configRoots);
	static auto si	= GetConfigFromFile(fmt::format("integrated.json"), configRoots);
	static map<string, json> cache;
	json					 ret;	//
	string indexs[] = { format("integrated.default.{}", name), format("integrated.{}", name),
		format("{}.default", name), format("{}", name) };
	if (cache[format("integrated.default.{}", name)].empty() || reload) {
		cache[format("integrated.default.{}", name)]
			= GetConfigFromFile("integrated.default.json", configRoots)[name];
	}
	if (cache[format("integrated.{}", name)].empty() || reload) {
		cache[format("integrated.{}", name)]
			= GetConfigFromFile("integrated.json", configRoots)[name];
	}
	if (cache[format("{}.default", name)].empty() || reload) {
		cache[format("{}.default", name)]
			= GetConfigFromFile(format("{}.default.json", name), configRoots);
	}
	if (cache[format("{}", name)].empty() || reload) {
		cache[format("{}", name)] = GetConfigFromFile(format("{}.json", name), configRoots);
	}
	for (auto index : indexs)
		// tedious longy
		if (!(cache[index])[key].empty()/*id.find(name) != id.end() && id.find(name)->find(key) != id.find(name)->end()*/) {
			ret = (cache[index])[key];
		}
	// read default_class
	if (ret.empty()) {
		LOG_WARN("config",
			fmt::format(
				"get [{}:{}],default:[{}] failed,may try default", name, key, default_class),
			true);
		if (default_class != "") {
			ret = GetConfig(default_class, key, "", reload);
			if (ret.empty()) {
				LOG_WARN("config",
					fmt::format("get [{}:{}],default:[{}] failed,try default failed too", name, key,
						default_class),
					true);
			}
		}
	}
	return ret;
}
/**
 * use this replace directly GetConfig to avoid some boring result-empty checking
 * @note for complicated config,still suggest using GetConfig
 * @return if GetConfig successfully read something, will return it, otherwise return defalur_value
 * \sa GetConfig Context::load()
 */
template <typename T>
inline T GetconfigOverWrite(T default_value, const string& default_class, const string& name,
	const string& key, bool reload = false)
{
	auto r = GetConfig(name, key, default_class, reload);
	if (r.empty()) {
		LOG_ERROR("config",
			fmt::format(
				"can't read the [{}:{}:{}] ,use default {}, but this is not allowd in realtime",
				default_class, name, key, default_value));
		return default_value;
	} else {
		try {
			return r.get<T>();
		} catch (std::exception& e) {
			LOG_ERROR("config",
				fmt::format("catch error[{}],when getconfig of [{}:{}:{}]", e.what(), default_class,
					name, key));
			return default_value;
		}
	}
}
#endif	 // GD_CONFIG_HEAD