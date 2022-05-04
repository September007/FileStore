#include <FileStore.h>
#include <config.h>
#include <test_head.h>
#define head TMP2

TEST(TMP2, basic)
{
	Context ctx;
	// default context
	ctx.load("");

	FileStore fs;
	fs.Mount(&ctx);
	GHObject_t gh;
	gh.hobj.oid.name = "test_obj";
	auto new_gh		 = gh;
	new_gh.generation++;

	WOPE	   wope(gh, new_gh, vector<WOPE::opetype> { 4, WOPE::opetype::Insert }, { 0, 1, 2, 3 },
			  { "aaa", "bbb", "ccc", "ddd" });
	atomic_int cnt = 0;
	auto	   cb  = [&] { cnt++; };
	fs.Submit_wope(wope, cb, cb, cb);
	while (cnt < 3)
		;
	cnt = 0;
	ROPE_Result result;
	fs.Submit_rope(ROPE(new_gh, { 0, 1, 2, 3 }), result, cb);
	while (cnt < 1)
		;
	EXPECT_EQ(wope.block_datas, result.datas);
}
TEST(TMP2, omap)
{
	ObjectMap omap;
	Context	  ctx;
	ctx.load("");
	omap.Mount(&ctx);
	omap.Write_Meta<int, string>(1, "str1");
	auto r11 = omap.Read_Meta<int, string>(1);

	omap.Write_Meta<int, string>(1, "str1");
	omap.Write_Meta<int, string>(2, "str2");
	omap.Write_Meta<int, string>(3, "str3");
	auto r1 = omap.Read_Meta<int, string>(1);
	auto r2 = omap.Read_Meta<int, string>(2);
	auto r3 = omap.Read_Meta<int, string>(3);
	EXPECT_EQ(r1, "str1");
	EXPECT_EQ(r2, "str2");
	EXPECT_EQ(r3, "str3");
	omap.Erase_Meta<int>(1);
	omap.Erase_Meta<int>(2);
	omap.Erase_Meta<int>(3);
	r1 = omap.Read_Meta<int, string>(1);
	r2 = omap.Read_Meta<int, string>(2);
	r3 = omap.Read_Meta<int, string>(3);
	EXPECT_EQ(r1, "");
	EXPECT_EQ(r2, "");
	EXPECT_EQ(r3, "");
}