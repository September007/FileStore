#include<test_head.h>
#include<FileStore.h>
#include<Config.h>
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

}