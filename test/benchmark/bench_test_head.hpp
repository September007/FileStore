#pragma once
#include<FileStore.h>

inline void Run_FileStore_Write(Context ctx, const vector<WOPE>& wopes) {
	FileStore fs;
	fs.Mount(&ctx);

	atomic_int cnt = 0;
	int expect = wopes.size() * 3;
	auto inc = [&] {
		++cnt;
	};
	for (auto& ope : wopes)
		fs.Submit_wope(ope, inc, inc, inc);
	while (cnt < expect);
}
inline vector<ROPE_Result> Run_FileStore_Read(Context ctx, const vector<ROPE>& ropes) {
	vector<ROPE_Result> ret(ropes.size());
	FileStore fs;
	fs.Mount(&ctx);

	atomic_int cnt = 0;
	int expect = ropes.size();
	auto inc = [&] {
		++cnt;
	};
	for (int i = 0; i < ropes.size(); ++i)
		fs.Submit_rope(ropes[i], ret[i], inc);
	while (cnt < expect);
	return ret;
}