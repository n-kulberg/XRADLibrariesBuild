// file TestFunctionsArchiveUnused.cpp
// Здесь находится архив тестовых функций, которые не используются в данный момент,
// но должны быть под рукой.
//--------------------------------------------------------------
#include "pre.h"

#if 0
//--------------------------------------------------------------

// [1]
// Почему ADL имеет приоритет над функцией в "std namespace", но равна функции в пользовательском пространстве имен?
// http://qaru.site/questions/927033/why-does-adl-take-precedence-over-a-function-in-std-namespace-but-is-equal-to-function-in-user-defined-namespace

// [2]
// How to overload std::swap()
// https://stackoverflow.com/questions/11562/how-to-overload-stdswap

namespace x_std
{
	template <class T>
	void x_swap(T &a, T &b) = delete;
}

namespace x_my
{
	template <class T>
	struct A
	{
		// Здесь можно запретить (=delete) перемещающий operator=.
		A &operator=(const A &) { return *this; }
	};
}

namespace x_std
{
	template <class T>
	void x_swap(x_my::A<T> &a, x_my::A<T> &b) {}
}

void test_swap()
{
	using namespace x_std;
	x_my::A<int> a, b;
	x_swap(a, b);
	swap(a, b);
}

//--------------------------------------------------------------
#endif
