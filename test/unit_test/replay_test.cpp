#include <FileStore.h>
#include <test_head.h>
#include <vector>
using namespace std;
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
string lines(int i) { return string(i, '*'); }

struct RePlay_Test {

	Context				 ctx;
	static constexpr int tries		  = 50;
	atomic_int			 log_done	  = 0;
	atomic_int			 flush_done	  = 0;
	atomic_int			 journal_done = 0;
	atomic_bool			 shut_down	  = false;
	vector<WOPE>		 wopes		  = create_test_WOPES(tries, 20, 4096);
	// states of wopes
	bool				 states[tries][3] = { 0 };

	string states_as_string(int i)
	{
		string ret(tries, '-');
		for (int j = 0; j < tries; ++j)
			if (states[j][i])
				ret[j] = '*';
		return ret;
	}
	void print_states(int i, string s)
	{
		auto   percs = [&](int ii) { return 100 * ii / tries; };
		string str	 = fmt::format(R"(
{}
------{:10}------
{:<10}{}
{:<10}{}
{:<10}{}
total:	submit:{}%	journal:{}%	flush:{}%
{})",
			  lines(i), s, "submit", states_as_string(0), "journal", states_as_string(1), "flush",
			  states_as_string(2), percs(log_done), percs(journal_done), percs(flush_done), lines(i));
		cout << str << endl;
	}
	void init()
	{
		ctx.load("");
		ctx.replay_default_callback_when_journal_done = [&](shared_ptr<WOPE> wope) {
			journal_done++;
			auto p		 = wope->new_ghobj.generation;
			states[p][1] = 1;
		};
		ctx.replay_default_callback_when_flush_done = [&](shared_ptr<WOPE> wope) {
			flush_done++;
			auto p		 = wope->new_ghobj.generation;
			states[p][2] = 1;
		};
	}

	// i mean the first wope should be submit
	void Run_Main()
	{
		FileStore fs;
		print_states(60, "before start run filestore");
		fs.Mount(&ctx);
		auto p = fs.RePlay();
		cout << "reloaded " << p.size() << "ope" << endl;
		map<int, int> m;
		for (auto& wope : p)
			m[wope->new_ghobj.generation] = 1;
		for (int j = 0; j < wopes.size(); ++j) {
			if (states[j][2] || m[j + 1])
				continue;
			fs.Submit_wope(
				wopes[j],
				[j = j, ld = &log_done, sta = states] {
					(*ld)++;
					sta[j][0] = true;
				},
				[j = j, ld = &journal_done, sta = states] {
					(*ld)++;
					sta[j][1] = true;
				},
				[j = j, ld = &flush_done, sta = states] {
					(*ld)++;
					sta[j][2] = true;
				},
				{ true, true, true });
		}

		while (unfinished() || shut_down)
			if (!shut_down)
				this_thread::sleep_for(chrono::milliseconds(100));
			else {
				shut_down = false;
				FlushOptions fo;
				fo.allow_write_stall = true;

				auto db = fs.GetOmap().GetDB();
				db->Flush(fo);
				break;
			}
	}
	bool unfinished()
	{
		int cur = 0;
		for (int i = 0; i < tries; ++i)
			cur += states[i][2];
		return cur < tries;
	}
};
TEST(TMP2, replay)
{
	RePlay_Test rt;
	rt.init();
	while (rt.unfinished()) {
		thread th(&RePlay_Test::Run_Main, &rt);
		this_thread::sleep_for(chrono::milliseconds(2000));
		rt.shut_down = true;
		th.join();
	}
	rt.print_states(60, "after all");
}
