#include <bench_test_head.hpp>

auto wopes = create_test_WOPES(100);
void rb_deep_2(State& state)
{
	Context ctx;
	ctx.load("");

	ctx.m_GetReferedBlockStoragePath = ::GetReferedBlockStoragePath_deep2;

	for (auto _ : state)
		Run_FileStore_Write(ctx, wopes);
}

void rb_deep_4(State& state)
{
	Context ctx;
	ctx.load("");

	ctx.m_GetReferedBlockStoragePath = ::GetReferedBlockStoragePath_deep4;

	for (auto _ : state)
		Run_FileStore_Write(ctx, wopes);
}
// cross test, seems like deep2 is good
BENCHMARK(rb_deep_2);
BENCHMARK(rb_deep_4);
BENCHMARK(rb_deep_2);
BENCHMARK(rb_deep_4);
BENCHMARK(rb_deep_2);
BENCHMARK(rb_deep_4);
BENCHMARK(rb_deep_2);
BENCHMARK(rb_deep_4);
BENCHMARK(rb_deep_2);
BENCHMARK(rb_deep_4);
BENCHMARK(rb_deep_2);
BENCHMARK(rb_deep_4);
BENCHMARK(rb_deep_2);
BENCHMARK(rb_deep_4);
BENCHMARK(rb_deep_2);
BENCHMARK(rb_deep_4);

BENCHMARK_MAIN();