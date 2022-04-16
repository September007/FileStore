#pragma once 
#include<assistant_utility.h>
template<typename T>
auto GetKey(T& t) {
	return t.GetKey();
}
/**
* separate key and attr, this is access to attr
*/
template<typename T>
auto GetAttr(T& t) {
	if constexpr (requires(T _t) { _t.GetAttr(); }) {
		return t.GetAttr();
	}
	else if constexpr(requires(T _t) { _t.GetES(); }) {
		return t.GetES();
	}
	else if constexpr (is_same_v<T, T>) {
		//static_assert(!is_same_v<T, T>, "type have not attr access,check imple");
	}
}
template<typename T>
concept TableType = requires(T & t) {
	//check if it have key_type
	GetKey(t);
	//check if it have attr access
	GetAttr(t);
};

/**
* assemble key and attr together as TableType
*/
template<TableType T>
decay_t<T> GetAssemble( buffer& key,  buffer& attr) {
	using RT = decay_t<T>;
	uint8_t b[sizeof(RT)];
	RT* rt = reinterpret_cast<RT*>(b);
	auto attr_ptr = GetAttr(*rt);
	auto key_ptr = GetKey(*rt);
	Read(attr, &attr_ptr);
	Read(key, &key_ptr);
	return move(*rt);
}
template<TableType T>
using TableKeyType = decltype(GetKey(declval<T&>()));

template<TableType T>
using TableAttrType = decltype(GetAttr(declval<T&>()));


