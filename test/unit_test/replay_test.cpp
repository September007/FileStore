#include <FileStore.h>
#include <test_head.h>
#include <vector>
using namespace std;

string lines(int i) { return string(i, ' '); }

struct RePlay_Test {

	Context				 ctx;
	static constexpr int tries		  = 200;
	atomic_int			 log_done	  = 0;
	atomic_int			 flush_done	  = 0;
	atomic_int			 journal_done = 0;
	atomic_bool			 shut_down	  = false;
	vector<WOPE>		 wopes		  = create_test_WOPES(tries, 10, 4096);
	// states of wopes
	vector<vector<int>>	 states = vector<vector<int>> { tries, { false, false, false } };

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
before circle  {:10}   
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
			auto p			 = wope->new_ghobj.generation;
			states[p - 1][1] = 1;
		};
		ctx.replay_default_callback_when_flush_done = [&](shared_ptr<WOPE> wope) {
			flush_done++;
			auto p			 = wope->new_ghobj.generation;
			states[p - 1][2] = 1;
		};
	}

	// i mean the first wope should be submit
	void Run_Main()
	{
		FileStore fs;
		// print_states(60, "before start run filestore");
		fs.Mount(&ctx);
		// static int cnt = 0;
		// cnt++;
		// if (cnt % 2 == 0) {
		//	auto wopes = fs.GetOmap().GetMatchPrefix(ctx.wope_log_head);
		//	cout << "founded " << wopes.size() << " wopes in omap" << endl;
		// }
		auto p = fs.RePlay();
		cout << "reloaded " << p.size() << " wope" << endl;
		map<int, int> m;
		for (auto& wope : p)
			m[wope->new_ghobj.generation] = 1;
		for (int j = 0; j < wopes.size(); ++j) {
			if (states[j][0] || m[j + 1])
				continue;
			fs.Submit_wope(
				wopes[j],
				[j = j, ld = &log_done, sta = &states] {
					(*ld)++;
					(*sta)[j][0] = 1;
				},
				[j = j, ld = &journal_done, sta = &states] {
					(*ld)++;
					(*sta)[j][1] = true;
				},
				[j = j, ld = &flush_done, sta = &states] {
					(*ld)++;
					(*sta)[j][2] = true;
				},
				{ true, true, true });
		}

		while (unfinished() || shut_down)
			if (!shut_down)
				this_thread::sleep_for(chrono::milliseconds(100));
			else {
				shut_down = false;
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
try {
	RePlay_Test rt;
	rt.init();
	{

		FileStore fs;
		fs.Mount(&rt.ctx);
		{
			set<string> ss;
			for (auto& wope : rt.wopes) {
				auto p = ss.insert(fs.GetOpeId(wope));
				if (!p.second)
					p.second;
			}
		}
		// auto rds		= fs.GetOmap().GetMatchPrefix(fs.ctx->wope_log_head);
		// auto rds_replay = fs.RePlay();
		// for (auto& wope : rt.wopes)
		//	fs.GetOmap().Write_Meta(fs.GetOpeId(wope), as_string(wope));
	}
	while (rt.unfinished()) {
		static int tms = 0;
		tms++;
		rt.print_states(100, fmt::format("{:>2}", tms));
		{
			thread th(&RePlay_Test::Run_Main, &rt);
			this_thread::sleep_for(chrono::milliseconds(2000));
			rt.shut_down = true;
			th.join();
		}
	}
	rt.print_states(60, "after all");
} catch (std::exception& e) {
	cout << e.what() << endl;
}