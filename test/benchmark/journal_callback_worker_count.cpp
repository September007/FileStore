#include <bench_test_head.hpp>
#include <benchmark\benchmark.h>
using namespace benchmark;
#define head journal_work_thread

constexpr int ope_tries = 100;
void		  head(State& state)
{
	int		work_thread_count = state.range(0);
	Context ctx;
	// load deault
	ctx.load("");
	ctx.journal_callback_worker_count = work_thread_count;

	GHObject_t gh;
	gh.hobj.oid.name = "test_obj";
	auto new_gh		 = gh;
	new_gh.generation++;
	WOPE wope(gh, new_gh, vector<WOPE::opetype> { 4, WOPE::opetype::Insert }, { 0, 1, 2, 3 },
		{ "aaa", "bbb", "ccc", "ddd" });
	vector<WOPE> wopes(ope_tries, wope);

	for (int i = 0; i < ope_tries; ++i)
		wopes[i].new_ghobj.generation = i + 1;
	for (auto _ : state) {
		Run_FileStore_Write(ctx, wopes);
	}
}

BENCHMARK(head)->DenseRange(1, 30, 1);

BENCHMARK_MAIN();