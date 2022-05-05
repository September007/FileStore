#include <FileStore.h>
#include <config.h>
#include <test_head.h>
#define head TMP2
#include <csignal>
// TEST(TMP2, basic)
//{
//	Context ctx;
//	// default context
//	ctx.load("");
//
//	FileStore fs;
//	fs.Mount(&ctx);
//	GHObject_t gh;
//	gh.hobj.oid.name = "test_obj";
//	auto new_gh		 = gh;
//	new_gh.generation++;
//
//	WOPE	   wope(gh, new_gh, vector<WOPE::opetype> { 4, WOPE::opetype::Insert }, { 0, 1, 2, 3 },
//			  { "aaa", "bbb", "ccc", "ddd" });
//	atomic_int cnt = 0;
//	auto	   cb  = [&] { cnt++; };
//	fs.Submit_wope(wope, cb, cb, cb);
//	while (cnt < 3)
//		;
//	cnt = 0;
//	ROPE_Result result;
//	fs.Submit_rope(ROPE(new_gh, { 0, 1, 2, 3 }), result, cb);
//	while (cnt < 1)
//		;
//	EXPECT_EQ(wope.block_datas, result.datas);
// }
// TEST(TMP2, omap)
//{
//	ObjectMap omap;
//	Context	  ctx;
//	ctx.load("");
//	omap.Mount(&ctx);
//	omap.Write_Meta<int, string>(1, "str1");
//	auto r11 = omap.Read_Meta<int, string>(1);
//
//	omap.Write_Meta<int, string>(1, "str1");
//	omap.Write_Meta<int, string>(2, "str2");
//	omap.Write_Meta<int, string>(3, "str3");
//	auto r1 = omap.Read_Meta<int, string>(1);
//	auto r2 = omap.Read_Meta<int, string>(2);
//	auto r3 = omap.Read_Meta<int, string>(3);
//	EXPECT_EQ(r1, "str1");
//	EXPECT_EQ(r2, "str2");
//	EXPECT_EQ(r3, "str3");
//	omap.Erase_Meta<int>(1);
//	omap.Erase_Meta<int>(2);
//	omap.Erase_Meta<int>(3);
//	r1 = omap.Read_Meta<int, string>(1);
//	r2 = omap.Read_Meta<int, string>(2);
//	r3 = omap.Read_Meta<int, string>(3);
//	EXPECT_EQ(r1, "");
//	EXPECT_EQ(r2, "");
//	EXPECT_EQ(r3, "");
// }
TEST(TMP2, eraseMathingPRefix)
{
	ObjectMap omap;
	Context	  ctx;
	ctx.load("");
	omap.Mount(&ctx);

	map<string, map<string, string>> mm;
	mm["01::"]["1"] = "11";
	mm["01::"]["2"] = "12";
	mm["01::"]["3"] = "13";

	mm["02::"]["1"] = "21";
	mm["02::"]["2"] = "22";
	mm["02::"]["3"] = "23";
	for (auto& m : mm)
		for (auto& p : m.second)
			omap.Write_Meta(m.first + p.first, p.second);
	for (auto& m : mm)
		for (auto& p : m.second) {
			string rd;
			auto   k = m.first + p.first;
			omap.Read_Meta(k, rd);
			EXPECT_EQ(rd, p.second);
		}
	this_thread::sleep_for(chrono::milliseconds(100));
	auto p = omap.GetMatchPrefix("01::");
	omap.EraseMatchPrefix("01::");
	for (auto& p : mm["01::"]) {
		string rd;
		omap.Read_Meta("01::" + p.first, rd);
		EXPECT_EQ(rd, "");
	}
}