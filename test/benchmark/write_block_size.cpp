#include <bench_test_head.hpp>

#define ope_cnt 50

void write50_obj_of_block__cnt_len(State& state)
{
	int		block_cnt = state.range(0);
	int		len		  = state.range(1);
	Context ctx;
	ctx.load("");

	auto wopes = create_test_WOPES(ope_cnt, block_cnt, len);

	for (auto _ : state)
		Run_FileStore_Write(ctx, wopes);
}
BENCHMARK(write50_obj_of_block__cnt_len)
	->ArgsProduct({ { 1, 2, 3, 4, 5 }, { 1, 1024, 1024 * 1024, 4194304 } });
// clang-format off
// special test for block_length as 4M+1
BENCHMARK(write50_obj_of_block__cnt_len)
    ->Args({5, 4194305});
// clang-format on
