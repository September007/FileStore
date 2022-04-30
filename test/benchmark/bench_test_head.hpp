#pragma once
#include <FileStore.h>
#include <benchmark/benchmark.h>
#include <chrono>
using namespace benchmark;

inline void remove_ctx_dir(Context& ctx)
{
	filesystem::remove_all(ctx.fsPath);
	filesystem::remove_all(ctx.journalPath);
	filesystem::remove_all(ctx.kvPath);
	filesystem::remove_all(ctx.rbPath);
}
inline void Run_FileStore_Write(Context ctx, const vector<WOPE>& wopes)
{
	FileStore fs;
	fs.Mount(&ctx);

	atomic_int cnt	  = 0;
	int		   expect = wopes.size() * 3;
	auto	   inc	  = [&] { ++cnt; };
	for (auto& ope : wopes)
		fs.Submit_wope(ope, inc, inc, inc);
	while (cnt < expect)
		this_thread::sleep_for(chrono::milliseconds(10));
}
inline vector<ROPE_Result> Run_FileStore_Read(Context ctx, const vector<ROPE>& ropes)
{
	vector<ROPE_Result> ret(ropes.size());
	FileStore			fs;
	fs.Mount(&ctx);

	atomic_int cnt	  = 0;
	int		   expect = ropes.size();
	auto	   inc	  = [&] { ++cnt; };
	for (int i = 0; i < ropes.size(); ++i)
		fs.Submit_rope(ropes[i], ret[i], inc);
	while (cnt < expect)
		this_thread::sleep_for(chrono::milliseconds(10));
	return ret;
}

inline vector<WOPE> create_test_WOPES(
	int sz, const unsigned int block_cnt = 4, int block_data_length = 3)
{
	GHObject_t gh;
	gh.hobj.oid.name = "test_obj";
	auto new_gh		 = gh;
	new_gh.generation++;
	WOPE wope(gh, new_gh, vector<WOPE::opetype> { block_cnt, WOPE::opetype::Insert }, {}, {});
	wope.block_datas.reserve(block_cnt);
	for (int i = 0; i < int(block_cnt); ++i) {
		wope.block_nums.push_back(i);
		wope.block_datas.push_back(string(block_data_length, '0' + i));
	}
	vector<WOPE> wopes(sz, wope);

	for (int i = 0; i < sz; ++i)
		wopes[i].new_ghobj.generation = i + 1;
	return move(wopes);
}