#include <benchmark\benchmark.h>
using namespace benchmark;
void t(State& state)
{
	int i = 1;
	for (auto _ : state) {
		i *= 3;
	}
}
void t1(State& state)
{
	int i = 1;
	for (auto _ : state) {
		i *= 2;
	}
}
BENCHMARK(t);
BENCHMARK(t1);

int main(int argc, char** argv)
{

	::benchmark::Initialize(&argc, argv);
	if (::benchmark::ReportUnrecognizedArguments(argc, argv))
		return 1;
	::benchmark::RunSpecifiedBenchmarks();
	::benchmark::Shutdown();
	return 0;
}