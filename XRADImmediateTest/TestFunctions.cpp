#include "pre.h"

#include "TestFunctions.h"
#include "TestFunctionsArchive.h"

#include <XRADBasic/Sources/Utils/ValuePredicates.h>
#include <thread>

#ifdef XRAD_COMPILER_MSC
#pragma warning(disable:4189) // Local variable is initialized but not referenced
#endif // XRAD_COMPILER_MSC

XRAD_USING

//--------------------------------------------------------------

namespace
{

//--------------------------------------------------------------

void	PredicateTest()
{
	auto p1 = predicate::equals(2.);
	auto p2 = predicate::greater(3.);
	auto p3 = p1 | p2;
	auto p4 = !p3;
	p3(0);//false
	p3(2);//true
	p3(4);//true

	p4(0);//true
	p4(2);//false
	p4(4);//false


	p4(2.7);//true
	p4(2.3);//true

	p4 &= predicate::greater(2.5);
//	p4 &= predicate::less(2.5);

	p4(2.7);//true
	p4(2.3);//false
}

//--------------------------------------------------------------

namespace
{

#ifdef XRAD_COMPILER_MSC
#pragma optimize ("", off)
void RaiseNullPointerReference()
{
	*(int*)nullptr = 0;
}
void RaiseInvalidPointerReference()
{
	int i = *(int*)-0x123;
}
void RaiseDivisionByZero()
{
	int a = 0;
	int b = 1/a;
}
void RaiseFloatingPointError()
{
	double a = 0;
	double b = 1./a;
}
#pragma optimize ("", on)

#elif defined(XRAD_COMPILER_GNUC)

void __attribute__((optimize("O0"))) RaiseNullPointerReference()
{
	*(int*)nullptr = 0;
}
void __attribute__((optimize("O0"))) RaiseInvalidPointerReference()
{
	int i = *(int*)-0x123;
}
void __attribute__((optimize("O0"))) RaiseDivisionByZero()
{
	int a = 0;
	int b = 1/a;
}
void __attribute__((optimize("O0"))) RaiseFloatingPointError()
{
	double a = 0;
	double b = 1./a;
}

#else
#error This code is nonportable in case of optimization.
#endif

} // namespace

#if defined(XRAD_COMPILER_MSC)
#pragma float_control(push)
#pragma float_control(precise, on)
#pragma fenv_access(on)
#pragma float_control(except, on)
#elif defined(XRAD_COMPILER_GNUC)
#pragma STDC FENV_ACCESS ON
#else
#error Unknown compiler (see #pragma STDC FENV_ACCESS ON)
#endif

void ExceptionsTest()
{
	ThreadSetup ts; (void)ts;
	for (;;)
	{
		auto f = GetButtonDecision(L"Select exception type to throw",
				{
					MakeButton(L"runtime_error", make_fn([]()
							{ throw runtime_error("Test runtime_error."); })),
					MakeButton(L"canceled_operation", make_fn([]()
							{ throw canceled_operation("Test canceled_operation."); })),
					MakeButton(L"quit_application", make_fn([]()
							{ throw quit_application("Test quit_application.", 0); })),
					MakeButton(L"int", make_fn([]()
							{ throw int(7); })),
					MakeButton(L"Null pointer reference (OS exception)", make_fn([]()
							{ RaiseNullPointerReference(); })),
					MakeButton(L"Invalid pointer reference (OS exception)", make_fn([]()
							{ RaiseInvalidPointerReference(); })),
					MakeButton(L"Division by zero (OS exception)", make_fn([]()
							{ RaiseDivisionByZero(); })),
					MakeButton(L"Floating point error (divide by zero, possible OS exception)", make_fn([]()
							{
								std::feclearexcept(FE_ALL_EXCEPT);
								RaiseFloatingPointError();
								int fe = std::fetestexcept(FE_ALL_EXCEPT);
								string fe_legend =
										ssprintf("FE_DIVBYZERO=0x%X\n", (int)FE_DIVBYZERO) +
										ssprintf("FE_INEXACT=0x%X\n", (int)FE_INEXACT) +
										ssprintf("FE_INVALID=0x%X\n", (int)FE_INVALID) +
										ssprintf("FE_OVERFLOW=0x%X\n", (int)FE_OVERFLOW) +
										ssprintf("FE_UNDERFLOW=0x%X\n", (int)FE_UNDERFLOW);
								ShowString("FP error", ssprintf("fe=0x%X", fe) + "\n" + fe_legend);
							})),
					MakeButton(L"Floating point round (overflow)", make_fn([]()
							{
								int result = 0; try_iround_n(1e100, &result); // Здесь не должно быть исключения.
								string fe_legend =
										ssprintf("FE_DIVBYZERO=0x%X\n", (int)FE_DIVBYZERO) +
										ssprintf("FE_INEXACT=0x%X\n", (int)FE_INEXACT) +
										ssprintf("FE_INVALID=0x%X\n", (int)FE_INVALID) +
										ssprintf("FE_OVERFLOW=0x%X\n", (int)FE_OVERFLOW) +
										ssprintf("FE_UNDERFLOW=0x%X\n", (int)FE_UNDERFLOW);
								{
									std::feclearexcept(FE_ALL_EXCEPT);
									long long i_value = std::llrint(1e100); // Здесь не должно быть исключения.
									int fe = std::fetestexcept(FE_ALL_EXCEPT);
									// В релизной сборке важно использовать значение i_value.
									ShowString("E(std::llrint(1e100))", ssprintf("fe=0x%X, i=%lli", fe, i_value) +
											"\n" + fe_legend);
								}
								try
								{
									std::feclearexcept(FE_ALL_EXCEPT);
									double d = 1e100;
									long long i = d; // Здесь может быть исключение, зависит от настроек.
									int fe = std::fetestexcept(FE_ALL_EXCEPT);
									// В релизной сборке важно использовать значение i,
									// иначе код i=d может быть выкинут.
									ShowString("E(i = d)", ssprintf("fe=0x%X, i=%lli", fe, i) +
											"\n" + fe_legend);
								}
								catch (...)
								{
									string message = GetExceptionStringOrRethrow();
									ShowString("Exception in E(i = d)", message);
								}
							})),
					MakeButton(L"Inter-thread exceptions", make_fn([]()
							{
								std::exception_ptr ep;
								std::thread t([&ep]()
										{
											ThreadSetup ts; (void)ts;
											try
											{
												RaiseDivisionByZero();
											}
											catch (...)
											{
												ep = std::current_exception();
											}
										});
								t.join();
								if (ep)
								{
									std::rethrow_exception(ep);
								}
							})),
					MakeButton(L"XRAD_ASSERT_THROW", make_fn([]()
							{
								XRAD_ASSERT_THROW(2 + 2 == 5);
							})),
					MakeButton(L"XRAD_ASSERT_THROW_EX", make_fn([]()
							{
								XRAD_ASSERT_THROW_EX(2 + 2 == 5, logic_error);
							})),
					MakeButton(L"XRAD_ASSERT_THROW_M", make_fn([]()
							{
								XRAD_ASSERT_THROW_M(2 + 2 == 5, logic_error, "Learn math!");
							})),
					MakeButton(L"Nested exception (runtime_error)", make_fn([]()
							{
								try
								{
									throw std::runtime_error("Runtime error.");
								}
								catch (...)
								{
									std::throw_with_nested(std::runtime_error(
											"Error in the Nested exception (runtime_error)."));
								}
							})),
					MakeButton(L"Nested exception (canceled_operation)", make_fn([]()
							{
								try
								{
									throw canceled_operation("Canceled.");
								}
								catch (...)
								{
									std::throw_with_nested(std::runtime_error(
											"Error in the Nested exception (canceled_operation)."));
								}
							})),
					MakeButton(L"Cancel", function<void ()>()),
				});
		if (!f)
			break;
		try
		{
			f();
		}
		catch (...)
		{
			try
			{
				string message = GetExceptionStringOrRethrow();
				ShowString("GetExceptionStringOrRethrow result", message);
			}
			catch (...)
			{
				string message = GetExceptionString();
				ShowString("GetExceptionString result", message);
			}
		}
	}
}

#if defined(XRAD_COMPILER_MSC)
#pragma float_control(pop)
#elif defined(XRAD_COMPILER_GNUC)
#pragma STDC FENV_ACCESS OFF
#else
#error Unknown compiler (see #pragma STDC FENV_ACCESS ...)
#endif

//--------------------------------------------------------------

void	MemoryLeakTest()
{

	size_t N = 31415926;
	char *leak = new char[N];
}

//--------------------------------------------------------------

void	VectorTest()
{
	int	t;
	int	&u(t), &v(t);
	point2_F64 a(10);
	point2_F64 b(a);
	point2_F64 c(2, 1);
	point3_F64 d, e(0), f(1, 2, 3);
	point3_F64 g(f);

	range2_F64 r(a, b);
	range3_F64 r3(d, e);
	range3_F64 s3(0, 0, 0, 1, 1, 1);

	ShowVector("b", b);
	ShowVector("b", c);

	a=b+c;
	ShowVector("b+c", a);
	a=b-c;
	ShowVector("b-c", a);
	a+=b;
	ShowVector("a+=b", a);
	a-=b;
	ShowVector("a-=b", a);
	a*=2;
	ShowVector("a*=2", a);
	a/=3;
	ShowVector("a/=3", a);

	c*=2;
	c/=2;

	c=a/2;
	c=a*2;

	e=-f;
	double x = sp(a,b);
	if(a==c){
		b=c;
	}
	if(a!=c){
		a=b;
	}

//TODO два следующих действия неуместны для FieldElement. Предварительно решено перенести их в алгебру
	//a+=d; // Compile error: size mismatch
	e+=1;//done!
};

//--------------------------------------------------------------

void	template_test()
{
//	LinearVector<double, double>	g(100,1);
//	MathFunction<double, double>	g(100,1);
	RealFunctionF32	fr(100, 1);
	ComplexFunctionF32	fc(100, complexF32(1));
	ComplexFunctionF32	fc1(100, complexF32(1));
	double	nr = 100.;
	complexF64 nc(100);

	fc += fc1 + fc1;
	fc -= fc1 - fc1;
	fc *= fc1 * fc1;
	fc /= fc1 / fc1;
	fc %= fc1 % fc1;

	fc += nr;
	fc = fc + nr;
	fc -= nr;
	fc = fc - nr;
	fc *= nr;
	fc = fc * nr;
	fc /= nr;
	fc = fc / nr;
	//g %= nr; // nr -- действительное число, запрещено

	fc += nc;
	fc = fc + nc;
	fc -= nc;
	fc = fc - nc;
	fc *= nc;
	fc = fc * nc;
	fc /= nc;
	fc = fc / nc;
	fc %= nc;

	fc = ~fc1;
	fc.conjugate();

	fc.add(fc, fc1);
	fc.subtract(fc, fc1);
	fc.multiply(fc, fc1);
	fc.divide(fc, fc1);
	fc.add_multiply(fc, fc1);
	fc.add_divide(fc, fc1);
	fc.subtract_divide(fc, fc1);
	fc.subtract_multiply(fc, fc1);
	fc.mix(fc, fc1, 0.1, 0.5);
	fc.multiply_conj(fc, fc1);
	//g.multiply_conj(g, nr); // умножение на сопряженное действительное число равно обычному умножению, запрещено
	fc.multiply_conj(fc, nc);
}

//--------------------------------------------------------------

template <class T>
void TTestHelper();

class MyClass
{
	public:
		MyClass(double /*d*/) {}
};

using double_ = MyClass;

template <>
void TTestHelper<double_>()
{
}

template <>
void TTestHelper<double_&>()
{
}

template <>
void TTestHelper<const double_&>()
{
}

template <>
void TTestHelper<double_&&>()
{
}

template <class T>
void TTest(T &&t)
{
	(void)t;//to avoid C4100 in MSVC 2015
	TTestHelper<decltype(std::forward<T>(t))>();
}

void TestCpp()
{
	TTest(double_(1.));
	double_ d = 1;
	TTest(d);
	const double_ dc = 1;
	TTest(dc);
	TTest(double_(fabs(1.)));
	int breakpoint = 0;
}

//--------------------------------------------------------------

struct TaggedValue
{
	public:
		int value;
		int tag;
		constexpr TaggedValue(int v, int t = 0): value(v), tag(t) {}
};
constexpr bool operator<(const TaggedValue &left, const TaggedValue &right)
{
	return left.value < right.value;
}

void TestMinMax_complie_time()
{
	static_assert(are_same<int>::value, "Error in are_same.");
	static_assert(are_same<int, int>::value, "Error in are_same.");
	static_assert(are_same<int, int, int>::value, "Error in are_same.");
	static_assert(are_same<int, int, int, int>::value, "Error in are_same.");
	static_assert(!are_same<int, double>::value, "Error in are_same.");
	static_assert(!are_same<int, double, int>::value, "Error in are_same.");
	static_assert(!are_same<int, int, int, double>::value, "Error in are_same.");
	enum { vmin2 = vmin(1, 2), vmax2 = vmax(1, 2) };
	static_assert(vmin2 == 1 && vmax2 == 2, "Error in vmin/vmax (2).");
	enum { vmin3 = vmin(1, 2, 3), vmax3 = vmax(1, 2, 3) };
	static_assert(vmin3 == 1 && vmax3 == 3, "Error in vmin/vmax (3).");
	enum { vmin4 = vmin(1, 2, 3, 4), vmax4 = vmax(1, 2, 3, 4) };
	static_assert(vmin4 == 1 && vmax4 == 4, "Error in vmin/vmax (4).");
	constexpr TaggedValue vmin_e4 = vmin(TaggedValue(2), TaggedValue(1, 1), TaggedValue(1), TaggedValue(1));
	constexpr TaggedValue vmin_e5 = vmin(TaggedValue(2), TaggedValue(1, 1), TaggedValue(1), TaggedValue(1), TaggedValue(2));
	static_assert(vmin_e4.tag == 1 && vmin_e5.tag == 1, "Error in vmin (e)");
	constexpr TaggedValue vmax_e4 = vmax(TaggedValue(1), TaggedValue(1), TaggedValue(1, 1), TaggedValue(0));
	constexpr TaggedValue vmax_e5 = vmax(TaggedValue(0), TaggedValue(1), TaggedValue(1), TaggedValue(1, 1), TaggedValue(0));
	static_assert(vmax_e4.tag == 1 && vmax_e5.tag == 1, "Error in vmax (e)");
	//auto vmin_d2 = vmin(1, 1.1); // Error: different types.
	//auto vmin_d3 = vmin(1, 1.1, 1); // Error: different types.
	//auto vmax_d2 = vmax(1, 1.1); // Error: different types.
	//auto vmax_d3 = vmax(1, 1.1, 1); // Error: different types.
}

//--------------------------------------------------------------

} // namespace

//--------------------------------------------------------------

void TestFunctions()
{
	for (;;)
	{
		using func = function<void()>;
		auto response = GetButtonDecision(L"Choose test",
				{
					MakeButton(L"Force memory leak", func(MemoryLeakTest)),
					MakeButton(L"Predicates test", func(PredicateTest)),
					MakeButton(L"Exceptions test", func(ExceptionsTest)),
					MakeButton(L"Vector test", func(VectorTest)),
					MakeButton(L"Template test", func(template_test)),
					MakeButton(L"C++ test", func(TestCpp)),
					MakeButton(L"Exit", func())
				});
		if (!response)
			break;
		SafeExecute(response);
	}
}

//--------------------------------------------------------------
