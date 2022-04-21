#include<test_head.h>
#include<FileStore.h>
#include<config.h>
#define head TMP2

TEST(head, basic) {
	Context ctx;
	//default context
	ctx.load("");

	FileStore fs;
	fs.Mount(&ctx);
	GHObject_t gh;
	gh.hobj.oid.name = "test_obj";
	auto new_gh = gh;	new_gh.generation++;

	WOPE wope(gh, new_gh, vector<WOPE::opetype>{4, WOPE::opetype::Insert}, { 0,1,2,3 }, { "aaa","bbb","ccc","ddd" });
	atomic_int cnt = 0;
	auto cb = [&] {
		cnt++;
	};
	fs.Submit_wope(wope, cb, cb, cb);
	while (cnt < 3);
	cnt = 0;
	ROPE_Result result;
	fs.Submit_rope(ROPE(new_gh, { 0,1,2,3 }), result,cb);
	while (cnt < 1);
	EXPECT_EQ(wope.block_datas, result.datas);
}