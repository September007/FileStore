#include<test_head.h>
#include<FileStore.h>
#include<config.h>
#define head TMP
TEST(head, config) {
	EXPECT_EQ(GetConfig("test", "val1").get<string>(), "integrated.default-1");
	EXPECT_EQ(GetConfig("test", "val2").get<string>(), "integrated-2");
	EXPECT_EQ(GetConfig("test", "val3").get<string>(), "test.default-3");
	EXPECT_EQ(GetConfig("test", "val4").get<string>(), "test-4");
	EXPECT_EQ(GetConfig("test", "val5").get<string>(), "test-5");
	EXPECT_EQ(GetConfig("test", "val6").empty(), true);
}
TEST(head,use_lib) {
	vector<int> vi;

	auto tries = 10000, ts = 50;
	auto p = [&] {
		for (int i = 0; i < tries; ++i) {
			vi.push_back(i);
			vi.push_back(i);
			vi.pop_back();
		}
	};
	vector<thread> tts;
	for (int i = 0; i < ts; ++i)
		tts.emplace_back(p);
	for (auto& t : tts)
		t.join();
	EXPECT_EQ(vi.size(), 50'0000);
}