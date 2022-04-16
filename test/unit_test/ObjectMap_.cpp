#include<test_head.h>
#include<ObjectMap.h>
#include<referedBlock.h>
#define head objectMap
auto root=filesystem::absolute("./unit_test/objectMap").string();
class wulala {
public:
	string s = "dasd";
	int i = 0;
	wulala(string s, int i) :s(s), i(i) {}
	bool operator==(const wulala&)const = default;
	auto GetKey()const { return make_tuple(&i); }
	auto GetAttr()const { return make_tuple(&s); }
};
TEST(head, base) {
	Context ct;
	ct.kvpath = root;
	ObjectMap omap;
	omap.Mount(&ct);

	wulala ws[] = { {"abc", 1},
		{"abc1",2},
		{"abc2",3},
		{"abc3",3}
	};

	wulala rws[] = { {"abc", 1},
		{"abc1",2},
		{"abc3",3},
		{"abc3",3}
	};

	for (auto& w : ws)
		omap.Write_Meta<wulala>(w);
	for (int i = 0; i < 4; ++i) {
		auto r0 = omap.Read_Meta<wulala>(ws[i]);
		auto r1 = omap.Read_Meta<wulala>(GetKey<wulala>(ws[i]));
		EXPECT_EQ(r0, rws[i]);
		EXPECT_EQ(r1, rws[i]);
	}
	ObjectWithRB orb;
	auto x= GetAttr(orb);
}