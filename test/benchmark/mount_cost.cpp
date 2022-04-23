#include<bench_test_head.hpp>

class B {
	virtual int getI() {
		return 1;
	}
};
class D :public B {
public:
	int getI()override {

	}
};
