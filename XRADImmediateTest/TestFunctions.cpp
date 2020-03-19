#include "pre.h"
#include <XRAD.h>

#include "TestFunctions.h"
#include "TestArrays.h"
#include "TestGUIFunctions.h"
#include "TestMisc.h"
#include "TestFunctionsArchive.h"
#include "TestPerformanceCounter.h"
#include "TestThreads.h"
#include "TestJson.h"
#include "TestFileOperations.h"
#include "TestMath.h"
#include "TestFFT.h"

#include <XRADBasic/BooleanFunctionTypes.h>
#include <MatrixVectorGUI.h>
#include <XRADBasic/Sources/Containers/ColorContainer.h>
#include <XRADBasic/Sources/PlatformSpecific/MSVC/XRADNatvisTest.h>
#include <iostream>
#include <cmath>
#include <XRADBasic/Sources/Utils/ValuePredicates.h>
#include <XRAD/Utils/AutoProgressIndicatorScheduler.h>

#include <vld.h>

#pragma warning(disable:4101) // Unreferenced local variable.
#pragma warning(disable:4189) // Local variable is initialized but not referenced

XRAD_USING


// template<class T, class ST, class FIELD_TAG>
// void	ShowVector(const string& title, const point_2<T, ST, FIELD_TAG> &p)
// {
// 	string result = ssprintf("x=%g, y=%g", double(p.x()), double(p.y()));
// 	ShowString(title, result);
// }

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

#ifdef _MSC_VER
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
#else
#error This code is nonportable in case of optimization.
#endif

#ifdef _MSC_VER
#pragma float_control(push)
#pragma float_control(precise, on)
#pragma fenv_access(on)
#pragma float_control(except, on)
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

#ifdef _MSC_VER
#pragma float_control(pop)
#else
#error Unknown compiler (see #pragma STDC FENV_ACCESS ...)
#endif

void	MemoryLeakTest()
{

	size_t N = 31415926;
	char *leak = new char[N];
}


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



template <class Array1D, class OtherArray1D>
pair<bool, string> TestMoveSemanticsHelper1D()
{
	const size_t size = 1024;
	Array1D array00(size);
	//array00 = size;
	//Array1D array01 = size;
	Array1D array1;
	array1.realloc(size);
	void *ptr1 = array1.data();
	Array1D array2c = array1;
	Array1D array2 = std::move(array1);
	void *ptr2 = array2.data();
	Array1D array3c;
	array3c = array2c;
	Array1D array3;
	array3 = std::move(array2);
	Array1D array4c;
	array4c = OtherArray1D(10);
	auto data_ptr = [](const Array1D &a) -> const void* { return a.size()? a.data(): nullptr; };
	bool is_good = ptr1 == ptr2 &&
			data_ptr(array1) == nullptr &&
			data_ptr(array2) == nullptr &&
			data_ptr(array3) == ptr2;
#if 0
	ShowString(L"Moved arrays 2D", ssprintf(
			L"array1 ptr=%p\n"
			L"array2 ptr=%p\n"
			L"array1: %zu (ptr=%p) ->\n"
			L"array2: %zu (ptr=%p) ->\n"
			L"array3: %zu (ptr=%p)",
			ptr1,
			ptr2,
			EnsureType<size_t>(array1.size()), data_ptr(array1),
			EnsureType<size_t>(array2.size()), data_ptr(array2),
			EnsureType<size_t>(array3.size()), data_ptr(array3)));
#endif
	return make_pair(is_good, ssprintf("Move result = %s, type = %s\n",
				is_good? "OK": "Error",
				EnsureType<const char*>(typeid(Array1D).name())));
}

template <class Array1D, class OtherArray1D>
pair<bool, string> TestMoveSemanticsHelper1DM()
{
	Array1D array01;
	(array01 = OtherArray1D(10)) *= 10; // Проверка возвращаемого типа для operator=.
	return TestMoveSemanticsHelper1D<Array1D, OtherArray1D>();
}

bool TestMoveSemantics1D()
{
	bool is_good = true;
	string message;
	auto test = [&is_good, &message](pair<bool, string> test_result)
	{
		if (!test_result.first)
		{
			is_good = false;
		}
		if (message.length())
			message += "\n";
		message += test_result.second;
	};
	using OtherArray1D = DataArray<int>;
	test(TestMoveSemanticsHelper1D<DataArray<int>, OtherArray1D>());
	test(TestMoveSemanticsHelper1D<AlgebraicStructures::GenericFieldElement<DataArray<double>, DataArray<double>, double, double, AlgebraicStructures::AlgebraicAlgorithmsDataArray>, OtherArray1D>());
	test(TestMoveSemanticsHelper1D<AlgebraicStructures::FieldElement<DataArray<double>, DataArray<double>, double, double, AlgebraicStructures::AlgebraicAlgorithmsDataArray, AlgebraicStructures::FieldTagScalar>, OtherArray1D>());
	test(TestMoveSemanticsHelper1D<Algebra1D<DataArray<double>, double, double, AlgebraicStructures::FieldTagScalar>, OtherArray1D>());
	test(TestMoveSemanticsHelper1DM<MathFunction<double, double, AlgebraicStructures::FieldTagScalar>, OtherArray1D>());
	test(TestMoveSemanticsHelper1DM<RealFunctionF32, OtherArray1D>());
	test(TestMoveSemanticsHelper1DM<ComplexFunctionF32, OtherArray1D>());
	test(TestMoveSemanticsHelper1DM<ColorFunctionF64, OtherArray1D>());
	test(TestMoveSemanticsHelper1DM<LinearVector<double, double, AlgebraicStructures::FieldTagScalar>, LinearVector<float, double, AlgebraicStructures::FieldTagScalar>>());
	test(TestMoveSemanticsHelper1DM<index_vector, index_vector>());
	test(TestMoveSemanticsHelper1DM<offset_vector, offset_vector>());
	test(TestMoveSemanticsHelper1DM<FilterKernelReal, FilterKernelReal>());
	test(TestMoveSemanticsHelper1DM<FilterKernelComplex, FilterKernelComplex>());
	test(TestMoveSemanticsHelper1D<BooleanFunctionLogical8, DataArray<int8_t>>());
	test(TestMoveSemanticsHelper1D<BooleanFunctionBitwise<int8_t>, DataArray<int8_t>>());

	ShowString("Move result 1D", ssprintf("Result = %s\n", is_good? "OK": "Error") + message);
	return is_good;
}



//--------------------------------------------------------------



string to_string_value(int v)
{
	return ssprintf("%i", v);
}

string to_string_value(double v)
{
	return ssprintf("%.3lf", v);
}

template <class PT, class ST>
string to_string_value(ComplexSample<PT, ST> v)
{
	return to_string_value(v.re);
}

template <class RGB_TRAITS_T>
string to_string_value(const RGBColorSample<RGB_TRAITS_T> &v)
{
	return to_string_value(v.red());
}

template <class Array2D, class OtherArray2D>
pair<bool, string> TestMoveSemanticsHelper2D()
{
	const size_t base_size = 128;
	const size_t size_h = 2*base_size, size_v = base_size/2;
	Array2D array00(size_v, size_h);
	Array2D array1;
	array1.realloc(size_v, size_h, Array2D::value_type(-1));
	array1.at(0,0) = 1;
	array1.at(0,1) = 2;
	array1.at(1,0) = 20;
	const void *ptr1 = array1.data();
	const void *ptr1r = &array1.row(0);
	const void *ptr1c = &array1.col(0);
	Array2D array2c = array1;
	Array2D array2 = std::move(array1);
	const void *ptr2 = array2.data();
	const void *ptr2r = &array2.row(0);
	const void *ptr2c = &array2.col(0);
	Array2D array3c;
	array3c = array2c;
	Array2D array3;
	array3 = std::move(array2);
	Array2D array4c;
	array4c = OtherArray2D(10, 10);
	auto data_ptr = [](const Array2D &a) -> const void* { return !a.empty()? a.data(): nullptr; };
	auto data_ptr_row = [](const Array2D &a) -> const void* { return !a.empty()? &a.row(0): nullptr; };
	auto data_ptr_col = [](const Array2D &a) -> const void* { return !a.empty()? &a.col(0): nullptr; };
	bool is_good =
			ptr1 == ptr2 && ptr1r == ptr2r && ptr1c == ptr2c &&
			data_ptr(array1) == nullptr &&
			data_ptr(array2) == nullptr &&
			data_ptr(array3) == ptr2 && data_ptr_row(array3) == ptr2r && data_ptr_col(array3) == ptr2c;
#if 0
	auto to_string = [](const Array2D &a, size_t v, size_t h)
	{
		if (a.empty())
			return string("?");
		return to_string_value(a.at(v, h));
	};
	auto array_dump = [&data_ptr, &data_ptr_row, &data_ptr_col, &to_string](const Array2D &a) -> string
	{
		return ssprintf("(%zu, %zu) st(%zi, %zi) ptr(%p, %p, %p) [%s, %s, %s]",
				EnsureType<size_t>(a.hsize()),
				EnsureType<size_t>(a.vsize()),
				EnsureType<ptrdiff_t>(a.hstep()),
				EnsureType<ptrdiff_t>(a.vstep()),
				EnsureType<const void*>(data_ptr(a)),
				EnsureType<const void*>(data_ptr_row(a)),
				EnsureType<const void*>(data_ptr_col(a)),
				EnsureType<const char*>(to_string(a,0,0).c_str()),
				EnsureType<const char*>(to_string(a,0,1).c_str()),
				EnsureType<const char*>(to_string(a,1,0).c_str())
				);
	};
	ShowString("Moved arrays 2D", ssprintf(
			"type=%s\n"
			"array1 ptr=%p, %p, %p\n"
			"array2 ptr=%p, %p, %p\n"
			"array1: %s ->\n"
			"array2: %s ->\n"
			"array3: %s",
			EnsureType<const char*>(typeid(Array2D).name()),
			ptr1, ptr1r, ptr1c,
			ptr2, ptr2r, ptr2c,
			EnsureType<const char*>(array_dump(array1).c_str()),
			EnsureType<const char*>(array_dump(array2).c_str()),
			EnsureType<const char*>(array_dump(array3).c_str())));
#endif
	return make_pair(is_good, ssprintf("Move result = %s, type = %s\n",
				is_good? "OK": "Error",
				EnsureType<const char*>(typeid(Array2D).name())));
}

template <class Array2D, class OtherArray2D>
pair<bool, string> TestMoveSemanticsHelper2DM()
{
	Array2D array01;
	(array01 = OtherArray2D(10, 20, 1)) *= 10;
	return TestMoveSemanticsHelper2D<Array2D, OtherArray2D>();
}

bool TestMoveSemantics2D()
{
	bool is_good = true;
	string message;
	auto test = [&is_good, &message](pair<bool, string> test_result)
	{
		if (!test_result.first)
		{
			is_good = false;
		}
		if (message.length())
			message += "\n";
		message += test_result.second;
	};
	using OtherArray2D = DataArray2D<DataArray<int>>;
	test(TestMoveSemanticsHelper2D<DataArray2D<DataArray<int>>, OtherArray2D>());
	test(TestMoveSemanticsHelper2D<AlgebraicStructures::GenericFieldElement<DataArray2D<DataArray<double>>, DataArray2D<DataArray<double>>, double, double, AlgebraicStructures::AlgebraicAlgorithmsDataArray>, OtherArray2D>());
	test(TestMoveSemanticsHelper2D<AlgebraicStructures::FieldElement<DataArray2D<DataArray<double>>, DataArray2D<DataArray<double>>, double, double, AlgebraicStructures::AlgebraicAlgorithmsDataArray, AlgebraicStructures::FieldTagScalar>, OtherArray2D>());
	test(TestMoveSemanticsHelper2D<Algebra2D<DataArray2D<DataArray<double>>, DataArray<double>, double, double, AlgebraicStructures::FieldTagScalar>, OtherArray2D>());
	test(TestMoveSemanticsHelper2DM<MathFunction2D<RealFunctionF32>, OtherArray2D>());
	test(TestMoveSemanticsHelper2DM<RealFunction2D_F32, OtherArray2D>());
	test(TestMoveSemanticsHelper2DM<ComplexFunction2D_F32, OtherArray2D>());
	test(TestMoveSemanticsHelper2DM<ColorImageF32, OtherArray2D>());
	test(TestMoveSemanticsHelper2D<FIRFilter2DReal, FIRFilter2DReal>());
	test(TestMoveSemanticsHelper2D<FIRFilter2DComplex, FIRFilter2DComplex>());
	test(TestMoveSemanticsHelper2D<BooleanFunctionLogical2D<int8_t>, DataArray2D<DataArray<int8_t>>>());
	test(TestMoveSemanticsHelper2D<BooleanFunctionBitwise2D<int8_t>, DataArray2D<DataArray<int8_t>>>());

	ShowString("Move result 2D", ssprintf("Result = %s\n", is_good? "OK": "Error") + message);
	return is_good;
}



//--------------------------------------------------------------



string to_string_value(const index_vector &v)
{
	string result("(");
	for (size_t i = 0; i < v.size(); ++i)
	{
		if (i)
			result += ", ";
		result += ssprintf("%zu", EnsureType<size_t>(v[i]));
	}
	result += ")";
	return result;
}

string to_string_value(const offset_vector &v)
{
	string result("(");
	for (size_t i = 0; i < v.size(); ++i)
	{
		if (i)
			result += ", ";
		result += ssprintf("%ti", EnsureType<ptrdiff_t>(v[i]));
	}
	result += ")";
	return result;
}

template <class ArrayMD, class OtherArrayMD>
pair<bool, string> TestMoveSemanticsHelperMD()
{
	const size_t base_size = 4;
	const size_t size_0 = 8*base_size, size_1 = 4*base_size, size_2 = 2*base_size, size_3 = base_size;
	index_vector size {size_0, size_1, size_3, size_3};
	ArrayMD array00(size);
	ArrayMD array1;
	array1.realloc(size, ArrayMD::value_type(-1));
	array1.at({0, 0, 0, 0}) = 1;
	array1.at({0, 0, 0, 1}) = 2;
	array1.at({0, 0, 1, 0}) = 3;
	array1.at({0, 1, 0, 0}) = 4;
	array1.at({1, 0, 0, 0}) = 5;
	const void *ptr1 = &array1.at({0, 0, 0, 0});
	ArrayMD array2c = array1;
	ArrayMD array2 = std::move(array1);
	const void *ptr2 = &array2.at({0, 0, 0, 0});
	ArrayMD array3c;
	array3c = array2c;
	ArrayMD array3;
	array3 = std::move(array2);
	//ArrayMD array4c;
	//array4c = DataArrayMD<DataArray2D<DataArray<Array2D::value_type>>>({10, 10, 10, 10});
	auto data_ptr = [](const ArrayMD &a) -> const void* { return !a.empty()? &a.at({0, 0, 0, 0}): nullptr; };
	bool is_good =
			ptr1 == ptr2 &&
			data_ptr(array1) == nullptr &&
			data_ptr(array2) == nullptr &&
			data_ptr(array3) == ptr2;
#if 0
	auto to_string = [](const ArrayMD &a, const index_vector &index)
	{
		if (a.empty())
			return string("?");
		return to_string_value(a.at(index));
	};
	auto array_dump = [&data_ptr, &to_string](const ArrayMD &a) -> string
	{
		return ssprintf("%s st %s ptr(%p) [%s, %s, %s, %s, %s]",
				EnsureType<const char*>(to_string_value(a.sizes()).c_str()),
				EnsureType<const char*>(to_string_value(a.steps()).c_str()),
				EnsureType<const void*>(data_ptr(a)),
				EnsureType<const char*>(to_string(a,{0,0,0,0}).c_str()),
				EnsureType<const char*>(to_string(a,{0,0,0,1}).c_str()),
				EnsureType<const char*>(to_string(a,{0,0,1,0}).c_str()),
				EnsureType<const char*>(to_string(a,{0,1,0,0}).c_str()),
				EnsureType<const char*>(to_string(a,{1,0,0,0}).c_str())
				);
	};
	ShowString("Moved arrays MD", ssprintf(
			"type=%s\n"
			"array1 ptr=%p\n"
			"array2 ptr=%p\n"
			"array1: %s ->\n"
			"array2: %s ->\n"
			"array3: %s",
			EnsureType<const char*>(typeid(ArrayMD).name()),
			ptr1,
			ptr2,
			EnsureType<const char*>(array_dump(array1).c_str()),
			EnsureType<const char*>(array_dump(array2).c_str()),
			EnsureType<const char*>(array_dump(array3).c_str())));
#endif
	return make_pair(is_good, ssprintf("Move result = %s, type = %s\n",
				is_good? "OK": "Error",
				EnsureType<const char*>(typeid(ArrayMD).name())));
}

template <class ArrayMD, class OtherArrayMD>
pair<bool, string> TestMoveSemanticsHelperMDM()
{
	ArrayMD array01;
	(array01 = OtherArrayMD({4, 5, 6, 7}, 1)) *= 10;
	return TestMoveSemanticsHelperMD<ArrayMD, OtherArrayMD>();
}

bool TestMoveSemanticsMD()
{
	bool is_good = true;
	string message;
	auto test = [&is_good, &message](pair<bool, string> test_result)
	{
		if (!test_result.first)
		{
			is_good = false;
		}
		if (message.length())
			message += "\n";
		message += test_result.second;
	};
	//ComplexFunctionMD_F32;
	using OtherArrayMD = DataArrayMD<DataArray2D<DataArray<int>>>;
	test(TestMoveSemanticsHelperMD<DataArrayMD<DataArray2D<DataArray<int>>>, OtherArrayMD>());
	test(TestMoveSemanticsHelperMD<AlgebraicStructures::GenericFieldElement<DataArrayMD<DataArray2D<DataArray<double>>>, DataArray2D<DataArray<double>>, double, double, AlgebraicStructures::AlgebraicAlgorithmsDataArray>, OtherArrayMD>());
	test(TestMoveSemanticsHelperMD<AlgebraicStructures::FieldElement<DataArrayMD<DataArray2D<DataArray<double>>>, DataArrayMD<DataArray2D<DataArray<double>>>, double, double, AlgebraicStructures::AlgebraicAlgorithmsDataArray, AlgebraicStructures::FieldTagScalar>, OtherArrayMD>());
	test(TestMoveSemanticsHelperMD<AlgebraMD<DataArrayMD<DataArray2D<DataArray<double>>>, DataArray2D<DataArray<double>>, double, double, AlgebraicStructures::FieldTagScalar>, OtherArrayMD>());
	test(TestMoveSemanticsHelperMDM<MathFunctionMD<RealFunction2D_F32>, OtherArrayMD>());
	test(TestMoveSemanticsHelperMDM<RealFunctionMD_F32, OtherArrayMD>());
	test(TestMoveSemanticsHelperMDM<ComplexFunctionMD_F32, OtherArrayMD>());
	test(TestMoveSemanticsHelperMDM<ColorImageMD_F32, OtherArrayMD>());
	ShowString("Move result MD", ssprintf("Result = %s\n", is_good? "OK": "Error") + message);
	return is_good;
}

void TestMoveSemantics()
{
	for (;;)
	{
		using func = function<void ()>;
		auto action = GetButtonDecision(L"Choose test",
				{
					MakeButton(L"Move semantics 1D", func(TestMoveSemantics1D)),
					MakeButton(L"Move semantics 2D", func(TestMoveSemantics2D)),
					MakeButton(L"Move semantics MD", func(TestMoveSemanticsMD)),
					MakeButton(L"OK", func())
				});
		if (!action)
			break;
		SafeExecute(action);
	}
}

//--------------------------------------------------------------

template <class TI>
void TestRoundItem(double v,
		bool sample_flag, TI sample_value)
{
	auto Error = [](const string &s)
	{
		printf("%s\n", s.c_str());
	};

	TI value;
	auto conv = try_iround_n(v, &value);
	if (conv != sample_flag || value != sample_value)
	{
		string result_str;
		if (std::is_unsigned<TI>::value)
			result_str = ssprintf("%llu", (unsigned long long)value);
		else
			result_str = ssprintf("%lli", (long long)value);
		string sample_value_str;
		if (std::is_unsigned<TI>::value)
			sample_value_str = ssprintf("%llu", (unsigned long long)sample_value);
		else
			sample_value_str = ssprintf("%lli", (long long)sample_value);
		Error(ssprintf("Error in try_iround_n(double v, %s* result): v=%lf, return=%s, *result=%s (expected: %s, %s)",
				EnsureType<const char*>(typeid(TI).name()),
				EnsureType<double>(v),
				EnsureType<const char*>(conv? "true": "false"),
				EnsureType<const char*>(result_str.c_str()),
				EnsureType<const char*>(sample_flag? "true": "false"),
				EnsureType<const char*>(sample_value_str.c_str())));
	}
	TI nc_value = iround_n<TI>(v);
	if (nc_value != value)
	{
		string result_str;
		if (std::is_unsigned<TI>::value)
			result_str = ssprintf("%llu", (unsigned long long)nc_value);
		else
			result_str = ssprintf("%lli", (long long)nc_value);
		string try_result_str;
		if (std::is_unsigned<TI>::value)
			try_result_str = ssprintf("%llu", (unsigned long long)value);
		else
			try_result_str = ssprintf("%lli", (long long)value);
		Error(ssprintf("Error in iround_n<%s>(double v): v=%lf, return=%s (expected: %s)",
				EnsureType<const char*>(typeid(TI).name()),
				EnsureType<double>(v),
				EnsureType<const char*>(result_str.c_str()),
				EnsureType<const char*>(try_result_str.c_str())));
	}
}

void TestRound()
{
	TestRoundItem<int>(0, true, 0);
	TestRoundItem<int>(1, true, 1);
	TestRoundItem<int>(-1, true, -1);
	TestRoundItem<int>(0.4, true, 0);
	TestRoundItem<int>(0.6, true, 1);
	TestRoundItem<int>(-0.4, true, 0);
	TestRoundItem<int>(-0.6, true, -1);
	TestRoundItem<int>(0.5, true, 0);
	TestRoundItem<int>(1.5, true, 2);
	TestRoundItem<int>(-0.5, true, 0);
	TestRoundItem<int>(-1.5, true, -2);
	TestRoundItem<int>(numeric_limits<int>::max(), true, numeric_limits<int>::max());
	TestRoundItem<int>(numeric_limits<int>::min(), true, numeric_limits<int>::min());
	TestRoundItem<int>(double(numeric_limits<int>::max())+1, false, numeric_limits<int>::max());
	TestRoundItem<int>(double(numeric_limits<int>::min())-1, false, numeric_limits<int>::min());
	TestRoundItem<unsigned int>(0, true, 0);
	TestRoundItem<unsigned int>(1, true, 1);
	TestRoundItem<unsigned int>(-1, false, 0);
	TestRoundItem<unsigned int>(numeric_limits<unsigned int>::max(), true, numeric_limits<unsigned int>::max());
	TestRoundItem<unsigned int>(double(numeric_limits<unsigned int>::max())+1, false, numeric_limits<unsigned int>::max());

	// Максимальные по модулю 64-битные целые значения (long long) не могут быть представлены точно
	// в 64-битном типе с плавающей точкой (double).
	// Поэтому здесь задаются и вычисляются нужные для теста константы под конкретные типы double
	// и long long.
	static_assert(numeric_limits<long long>::digits == 63, "This test expects 64 bit long long.");
	static_assert(numeric_limits<double>::digits == 53, "This test expects 64 bit double (53 bit mantissa).");
	double long_long_max_d = 9223372036854774784.0; // = std::nextafter(double(numeric_limits<long long>::max()), 0)
	double long_long_max_d_ll = 9223372036854774784ll;
	double long_long_max_d_next = 9223372036854775808.0; // 2^63, точно представимо в double.
	double long_long_min_d = -9223372036854775808.0; // -2^63, точно представимо в double.
	double long_long_min_d_ll = -9223372036854775808ll;
	double long_long_min_d_next = std::nextafter(long_long_min_d, numeric_limits<double>::lowest());
	TestRoundItem<long long>(0, true, 0);
	TestRoundItem<long long>(1, true, 1);
	TestRoundItem<long long>(-1, true, -1);
	TestRoundItem<long long>(long_long_max_d, true, long_long_max_d_ll);
	TestRoundItem<long long>(long_long_min_d, true, long_long_min_d_ll);
	TestRoundItem<long long>(long_long_max_d_next, false, numeric_limits<long long>::max());
	TestRoundItem<long long>(long_long_min_d_next, false, numeric_limits<long long>::min());

	TestRoundItem<unsigned long long>(0, true, 0);
	TestRoundItem<unsigned long long>(1, true, 1);
	TestRoundItem<unsigned long long>(-1, false, 0);
	TestRoundItem<unsigned long long>(long_long_max_d, true, long_long_max_d_ll);
	// Здесь возвращаемое значение (result) больше исходного значения double (существенно больше),
	// см. комментарии к функции:
	TestRoundItem<unsigned long long>(long_long_max_d_next, false, numeric_limits<unsigned long long>::max());
	//TestRoundItem<unsigned long long>(numeric_limits<unsigned long long>::max(), true, numeric_limits<unsigned long long>::max());
	TestRoundItem<unsigned long long>(long_long_min_d, false, 0);

	TestRoundItem<int>(numeric_limits<double>::infinity(), false, numeric_limits<int>::max());
	TestRoundItem<int>(-numeric_limits<double>::infinity(), false, numeric_limits<int>::min());
	TestRoundItem<int>(numeric_limits<double>::quiet_NaN(), false, 0);
	TestRoundItem<int>(numeric_limits<double>::quiet_NaN(), false, 0);
	TestRoundItem<int>(numeric_limits<double>::signaling_NaN(), false, 0);
}

//--------------------------------------------------------------

int xrad::xrad_main(int in_argc, char *in_argv[])
{
	XRAD_USING

	try
	{
		printf("command string arguments:\n");
		for(int i = 0; i < in_argc; ++i)
		{
			printf("%d:\t%s\n", i, in_argv[i]);
		}
		fflush(stderr);
		fflush(stdout);

		for (;;)
		{
			using func = function<void()>;
			auto response = GetButtonDecision(L"Choose test",
					{
						MakeButton(L"Move semantics", func(TestMoveSemantics)),
						MakeButton(L"Test arrays", func(TestArrays)),
						MakeButton(L"Test math", func(TestMathFunctions)),
						MakeButton(L"JSON", func(TestJson)),
						MakeButton(L"Handy", func(TestHandy)),
						MakeButton(L"Text handling", func(TestTextHandling)),
						MakeButton(L"I/O and file operations test", func(TestIO)),
						MakeButton(L"GUI", func(TestGUIFunctions)),
						MakeButton(L"Performance counter", func(TestPerformanceCounter)),
						MakeButton(L"Threads", func(TestThreads)),
						MakeButton(L"Round", func(TestRound)),
						MakeButton(L"Natvis (debugger visualization)", func(XRADNatvisTest)),
						MakeButton(L"Force memory leak", func(MemoryLeakTest)),
						MakeButton(L"Predicates test", func(PredicateTest)),
						MakeButton(L"Exceptions test", func(ExceptionsTest)),
						MakeButton(L"Test FFT", func(TestFFT)),
						MakeButton(L"Exit", func())
					});
			if (!response)
				break;

			printf("\nTest begin\n");
			fflush(stdout);
			try
			{
				response();
			}
			catch (canceled_operation &ex)
			{
				fflush(stderr);
				cerr << "\ntest canceled\n";
				cerr.flush();
			}
			catch (quit_application &ex)
			{
				throw;
			}
			catch (...)
			{
				fflush(stderr);
				string message = GetExceptionString();
				cerr << "\ntest failed:\n" << message << "\n";
				cerr.flush();
				Error(message);
			}
			fflush(stdout);
			cout << "\rtest ended\n";
			cout.flush();
		}
	}
	catch(quit_application &ex)
	{
		fflush(stdout);
		cout << "\n" << ex.what() << ", exit code = " << ex.exit_code << "\n";
		cout.flush();
		return ex.exit_code;
	}
	catch (...)
	{
		fflush(stderr);
		cerr << "\nUnhandled exception:\n" << GetExceptionString() << "\n";
		cerr.flush();
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}



XRAD_BEGIN
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

void TestMinMax()
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

void	TestAutoProgressScheduler()
{
//	Операция, занимающая N*d миллисекунд
	auto	lambda = [](int N, int d, ProgressProxy pp)
	{
		ProgressBar	p(pp);
		p.start("Processing", N);
		for (int i = 0; i < N; ++i)
		{
			Delay(msec(d));
			++p;
		}
		//		Pause();
	};
	RandomProgressBar	p(GUIProgressProxy());
	int	slow_delay(500), fast_delay(100);

	bool	proportional_init = false;
	//	Начальные значения имеют смысл только при удаленной ветке
	//	HKEY_CURRENT_USER\Software\XRAD\XRADImmediateTest.exe\AutoProgressScheduler\TestSubprogress
	//	При proportional_init = true прогресс двигается равномерно сразу (время выполнения 18 с, прогноз сразу верный)
	//	При proportional_init = false прогресс вначале движется быстро, прогноз 6 с, потом замедляется и вырастает до 18
	//	После нескольких запусков ситуация выравнивается.
	//	Быстрота автоподстройки определяется параметром AutoProgressScheduler::auto_tune_speed.
	//	Если эта величина слишком близка к 1, прогресс станет подвержен случайным изменениям от вызова к вызову и вновь станет нестабильным.
	AutoProgressIndicatorScheduler scheduler(L"TestSubprogress",
	{
		ProgressOperation(L"fast", proportional_init ? fast_delay : 50, true),
		ProgressOperation(L"slow", proportional_init ? slow_delay : 50, true),
	});
	scheduler.SetAutoTuneSpeed(0.75);
	p.start("Fast, slow", scheduler.n_steps());
	scheduler.run_and_register_cost(0, p, [&lambda, &fast_delay](auto subpp) {lambda(30, fast_delay, subpp); });
	scheduler.run_and_register_cost(1, p, [&lambda, &slow_delay](auto subpp) {lambda(30, slow_delay, subpp); });
}



void	TestCaseChange()
{
	vector<wstring>	v =
	{
		L"жил бы цитрус в чащах юга? да, но фальшивый экземпляр",
		L"Zoë Saldaña played in La maldición del padre Cardona. Blumenstraße",
		L"'Ότι μὲν ὑμει̃σ, ὠ̃ ἄνδρες 'Αθηναι̃οι, πεπόνθατε ὑπὸ τω̃ν ἐμω̃ν",
		L"Οι σημαντικότερες μαρτυρίες που πιστοποιούν την ύπαρξη ζωής στον ελλαδικό χώρο από την Λίθινη εποχή",
		L"Չնայած այն հանգամանքին, որ մայրցամաքային Հունաստանի և Էգեյան ծովում գտնվող կղզիների"
	};

	for(auto &s: v)
	{
		auto S = get_upper(s);
		ShowText(L"Text sample", s + L"\n" + S);
	}

	// Проверка, что глобальная локаль не стала русской. В числе должна быть точка, а не запятая
	ShowString(L"Decimal dot", ssprintf(L"pi = %g", 3.1415926));
}

void	TestExtendedSprintf()
{
	string	s0 = " 'string to insert'";
	wstring	ws0 = L"'wstring to insert'";

	string	s1 = ssprintf_core("Pointer: %s", s0.c_str());
	string	s3 = ssprintf("Ext Pointer: %s", s0.c_str());
	string	s4 = ssprintf("Ext String: %s", s0);
	string	s5 = ssprintf("Ext wPointer: %s", ws0.c_str());
	string	s6 = ssprintf("Ext wString: %s", ws0);

	wstring	ws1 = ssprintf_core(L"wPointer: %s", ws0.c_str());
	wstring	ws3 = ssprintf(L"Ext wPointer: %s", ws0.c_str());
	wstring	ws4 = ssprintf(L"Ext wString: %s", ws0);


	//неудавшаяся попытка вызвать warning C4477: format string '%d' requires an argument of type 'int', but variadic argument 1 has type 'double'
	int	i;
	double d;

	ForceDebugBreak();
}

void	TestHandy()
{
	TestCaseChange();
//	TestExtendedSprintf();
//	TestAutoProgressScheduler();
}



XRAD_END
