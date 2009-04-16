/*
This unit test doesn't test thread safety. It only tests whether the
locking_shared_ptr compiles.
*/

//include
#include <locking_shared_ptr.hpp>

class test
{
public:
	void f(){}
};

int main()
{
	locking_shared_ptr<test> x(new test());
	x->f();
}
