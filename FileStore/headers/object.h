/*****************************************************************/
/**
 * \file   object.h
 * \brief  define filestore relative objects, such as GHObject_t ReferedBlock, and WOPE
 *
 *//***************************************************************/
#pragma once
#include <assistant_utility.h>
#include <boost/functional/hash.hpp>
#include <fmt/format.h>
#include <string>
#include <vector>

#include <connection.h>
#define declare_default_cmp_operator(cls)                                                          \
	bool operator<(const cls&) const  = default;                                                   \
	bool operator==(const cls&) const = default;                                                   \
	bool operator!=(const cls&) const = default;

using std::string;
// class Object {
// public:
//	string name;
//	Object(const string& name = "") :name(name) {}
//	declare_default_cmp_operator(Object)
// };
//  the projection from a global object to a certain osd storage object is
//  pool->pg->obj
//  different pool mean or born to different deploy rule

// copied
#define CEPH_SNAPDIR ((__u64)(-1)) /* reserved for hidden .snap dir */
#define CEPH_NOSNAP ((__u64)(-2))  /* "head", "live" revision */
#define CEPH_MAXSNAP ((__u64)(-3)) /* largest valid snapid */

class Snapid_t {
public:
	// when this been created
	time_t time_stamp = 0;
	Snapid_t(time_t t = 0)
		: time_stamp(t)
	{
	}
	declare_default_cmp_operator(Snapid_t)
	// Snapid_t& operator+=(const Snapid_t& t) { this->time_stamp += t.time_stamp; return *this; }
	// Snapid_t operator+(const Snapid_t& t) { return Snapid_t(this->time_stamp +t.time_stamp ); }
	operator time_t()
	{
		return this->time_stamp;
	}
	auto GetES() const { return make_tuple(&time_stamp); }
};
//
class Object_t {
public:
	string name;
	Object_t(const string name = "")
		: name(name)
	{
	}
	declare_default_cmp_operator(Object_t) auto GetES() const { return make_tuple(&name); }
};
// hash object
//  pool is also a hash object
class HObject_t {
public:
	Object_t oid;
	Snapid_t snap;
	HObject_t(Object_t oid = Object_t(), Snapid_t snap = Snapid_t())
		: oid(oid)
	{
	}
	declare_default_cmp_operator(HObject_t)
		/*
		 *  don't wanna use these
		 */
		private :
		// genrated by Hash<string>()(oid.name)
		//[[maybe_unused]]
		uint32_t hash = 0;
	//[[maybe_unused]]
	bool max = false;

public:
	// ignoring
	static const int64_t POOL_META		 = -1;
	static const int64_t POOL_TEMP_START = -2;	 // and then negative
	int64_t				 pool			 = 0;
	// default counts of pg in a pool
	static constexpr int default_pg_numbers = 100;
	static bool			 is_temp_pool(int64_t pool) { return pool <= POOL_TEMP_START; }
	static int64_t		 get_temp_pool(int64_t pool) { return POOL_TEMP_START - pool; }
	static bool			 is_meta_pool(int64_t pool) { return pool == POOL_META; }
	auto				 GetES() const { return make_tuple(&oid, &snap, &hash, &max); }
};

using gen_t = int64_t;
struct shard_id_t {
	int8_t id;

	shard_id_t()
		: id(0)
	{
	}
	declare_default_cmp_operator(shard_id_t) explicit shard_id_t(int8_t _id)
		: id(_id)
	{
	}

	operator int8_t() const { return id; }

	static inline shard_id_t NO_SHARD() { return shard_id_t(0); };
	auto					 GetES() const { return make_tuple(&id); }
};

namespace boost {
//@follow difinition of shard_id_t
inline size_t hash_value(const shard_id_t& sh) { return boost::hash_value(sh.id); }
}
// generation hash object, or the global balah object
class GHObject_t {
public:
	HObject_t  hobj;
	gen_t	   generation;
	shard_id_t shard_id;
	//@new point out the owner of this obj
	string	   owner = "default-user";
	// these implement is needed
	GHObject_t(const HObject_t& hobj = HObject_t(), gen_t generation = 0,
		shard_id_t shard_id = shard_id_t::NO_SHARD())
		: hobj(hobj)
		, generation(generation)
		, shard_id(shard_id)
	{
	}
	//@Follow
	bool operator<(const GHObject_t& obj) const
	{
		return this->hobj.oid.name < obj.hobj.oid.name
			|| this->hobj.oid.name == obj.hobj.oid.name && this->generation < obj.generation;
	}
	bool operator==(const GHObject_t& obj) const
	{
		return this->hobj.oid.name == obj.hobj.oid.name && this->generation == obj.generation
			&& this->shard_id == obj.shard_id && this->owner == obj.owner;
	}
	bool operator!=(const GHObject_t& obj) const { return !operator==(obj); }

	// get literal description of object
	//@Follow:GHObject_t definition
	string GetLiteralDescription(GHObject_t const& ghobj)
	{
		return fmt::format("{}.{}", ghobj.owner, ghobj.hobj.oid.name);
	}
	auto GetES() const { return make_tuple(&hobj, &generation, &shard_id, &owner); }
	auto GetKey() { return GetES(); }
	auto GetAttr() { return make_tuple(); }
};

//@follow GHObject_t
struct HashForGHObject_t {
	size_t operator()(const GHObject_t& ghobj) const
	{
		size_t seed = 0;
		boost::hash_combine(seed, ghobj.owner);
		boost::hash_combine(seed, ghobj.generation);
		boost::hash_combine(seed, ghobj.shard_id.id);
		boost::hash_combine(seed, ghobj.hobj.pool);
		boost::hash_combine(seed, ghobj.hobj.snap.time_stamp);
		boost::hash_combine(seed, ghobj.hobj.oid.name);
		return seed;
	}
};
// Page Group
class PageGroup {
public:
	string	 name;
	uint64_t pool = HObject_t::POOL_META;
	PageGroup(const string& name = "", uint64_t _pool = 1)
		: name(name)
		, pool(_pool)
	{
	}
	PageGroup(const string&& name)
		: name(name)
	{
	}
	declare_default_cmp_operator(PageGroup) auto GetES() const { return make_tuple(&name, &pool); }
	auto GetKey() const { return GetES(); }
	auto GetAttr() const { return make_tuple(); }
};
//@follow definition of GHObject_t
// get the unique decription str for a ghobj
inline auto GetObjUniqueStrDesc(GHObject_t const& ghobj)
{
	auto str = fmt::format("{}{:0>}{}{}{:0>}{:0>}", ghobj.owner, ghobj.hobj.pool,
		ghobj.hobj.oid.name, ghobj.hobj.snap.time_stamp, ghobj.generation, ghobj.shard_id);
	return str;
}

using opeIdType = std::string;

//@new operation
class WOPE {
public:
	enum class opetype { Insert, Delete, OverWrite };
	vector<opetype> types;
	// for the block support
	vector<int>		block_nums;
	vector<string>	block_datas;
	GHObject_t		ghobj;
	//@new new ghobj of new version
	GHObject_t		new_ghobj;
	WOPE(GHObject_t gh, GHObject_t new_gh, vector<opetype> types, vector<int> block_nums,
		vector<string> block_datas)
		: types(types)
		, block_nums(block_nums)
		, block_datas(block_datas)
		, ghobj(gh)
		, new_ghobj(new_gh)
	{
	}
	// support serialize
	auto GetES() { return make_tuple(&types, &ghobj, &new_ghobj, &block_nums, &block_datas); }
};

class ROPE {
public:
	GHObject_t	ghobj;
	// serial numbers of blocks
	vector<int> blocks;
	ROPE(const GHObject_t& gh, const std::vector<int>& blocks)
		: ghobj(gh)
		, blocks(blocks)
	{
	}
	auto GetES() { return make_tuple(&ghobj, &blocks); }
};
// result of read ope
class ROPE_Result {
public:
	opeIdType	   opeId;
	GHObject_t	   ghobj;
	// serial numbers of blocks
	vector<int>	   blocks;
	vector<string> datas;
	ROPE_Result(opeIdType opeId = "", const GHObject_t& gh = {},
		const std::vector<int>& blocks = {}, const vector<string>& datas = {})
		: opeId(opeId)
		, ghobj(gh)
		, blocks(blocks)
		, datas(datas)
	{
	}
	ROPE_Result(const ROPE_Result&) = default;

	auto GetES() { return make_tuple(&opeId, &ghobj, &blocks, &datas); }
	auto GetKey() { return make_tuple(&opeId); }
	auto GetAttr() { return make_tuple(&ghobj, &blocks, &datas); }
};
//
// class WOpeLog {
// public:
//	opeIdType opeId;
//	WOPE wope;
//	InfoForNetNode from;
//	enum class wope_state_Type {
//		other, init,onJournal, onDisk
//	} wope_state;
//	WOpeLog(opeIdType opeId,WOPE wope,InfoForNetNode from,wope_state_Type wope_state):
//		opeId(opeId),wope(wope),from(from),wope_state(wope_state){ }
//	auto GetES() { return make_tuple(&opeId, &wope,&from, &wope_state); }
//	auto GetKey() { return make_tuple(&opeId); }
//	using Attr_Type = WOpeLog;
//};
//
// class ROpeLog {
// public:
//	opeIdType opeId;
//	ROPE rope;
//	InfoForNetNode from;
//
//	ROpeLog(opeIdType opeId, ROPE rope, InfoForNetNode from) :
//		opeId(opeId), rope(rope), from(from) {
//	}
//	auto GetES() { return make_tuple(&opeId, &rope, &from); }
//	auto GetKey() { return make_tuple(&opeId); }
//	using Attr_Type = ROpeLog;
//};

// table support
class WOPE_TABLE {
public:
	WOPE	  wope;
	opeIdType opeId;
	auto	  GetKey() { return make_tuple(&opeId); }
	auto	  GetAttr() { return make_tuple(&wope); }
};
class ROPE_TABLE {
public:
	ROPE	  rope;
	opeIdType opeId;
	auto	  GetKey() { return make_tuple(&opeId); }
	auto	  GetAttr() { return make_tuple(&rope); }
};
/**
 * @class for refered block.
 */
class ReferedBlock {
public:
	/** unique serial number in a range of local filestore
	 * @note this is not requrested to be unique or sychronous between each filestore, because each
	 * filestore remember GHObject_t which is reprented as ObjectWithRB which is contain neccessary
	 * info for sychronous between every filestore, that is ,compare and matching
	 * ReferedBlock::serial by GHObject_t's ObjectWithRB::serials_list
	 */
	int64_t serial;
	/**
	 * count of references of GHObject_t.
	 * @note when refer_count is 0, this object will be delete
	 */
	int32_t refer_count;
	ReferedBlock(int64_t serial = 0, int32_t refer_count = 0)
		: serial(serial)
		, refer_count(refer_count)
	{
	}
	static ReferedBlock getNewReferedBlock()
	{
		// this just create a increment serial for observe
		return ReferedBlock { chrono::system_clock::now().time_since_epoch().count(), 0 };
	}
		 operator int64_t() { return serial; }
	auto GetES() const { return make_tuple(&serial, &refer_count); }
	auto GetKey() const { return make_tuple(&serial); }
	auto GetAttr() const { return make_tuple(&refer_count); }
};

// storage data object represented by refered block
class ObjectWithRB {
public:
	list<int64_t> serials_list;
	GHObject_t	  orb;
	auto		  GetAttr() { return make_tuple(&serials_list); }
	auto		  GetKey() { return make_tuple(&orb); }
	auto		  GetES() { return make_tuple(&serials_list, &orb); }
};

// under root path, referedBlock managed as multi-demension array
// now assusm it's 4-demension,
// this only interset in referedBlock.serial