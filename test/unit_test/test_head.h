#pragma once
#define GTEST_CATCH_EXCEPTIONS 0
#include <gtest/gtest.h>
#include <object.h>

inline vector<WOPE> create_test_WOPES(
	int sz, const unsigned int block_cnt = 4, int block_data_length = 3)
{
	GHObject_t gh;
	gh.hobj.oid.name = "test_obj";
	auto new_gh		 = gh;
	WOPE wope(gh, new_gh, vector<WOPE::opetype> { block_cnt, WOPE::opetype::Insert }, {}, {});
	wope.block_datas.reserve(block_cnt);
	for (int i = 0; i < int(block_cnt); ++i) {
		wope.block_nums.push_back(i);
		wope.block_datas.push_back(string(block_data_length, '0' + i));
	}
	vector<WOPE> wopes(sz, wope);

	for (int i = 0; i < sz; ++i)
		wopes[i].new_ghobj.generation = gh.generation + i + 1;

	for (int i = 0; i < sz; ++i)
		wopes[i].new_ghobj.generation = i + 1;
	return move(wopes);
}
