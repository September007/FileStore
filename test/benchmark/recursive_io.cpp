#include <bench_test_head.hpp>
#include <limits>
#pragma warning(disable : 4244)
constexpr int ope_cnt = 100;
void		  normal_io(State& state)
{
	Context ctx;
	ctx.load("");
	ctx.journal_write_worker_count = state.range(0);

	auto wopes = create_test_WOPES(ope_cnt);

	for (auto _ : state) {
		Run_FileStore_Write(ctx, wopes);
	}
	// remove_ctx_dir(ctx);
}

void keep_tring_io(State& state)
{
	Context ctx;
	ctx.load("");
	ctx.journal_write_worker_count = state.range(0);

	ctx.m_ReadFile = [](const string& path) -> string {
		return keep_ReadFile(path, std::numeric_limits<int>::max());
	};
	ctx.m_WriteFile = keep_WriteFile;

	auto wopes = create_test_WOPES(ope_cnt);

	for (auto _ : state) {
		Run_FileStore_Write(ctx, wopes);
	}
	// remove_ctx_dir(ctx);
}

BENCHMARK(normal_io)->DenseRange(1, 4, 1);
BENCHMARK(keep_tring_io)->DenseRange(1, 4, 1);

BENCHMARK_MAIN();
