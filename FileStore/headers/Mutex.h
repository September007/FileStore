/*****************************************************************/
/** \file   Mutex.h
 * \brief   define method GetMutex to seek for named mutex
 * \note	this is working on uncertain requirement for mutex. if somewhere use this already verify
 * the need of mutex,then this maybe need to be replaced by using class member(or function-static
 * mutex)
 * \deprecated almost useless
 *
 * \author 28688
 * \date   May 2022
 *//***************************************************************/
#pragma once
#ifndef GD_MUTEX_HEAD
#define GD_MUTEX_HEAD
#include <map>
#include <mutex>
using std::lock_guard;
using std::mutex;
enum class mutex_enum {
	// for x
	header_lock,
};
/**
 * for named or binded mutex, this is more freer.
 */
template <typename... ArgsType> std::mutex& GetMutex(ArgsType... args)
{
	using keyT						 = std::tuple<ArgsType...>;
	auto						 key = make_tuple<ArgsType...>(std::forward<ArgsType>(args)...);
	static std::map<keyT, mutex> m;
	static mutex				 m_mutex;
	lock_guard					 lg(m_mutex);
	return m[key];
}

#endif	 // GD_MUTEX_HEAD