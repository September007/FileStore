
/*****************************************************************/
/**
 * \file   assistant_utility.h
 * formatted time,IO,serialize,
 *	1.	if specified class is plained(has no pointer memeber like raw pointer or shared_ptr),
 *		you only need achieve the member function GetES() to point out member address,
 *		but if not, you should achieve three more function to do some spcified work
 *		(you still could use GetES() to do with other non-pointer member),check class::Slice
 *		seedetail;
 *  2.	randomValue,Write,Read do not expect type tuple(lazy)
 */
/*****************************************************************/

#pragma once
#pragma warning(disable : 4267)
#ifndef ASSISTANT_UTILITY_HEAD
#define ASSISTANT_UTILITY_HEAD

#include <chrono>
#include <concepts>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <list>
#include <logger.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <random>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <type_traits>
#include <vector>
/**
 * using to judge if type T is generated from template templateType.
 */
using namespace std;
template <template <typename... args> typename templateType, typename T>
constexpr bool is_from_template = false;
/**
 * \sa is_from_template.
 */
template <template <typename... args> typename templateType, typename... args>
constexpr bool is_from_template<templateType, templateType<args...>> = true;

template <typename T> constexpr bool is_tuple = is_from_template<tuple, T>;

nlohmann::json GetSetting(const string& settingFile);

void			   Error_Exit(const string& msg = "");
std::string		   getTimeStr(std::string_view fmt);
inline std::string getTimeStr() { return getTimeStr("%Y-%m-%d %H:%M:%S"); }

template <typename T> auto randomValue();
template <typename T> void randomValue(T* t, int len = 1, bool call_des = true);
template <typename T, int n = 0>
requires is_from_template<tuple, T>
void _tupleRandomValue(T* t)
{
	if constexpr (n == tuple_size_v<T>)
		return;
	else {
		randomValue(std::get<n>(*t));
		_tupleRandomValue<T, n + 1>(t);
	}
}
// suppose pointer t don't need call destruction
template <typename T> void randomValue(T* t, int len, bool call_des)
{
	static mt19937_64 rando(chrono::system_clock::now().time_since_epoch().count());
	if (call_des)
		for (int i = 0; i < len; ++i)
			(t + i)->~T();
	using DT = decay_t<T>;
	if constexpr (is_same_v<decay_t<T>, uint8_t>) {
		for (int i = 0; i < len; ++i)
			*(uint8_t*)(t + i) = (rando() % (UINT8_MAX + 1));
	} else if constexpr (is_integral_v<T> || is_enum_v<T>) {
		randomValue<uint8_t>(const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(t)),
			len * sizeof(T) / sizeof(uint8_t));
	} else if constexpr (is_floating_point_v<T>) {
		static uniform_real_distribution<DT> urd(
			numeric_limits<DT>::min(), numeric_limits<DT>::max());
		for (int i = 0; i < len; ++i)
			*const_cast<DT*>(t + i) = urd(rando);
	} else if constexpr (is_same_v<string, decay_t<T>>) {
		for (int i = 0; i < len; ++i) {
			int slen = rando() % 100 + 1;
			new ((string*)(t) + i) string(slen, '\0');
			randomValue<uint8_t>(
				const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(t[i].data())), slen);
		}
	} else if constexpr (requires(T _t) { _t.GetES(); }) {
		for (int i = 0; i < len; ++i) {
			auto es = t[i].GetES();
			_tupleRandomValue(&es);
			// for more random
			if constexpr (requires(decay_t<T> _t) { T::RandomValue(&_t); })
				T::RandomValue((T*)(t) + i);
		}
	} else if constexpr (is_tuple<T>) {
		// expect tuple only come from class.GetES()
		LOG_EXPECT_EQ("randomValue", len, 1);
		_tupleRandomValue(t);
	} else if constexpr (is_from_template<vector, T>) {
		for (int i = 0; i < len; ++i) {
			int sz = rando() % 10 + 1;
			new (t + i) T(sz);
			randomValue<typename T::value_type>(&(*t)[0], sz);
		}
	} else if constexpr (is_same_v<T, T>) {
		static_assert(!is_same_v<T, T>, "can't found suited imple,plz make it");
	}
}
template <typename T> auto randomValue()
{
	uint8_t buf[sizeof(T)];
	randomValue<T>(reinterpret_cast<T*>(buf), 1, false);
	return move(*reinterpret_cast<T*>(buf));
}

// Serialize and Unserialize
// buffer is obligate to control data block
struct buffer {
public:
	string data;
	int	   offset = 0;
	buffer(const string& data = "", int offset = 0)
		: data(data)
		, offset(offset)
	{
	}
	buffer(string&& data)
		: data(data)
		, offset(0)
	{
	}
	bool operator==(const buffer& buf)
	{
		return this->data == buf.data && this->offset == buf.offset;
	}
	void append(int len, const void* src)
	{
		data.append(len, '\0');
		memcpy(data.data() + data.size() - len, src, len);
	}
	void drawback(int len, const void* des)
	{
		memcpy(const_cast<void*>(des), data.data() + offset, len);
		offset += len;
	}
	string universal_str() { return data; }
	auto   GetES() const { return make_tuple(&data, &offset); }
};
/**
 * refer data from buffer.
 * @note almost \deprecated
 */
class Slice {
public:
	shared_ptr<buffer> data;
	int				   start;
	int				   end;
	Slice(shared_ptr<buffer> data = nullptr, int start = 0, int end = 0)
		: data(data)
		, start(start)
		, end(end)
	{
	}
	explicit Slice(shared_ptr<buffer> data)
		: data(data)
		, start(0)
		, end(data->data.length())
	{
	}
	explicit Slice(const string& str);
	Slice(const Slice&) = default;
	// value cmp
	bool operator==(const Slice& sl) const
	{
		return this->start == sl.start && this->end == sl.end
			&& ((this->data == sl.data && this->data == nullptr)
				|| (this->data != nullptr && sl.data != nullptr && (*this->data == *sl.data)));
	}
	int	  GetSize() const { return end - start; }
	Slice SubSlice(int _start, int _size)
	{
		if (_start + _size < GetSize())
			return Slice(data, this->start + _start, this->start + _start + _size);
		else
			return Slice(data, this->start + _start, this->end);
	}
	// support for serilize and rando
	static void Read(buffer& buf, const Slice* sli);
	static void Write(buffer& buf, const Slice* s);
	static void RandomValue(const Slice* sli)
	{
		new (&const_cast<Slice*>(sli)->data) shared_ptr<buffer>(make_shared<buffer>());
	}
	auto GetES() const { return make_tuple(&start, &end); }
};

template <typename T> void		 Write(buffer& buf, T* t);				 /**< serialize interface*/
template <typename T> void		 Read(buffer& buf, T* t);				 /**< serialize interface*/
template <typename T> decay_t<T> Read(buffer& buf);						 /**< serialize interface*/
template <typename T> void		 WriteArray(buffer& buf, T* t, int len); /**< serialize interface*/
template <typename T> void		 ReadArray(buffer& buf, T* t, int len);	 /**< serialize interface*/

template <typename T, int n = 0>
requires is_from_template<tuple, T>
void _TupleWrite(buffer& buf, T* t) /**< serialize interface*/
{
	if constexpr (n == tuple_size_v<T>)
		return;
	else {
		Write(buf, std::get<n>(*t));
		_TupleWrite<T, n + 1>(buf, t);
	}
}

template <typename T, int n = 0>
requires is_from_template<tuple, T>
void _TupleRead(buffer& buf, T* t) /**< serialize interface*/
{
	if constexpr (n == tuple_size_v<T>)
		return;
	else {
		if constexpr (is_same_v<T, decay_t<T>>)
			Read(buf, std::get<n>(*t));
		else
			Read(buf, const_cast<decay_t<decltype(std::get<n>(*t))>>(*t));
		_TupleRead<T, n + 1>(buf, t);
	}
}
template <typename T> void Write(buffer& buf, T* t)
{
	constexpr int TLEN = sizeof(T);
	using DT		   = std::decay_t<T>;
	if constexpr (is_arithmetic_v<DT> || is_enum_v<DT>) {
		buf.append(TLEN, (void*)t);
	} else if constexpr (is_same_v<string, decay_t<DT>>) {
		int len = t->length();
		buf.append(sizeof(int), &len);
		buf.append(len * sizeof(char), t->data());
	} else if constexpr (requires(DT _t) { _t.GetES(); }) {
		auto es = const_cast<DT*>(t)->GetES();
		_TupleWrite(buf, &es);
		// more action ,may for shared_ptr<T> balabalah
		if constexpr (requires(DT t) {
						  DT::Write(declval<buffer&>(), &t);
						  DT::Read(declval<buffer&>(), &t);
					  }) {
			T::Write(buf, const_cast<DT*>(t));
		}
	} else if constexpr (is_from_template<tuple, DT>) {
		_TupleWrite(buf, const_cast<DT*>(t));
	} else if constexpr (is_from_template<vector, DT>) {
		int sz = t->size();
		buf.append(sizeof(sz), &sz);
		if (sz == 0)
			return;
		WriteArray(buf, &(*t)[0], sz);
	} else if constexpr (is_from_template<list, T>) {
		int sz = t->size();
		buf.append(sizeof(sz), &sz);
		if (sz == 0)
			return;
		for (auto& e : *t)
			Write(buf, &e);
	} else if constexpr (is_same_v<T, T>) {
		static_assert(!is_same_v<T, T>, "can't found right imple");
		LOG_ERROR("integrated", "can't found suited imple of write");
	}
}

template <typename T> decay_t<T> Read(buffer& buf)
{
	constexpr int TLEN = sizeof(T);
	using DT		   = decay_t<T>;
	char cache[TLEN];
	Read<DT>(buf, reinterpret_cast<DT*>(cache));
	return move(*(reinterpret_cast<DT*>(cache)));
}

template <typename T> void Read(buffer& buf, T* t)
{
	// if (call_Des)
	// t->~T();
	constexpr int TLEN = sizeof(T);
	using DT		   = decay_t<T>;
	if constexpr (is_arithmetic_v<T> || is_enum_v<T>) {
		buf.drawback(TLEN, t);
	} else if constexpr (is_same_v<DT, string>) {
		auto len = Read<int>(buf);
		new ((string*)(t)) string(size_t(len), '\0');
		buf.drawback(len, const_cast<char*>(t->data()));
	} else if constexpr (requires(DT _t) { _t.GetES(); }) {
		auto es = t->GetES();
		_TupleRead(buf, &es);
		// more action ,may for shared_ptr<T> balabalah
		if constexpr (requires(T _t) {
						  T::Write(declval<buffer&>(), &_t);
						  T::Read(declval<buffer&>(), &_t);
					  }) {
			T::Read(buf, t);
		}
	} else if constexpr (is_from_template<tuple, DT>) {
		_TupleRead(buf, t);
	} else if constexpr (is_from_template<vector, DT>) {
		int sz;
		buf.drawback(sizeof(sz), &sz);
		new (t) T(sz);
		if (sz == 0)
			return;
		ReadArray(buf, &(*t)[0], sz);
	} else if constexpr (is_from_template<list, DT>) {
		int sz;
		buf.drawback(sizeof(sz), &sz);
		new (t) T();
		if (sz == 0)
			return;
		while (sz--) {
			auto r = Read<typename T::value_type>(buf);
			t->push_back(r);
		}
	} else if constexpr (is_same_v<T, T>) {
		static_assert(!is_same_v<T, T>, "can't found right imple");
	}
}

template <typename T> void WriteArray(buffer& buf, T* t, int len)
{
	for (int i = 0; i < len; ++i)
		Write(buf, t + i);
}
template <typename T> void ReadArray(buffer& buf, T* t, int len)
{
	for (int i = 0; i < len; ++i)
		Read(buf, t + i);
}
template <typename T> vector<decay_t<T>> ReadArray(buffer& buf, int len)
{
	static_assert(is_constructible_v<decay_t<T>>,
		"ReadArray will call default contruction,but there is not one");
	vector<decay_t<T>> ret(len);
	for (int i = 0; i < len; ++i)
		Read(buf, (&ret[0] + i));
	return ret;
}
template <typename T>
requires(is_arithmetic_v<T> || is_enum_v<T>) void WriteSequence(buffer& buf, T* t, int len)
{
	buf.append(sizeof(T) * len, t);
}
template <typename T>
requires(is_arithmetic_v<T> || is_enum_v<T>) void ReadSequence(buffer& buf, T* t, int len)
{
	buf.drawback(sizeof(T) * len, t);
}

template <typename... Args> void MultiWrite(buffer& buf, Args&... ags)
{
	auto tp = make_tuple(&(ags)...);
	Write(buf, &tp);
}

template <typename T> inline buffer as_buffer(const T& t)
{
	buffer b;
	Write(b, &t);
	return b;
}
template <typename T> inline string as_string(const T& t) /**< serialize interface*/
{
	return as_buffer(t).universal_str();
}
template <typename T> inline T from_buffer(buffer& buf) /**< serialize interface*/
{
	return Read<T>(buf);
}
template <typename T> inline T from_string(string str) /**< serialize interface*/
{
	auto b = buffer(move(str));
	return from_buffer<T>(b);
}
/**
 * get parent dir as name indicating.
 * @note instead of filesystem::path::parent_path(),so \deprecated
 */
string GetParentDir(string& path);
/**
 * get thread id for format.
 */
DLL_INTERFACE_API
unsigned int Get_Thread_Id();
#endif	 // ASSISTANT_UTILITY_HEAD